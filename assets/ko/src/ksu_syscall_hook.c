// SPDX-License-Identifier: GPL-2.0
/*
 * KernelSU sys_call_table hook backend (no-kprobe, CFI-safe variant)
 * for Xiaomi cannon 4.14.186 stock kernel.
 *
 * Hooked syscalls: execve/execveat (su -> ksud rewrite + escape),
 * faccessat/newfstatat (su -> sh rewrite), prctl (legacy KSU ABI).
 *
 * sys_call_table location strategy:
 *   runtime_delta = kallsyms_lookup_name("sys_execve") - OFFLINE_SYS_EXECVE
 *   table = OFFLINE_SYS_CALL_TABLE + runtime_delta
 * Sanity check: table[KSU_NR_EXECVE] must equal the runtime address of the
 * CFI jump-table thunk "sys_execve.cfi_jt" (or sys_execve itself on
 * non-CFI kernels).
 *
 * Patching: table lives in .rodata; write through a fresh writable vmap
 * alias of the backing page (same technique as KSU's LSM head override).
 * Aligned 8-byte stores are single-copy atomic on arm64; old entries are
 * kept valid, so racy readers either get old or new pointer, both callable.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/page.h>
#include <asm/memory.h>
#include <asm/pgtable.h>
#include <asm/cacheflush.h>

#include "ksu.h"
#include "ksu_syscall_hook.h"
#include "ksu_supercall.h"
#include "uapi_supercall.h"
#include "manager.h"
#include "klog.h"

/* from sucompat.c (non-static handlers) */
extern int ksu_handle_faccessat(int *dfd, const char __user **filename_user,
				int *mode, int *__unused_flags);
extern int ksu_handle_stat(int *dfd, const char __user **filename_user,
			   int *flags);
extern int ksu_handle_execve_sucompat(int *fd, const char __user **filename_user,
				      void *__never_use_argv,
				      void *__never_use_envp,
				      int *__never_use_flags);
/* from core_hook.c */
extern int ksu_handle_prctl(int option, unsigned long arg2, unsigned long arg3,
			    unsigned long arg4, unsigned long arg5);
/* from ksu_supercall.c (38-r74 supercall port) */
extern void ksu_supercall_on_setresuid(uid_t new_uid);

static unsigned long *ksu_sys_call_table;

/* 38-r62 OFFLINE VERDICT: the generic long(*)(long,long,long,long,long)
 * type hash 0x2b9c83ebf5cde5c6 IS in the kernel __cfi_check binary-search
 * table (in-image extraction, 384KB sweep), while the SyS-exact variant is
 * NOT — the kernel registers syscall pointers under the generic form.
 * Reverted to the generic type; r57c's __cfi_check_fail must therefore be
 * the ptr-membership test (jt vs fn address), to be settled by P4 (NOP the
 * slowpath stub entirely). */
typedef long (*ksu_syscall_fn_t)(long, long, long, long, long);

static ksu_syscall_fn_t orig_execve;
static ksu_syscall_fn_t orig_execveat;
static ksu_syscall_fn_t orig_faccessat;
static ksu_syscall_fn_t orig_newfstatat;
static ksu_syscall_fn_t orig_prctl;
static ksu_syscall_fn_t orig_reboot;
static ksu_syscall_fn_t orig_setresuid;

/* ------------------------------------------------------------------ */
/* table patching                                                      */
/* ------------------------------------------------------------------ */

static int ksu_write_ro_pointer(unsigned long *addr, unsigned long val)
{
	int ret = 0;
	unsigned long base = (unsigned long)addr & PAGE_MASK;
	unsigned long offset = offset_in_page(addr);
	struct page *page;
	void *vaddr;

	page = pfn_to_page(__pa_symbol(base) >> PAGE_SHIFT);
	if (!page)
		return -EFAULT;

	vaddr = vmap(&page, 1, VM_MAP, PAGE_KERNEL);
	if (!vaddr)
		return -ENOMEM;

	smp_wmb();
	*(unsigned long *)(vaddr + offset) = val;
	smp_wmb();

	vunmap(vaddr);
	return ret;
}

static unsigned long
ksu_resolve_hooked_entry(int nr, const char *sym, const char *cfi_sym)
{
	unsigned long a;

	if (cfi_sym && (a = kallsyms_lookup_name(cfi_sym)))
		return a;
	if (sym && (a = kallsyms_lookup_name(sym)))
		return a;
	return 0;
}

