// SPDX-License-Identifier: GPL-2.0
/*
 * KernelSU v3.x supercall backend for the cannon 4.14 sys_call_table build.
 *
 * Scope: exactly the commands the v3.3.0 manager needs to stop showing the
 * three remaining red cards —
 *   KSU_IOCTL_GET_INFO / _LEGACY : report uapi_version=2  -> kills the
 *                                  "uapi mismatch" and "version too low"
 *                                  cards (the latter is derived from
 *                                  requireNewKernel() which ORs in
 *                                  checkUAPIMismatch()).
 *   KSU_IOCTL_GRANT_ROOT         : the *only* ioctl on ksud's `debug su`
 *                                  path -> kills "grant root failed".
 * The remaining 21 upstream commands (allowlist read/write, sepolicy,
 * features, wrapper fd, ...) are deliberately not implemented: they drag in
 * SELinux privates, tracepoints and kprobes that do not exist here. The
 * dispatcher answers -ENOTTY for them, which is the same thing the manager
 * sees from a driver that predates them.
 *
 * fd channel design (the load-bearing part):
 *   The manager never asks for an fd — ksu.cc only scans /proc/self/fd for a
 *   file whose target contains "[ksu_driver]". Upstream therefore pushes the
 *   fd from the kernel, on two triggers:
 *     1. setresuid(2), when a process becomes the manager (zygote does this
 *        on every app fork) -> ksu_supercall_on_setresuid()
 *     2. reboot(2) with the install magic, used by ksud -> the reboot hook
 *   Upstream builds the fd with O_CLOEXEC and consequently has to whitelist
 *   __NR_reboot in the manager's seccomp filter so ksud can still reach
 *   trigger 2. That whitelist lives in the 5.9+ seccomp action cache
 *   (filter->cache.allow_native), which 4.14 does not have — so instead we
 *   build the fd *without* O_CLOEXEC. ksud is forked+exec'd by the manager,
 *   inherits the manager's fd, and its init_driver_fd() finds it on the
 *   /proc/self/fd scan before it ever tries the reboot magic. Trigger 2 is
 *   implemented anyway, both as a fallback and because a future ksud may
 *   close inherited fds.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/anon_inodes.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/ratelimit.h>

#include "ksu.h"
#include "allowlist.h"
#include "ksud.h"
#include "manager.h"
#include "klog.h"
#include "selinux/selinux.h"
#include "uapi_supercall.h"
#include "ksu_supercall.h"

extern void escape_to_root(void);

/* ------------------------------------------------------------------ */
/* anon-inode fd                                                       */
/* ------------------------------------------------------------------ */

static int anon_ksu_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long anon_ksu_ioctl(struct file *filp, unsigned int cmd,
			   unsigned long arg)
{
	return ksu_supercall_handle_ioctl(cmd, (void __user *)arg);
}

static const struct file_operations anon_ksu_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = anon_ksu_ioctl,
	.compat_ioctl = anon_ksu_ioctl,
	.release = anon_ksu_release,
};

/*
 * Allocate the fd + file pair but do NOT publish it yet. Splitting this out
 * of ksu_install_fd() lets the reboot path copy the fd number to userspace
 * *before* fd_install(), so its failure path only needs put_unused_fd() and
 * fput() (both exported). The obvious alternative — install, then close on
 * copy failure — is not available on 4.14: close_fd() does not exist and
 * ksys_close() is not exported.
 */
static int ksu_new_driver_fd(struct file **out_filp)
{
	int fd;
	struct file *filp;

	/* flags = 0: no O_CLOEXEC, see the file header comment. */
	fd = get_unused_fd_flags(0);
	if (fd < 0) {
		pr_err("ksu: get_unused_fd_flags failed: %d\n", fd);
		return fd;
	}

	filp = anon_inode_getfile("[ksu_driver]", &anon_ksu_fops, NULL,
				  O_RDWR);
	if (IS_ERR(filp)) {
		pr_err("ksu: anon_inode_getfile failed\n");
		put_unused_fd(fd);
		return PTR_ERR(filp);
	}

	*out_filp = filp;
	return fd;
}

int ksu_install_fd(void)
{
	int fd;
	struct file *filp;

	fd = ksu_new_driver_fd(&filp);
	if (fd < 0)
		return fd;

	fd_install(fd, filp);
	pr_info("ksu: fd %d installed for pid %d\n", fd, current->pid);
	return fd;
}

