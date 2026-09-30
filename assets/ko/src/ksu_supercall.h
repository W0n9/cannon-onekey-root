/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KSU_SUPERCALL_H
#define __KSU_SUPERCALL_H

/*
 * KernelSU v3.x supercall backend (anon-inode fd + ioctl dispatch) for the
 * cannon 4.14 sys_call_table hook build.
 *
 * Why this exists: manager v3.3.0 reads the driver's UAPI version through
 * KSU_IOCTL_GET_INFO on an anon-inode fd named "[ksu_driver]". The v0.9.5
 * prctl ABI cannot express that field at all, so get_info() in the manager
 * falls back to the legacy ioctl path and hardcodes uapi_version = 0, which
 * lights up the "uapi mismatch" and "version too low" red cards. Likewise
 * ksud (the manager's libksud.so) obtains root solely through
 * KSU_IOCTL_GRANT_ROOT. Both are reachable only via an fd, so the fd channel
 * is the load-bearing part of this port.
 *
 * Upstream implements the channel with a kprobe on __arm64_sys_reboot plus
 * task_work_add. Neither is available here (CONFIG_KPROBES=n; task_work.c
 * exports nothing), so the reboot syscall is fully taken over through
 * sys_call_table instead and the fd is installed inline.
 */

/* Install a "[ksu_driver]" anon-inode fd into the current process.
 * Returns the fd number, or a negative errno. */
int ksu_install_fd(void);

/* reboot(2) magic path: install the fd into the current process and write
 * its number back to userspace. Returns 0 or a negative errno. */
int ksu_install_fd_to_user(int __user *outp);

/* ioctl dispatch — called from the anon-inode file_operations. */
long ksu_supercall_handle_ioctl(unsigned int cmd, void __user *arg);

/* Called from the setresuid hook: install the fd when a process becomes the
 * manager. new_uid is the euid the syscall just set. */
void ksu_supercall_on_setresuid(uid_t new_uid);

/* 38-r84: fd-table probe + idempotent install, used by the execve trigger
 * so the manager's forked children re-acquire the fd without leaking one
 * per exec. Neither touches user memory. */
bool ksu_has_driver_fd(void);
int ksu_install_fd_if_absent(void);

#endif /* __KSU_SUPERCALL_H */