/*
 * Sanity-check every table entry we may ever patch. The legacy five entries
 * are checked in the block above; the two supercall entries (38-r74) are
 * checked here as well: if KSU_NR_REBOOT/KSU_NR_SETRESUID were wrong,
 * patching would silently hijack an unrelated syscall. ksu_supercall_hook_arm()
 * runs after this locate (via CMD_SUPERCALL_ARM), so a wrong nr must fail
 * the load here, once, before any patching can happen.
 */
static int ksu_locate_sys_call_table(void)
{
	unsigned long rt_execve = kallsyms_lookup_name("sys_execve");
	unsigned long want;
	unsigned long delta;

	if (!rt_execve) {
		pr_err("ksu_hook: sys_execve not found in kallsyms\n");
		return -ENOENT;
	}

	delta = rt_execve - KSU_OFFLINE_SYS_EXECVE;
	ksu_sys_call_table =
		(unsigned long *)(KSU_OFFLINE_SYS_CALL_TABLE + delta);

	want = ksu_resolve_hooked_entry(KSU_NR_EXECVE, "sys_execve",
					"sys_execve.cfi_jt");
	if (!want ||
	    ksu_sys_call_table[KSU_NR_EXECVE] != want ||
	    ksu_sys_call_table[KSU_NR_PRCTL] !=
		    ksu_resolve_hooked_entry(KSU_NR_PRCTL, "sys_prctl",
					     "sys_prctl.cfi_jt")) {
		pr_err("ksu_hook: sys_call_table sanity check failed (delta=0x%lx)\n",
		       delta);
		ksu_sys_call_table = NULL;
		return -EINVAL;
	}

	if (ksu_sys_call_table[KSU_NR_REBOOT] !=
		    ksu_resolve_hooked_entry(KSU_NR_REBOOT, "sys_reboot",
					     "sys_reboot.cfi_jt") ||
	    ksu_sys_call_table[KSU_NR_SETRESUID] !=
		    ksu_resolve_hooked_entry(KSU_NR_SETRESUID,
					     "sys_setresuid",
					     "sys_setresuid.cfi_jt")) {
		pr_err("ksu_hook: sys_call_table sanity check failed (nr %d/%d, delta=0x%lx)\n",
		       KSU_NR_REBOOT, KSU_NR_SETRESUID, delta);
		ksu_sys_call_table = NULL;
		return -EINVAL;
	}

	pr_info("ksu_hook: sys_call_table @ %px (delta=0x%lx)\n",
		ksu_sys_call_table, delta);
	return 0;
}

/* ------------------------------------------------------------------ */
/* syscall wrappers (arm64 4.14: raw C ABI args, no pt_regs wrapper)   */
/* ------------------------------------------------------------------ */

asmlinkage long ksu_sys_execve(const char __user *filename_user,
			       const char __user *const __user *argv,
			       const char __user *const __user *envp)
{
	ksu_handle_execve_sucompat(AT_FDCWD, &filename_user, NULL, NULL, NULL);
	return orig_execve((long)filename_user, (long)argv, (long)envp, 0, 0);
}

asmlinkage long ksu_sys_execveat(int dfd, const char __user *filename_user,
				 const char __user *const __user *argv,
				 const char __user *const __user *envp,
				 int flags)
{
	ksu_handle_execve_sucompat(&dfd, &filename_user, NULL, NULL, NULL);
	return orig_execveat((long)dfd, (long)filename_user, (long)argv,
			     (long)envp, (long)flags);
}

asmlinkage long ksu_sys_faccessat(int dfd, const char __user *filename_user,
				  int mode)
{
	ksu_handle_faccessat(&dfd, &filename_user, &mode, NULL);
	return orig_faccessat((long)dfd, (long)filename_user, (long)mode, 0,
			      0);
}

asmlinkage long ksu_sys_newfstatat(int dfd, const char __user *filename_user,
				   struct kstat __user *statbuf, int flag)
{
	ksu_handle_stat(&dfd, &filename_user, &flag);
	return orig_newfstatat((long)dfd, (long)filename_user, (long)statbuf,
			       (long)flag, 0);
}

asmlinkage long ksu_sys_prctl(int option, unsigned long arg2, unsigned long arg3,
			      unsigned long arg4, unsigned long arg5)
{
	ksu_handle_prctl(option, arg2, arg3, arg4, arg5);
	return orig_prctl((long)option, (long)arg2, (long)arg3, (long)arg4,
			  (long)arg5);
}

/* ------------------------------------------------------------------ */
/* 38-r74: supercall fd channel                                        */
/* ------------------------------------------------------------------ */

