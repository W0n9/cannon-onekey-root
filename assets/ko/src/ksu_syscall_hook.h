/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KSU_SYSCALL_HOOK_H
#define __KSU_SYSCALL_HOOK_H

/*
 * sys_call_table based syscall hooks for kernels WITHOUT CONFIG_KPROBES
 * and WITHOUT kallsyms data symbols (CONFIG_KALLSYMS_ALL=n).
 *
 * Target: Xiaomi cannon (MT6853) 4.14.186-perf stock kernel:
 *  - kprobes disabled in stock kernel (no register_kprobe symbol)
 *  - sys_call_table is a DATA symbol: not resolvable via kallsyms
 *    -> its address is recovered offline from the stock kernel Image
 *       (RELA.dyn relative-relocation analysis) and embedded below.
 *  - kernel is LTO+CFI_CLANG: module functions must never be called via
 *    CFI-instrumented indirect calls from kernel C code. arm64 syscall
 *    dispatch (entry.S `ldr x16, [stbl, xscno, lsl #3]; blr x16`) is
 *    uninstrumented assembly, so patching the table is CFI-safe.
 */

/*
 * offline constants recovered from 4.14.186-perf-00243-g9a0d96109bda Image
 *
 * Re-verified 2026-09-07 (issue #11) against the corrected RELA/kallsyms
 * analysis (research/kallsyms-addresses-corrected.txt, address = addend,
 * file = VA - 0xffffff8008080000):
 *  - SYS_CALL_TABLE 0xffffff8009a35000: confirmed EXACT. It is the r_offset
 *    of a 435-entry stride-8 R_AARCH64_RELATIVE run (r_offset = link VA of
 *    the slot itself, unaffected by the old addend+text_offset bug);
 *    all 435 addends resolve to sys_*.cfi_jt thunks (compat table at
 *    0xffffff8009a38000 likewise). Slots are zeroed in the Image at
 *    file offset VA-0xffffff8008080000, as RELA-populated data must be.
 *  - SYS_EXECVE: was 0xffffff80084b9c24 (polluted +0x80000: anchored from
 *    a file-offset search with the wrong pre-fix file base 0xffffff8008100000).
 *    Corrected to the kallsyms-verified VA; cross-checked by decoding
 *    sys_call_table[221] = sys_execve.cfi_jt @0xffffff800941c71c whose `b`
 *    lands exactly on 0xffffff8008439c24 = sys_execve (function-start
 *    prologue stp x29,x30 / stp x28...). With the old constant the runtime
 *    delta would be KASLR-0x80000 and the computed table pointer would sit
 *    0x80000 below the real sys_call_table (sanity check would refuse load).
 */
#define KSU_OFFLINE_SYS_CALL_TABLE 0xffffff8009a35000UL
#define KSU_OFFLINE_SYS_EXECVE     0xffffff8008439c24UL
#define KSU_NR_SYSCALLS            435

/* syscall numbers (native table) verified against the decoded table */
#define KSU_NR_FACCESSAT   48
#define KSU_NR_NEWFSTATAT  79
#define KSU_NR_PRCTL       167
#define KSU_NR_EXECVE      221
#define KSU_NR_EXECVEAT    281

/*
 * 38-r74 (supercall port): arm64 uses include/uapi/asm-generic/unistd.h, so
 * these live in the same numbering space as the entries above (verified at
 * cannon-r-oss/include/uapi/asm-generic/unistd.h:426 and :436-437):
 *   __NR_reboot 142, __NR_setuid 146, __NR_setresuid 147.
 * Both are inside the 435-entry native table.
 *
 * reboot   — installs the "[ksu_driver]" fd when called with the magic pair.
 *            Replaces upstream's __arm64_sys_reboot kprobe (no kprobes here)
 *            and, unlike upstream, does the install inline instead of via
 *            task_work_add (kernel/task_work.c exports nothing on 4.14).
 * setresuid — installs the fd when a process becomes the manager. The manager
 *            app never asks for one itself; it only scans /proc/self/fd, so
 *            without this hook it would never see a driver fd at all.
 */
#define KSU_NR_REBOOT      142
#define KSU_NR_SETRESUID   147

int ksu_syscall_hook_init(void);
void ksu_syscall_hook_exit(void);

/* 38-r76: install the supercall (reboot/setresuid) hooks on demand.
 * Idempotent. Called via the legacy prctl channel, CMD_SUPERCALL_ARM. */
int ksu_supercall_hook_arm(void);

#endif
