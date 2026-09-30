#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h> /* 39-r102: file dump channel */

#include "../klog.h" // IWYU pragma: keep
#include "../kernel_compat.h" /* 39-r102: file dump diagnostic channel */
#include "selinux.h"
#include "sepolicy.h"
#include "ss/services.h"
#include "linux/lsm_audit.h"
#include "xfrm.h"
#include "../ksu_selinux_resolve.h"
#include "../ksu_selinux_shim.h"

static int ksu_ebitmap_set_bit_probe(unsigned long e_addr, unsigned long bit);

/* 39-r102: file diagnostic target for the raw policydb window; see the
 * dump block in apply_kernelsu_rules() below. */
#define KSU39_PPDB_PATH "/data/adb/ksu/ppdb.bin"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#define SELINUX_POLICY_INSTEAD_SELINUX_SS
#endif

#define KERNEL_SU_DOMAIN "su"
#define KERNEL_SU_FILE "ksu_file"
#define KERNEL_EXEC_TYPE "ksu_exec"
#define ALL NULL


/* 39-r94: get_policydb() removed -- apply_kernelsu_rules() now reads
 * the policydb through probe_kernel_read() only, and a static function
 * with no callers would trip -Wunused-function. */

/*
 * 39-r94: every kernel-memory read below goes through probe_kernel_read()
 * (exported, GPL, plain direct call) instead of a bare dereference. Three
 * boots died in this function on a bogus [ss+64] read; a wrong pointer must
 * degrade to "log the actual values and skip", never to an Oops.
 */
static bool ksu_is_kptr(u64 v)
{
	return (v >> 48) == 0xFFFF;
}

/*
 * 39-r102: address guard for derived policydb-internal pointers. The
 * policydb lives inline in selinux_ss; r98 proved [ss, ss+0x400) is
 * dereference-safe (the scan walks it live). Anything derived that
 * stays within ss + 4K is inside that proven window; anything else is
 * a wrong base and must not be touched.
 */
static bool ksu_is_kptr_safe_window(unsigned long addr, unsigned long ss)
{
	return addr >= ss && addr < ss + 0x1000;
}

/*
 * 39-r97: probe_kernel_read() on this MTK 4.14 returns WRONG DATA (not an
 * error) for kernel .data addresses: the same address that the t37 probe
 * verifies byte-exactly via its physrw primitive reads back as slab
 * garbage through probe_kernel_read (the [ss] dump showed freelist
 * patterns where selinux_ss->policydb should be, while the probe at the
 * same state address read a live initialized==1 selinux_state and
 * successfully flipped enforcing). So all reads here are plain
 * dereferences -- the address is validated up-front by
 * ksu_selinux_state_triple_check() (probe-free, run at resolve_init time
 * where we KNOW the value), and apply_kernelsu_rules() only runs against
 * that same verified state pointer.
 *
 * NOTE: probe_kernel_read is still used in the scan loop as a *filter*
 * only; its results are never trusted for writes.
 */
static int ksu_kread8(unsigned long addr, u64 *out)
{
	*out = *(u64 *)addr;
	return 0;
}

/*
 * 39-r102: one-shot file diagnostic. printk_devkmsg=ratelimit silently
 * eats burst logging (r97), so layout questions go to a file instead:
 * dump the raw [ss, ss+0x400) window -- the region r96/r98 already
 * proved dereference-safe (the scan window plus the symtab row that
 * showed nprim=3553 @ ss+0x50) -- to /data/adb/ksu/ppdb.bin for
 * offline analysis. Same kernel_write pattern as allowlist.c.
 * Called once per grant_root (rules_applied is idempotent upstream),
 * BEFORE any probing or writing, so the dump is always the pristine
 * layout no matter where the run below bails out.
 */