/*
 * reboot(2) carrying the install magic installs a "[ksu_driver]" fd into the
 * caller and writes the fd number back through arg4. Upstream does this from
 * a kprobe pre_handler, which cannot alter the return value — hence its
 * task_work_add dance. Taking the syscall over outright lets us just do the
 * install and return 0. Every other reboot call passes through untouched, so
 * `adb reboot` and friends keep working.
 *
 * The magic pair alone must not bypass reboot(2)'s CAP_SYS_BOOT check: only
 * the crowned manager (or real root) may use the channel. A magic call from
 * anyone else falls through to orig_reboot, so the kernel's own permission
 * check rejects it with EPERM exactly as if the magic were absent.
 */
asmlinkage long ksu_sys_reboot(int magic1, int magic2, unsigned int cmd,
			       void __user *arg)
{
	if (magic1 == KSU_INSTALL_MAGIC1 && magic2 == KSU_INSTALL_MAGIC2) {
		if (is_manager() || current_uid().val == 0)
			return ksu_install_fd_to_user((int __user *)arg);
	}

	return orig_reboot((long)magic1, (long)magic2, (long)cmd, (long)arg,
			   0);
}

/*
 * zygoe calls setresuid(3) on every app fork. When the new euid is the
 * crowned manager, hand that process a driver fd — the manager app never
 * asks for one itself, it only scans /proc/self/fd. Run after the original
 * so current_uid() already reflects the new value.
 */
asmlinkage long ksu_sys_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
	long ret = orig_setresuid((long)ruid, (long)euid, (long)suid, 0, 0);

	if (ret == 0)
		ksu_supercall_on_setresuid(euid);

	return ret;
}

/* ------------------------------------------------------------------ */
/* init / exit                                                         */
/* ------------------------------------------------------------------ */

typedef struct {
	int nr;
	const char *sym;
	const char *cfi_sym;
	asmlinkage long (*hook)(long, long, long, long, long);
	ksu_syscall_fn_t *orig_slot;
} ksu_hook_entry_t;

static const ksu_hook_entry_t ksu_hooks[] = {
	{ KSU_NR_EXECVE, "sys_execve", "sys_execve.cfi_jt",
	  (void *)ksu_sys_execve, &orig_execve },
	{ KSU_NR_EXECVEAT, "sys_execveat", "sys_execveat.cfi_jt",
	  (void *)ksu_sys_execveat, &orig_execveat },
	{ KSU_NR_FACCESSAT, "sys_faccessat", "sys_faccessat.cfi_jt",
	  (void *)ksu_sys_faccessat, &orig_faccessat },
	{ KSU_NR_NEWFSTATAT, "sys_newfstatat", "sys_newfstatat.cfi_jt",
	  (void *)ksu_sys_newfstatat, &orig_newfstatat },
	{ KSU_NR_PRCTL, "sys_prctl", "sys_prctl.cfi_jt",
	  (void *)ksu_sys_prctl, &orig_prctl },
};

/*
 * 38-r76: the v3.x supercall fd-channel hooks are installed ON DEMAND via
 * CMD_SUPERCALL_ARM, not at module_init. Installing them at init puts them
 * live while the module's CFI stubs are still un-NOPed, and that
 * finit->NOP window is where the AG/AH panics came from — a setresuid
 * (zygote does one per app fork) reaching the hook would make a cross-DSO
 * call through a live __cfi_slowpath stub. The probe NOPs every stub ~80ms
 * after finit, so the window is short but not zero, and these two hooks sit
 * on hotter paths than the original five.
 */
static const ksu_hook_entry_t ksu_hooks_supercall[] = {
	{ KSU_NR_REBOOT, "sys_reboot", "sys_reboot.cfi_jt",
	  (void *)ksu_sys_reboot, &orig_reboot },
	{ KSU_NR_SETRESUID, "sys_setresuid", "sys_setresuid.cfi_jt",
	  (void *)ksu_sys_setresuid, &orig_setresuid },
};

static bool ksu_supercall_armed;