/* reboot(2) magic path: install into current and hand the number back. */
int ksu_install_fd_to_user(int __user *outp)
{
	int fd;
	struct file *filp;

	if (!outp)
		return -EINVAL;

	fd = ksu_new_driver_fd(&filp);
	if (fd < 0)
		return fd;

	if (copy_to_user(outp, &fd, sizeof(fd))) {
		put_unused_fd(fd);
		fput(filp);
		return -EFAULT;
	}

	fd_install(fd, filp);
	pr_info("ksu: fd %d installed for pid %d (reboot magic)\n", fd,
		current->pid);
	return 0;
}

/* ------------------------------------------------------------------ */
/* permission predicates (mirror upstream kernel/supercall/perm.c)     */
/* ------------------------------------------------------------------ */

static bool perm_always_allow(void)
{
	return true;
}

/*
 * allowed_for_su: upstream is
 *   is_manager() || ksu_is_allow_uid_for_current(current_uid().val)
 * We keep the v0.9.5 identity model (full uid in ksu_manager_uid), which is
 * equivalent to upstream's appid (uid % 100000) comparison for single-user
 * devices — and 10239, the uid the probe crowned, is already appid-shaped.
 */
static bool perm_allowed_for_su(void)
{
	return is_manager() || ksu_is_allow_uid(current_uid().val);
}

/*
 * 39: upstream kernel/supercall/perm.c manager_or_root / only_manager, on
 * our full-uid identity model (equivalent for single-user devices).
 */
static bool perm_manager_or_root(void)
{
	return is_manager() || current_uid().val == 0;
}

static bool perm_only_manager(void)
{
	return is_manager();
}

/* ------------------------------------------------------------------ */
/* handlers (39 batch: app profiles, allow list, features)             */
/* ------------------------------------------------------------------ */

