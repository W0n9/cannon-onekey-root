#include <linux/compiler.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/namei.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
#include <linux/compiler_types.h>
#endif

#include "ksu.h"
#include "klog.h" // IWYU pragma: keep
#include "selinux/selinux.h"
#include "kernel_compat.h"
#include "allowlist.h"
#include "manager.h"

#define FILE_MAGIC 0x7f4b5355 // ' KSU', u32
#define FILE_FORMAT_VERSION 3 // u32

#define KSU_APP_PROFILE_PRESERVE_UID 9999 // NOBODY_UID
#define KSU_DEFAULT_SELINUX_DOMAIN "u:r:su:s0"

static DEFINE_MUTEX(allowlist_mutex);

// default profiles, these may be used frequently, so we cache it
static struct root_profile default_root_profile;
static struct non_root_profile default_non_root_profile;

static int allow_list_arr[PAGE_SIZE / sizeof(int)] __read_mostly __aligned(PAGE_SIZE);
static int allow_list_pointer __read_mostly = 0;

static void remove_uid_from_arr(uid_t uid)
{
	int *temp_arr;
	int i, j;

	if (allow_list_pointer == 0)
		return;

	temp_arr = kmalloc(sizeof(allow_list_arr), GFP_KERNEL);
	if (temp_arr == NULL) {
		pr_err("%s: unable to allocate memory\n", __func__);
		return;
	}

	for (i = j = 0; i < allow_list_pointer; i++) {
		if (allow_list_arr[i] == uid)
			continue;
		temp_arr[j++] = allow_list_arr[i];
	}

	allow_list_pointer = j;

	for (; j < ARRAY_SIZE(allow_list_arr); j++)
		temp_arr[j] = -1;

	memcpy(&allow_list_arr, temp_arr, PAGE_SIZE);
	kfree(temp_arr);
}

static void init_default_profiles()
{
	default_root_profile.uid = 0;
	default_root_profile.gid = 0;
	default_root_profile.groups_count = 1;
	default_root_profile.groups[0] = 0;
	memset(&default_root_profile.capabilities, 0xff,
	       sizeof(default_root_profile.capabilities));
	default_root_profile.namespaces = 0;
	strcpy(default_root_profile.selinux_domain, KSU_DEFAULT_SELINUX_DOMAIN);

	// This means that we will umount modules by default!
	default_non_root_profile.umount_modules = true;
}

struct perm_data {
	struct list_head list;
	struct app_profile profile;
};

static struct list_head allow_list;

static uint8_t allow_list_bitmap[PAGE_SIZE] __read_mostly __aligned(PAGE_SIZE);
#define BITMAP_UID_MAX ((sizeof(allow_list_bitmap) * BITS_PER_BYTE) - 1)

#define KERNEL_SU_ALLOWLIST "/data/adb/ksu/.allowlist"

static struct work_struct ksu_save_work;
static struct work_struct ksu_load_work;

bool persistent_allow_list(void);

/* 42-c6: cannon integration has no ksud, so /data/adb/ksu was never
 * created by anyone reliable and on_post_fs_data() never fires (the only
 * ksu_load_allow_list() caller). Ensure the dir here; kernelsu_init()
 * then calls ksu_load_allow_list() right after ksu_allowlist_init().
 * finit_module context is sleepable; direct calls only (CFI-safe before
 * the probe's P4.2 de-CFI window). */
void ksu_ensure_allowlist_dir(void)
{
	struct path path;
	struct dentry *dentry;
	int err;

	if (!kern_path("/data/adb/ksu", LOOKUP_DIRECTORY, &path)) {
		path_put(&path);
		return;
	}
	dentry = kern_path_create(AT_FDCWD, "/data/adb/ksu", &path,
				  LOOKUP_DIRECTORY);
	if (IS_ERR(dentry)) {
		pr_err("allowlist: create /data/adb/ksu fail: %ld\n",
		       PTR_ERR(dentry));
		return;
	}
	err = vfs_mkdir(path.dentry->d_inode, dentry, 0700);
	if (err && err != -EEXIST)
		pr_err("allowlist: mkdir /data/adb/ksu fail: %d\n", err);
	else
		pr_info("allowlist: /data/adb/ksu ensured\n");
	done_path_create(&path, dentry);
}