/* ------------------------------------------------------------------ */
/* 41-r113: F1 P-A — kernel-side CFI check-block takeover              */
/*                                                                     */
/* The 7 syscall wrappers call their orig_* slots through the generic  */
/* 5-arg ksu_syscall_fn_t; LTO materializes each call site as          */
/*   movz/movk x0, #0x2b9c83ebf5cde5c6 ; mov x1, xN ; bl __cfi_slowpath*/
/*   <args> ; blr xN                          (38-r62 generic hash)    */
/* The probe NOPs these from userland via physrw alias writes with NO  */
/* I-cache maintenance (H1'): the fetch side keeps executing the stale */
/* check or a mixed stream — the (b)-state 10/10 blob-crash carrier    */
/* (37-r112). We take the 7 hottest sites over at finit, in-kernel,    */
/* with per-instruction verification and a proper flush_icache_range,  */
/* then rewrite the slots to the raw sys_* addresses (.bss plain store)*/
/* so blr goes straight to the real syscall (no check, no jt).         */
/*                                                                     */
/* Order invariant (anti-panic, cf. ksu.c:69-81): NOP the check block  */
/* BEFORE any slot may hold a raw address; a site whose verification   */
/* fails is left completely untouched (= today's behavior). The table  */
/* patch runs after the NOP, so wrappers are unreachable (no caller)   */
/* while we patch: no concurrent execution can observe partial state.  */
/* Slot rewrite happens after the table patch: with the checks gone,   */
/* blr-through-jt and blr-raw are both safe, and the aligned 8-byte    */
/* store is single-copy atomic — readers see either, both callable.    */
/* ------------------------------------------------------------------ */

#define KSU_F1_HASH_LOW   0xe5c6u
#define KSU_F1_HASH_16    0xf5cdu
#define KSU_F1_HASH_32    0x83ebu
#define KSU_F1_HASH_48    0x2b9cu
#define KSU_F1_ARM64_NOP  0xD503201Fu
#define KSU_F1_WINDOW     8   /* bl + up-to-7 preceding instructions */
#define KSU_F1_EXPECTED   7   /* one per syscall wrapper (38-r62 hash)  */

static u32 ksu_f1_snapshot[KSU_F1_EXPECTED][KSU_F1_WINDOW];
static int ksu_f1_sites;     /* sites actually NOPed */
static int ksu_f1_active;    /* 1 == all 7 taken over; slots may go raw */

/*
 * Find the 38-r62 generic-hash materialization in the instructions
 * before a `bl __cfi_slowpath`. The probe-side census and the r113
 * on-device scan both showed the six straight-line wrappers pack the
 * four hash moves contiguously right before the call, but setresuid
 * interleaves argument moves into the block (41 §3.1 row 7: mov w19,w1
 * / mov w20,w2 between the movk's) — so instead of a rigid 5-insn
 * pattern we require the four hash instructions (movz #0xe5c6 then
 * movk #0xf5cd/#0x83eb/#0x2b9c, each with Rd == x0) to appear IN ORDER
 * anywhere within the window, with no other x0-writer between them.
 */
static bool ksu_f1_hash_window_ok(const u32 *win, int n)
{
	static const struct {
		u32 op_mask, op_val, imm;
	} want[4] = {
		{ 0xFFE0001F, 0xD2800000, KSU_F1_HASH_LOW  },
		{ 0xFFE0001F, 0xF2A00000, KSU_F1_HASH_16   },
		{ 0xFFE0001F, 0xF2C00000, KSU_F1_HASH_32   },
		{ 0xFFE0001F, 0xF2E00000, KSU_F1_HASH_48   },
	};
	int stage = 0, i;

	for (i = 0; i < n; i++) {
		u32 insn = win[i];

		if (stage < 4) {
			if ((insn & want[stage].op_mask) == want[stage].op_val &&
			    ((insn >> 5) & 0xFFFF) == want[stage].imm)
				stage++;
			else if ((insn & 0x1F) == 0)          /* Rd == x0? */
				return false;                 /* other x0 use breaks the id setup */
		}
	}
	return stage == 4;
}

/* writable alias write for module-vmalloc .text; NOP words only */
static int ksu_f1_nop_range(unsigned long start, unsigned long end)
{
	while (start < end) {
		unsigned long pg = start & PAGE_MASK;
		unsigned long pg_end = ((end - 1) & PAGE_MASK) + PAGE_SIZE;
		unsigned long off = start - pg;
		size_t len = min_t(unsigned long, end - start, pg_end - start);
		struct page *page = vmalloc_to_page((void *)pg);
		u32 *dst;
		void *va;
		size_t j;

		if (!page)
			return -EFAULT;
		va = vmap(&page, 1, VM_MAP, PAGE_KERNEL);
		if (!va)
			return -ENOMEM;

		smp_wmb();
		dst = (u32 *)((char *)va + off);
		for (j = 0; j < len / 4; j++)
			dst[j] = KSU_F1_ARM64_NOP;
		smp_wmb();

		vunmap(va);
		start += len;
	}
	return 0;
}

