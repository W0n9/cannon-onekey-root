// SPDX-License-Identifier: GPL-2.0
/*
 * cannon: runtime resolution of `struct selinux_state` address.
 *
 * The stock cannon 4.14 kernel has the Android selinux_state backport but:
 *  - selinux_state is an unexported DATA symbol;
 *  - CONFIG_KALLSYMS_ALL=n -> kallsyms_lookup_name() cannot find data symbols.
 *
 * 39-r94: the original approach (disassemble a text anchor and decode its
 * ADRP/ADD pair) is retired. It never ran for real until this ticket and
 * then NULL-dereferenced twice in apply_kernelsu_rules ([ss+64] read from a
 * bogus state address): the first-ADRP-match heuristic is fragile against
 * ftrace patching and instruction-layout assumptions, and its sanity check
 * (enforcing byte == 0/1) passes on plenty of wrong addresses.
 *
 * Replacement is deterministic, mirroring what the t37 probe has verified
 * for 8+ boots (research/38-selinux-state-offset.md):
 *   selinux_state runtime VA = LINKED_VA + KASLR slide
 *   slide = kallsyms_lookup_name("_stext") - STEXT_LINKED_VA
 * Both linked VAs are fixed constants of this exact kernel Image. The
 * result is triple-checked with probe_kernel_read() (exported; a direct
 * call, no CFI hazard): avc@+0x10 and ss@+0x18 must look like kernel
 * pointers and initialized@+3 must be 1. On any failure we stay invalid
 * and callers skip cleanly.
 */

#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/printk.h>
#include <linux/uaccess.h>

#include "ksu_selinux_resolve.h"

/* linked-time VAs, this exact Image (4.14.186-perf-00243-g9a0d96109bda) */
#define KSU_STEXT_LINKED_VA 0xFFFFFF8008081000ULL
#define KSU_SELINUX_STATE_LINKED_VA 0xFFFFFF800AB47D90ULL

static unsigned long ksu_selinux_state_addr;
static bool ksu_selinux_state_valid;

static bool ksu_selinux_state_triple_check(unsigned long addr)
{
	u64 q0 = 0, qavc = 0, qss = 0;

	if (probe_kernel_read(&q0, (void *)addr, sizeof(q0)))
		return false;
	if (probe_kernel_read(&qavc, (void *)(addr + 0x10), sizeof(qavc)))
		return false;
	if (probe_kernel_read(&qss, (void *)(addr + 0x18), sizeof(qss)))
		return false;

	/* avc / ss must be kernel VAs; initialized@+3 must be 1 */
	if ((qavc >> 48) != 0xFFFF || (qss >> 48) != 0xFFFF)
		return false;
	if (((u8 *)&q0)[3] != 1)
		return false;

	return true;
}

int ksu_selinux_resolve_init(void)
{
	unsigned long stext_rt, slide, addr;

	stext_rt = kallsyms_lookup_name("_stext");
	if (!stext_rt) {
		pr_err("ksu_selinux: _stext not found\n");
		return -ENOENT;
	}

	slide = stext_rt - KSU_STEXT_LINKED_VA;
	addr = (unsigned long)KSU_SELINUX_STATE_LINKED_VA + slide;

	if (!ksu_selinux_state_triple_check(addr)) {
		pr_err("ksu_selinux: triple check failed (addr %px, slide %lx)\n",
		       (void *)addr, slide);
		ksu_selinux_state_addr = 0;
		return -EINVAL;
	}

	ksu_selinux_state_addr = addr;
	ksu_selinux_state_valid = true;
	pr_info("ksu_selinux: selinux_state @ %px (slide %lx)\n",
		(void *)addr, slide);
	return 0;
}

void ksu_selinux_resolve_exit(void)
{
	ksu_selinux_state_valid = false;
	ksu_selinux_state_addr = 0;
}

void *ksu_selinux_state(void)
{
	return (void *)ksu_selinux_state_addr;
}

void ksu_selinux_set_enforce(bool enforce)
{
	unsigned long addr = ksu_selinux_state_addr;

	if (!ksu_selinux_state_valid || !addr)
		return;
	/* selinux_state.enforcing, offset 1 (CONFIG_SECURITY_SELINUX_DEVELOP) */
	*(unsigned char *)(addr + 1) = enforce ? 1 : 0;
}

bool ksu_selinux_get_enforce()
{
	unsigned long addr = ksu_selinux_state_addr;

	if (!ksu_selinux_state_valid || !addr)
		return true;
	return *(unsigned char *)(addr + 1);
}