void ksu_show_allow_list(void)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	pr_info("ksu_show_allow_list\n");
	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		pr_info("uid :%d, allow: %d\n", p->profile.current_uid,
			p->profile.allow_su);
	}
}

#ifdef CONFIG_KSU_DEBUG
static void ksu_grant_root_to_shell()
{
	struct app_profile profile = {
		.allow_su = true,
		.current_uid = 2000,
	};
	strcpy(profile.key, "com.android.shell");
	strcpy(profile.rp_config.profile.selinux_domain, KSU_DEFAULT_SELINUX_DOMAIN);
	ksu_set_app_profile(&profile, false);
}
#endif

bool ksu_get_app_profile(struct app_profile *profile)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	bool found = false;

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		bool uid_match = profile->current_uid == p->profile.current_uid;
		if (uid_match) {
			// found it, override it with ours
			memcpy(profile, &p->profile, sizeof(*profile));
			found = true;
			goto exit;
		}
	}

exit:
	return found;
}

static inline bool forbid_system_uid(uid_t uid) {
	#define SHELL_UID 2000
	#define SYSTEM_UID 1000
	return uid < SHELL_UID && uid != SYSTEM_UID;
}

static bool profile_valid(struct app_profile *profile)
{
	if (!profile) {
		return false;
	}

	if (forbid_system_uid(profile->current_uid)) {
		pr_err("uid lower than 2000 is unsupported: %d\n", profile->current_uid);
		return false;
	}

	if (profile->version < KSU_APP_PROFILE_VER) {
		pr_info("Unsupported profile version: %d\n", profile->version);
		return false;
	}

	if (profile->allow_su) {
		if (profile->rp_config.profile.groups_count > KSU_MAX_GROUPS) {
			return false;
		}

		if (strlen(profile->rp_config.profile.selinux_domain) == 0) {
			return false;
		}
	}

	return true;
}

bool ksu_set_app_profile(struct app_profile *profile, bool persist)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	bool result = false;

	if (!profile_valid(profile)) {
		pr_err("Failed to set app profile: invalid profile!\n");
		return false;
	}

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		// both uid and package must match, otherwise it will break multiple package with different user id
		if (profile->current_uid == p->profile.current_uid &&
		    !strcmp(profile->key, p->profile.key)) {
			// found it, just override it all!
			memcpy(&p->profile, profile, sizeof(*profile));
			result = true;
			goto out;
		}
	}

	// not found, alloc a new node!
	p = (struct perm_data *)kmalloc(sizeof(struct perm_data), GFP_KERNEL);
	if (!p) {
		pr_err("ksu_set_app_profile alloc failed\n");
		return false;
	}

	memcpy(&p->profile, profile, sizeof(*profile));
	if (profile->allow_su) {
		pr_info("set root profile, key: %s, uid: %d, gid: %d, context: %s\n",
			profile->key, profile->current_uid,
			profile->rp_config.profile.gid,
			profile->rp_config.profile.selinux_domain);
	} else {
		pr_info("set app profile, key: %s, uid: %d, umount modules: %d\n",
			profile->key, profile->current_uid,
			profile->nrp_config.profile.umount_modules);
	}
	list_add_tail(&p->list, &allow_list);