/*
 * Scan this module's .text for `bl __cfi_slowpath` sites whose preceding
 * window carries the 38-r62 generic-hash materialization; NOP the bl
 * itself (the check dies with the call — the hash/mov remnants become
 * dead code, and the argument setup after the bl site still runs, so
 * the orig call through `blr xN` is unaffected). Expect exactly 7 such
 * sites (the syscall wrappers); anything else leaves F1 inactive and
 * the module behaves exactly like f96aa996 (probe keeps taking sites).
 */
static void ksu_f1_takeover(void)
{
	unsigned long slowpath = kallsyms_lookup_name("__cfi_slowpath");
	u32 *base = (u32 *)THIS_MODULE->core_layout.base;
	size_t words = THIS_MODULE->core_layout.text_size / 4;
	size_t i;

	if (!slowpath || !base || !words) {
		pr_warn("ksu_hook F1: scan prerequisites missing (slowpath=%px base=%px words=%zu)\n",
			(void *)slowpath, (void *)base, words);
		return;
	}

	for (i = KSU_F1_WINDOW - 1; i < words; i++) {
		u32 insn = base[i];
		unsigned long target;
		long imm26;
		u32 *bl_site;
		int j;

		if ((insn & 0xFC000000) != 0x94000000)
			continue;
		imm26 = (long)(int)((insn & 0x03FFFFFF) << 6) >> 6;
		target = (unsigned long)(base + i) + imm26 * 4;
		if (target != slowpath)
			continue;

		bl_site = &base[i];
		if (!ksu_f1_hash_window_ok(bl_site - (KSU_F1_WINDOW - 1),
					   KSU_F1_WINDOW - 1)) {
			pr_info("ksu_hook F1: bl->%px matches slowpath but hash window differs — not a wrapper site, skipped\n",
				(void *)target);
			continue;
		}
		if (ksu_f1_sites >= KSU_F1_EXPECTED) {
			pr_warn("ksu_hook F1: more wrapper-shaped sites than expected — stopping at %d\n",
				ksu_f1_sites);
			break;
		}
		for (j = 0; j < KSU_F1_WINDOW; j++)
			ksu_f1_snapshot[ksu_f1_sites][j] =
				bl_site[j - (KSU_F1_WINDOW - 1)];
		if (ksu_f1_nop_range((unsigned long)bl_site,
				     (unsigned long)bl_site + 4)) {
			pr_warn("ksu_hook F1: site @%px NOP write failed — left untouched\n",
				(void *)bl_site);
			continue;
		}
		flush_icache_range((unsigned long)bl_site,
				   (unsigned long)bl_site + 4);
		ksu_f1_sites++;
		pr_info("ksu_hook F1: site #%d @%px check call NOPed + icache flushed\n",
			ksu_f1_sites, (void *)bl_site);
	}

	if (ksu_f1_sites == KSU_F1_EXPECTED) {
		ksu_f1_active = 1;
		pr_info("ksu_hook F1: %d/7 wrapper check calls taken over (kernel-side, architecture-determined)\n",
			ksu_f1_sites);
	} else {
		pr_warn("ksu_hook F1: %d/7 sites — F1 inactive, slots stay jt, probe takes over as before\n",
			ksu_f1_sites);
	}
}

static int ksu_patch_hook_set(const void *set, size_t n)
{
	const ksu_hook_entry_t *h = set;
	size_t i;
	int ret;

	for (i = 0; i < n; i++) {
		unsigned long orig = ksu_sys_call_table[h[i].nr];
		unsigned long hook = (unsigned long)h[i].hook;

		if (!orig) {
			pr_err("ksu_hook: nr %d empty\n", h[i].nr);
			return -EINVAL;
		}
		*h[i].orig_slot = (ksu_syscall_fn_t)orig;

		ret = ksu_write_ro_pointer(&ksu_sys_call_table[h[i].nr], hook);
		if (ret) {
			pr_err("ksu_hook: patch nr %d failed: %d\n", h[i].nr,
			       ret);
			/* roll back already-patched entries */
			while (i--)
				ksu_write_ro_pointer(
					&ksu_sys_call_table[h[i].nr],
					(unsigned long)*h[i].orig_slot);
			return ret;
		}
		pr_info("ksu_hook: hooked nr %d (%s)\n", h[i].nr, h[i].sym);
	}
	return 0;
}