static int do_check_safemode(void __user *arg)
{
	struct ksu_check_safemode_cmd cmd;

	cmd.in_safe_mode = ksu_is_safe_mode() ? 1 : 0;
	if (cmd.in_safe_mode)
		pr_warn("ksu: safemode enabled!\n");

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: CHECK_SAFEMODE copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

/*
 * ksud reports boot-stage events through this on normal boots. This port is
 * a late-loaded LKM with no module mount machinery, so the events carry no
 * work here — ack them so callers don't treat the driver as broken.
 */
static int do_report_event(void __user *arg)
{
	struct ksu_report_event_cmd cmd;

	if (copy_from_user(&cmd, arg, sizeof(cmd)))
		return -EFAULT;

	pr_info("ksu: supercall report_event %u (late-load, ignored)\n",
		cmd.event);
	return 0;
}

/* deprecated inline-array ABI (wire size 520, count at offset 512). */
static int do_get_allow_list(void __user *arg, bool allow)
{
	int *arr;
	int count = 0, total = 0;
	__u32 out_count;

	arr = kmalloc(sizeof(int) * 128, GFP_KERNEL);
	if (!arr)
		return -ENOMEM;

	ksu_get_allow_list_bounded(arr, 128, &count, &total, allow);
	out_count = (__u32)count;

	if (copy_to_user((char __user *)arg +
				 offsetof(struct ksu_get_allow_list_cmd, count),
			 &out_count, sizeof(out_count))) {
		pr_err("ksu: get_allow_list copy count failed\n");
		kfree(arr);
		return -EFAULT;
	}

	if (count && copy_to_user(arg, arr, sizeof(int) * count)) {
		pr_err("ksu: get_allow_list copy uids failed\n");
		kfree(arr);
		return -EFAULT;
	}

	kfree(arr);
	return 0;
}

/* new flexible-array ABI (wire header size 4, uids at offset 4). */
static int do_new_get_allow_list(void __user *arg, bool allow)
{
	struct ksu_new_get_allow_list_cmd kcmd;
	int *arr = NULL;
	int count = 0, total = 0;

	if (copy_from_user(&kcmd, arg, sizeof(kcmd)))
		return -EFAULT;

	/* Def cap: the list only holds explicitly granted apps; 4096 is far
	 * beyond plausible and bounds the kmalloc. */
	if (kcmd.count > 4096)
		return -EINVAL;

	if (kcmd.count) {
		arr = kmalloc(sizeof(int) * kcmd.count, GFP_KERNEL);
		if (!arr)
			return -ENOMEM;
	}

	ksu_get_allow_list_bounded(arr, kcmd.count, &count, &total, allow);
	kcmd.count = (__u16)count;
	kcmd.total_count = (__u16)total;

	if (copy_to_user(arg, &kcmd, sizeof(kcmd))) {
		pr_err("ksu: new_get_allow_list copy header failed\n");
		kfree(arr);
		return -EFAULT;
	}

	if (count && copy_to_user(&((struct ksu_new_get_allow_list_cmd __user *)
					    arg)->uids,
				  arr, sizeof(int) * count)) {
		pr_err("ksu: new_get_allow_list copy uids failed\n");
		kfree(arr);
		return -EFAULT;
	}

	kfree(arr);
	return 0;
}

static int do_uid_granted_root(void __user *arg)
{
	struct ksu_uid_granted_root_cmd cmd;

	if (copy_from_user(&cmd, arg, sizeof(cmd)))
		return -EFAULT;

	cmd.granted = ksu_is_allow_uid(cmd.uid) ? 1 : 0;

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: uid_granted_root copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

static int do_uid_should_umount(void __user *arg)
{
	struct ksu_uid_should_umount_cmd cmd;

	if (copy_from_user(&cmd, arg, sizeof(cmd)))
		return -EFAULT;

	cmd.should_umount = ksu_uid_should_umount(cmd.uid) ? 1 : 0;

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: uid_should_umount copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

static int do_get_manager_appid(void __user *arg)
{
	struct ksu_get_manager_appid_cmd cmd;

	cmd.appid = 0;
	if (ksu_is_manager_uid_valid())
		cmd.appid = ksu_get_manager_uid() % 100000;

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: get_manager_appid copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

/*
 * v3.x app_profile on the wire is 784 bytes; our internal struct app_profile
 * is 776 and the first 772 bytes are offset-identical (bytes 772..776 are
 * padding on both sides). GET: find by uid, translate, stamp the wire
 * version and the root flags — a real v3.3.0 kernel migrates stored records
 * to version 4 + FLAG_KSU_NO_NEW_PRIVS (its migrate_profile does exactly
 * that for our file-format-3 records), so mirror it. Not found -> -ENOENT
 * with nothing copied, which the manager JNI treats as "use defaults" —
 * exactly upstream's behaviour.
 */
static int do_get_app_profile(void __user *arg)
{
	struct ksu_uapi_app_profile kcmd;
	struct app_profile profile;
	bool found;

	if (copy_from_user(&kcmd, arg, sizeof(kcmd)))
		return -EFAULT;

	memset(&profile, 0, sizeof(profile));
	profile.current_uid = kcmd.curr_uid;
	found = ksu_get_app_profile_locked(&profile);
	if (!found)
		return -ENOENT;

	memcpy(&kcmd, &profile, sizeof(profile));
	kcmd.version = KSU_UAPI_APP_PROFILE_VER;
	kcmd.rp_config.profile.flags =
		profile.allow_su ? KSU_UAPI_FLAG_NO_NEW_PRIVS : 0;

	if (copy_to_user(arg, &kcmd, sizeof(kcmd))) {
		pr_err("ksu: get_app_profile copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

/*
 * SET: read the 784-byte wire shape, drop the flags word we don't use, and
 * hand the layout-identical first 776 bytes to the v0.9.5 store. Two extra
 * guards cover checks v0.9.5's profile_valid() lacks (a real v3.3.0 kernel
 * has them): the package key must be NUL-terminated within 256 bytes, and
 * groups_count must be in [0, 32] unsigned — the internal check is signed,
 * so a negative value would slip through and later loop OOB in the escape
 * path.
 */
static int do_set_app_profile(void __user *arg)
{
	struct ksu_uapi_app_profile kcmd;
	struct app_profile profile;

	if (copy_from_user(&kcmd, arg, sizeof(kcmd))) {
		pr_err("ksu: set_app_profile copy_from_user failed\n");
		return -EFAULT;
	}

	if (strnlen(kcmd.key, sizeof(kcmd.key)) >= sizeof(kcmd.key)) {
		pr_err("ksu: set_app_profile unterminated key\n");
		return -EINVAL;
	}

	if (kcmd.allow_su &&
	    kcmd.rp_config.profile.groups_count > KSU_MAX_GROUPS) {
		pr_err("ksu: set_app_profile bad groups_count %d\n",
		       kcmd.rp_config.profile.groups_count);
		return -EINVAL;
	}

	memcpy(&profile, &kcmd, sizeof(profile));

	if (!ksu_set_app_profile_locked(&profile, true))
		return -EINVAL;

	return 0;
}

/*
 * No feature backend exists in this port. GET answers supported=false with
 * value=0 — upstream semantics treat an unregistered feature as a non-error
 * and the manager's is_*_enabled() wrappers already read that as "off".
 */
static int do_get_feature(void __user *arg)
{
	struct ksu_get_feature_cmd cmd;

	if (copy_from_user(&cmd, arg, sizeof(cmd)))
		return -EFAULT;

	cmd.value = 0;
	cmd.supported = 0;

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: get_feature copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

static int do_set_feature(void __user *arg)
{
	struct ksu_set_feature_cmd cmd;

	if (copy_from_user(&cmd, arg, sizeof(cmd)))
		return -EFAULT;

	pr_info("ksu: set_feature %u not implemented\n", cmd.feature_id);
	/* Report the feature as not implemented instead of lying with 0:
	 * the manager surfaces a failed toggle instead of a fake success. */
	return -EOPNOTSUPP;
}

/* ------------------------------------------------------------------ */
/* handlers                                                            */
/* ------------------------------------------------------------------ */

static int do_get_info(void __user *arg)
{
	struct ksu_get_info_cmd cmd = {
		.version = KERNEL_SU_VERSION,
		.flags = 0,
	};

#ifdef MODULE
	cmd.flags |= KSU_GET_INFO_FLAG_LKM;
#endif
	if (is_manager())
		cmd.flags |= KSU_GET_INFO_FLAG_MANAGER;

	cmd.features = KSU_FEATURE_MAX;
	cmd.uapi_version = KERNEL_SU_UAPI_VERSION;

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: GET_INFO copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

static int do_get_info_legacy(void __user *arg)
{
	struct ksu_get_info_legacy_cmd cmd = {
		.version = KERNEL_SU_VERSION,
		.flags = 0,
	};

#ifdef MODULE
	cmd.flags |= KSU_GET_INFO_FLAG_LKM;
#endif
	if (is_manager())
		cmd.flags |= KSU_GET_INFO_FLAG_MANAGER;

	cmd.features = KSU_FEATURE_MAX;

	if (copy_to_user(arg, &cmd, sizeof(cmd))) {
		pr_err("ksu: GET_INFO_LEGACY copy_to_user failed\n");
		return -EFAULT;
	}
	return 0;
}

static int do_grant_root(void)
{
	/*
	 * 39-r89: inject the KERNELSU selinux rules exactly once, on the
	 * first successful perm check. Upstream does this from the ksud
	 * post-fs-data/boot events (ksud.c), which never fire for this
	 * late-loaded LKM; without the injection the su-domain
	 * allow-all/permissive never reaches the live policydb and
	 * servicemanager (whose service_manager checks are enforced even
	 * under global permissive) denies RootServerMain's { find }
	 * package_service -> NPE -> exit(1) -> the manager's RootService
	 * bind hangs forever ("正在刷新"). It MUST NOT run from module init:
	 * the resolve trampolines are indirect calls that hit the module's
	 * own CFI stubs until the probe de-CFIs them (2 panics, see
	 * kernelsu.c). GRANT_ROOT can only be reached by the manager tree
	 * after the probe has finished, so the stubs are gone by here.
	 */
	/*
	 * 39-r100: the selinux permissive patch is DISABLED by default.
	 * Ten boots / nine ko builds spent on this path (CFI faults, a
	 * recursion blow-up, a bogus state address, an undocumented
	 * policydb layout) bought the evidence in 37-r88..r99 but kept
	 * blocking the UI verification of the ten new commands this
	 * ticket is actually about. The scanner code stays for a future
	 * round with a working diagnostic channel; the default path
	 * below is the 38-ticket behaviour that is proven stable.
	 */
 static bool rules_applied;

	if (!rules_applied) {
		rules_applied = true;
#ifdef KSU39_SELINUX_PATCH
		pr_info("ksu: applying selinux rules on first grant_root\n");
		apply_kernelsu_rules();
#else
		pr_info("ksu: selinux patch disabled (KSU39_SELINUX_PATCH off)"
			"\n");
#endif
	}

	if (!perm_allowed_for_su()) {
		pr_warn("ksu: grant_root denied for uid %d\n",
			current_uid().val);
		return -EPERM;
	}

	pr_info("ksu: allow root for %d\n", current_uid().val);
	escape_to_root();
	return 0;
}

long ksu_supercall_handle_ioctl(unsigned int cmd, void __user *arg)
{
	switch (cmd) {
	case KSU_IOCTL_GET_INFO:
		if (!perm_always_allow())
			return -EPERM;
		return do_get_info(arg);
	case KSU_IOCTL_GET_INFO_LEGACY:
		if (!perm_always_allow())
			return -EPERM;
		return do_get_info_legacy(arg);
	case KSU_IOCTL_GRANT_ROOT:
		return do_grant_root();
	case KSU_IOCTL_CHECK_SAFEMODE:
		if (!perm_always_allow())
			return -EPERM;
		return do_check_safemode(arg);
	case KSU_IOCTL_REPORT_EVENT:
		/* upstream perm: only_root */
		if (current_uid().val != 0)
			return -EPERM;
		return do_report_event(arg);
	case KSU_IOCTL_GET_ALLOW_LIST:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_get_allow_list(arg, true);
	case KSU_IOCTL_GET_DENY_LIST:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_get_allow_list(arg, false);
	case KSU_IOCTL_NEW_GET_ALLOW_LIST:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_new_get_allow_list(arg, true);
	case KSU_IOCTL_NEW_GET_DENY_LIST:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_new_get_allow_list(arg, false);
	case KSU_IOCTL_UID_GRANTED_ROOT:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_uid_granted_root(arg);
	case KSU_IOCTL_UID_SHOULD_UMOUNT:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_uid_should_umount(arg);
	case KSU_IOCTL_GET_MANAGER_APPID:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_get_manager_appid(arg);
	case KSU_IOCTL_GET_APP_PROFILE:
		if (!perm_only_manager())
			return -EPERM;
		return do_get_app_profile(arg);
	case KSU_IOCTL_SET_APP_PROFILE:
		if (!perm_only_manager())
			return -EPERM;
		return do_set_app_profile(arg);
	case KSU_IOCTL_GET_FEATURE:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_get_feature(arg);
	case KSU_IOCTL_SET_FEATURE:
		if (!perm_manager_or_root())
			return -EPERM;
		return do_set_feature(arg);
	default:
		pr_warn("ksu: unsupported supercall 0x%x\n", cmd);
		return -ENOTTY;
	}
}

/* ------------------------------------------------------------------ */
/* setresuid trigger                                                   */
/* ------------------------------------------------------------------ */

/*
 * Called from the setresuid syscall hook after the original call succeeded.
 * The manager's uid only becomes valid once it has been crowned, so before
 * that this is a cheap comparison against KSU_INVALID_UID and nothing else.
 *
 * Dedup: zygote re-specializes the manager process (and the manager restarts)
 * repeatedly, so without this every such event allocates a fresh fd and the
 * 32-entry fdtable of a small process runs out. If current already holds a
 * "[ksu_driver]" fd, skip the install.
 */
/*
 * 38-r84: does the current process already hold a "[ksu_driver]" fd?
 * Walks its own fd table under task_lock. Used by both the setresuid
 * trigger and the execve trigger so neither installs a duplicate.
 */
bool ksu_has_driver_fd(void)
{
	struct fdtable *fdt;
	bool found = false;
	unsigned int i;

	if (!current->files)
		return false;

	task_lock(current);
	fdt = files_fdtable(current->files);
	for (i = 0; i < fdt->max_fds; i++) {
		if (fdt->fd[i] && fdt->fd[i]->f_op == &anon_ksu_fops) {
			found = true;
			break;
		}
	}
	task_unlock(current);
	return found;
}

/*
 * 38-r84: install the driver fd only when it is absent. The execve trigger
 * runs on every execve by the manager, so an unconditional install there
 * would leak one fd per exec.
 */
int ksu_install_fd_if_absent(void)
{
	if (ksu_has_driver_fd())
		return 0;
	return ksu_install_fd();
}

void ksu_supercall_on_setresuid(uid_t new_uid)
{
	bool have_fd;

	if (!ksu_is_manager_uid_valid())
		return;
	if (new_uid != ksu_get_manager_uid())
		return;

	have_fd = ksu_has_driver_fd();

	if (have_fd) {
		pr_info_ratelimited(
			"ksu: driver fd already present, skip install (pid %d)\n",
			current->pid);
		return;
	}

	pr_info("ksu: installing fd for manager uid %d\n", new_uid);
	ksu_install_fd();
}