out:
	if (profile->current_uid <= BITMAP_UID_MAX) {
		if (profile->allow_su)
			allow_list_bitmap[profile->current_uid / BITS_PER_BYTE] |= 1 << (profile->current_uid % BITS_PER_BYTE);
		else
			allow_list_bitmap[profile->current_uid / BITS_PER_BYTE] &= ~(1 << (profile->current_uid % BITS_PER_BYTE));
	} else {
		if (profile->allow_su) {
			/*
			 * 1024 apps with uid higher than BITMAP_UID_MAX
			 * registered to request superuser?
			 */
			if (allow_list_pointer >= ARRAY_SIZE(allow_list_arr)) {
				pr_err("too many apps registered\n");
				WARN_ON(1);
				return false;
			}
			allow_list_arr[allow_list_pointer++] = profile->current_uid;
		} else {
			remove_uid_from_arr(profile->current_uid);
		}
	}
	result = true;

	// check if the default profiles is changed, cache it to a single struct to accelerate access.
	if (unlikely(!strcmp(profile->key, "$"))) {
		// set default non root profile
		memcpy(&default_non_root_profile, &profile->nrp_config.profile,
		       sizeof(default_non_root_profile));
	}

	if (unlikely(!strcmp(profile->key, "#"))) {
		// set default root profile
		memcpy(&default_root_profile, &profile->rp_config.profile,
		       sizeof(default_root_profile));
	}

	if (persist)
		persistent_allow_list();

	return result;
}

bool __ksu_is_allow_uid(uid_t uid)
{
	int i;

	if (unlikely(uid == 0)) {
		// already root, but only allow our domain.
		return is_ksu_domain();
	}

	if (forbid_system_uid(uid)) {
		// do not bother going through the list if it's system
		return false;
	}

	if (likely(ksu_is_manager_uid_valid()) && unlikely(ksu_get_manager_uid() == uid)) {
		// manager is always allowed!
		return true;
	}

	if (likely(uid <= BITMAP_UID_MAX)) {
		return !!(allow_list_bitmap[uid / BITS_PER_BYTE] & (1 << (uid % BITS_PER_BYTE)));
	} else {
		for (i = 0; i < allow_list_pointer; i++) {
			if (allow_list_arr[i] == uid)
				return true;
		}
	}

	return false;
}

bool ksu_uid_should_umount(uid_t uid)
{
	struct app_profile profile = { .current_uid = uid };
	if (likely(ksu_is_manager_uid_valid()) && unlikely(ksu_get_manager_uid() == uid)) {
		// we should not umount on manager!
		return false;
	}
	bool found = ksu_get_app_profile(&profile);
	if (!found) {
		// no app profile found, it must be non root app
		return default_non_root_profile.umount_modules;
	}
	if (profile.allow_su) {
		// if found and it is granted to su, we shouldn't umount for it
		return false;
	} else {
		// found an app profile
		if (profile.nrp_config.use_default) {
			return default_non_root_profile.umount_modules;
		} else {
			return profile.nrp_config.profile.umount_modules;
		}
	}
}

struct root_profile *ksu_get_root_profile(uid_t uid)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		if (uid == p->profile.current_uid && p->profile.allow_su) {
			if (!p->profile.rp_config.use_default) {
				return &p->profile.rp_config.profile;
			}
		}
	}

	// use default profile
	return &default_root_profile;
}

bool ksu_get_allow_list(int *array, int *length, bool allow)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	int i = 0;
	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		// pr_info("get_allow_list uid: %d allow: %d\n", p->uid, p->allow);
		if (p->profile.allow_su == allow) {
			array[i++] = p->profile.current_uid;
		}
	}
	*length = i;

	return true;
}

/*
 * 39: capacity-safe allow list read for the v3.x supercall ABI. Unlike
 * ksu_get_allow_list() this cannot overflow: entries beyond capacity are
 * dropped from the array but still counted, and *out_total reports the true
 * number so the NEW_GET_ALLOW_LIST handler can fill total_count. Runs under
 * the allowlist mutex, which the plain readers above never take.
 */
bool ksu_get_allow_list_bounded(int *array, int capacity, int *out_count,
	int *out_total, bool allow)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	int i = 0, total = 0;
	bool have_manager = ksu_is_manager_uid_valid();
	uid_t manager_uid = ksu_get_manager_uid();

	mutex_lock(&allowlist_mutex);
	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		/* upstream skips the manager itself (is_uid_manager) */
		if (have_manager && p->profile.current_uid == manager_uid)
			continue;
		if (p->profile.allow_su != allow)
			continue;
		if (i < capacity)
			array[i++] = p->profile.current_uid;
		total++;
	}
	mutex_unlock(&allowlist_mutex);

	if (out_count)
		*out_count = i;
	if (out_total)
		*out_total = total;
	return true;
}