int ksu_syscall_hook_init(void)
{
	int ret = ksu_locate_sys_call_table();

	if (ret)
		return ret;

	/* 41-r113 F1: NOP the 7 wrapper check blocks BEFORE the table patch
	 * makes the wrappers reachable (order invariant: checks die first). */
	ksu_f1_takeover();

	/* 41-r116 hook-set decrement experiment: /data/local/tmp/ksu-hook-set
	 * holds a hex bitmask over the five legacy table entries (bit0 execve,
	 * bit1 execveat, bit2 faccessat, bit3 newfstatat, bit4 prctl) — bit
	 * CLEAR = entry stays original. Missing/unreadable file = install all
	 * (f96aa996 behavior). Used to bisect which hooked syscall carries
	 * the blob-crash (r115: hook activation itself, not CFI patches). */
	{
		static const int nr_to_bit[] = {
			[KSU_NR_EXECVE]     = 0,
			[KSU_NR_EXECVEAT]   = 1,
			[KSU_NR_FACCESSAT]  = 2,
			[KSU_NR_NEWFSTATAT] = 3,
			[KSU_NR_PRCTL]      = 4,
		};
		ksu_hook_entry_t eff[ARRAY_SIZE(ksu_hooks)];
		struct file *f;
		u32 mask = 0x1Fu;
		char buf[16];
		size_t i, n_eff = 0;

		f = filp_open("/data/local/tmp/ksu-hook-set", O_RDONLY, 0);
		if (!IS_ERR(f)) {
			loff_t pos = 0;
			ssize_t n = kernel_read(f, buf, sizeof(buf) - 1, &pos);

			filp_close(f, NULL);
			if (n > 0) {
				buf[n] = '\0';
				if (kstrtouint(strim(buf), 16, &mask) == 0)
					pr_info("ksu_hook: hook-set mask 0x%x from file\n",
						mask);
			}
		}

		for (i = 0; i < ARRAY_SIZE(ksu_hooks); i++) {
			int bit = nr_to_bit[ksu_hooks[i].nr];

			if (bit >= 0 && !(mask & (1u << bit))) {
				pr_info("ksu_hook: hook-set mask skips nr %d (%s)\n",
					ksu_hooks[i].nr, ksu_hooks[i].sym);
				continue;
			}
			eff[n_eff++] = ksu_hooks[i];
		}

		ret = ksu_patch_hook_set(eff, n_eff);
		if (ret)
			return ret;

		/* 41-r113 F1: slots → raw sys_* addresses (only meaningful
		 * for entries actually patched). */
		for (i = 0; i < n_eff; i++) {
			unsigned long fn;

			if (!ksu_f1_active)
				break;
			fn = kallsyms_lookup_name(eff[i].sym);
			if (fn) {
				*eff[i].orig_slot = (ksu_syscall_fn_t)fn;
				pr_info("ksu_hook F1: orig slot[%s] = %px (raw, jt bypassed)\n",
					eff[i].sym, (void *)fn);
			}
		}
		if (ksu_f1_active)
			smp_wmb();

		return 0;
	}
}

/*
 * 38-r76: arm the supercall hooks. Idempotent; called from the legacy prctl
 * channel (CMD_SUPERCALL_ARM) by the probe once every CFI stub is NOPed.
 */
int ksu_supercall_hook_arm(void)
{
	int ret;

	if (ksu_supercall_armed)
		return 0;
	if (!ksu_sys_call_table)
		return -ENODEV;

	ret = ksu_patch_hook_set(ksu_hooks_supercall,
				 ARRAY_SIZE(ksu_hooks_supercall));
	if (ret)
		return ret;

	ksu_supercall_armed = true;
	return 0;
}

void ksu_syscall_hook_exit(void)
{
	int i;

	if (!ksu_sys_call_table)
		return;

	/* supercall set first (it was armed last, and may be unarmed) */
	for (i = 0; i < ARRAY_SIZE(ksu_hooks_supercall); i++) {
		if (*ksu_hooks_supercall[i].orig_slot)
			ksu_write_ro_pointer(
				&ksu_sys_call_table[ksu_hooks_supercall[i].nr],
				(unsigned long)*ksu_hooks_supercall[i].orig_slot);
	}
	ksu_supercall_armed = false;

	for (i = 0; i < ARRAY_SIZE(ksu_hooks); i++) {
		if (*ksu_hooks[i].orig_slot)
			ksu_write_ro_pointer(
				&ksu_sys_call_table[ksu_hooks[i].nr],
				(unsigned long)*ksu_hooks[i].orig_slot);
	}
	ksu_sys_call_table = NULL;
}