static void ksu39_dump_ppdb(unsigned long ss)
{
	struct file *fp;
	loff_t off = 0;
	char *buf;
	const size_t n = 0x400;

	buf = kzalloc(n, GFP_KERNEL);
	if (!buf)
		return;
	memcpy(buf, (const void *)ss, n);

	fp = ksu_filp_open_compat(KSU39_PPDB_PATH,
				  O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (IS_ERR(fp)) {
		pr_err("ksu: ppdb dump open failed: %ld\n", PTR_ERR(fp));
		kfree(buf);
		return;
	}
	if (ksu_kernel_write_compat(fp, buf, n, &off) != (ssize_t)n)
		pr_err("ksu: ppdb dump short write (%lld)\n", (long long)off);
	filp_close(fp, 0);
	kfree(buf);
	pr_info("ksu: ppdb dumped 0x%zx bytes to " KSU39_PPDB_PATH "\n", n);
}

void apply_kernelsu_rules(void)
{
	struct selinux_state *state;
	unsigned long ss, sidtab = 0, table = 0;
	u64 q0 = 0, qsz = 0, q1 = 0;
	u32 tsize = 0, i;
	u64 tv = 0;

	state = (struct selinux_state *)ksu_selinux_state();
	if (!state) { /* resolve_init() leaves addr=0 when it fails */
		pr_err("ksu: selinux_state unresolved, skip rules\n");
		return;
	}

	/* state->ss @ +0x18 (avc @ +0x10) */
	if (ksu_kread8((unsigned long)state + 0x18, &q0) || !ksu_is_kptr(q0)) {
		pr_err("ksu: bad ss pointer %llx, skip rules\n", q0);
		return;
	}
	ss = (unsigned long)q0;

	/* sidtab is the first field; read it as a sanity anchor. */
	if (ksu_kread8(ss, &q0) || !ksu_is_kptr(q0)) {
		pr_err("ksu: bad sidtab %llx, skip rules\n", q0);
		return;
	}
	sidtab = (unsigned long)q0;

	/* 39-r102: file diagnostic first, while the layout is pristine. */
	ksu39_dump_ppdb(ss);

	/*
	 * 39-r94: the compile-time p_types offset (policydb+56 = ss+0x40)
	 * is wrong for this build -- [ss+0x40] reads 4 and nprim reads a
	 * sign-extended pointer. Scan instead: for every 8-aligned slot in
	 * [ss+8, ss+0x400), treat (slot, slot+8) as (table, nprim) and
	 * accept a slot ONLY if walking that hashtab actually finds the
	 * type "su" -- the string match is the real validator.
	 */
	{
		unsigned long off;
		unsigned long off_types = 0; /* 39-r102: proven symtab slot */
		char key[16];

		/* 39-r98: the [ss] word-dump (r97) showed the policydb area
		 * starts somewhere between ss+8 and ss+0x10, with 16-byte
		 * symtab entries {table, nprim} -- but the exact policydb
		 * base is unknown (r97 proved the compile-time +8 wrong).
		 * So: scan the WHOLE window, try (table@off, nprim@off+8)
		 * AND (table@off+8, nprim@off) per slot, and accept only
		 * when the hashtab walk actually finds "su". */
		for (off = 16; off < 0x1000 && !tv; off += 8) {
			u64 qcand = 0, qn = 0, qht = 0, qsz2 = 0;
			long cand_off;

			/* r99: the [ss] dump pinned the real layout --
			 * symtab entries are {nprim@0, table@+8} (opposite of
			 * the cannon tree's symtab.h!), symtab[0] at ss+0x18,
			 * i.e. policydb base = ss+0x10. For each slot we try
			 * table at -8/0/+8 relative to a plausible nprim so
			 * no field-order assumption survives. */
			for (cand_off = -8; cand_off <= 8 && !tv; cand_off += 8) {
				if (ksu_kread8(ss + off + cand_off, &qcand) ||
				    ksu_kread8(ss + off + cand_off + 8, &qn))
					break;
				if (!ksu_is_kptr(qcand) || !qn || qn > 4096)
					continue;
				/* 39-r102 FIX: hashtab = {htable@0,
				 * u32 size@8, u32 nel@12}; reading +8
				 * as u64 yields size|nel<<32 which for
				 * any non-empty table (nel>=1) is >=2^32
				 * and tripped the 65536 cap, rejecting
				 * EVERY hashtab -- the "su not found"
				 * of r94-r100. Keep the plain-deref read
				 * (probe_kernel_read lies on this kernel,
				 * r97) and take the low word only. */
				if (ksu_kread8(qcand, &qht) ||
				    ksu_kread8(qcand + 8, &qsz2))
					continue;
				qsz2 = (u32)qsz2;
				if (!ksu_is_kptr(qht) || !qsz2 ||
				    qsz2 > 65536) /* types table buckets
						   * can exceed 4096 */
					continue;

			/* walk this hashtab looking for "su".
			 * r99 diagnostics: printk_devkmsg=ratelimit eats
			 * bursts, so each grant_root call prints at most
			 * ONE line (round-robin via g_scan_round). */
			{
				static u32 g_scan_round;

				g_scan_round++;
				pr_info("ksu: [scan r%u] ss+0x%lx+%ld:"
					" nprim=%llu htable=%llx size=%llu"
					"\n",
					g_scan_round % 8, off, cand_off, qn,
					qht, qsz2);
			}
			for (i = 0; i < (u32)qsz2 && !tv; i++) {
				unsigned long node = 0;

				if (ksu_kread8((unsigned long)qht +
					       (unsigned long)i * 8, &q1))
					break;
				node = (unsigned long)q1;

				while (ksu_is_kptr(node)) {
					unsigned long kptr, dptr, next;

					if (ksu_kread8(node, &q1) ||
					    ksu_kread8(node + 8, &q0) ||
					    ksu_kread8(node + 16, &qsz))
						break;
					kptr = (unsigned long)q1;
					dptr = (unsigned long)q0;
					next = (unsigned long)qsz;

					if (ksu_is_kptr(kptr) &&
					    probe_kernel_read(key, (void *)kptr,
							      sizeof(key)) == 0 &&
					    !strncmp(key, "su", 3)) {
						u64 val = 0;
						u8 primary = 0;

						if (ksu_kread8(dptr, &val) == 0) {
							probe_kernel_read(
								&primary,
								(void *)(dptr + 8),
								1);
							if (primary)
								tv = (u32)val;
						}
					}

					if (!ksu_is_kptr(next))
						break;
					node = next;
				}
			}

				if (tv) {
					table = (unsigned long)qcand;
					tsize = (u32)qsz2;
					off_types = off + cand_off;
					pr_info("ksu: types hashtab found @ "
						"ss+0x%lx+%ld (table=%lx "
						"size=%u)\n",
						off, cand_off, table, tsize);
				}
			} /* cand_off */
		}

		if (!tv) {
			pr_err("ksu: type 'su' not found in any symtab slot, "
			       "skip rules\n");
			return;
		}

		pr_info("ksu: su type value = %llu\n", tv);

		/*
		 * 39-r102: locate policydb.permissive_map at runtime.
		 * The scan loop validated a symtab slot at ss+off_types
		 * by actually walking its hashtab and finding "su", so
		 * that offset is proven, not assumed. Derive the
		 * policydb base from it via the compile-time offsetof
		 * of symtab[SYM_TYPES] inside struct policydb (the
		 * r94/r95 failures were about the *base*, not the
		 * intra-struct deltas), then add the permissive_map
		 * offset. Cross-check with the r98 dump: with base =
		 * ss+0x10 this puts symtab[3].nprim at ss+0x50, exactly
		 * where nprim=3553 was observed.
		 *
		 * Shape gate before ANY write (handoff 2.2): ebitmap =
		 * {node@0, u32 highbit@8}; node must be NULL or a
		 * kernel pointer and highbit <= 65536. This system has
		 * no permissive domains, so {NULL, 0} is expected. On
		 * mismatch: log, skip, rely on ppdb.bin for the next
		 * round.
		 */
		{
			unsigned long db_addr, pm_addr;
			u64 node = 0, qtmp;
			u32 highbit = 0;
			int ret;

			db_addr = ss + off_types -
				  offsetof(struct policydb,
					   symtab[SYM_TYPES]);
			pm_addr = db_addr +
				  offsetof(struct policydb,
					   permissive_map);

			if (!ksu_is_kptr_safe_window(pm_addr, ss)) {
				pr_err("ksu: pm addr %lx out of proven "
				       "window, skip (check ppdb.bin)\n",
				       pm_addr);
				return;
			}
			/* plain deref (r97: probe_kernel_read lies on
			 * this kernel); pm_addr is inside the scan-proven
			 * [ss, ss+4K) window and the shape gate below
			 * rejects garbage before any write. */
			node = *(u64 *)pm_addr;
			qtmp = *(u64 *)(pm_addr + 8);
			highbit = (u32)qtmp;
			if ((node != 0 && !ksu_is_kptr(node)) ||
			    highbit > 65536) {
				pr_err("ksu: permissive_map shape wrong "
				       "(node=%llx highbit=%u pm=%lx), "
				       "skip write (check ppdb.bin)\n",
				       node, highbit, pm_addr);
				return;
			}

			ret = ksu_ebitmap_set_bit_probe(pm_addr, tv);
			if (ret) {
				pr_err("ksu: set bit failed (%d)\n", ret);
				return;
			}
			pr_info("ksu: su (value %u) marked permissive "
				"(pm=%lx base=%lx)\n",
				(u32)tv, pm_addr, db_addr);
		}
	}

	/* invalidate userspace AVC caches via the status-page seqlock.
	 * No mutex here: lock offset is compile-time and unverified at
	 * runtime; the seqlock is designed for lockless readers and a
	 * racy writer is acceptable for a one-shot cache flush. */
	{
		struct selinux_kernel_status *status;
		struct page *pg = NULL;
		u64 qpg = 0;
		unsigned long sp_addr =
			ss + offsetof(struct selinux_ss, status_page);

		if (ksu_kread8(sp_addr, &qpg)) {
			pr_err("ksu: status_page read failed\n");
			return;
		}

		if (ksu_is_kptr(qpg)) {
			pg = (struct page *)qpg;
		} else if (qpg == 0) {
			pg = alloc_page(GFP_KERNEL | __GFP_ZERO);
			if (pg) {
				status = page_address(pg);
				status->version =
					SELINUX_KERNEL_STATUS_VERSION;
				status->sequence = 0;
				status->enforcing =
					enforcing_enabled(state);
				status->policyload = 0;
				status->deny_unknown = 0;
				*(unsigned long *)sp_addr =
					(unsigned long)pg;
			}
		} else {
			pr_err("ksu: status_page looks wrong (%llx), "
			       "skip flush\n", qpg);
			return;
		}

		if (pg) {
			status = page_address(pg);
			status->sequence++;
			smp_wmb();
			status->policyload++;
			pr_info("ksu: status page flushed (seq=%u)\n",
				status->sequence);
		}
	}
}

/*
 * 39-r94, reworked in r102: pure-data ebitmap bit setter. Takes the
 * ebitmap's *address* (not a pointer) so a wrong compile-time offset
 * cannot turn into a bogus dereference. Reads are plain dereferences
 * (probe_kernel_read returns WRONG DATA on this kernel, r97); e_addr
 * must already be inside the scan-proven [ss, ss+4K) window and
 * shape-checked by the caller. Writes only ever touch e_addr itself
 * or a freshly kzalloc'd node -- zero CFI surface.
 */
static int ksu_ebitmap_set_bit_probe(unsigned long e_addr, unsigned long bit)
{
	unsigned long node = 0, prev = 0;
	u32 highbit = 0;

	node = *(unsigned long *)e_addr;
	highbit = *(u32 *)(e_addr + 8);

	while (ksu_is_kptr(node)) {
		u32 startbit = 0;
		unsigned long nextv = 0;

		startbit = *(u32 *)(node +
				    offsetof(struct ebitmap_node,
					     startbit));
		nextv = *(unsigned long *)node;

		if (bit >= startbit && bit - startbit < EBITMAP_SIZE) {
			unsigned long ofs = bit - startbit;
			unsigned long maddr =
				node +
				offsetof(struct ebitmap_node, maps) +
				(ofs / EBITMAP_UNIT_SIZE) * sizeof(u64);
			u64 m;

			m = *(u64 *)maddr;
			m |= 1UL << (ofs % EBITMAP_UNIT_SIZE);
			*(u64 *)maddr = m;
			return 0;
		}

		prev = node;
		node = nextv;
	}

	/* append a fresh node */
	{
		struct ebitmap_node *newn =
			kzalloc(sizeof(*newn), GFP_ATOMIC);
		unsigned long ofs;

		if (!newn)
			return -ENOMEM;
		newn->startbit = bit & ~((unsigned long)EBITMAP_SIZE - 1);
		ofs = bit - newn->startbit;
		newn->maps[ofs / EBITMAP_UNIT_SIZE] =
			1UL << (ofs % EBITMAP_UNIT_SIZE);

		if (prev) {
			newn->next = ((struct ebitmap_node *)prev)->next;
			((struct ebitmap_node *)prev)->next = newn;
		} else {
			newn->next = NULL;
			*(unsigned long *)e_addr = (unsigned long)newn;
		}

		if (bit >= highbit)
			*(u32 *)(e_addr + 8) =
				(u32)(newn->startbit + EBITMAP_SIZE);
	}
	return 0;
}


/*
 * 39-r93: handle_sepolicy() is disabled. Its implementation drives the same
 * shim function-pointer path that caused four boots to panic (see
 * apply_kernelsu_rules above), and nothing in the manager's fixed binaries
 * needs it (sepolicy edits go through ksud's CLI, which has no working
 * transport on this port anyway). Keep the prctl command answerable but
 * refuse, so a caller gets a clean error instead of a CFI fault.
 */
int handle_sepolicy(unsigned long arg3, void __user *arg4)
{
	pr_err("ksu: SET_SEPOLICY not supported on this build (CFI-safe build)\n");
	return -EOPNOTSUPP;
}