/*
 * 39: ksu_get_app_profile() under the allowlist mutex. The supercall
 * GET_APP_PROFILE handler can then read a consistent copy without racing a
 * concurrent ksu_set_app_profile() list mutation.
 */
bool ksu_get_app_profile_locked(struct app_profile *profile)
{
	bool found;

	mutex_lock(&allowlist_mutex);
	found = ksu_get_app_profile(profile);
	mutex_unlock(&allowlist_mutex);
	return found;
}

/*
 * 39: ksu_set_app_profile() under the allowlist mutex, for the supercall
 * SET_APP_PROFILE handler. The existing callers (boot load, core_hook prctl)
 * run long before the manager can issue an ioctl, so only the ioctl path
 * needs the lock — keeping it here avoids touching the upstream function.
 */
bool ksu_set_app_profile_locked(struct app_profile *profile, bool persist)
{
	bool result;

	mutex_lock(&allowlist_mutex);
	result = ksu_set_app_profile(profile, persist);
	mutex_unlock(&allowlist_mutex);
	return result;
}

/* 43-c1: cannon inline persistence runs in the CALLER's context (manager
 * supercall = uid 10239), but /data/adb is 0700 root:root — the save's
 * filp_open died with EACCES at the /data/adb component and nothing ever
 * persisted. Upstream never saw this: its save runs on a kworker with
 * init_cred. Override to a kernel-root cred for the file ops; SELinux is
 * permissive post-chain on this device, DAC was the blocker. */
static const struct cred *ksu_persist_cred;

static void __do_save_allow_list(void)
{
	u32 magic = FILE_MAGIC;
	u32 version = FILE_FORMAT_VERSION;
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	loff_t off = 0;

	struct file *fp =
		ksu_filp_open_compat(KERNEL_SU_ALLOWLIST, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (IS_ERR(fp)) {
		pr_err("save_allow_list create file failed: %ld\n", PTR_ERR(fp));
		return;
	}

	// store magic and version
	if (ksu_kernel_write_compat(fp, &magic, sizeof(magic), &off) !=
	    sizeof(magic)) {
		pr_err("save_allow_list write magic failed.\n");
		goto exit;
	}

	if (ksu_kernel_write_compat(fp, &version, sizeof(version), &off) !=
	    sizeof(version)) {
		pr_err("save_allow_list write version failed.\n");
		goto exit;
	}

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		pr_info("save allow list, name: %s uid :%d, allow: %d\n",
			p->profile.key, p->profile.current_uid,
			p->profile.allow_su);

		ksu_kernel_write_compat(fp, &p->profile, sizeof(p->profile),
					&off);
	}

exit:
	filp_close(fp, 0);
}

void do_save_allow_list(struct work_struct *work)
{
	const struct cred *old_cred;

	if (!ksu_persist_cred) {
		__do_save_allow_list();
		return;
	}
	old_cred = override_creds(ksu_persist_cred);
	__do_save_allow_list();
	revert_creds(old_cred);
}

void do_load_allow_list(struct work_struct *work)
{
	loff_t off = 0;
	ssize_t ret = 0;
	struct file *fp = NULL;
	u32 magic;
	u32 version;

#ifdef CONFIG_KSU_DEBUG
	// always allow adb shell by default
	ksu_grant_root_to_shell();
#endif

	// load allowlist now!
	fp = ksu_filp_open_compat(KERNEL_SU_ALLOWLIST, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_err("load_allow_list open file failed: %ld\n", PTR_ERR(fp));
		return;
	}

	// verify magic
	if (ksu_kernel_read_compat(fp, &magic, sizeof(magic), &off) !=
		    sizeof(magic) ||
	    magic != FILE_MAGIC) {
		pr_err("allowlist file invalid: %d!\n", magic);
		goto exit;
	}

	if (ksu_kernel_read_compat(fp, &version, sizeof(version), &off) !=
	    sizeof(version)) {
		pr_err("allowlist read version: %d failed\n", version);
		goto exit;
	}

	pr_info("allowlist version: %d\n", version);

	while (true) {
		struct app_profile profile;

		ret = ksu_kernel_read_compat(fp, &profile, sizeof(profile),
					     &off);

		if (ret <= 0) {
			pr_info("load_allow_list read err: %zd\n", ret);
			break;
		}

		pr_info("load_allow_uid, name: %s, uid: %d, allow: %d\n",
			profile.key, profile.current_uid, profile.allow_su);
		ksu_set_app_profile(&profile, false);
	}

exit:
	ksu_show_allow_list();
	filp_close(fp, 0);
}

