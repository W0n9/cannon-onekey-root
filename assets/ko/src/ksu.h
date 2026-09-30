#ifndef __KSU_H_KSU
#define __KSU_H_KSU

#include <linux/types.h>
#include <linux/workqueue.h>

#define KERNEL_SU_VERSION KSU_VERSION
#define KERNEL_SU_OPTION 0xDEADBEEF

#define CMD_GRANT_ROOT 0
#define CMD_BECOME_MANAGER 1
#define CMD_GET_VERSION 2
#define CMD_ALLOW_SU 3
#define CMD_DENY_SU 4
#define CMD_GET_ALLOW_LIST 5
#define CMD_GET_DENY_LIST 6
#define CMD_REPORT_EVENT 7
#define CMD_SET_SEPOLICY 8
#define CMD_CHECK_SAFEMODE 9
#define CMD_GET_APP_PROFILE 10
#define CMD_SET_APP_PROFILE 11
#define CMD_UID_GRANTED_ROOT 12
#define CMD_UID_SHOULD_UMOUNT 13

/*
 * 38-r76: arm the v3.x supercall syscall hooks (reboot/setresuid) on demand.
 *
 * They are deliberately NOT installed at module_init. Installing them there
 * puts them live at the same instant the module's CFI stubs are still
 * un-NOPed, and the finit->NOP window is exactly where the AG/AH panics came
 * from: any setresuid (zygote does one per app fork) reaching the hook would
 * make a cross-DSO call through a live __cfi_slowpath stub. The probe NOPs
 * every stub ~80ms after finit, so the window is short — but it is not zero,
 * and these two hooks are on hotter paths than the original five. Arming
 * them explicitly, from the probe, after NOP, removes the risk entirely.
 *
 * root-only, and idempotent.
 */
#define CMD_SUPERCALL_ARM 14

#define EVENT_POST_FS_DATA 1
#define EVENT_BOOT_COMPLETED 2
#define EVENT_MODULE_MOUNTED 3

#define KSU_APP_PROFILE_VER 2
#define KSU_MAX_PACKAGE_NAME 256
// NGROUPS_MAX for Linux is 65535 generally, but we only supports 32 groups.
#define KSU_MAX_GROUPS 32
#define KSU_SELINUX_DOMAIN 64

struct root_profile {
	int32_t uid;
	int32_t gid;

	int32_t groups_count;
	int32_t groups[KSU_MAX_GROUPS];

	// kernel_cap_t is u32[2] for capabilities v3
	struct {
		u64 effective;
		u64 permitted;
		u64 inheritable;
	} capabilities;

	char selinux_domain[KSU_SELINUX_DOMAIN];

	int32_t namespaces;
};

struct non_root_profile {
	bool umount_modules;
};

struct app_profile {
	// It may be utilized for backward compatibility, although we have never explicitly made any promises regarding this.
	u32 version;

	// this is usually the package of the app, but can be other value for special apps
	char key[KSU_MAX_PACKAGE_NAME];
	int32_t current_uid;
	bool allow_su;

	union {
		struct {
			bool use_default;
			char template_name[KSU_MAX_PACKAGE_NAME];

			struct root_profile profile;
		} rp_config;

		struct {
			bool use_default;

			struct non_root_profile profile;
		} nrp_config;
	};
};

bool ksu_queue_work(struct work_struct *work);

static inline int startswith(char *s, char *prefix)
{
	return strncmp(s, prefix, strlen(prefix));
}

static inline int endswith(const char *s, const char *t)
{
	size_t slen = strlen(s);
	size_t tlen = strlen(t);
	if (tlen > slen)
		return 1;
	return strcmp(s + slen - tlen, t);
}

#endif
