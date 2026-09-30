#include <linux/export.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include "allowlist.h"
#include "arch.h"
#include "core_hook.h"
#include "ksu_selinux_resolve.h"
#include "ksu_selinux_shim.h"
#include "selinux/selinux.h"
#include "ksu_syscall_hook.h"
#include "klog.h" // IWYU pragma: keep
#include "ksu.h"
#include "throne_tracker.h"

static struct workqueue_struct *ksu_workqueue;

bool ksu_queue_work(struct work_struct *work)
{
	return queue_work(ksu_workqueue, work);
}

extern int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr,
					void *argv, void *envp, int *flags);

extern int ksu_handle_execveat_ksud(int *fd, struct filename **filename_ptr,
				    void *argv, void *envp, int *flags);

int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv,
			void *envp, int *flags)
{
	ksu_handle_execveat_ksud(fd, filename_ptr, argv, envp, flags);
	return ksu_handle_execveat_sucompat(fd, filename_ptr, argv, envp,
					    flags);
}

extern void ksu_sucompat_init(void);
extern void ksu_sucompat_exit(void);
extern void ksu_ksud_init(void);
extern void ksu_ksud_exit(void);

int __init kernelsu_init(void)
{
#ifdef CONFIG_KSU_DEBUG
	pr_alert("*************************************************************");
	pr_alert("**     NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE    **");
	pr_alert("**                                                         **");
	pr_alert("**         You are running KernelSU in DEBUG mode          **");
	pr_alert("**                                                         **");
	pr_alert("**     NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE    **");
	pr_alert("*************************************************************");
#endif

	ksu_core_init();

	ksu_workqueue = alloc_ordered_workqueue("kernelsu_work_queue", 0);

	ksu_allowlist_init();

	/* 42-c6: no ksud here — on_post_fs_data() never fires, so the
	 * persisted allowlist was never reloaded across reboots (and saves
	 * could fail when /data/adb/ksu did not exist). Ensure the dir and
	 * load the list ourselves. Runs in the finit_module syscall context
	 * (sleepable); direct calls only, safe before the P4.2 de-CFI. */
	ksu_ensure_allowlist_dir();
	ksu_load_allow_list();

	ksu_throne_tracker_init();

#ifdef CONFIG_KPROBES
	ksu_sucompat_init();
	ksu_ksud_init();
#elif defined(KSU_SYSCALL_HOOK)
	/* cannon: stock kernel has no kprobes; hook via sys_call_table */
	ksu_selinux_shim_init();
	ksu_selinux_resolve_init();
	/*
	 * 39-r89 NOTE: do NOT call apply_kernelsu_rules() here. Its
	 * symbol-resolution trampolines are indirect calls (blr) to
	 * runtime-resolved kernel addresses, and every module indirect call
	 * goes through the module's own __cfi_check stubs — which stay armed
	 * until the probe NOPs them (P4.2, ~80ms after finit). Two boots
	 * (79e2241f, a028853a) both died in apply_kernelsu_rules+0xbc
	 * (blr x22 -> __cfi_check_fail) when this was called from
	 * kernelsu_init. The injection now rides the first GRANT_ROOT
	 * (ksu_supercall.c), which can only happen after de-CFI.
	 */
	ksu_syscall_hook_init();
#else
	pr_alert("KPROBES is disabled, KernelSU may not work, please check https://kernelsu.org/guide/how-to-integrate-for-non-gki.html");
#endif

#ifdef MODULE
#ifndef CONFIG_KSU_DEBUG
	kobject_del(&THIS_MODULE->mkobj.kobj);
#endif
#endif
	return 0;
}

void kernelsu_exit(void)
{
	ksu_allowlist_exit();

	ksu_throne_tracker_exit();

	destroy_workqueue(ksu_workqueue);

#ifdef CONFIG_KPROBES
	ksu_ksud_exit();
	ksu_sucompat_exit();
#endif
#ifdef KSU_SYSCALL_HOOK
	ksu_syscall_hook_exit();
	ksu_selinux_resolve_exit();
	ksu_selinux_shim_exit();
#endif

	ksu_core_exit();
}

module_init(kernelsu_init);
module_exit(kernelsu_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("weishu");
MODULE_DESCRIPTION("Android KernelSU");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