void ksu_prune_allowlist(bool (*is_uid_valid)(uid_t, char *, void *), void *data)
{
	struct perm_data *np = NULL;
	struct perm_data *n = NULL;

	bool modified = false;
	// TODO: use RCU!
	mutex_lock(&allowlist_mutex);
	list_for_each_entry_safe (np, n, &allow_list, list) {
		uid_t uid = np->profile.current_uid;
		char *package = np->profile.key;
		// we use this uid for special cases, don't prune it!
		bool is_preserved_uid = uid == KSU_APP_PROFILE_PRESERVE_UID;
		if (!is_preserved_uid && !is_uid_valid(uid, package, data)) {
			modified = true;
			pr_info("prune uid: %d, package: %s\n", uid, package);
			list_del(&np->list);
			if (likely(uid <= BITMAP_UID_MAX)) {
				allow_list_bitmap[uid / BITS_PER_BYTE] &= ~(1 << (uid % BITS_PER_BYTE));
			}
			remove_uid_from_arr(uid);
			smp_mb();
			kfree(np);
		}
	}
	mutex_unlock(&allowlist_mutex);

	if (modified) {
		persistent_allow_list();
	}
}

#ifdef KSU_SYSCALL_HOOK
/*
 * cannon CFI constraint: kernel worker threads invoke work callbacks via
 * CFI-instrumented indirect calls, which BUG() with a non-CFI module.
 * Run the persistence handlers inline (we are always in syscall/sleepable
 * context here).
 */
// make sure allow list works cross boot
bool persistent_allow_list(void)
{
	do_save_allow_list(NULL);
	return true;
}

bool ksu_load_allow_list(void)
{
	do_load_allow_list(NULL);
	return true;
}
#else
// make sure allow list works cross boot
bool persistent_allow_list(void)
{
	return ksu_queue_work(&ksu_save_work);
}

bool ksu_load_allow_list(void)
{
	return ksu_queue_work(&ksu_load_work);
}
#endif

void ksu_allowlist_init(void)
{
	int i;

	BUILD_BUG_ON(sizeof(allow_list_bitmap) != PAGE_SIZE);
	BUILD_BUG_ON(sizeof(allow_list_arr) != PAGE_SIZE);

	for (i = 0; i < ARRAY_SIZE(allow_list_arr); i++)
		allow_list_arr[i] = -1;

	INIT_LIST_HEAD(&allow_list);

	INIT_WORK(&ksu_save_work, do_save_allow_list);
	INIT_WORK(&ksu_load_work, do_load_allow_list);

	/* 43-c1: kernel-root cred for the inline save (see __do_save_allow_list
	 * comment). finit context is sleepable — prepare here, once. */
	ksu_persist_cred = prepare_kernel_cred(NULL);
	if (!ksu_persist_cred)
		pr_err("allowlist: prepare_kernel_cred failed — saves fall back to caller creds\n");

	init_default_profiles();
}

void ksu_allowlist_exit(void)
{
	struct perm_data *np = NULL;
	struct perm_data *n = NULL;

	do_save_allow_list(NULL);

	// free allowlist
	mutex_lock(&allowlist_mutex);
	list_for_each_entry_safe (np, n, &allow_list, list) {
		list_del(&np->list);
		kfree(np);
	}
	mutex_unlock(&allowlist_mutex);
}
