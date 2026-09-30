/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KSU_UAPI_SUPERCALL_H
#define __KSU_UAPI_SUPERCALL_H

/*
 * KernelSU v3.x supercall UAPI, ported onto the 4.14 cannon sys_call_table
 * hook backend.
 *
 * These values are copied verbatim from tiann/KernelSU v3.3.0
 * uapi/supercall.h (git 932014ab5b2c9b74a3d11e2ec4d17dd10fc9442e) — the
 * userspace side (manager v3.3.0's libkernelsu.so and ksud's libksud.so) is
 * a fixed binary we cannot change, so this header is an ABI contract: every
 * _IOC encoding below must stay bit-identical to upstream or the manager
 * silently falls back to the legacy prctl path (which is exactly the
 * uapi_version=0 red card we are fixing).
 *
 * Verified against userspace/ksuinit/src/lib.rs which hardcodes
 * KSU_IOCTL_GET_INFO = 0x80104b02 / _LEGACY = 0x80004b02.
 */

#include <linux/ioctl.h>
#include <linux/stddef.h>
#include <linux/types.h>

/* 2: allowlist v4 root profile flags */
#define KERNEL_SU_UAPI_VERSION 2

/* Magic numbers for the fd-install hook (reboot(2) entry point).
 * NOTE: KSU_INSTALL_MAGIC1 == KERNEL_SU_OPTION == 0xDEADBEEF, but they live
 * on different syscall entries (prctl vs reboot); the two ABIs are
 * disambiguated by the entry point, never by the argument value. */
#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE

/* GET_INFO flags */
#define KSU_GET_INFO_FLAG_LKM       (1U << 0)
#define KSU_GET_INFO_FLAG_MANAGER   (1U << 1)
#define KSU_GET_INFO_FLAG_LATE_LOAD (1U << 2)
#define KSU_GET_INFO_FLAG_PR_BUILD  (1U << 3)

/* Feature ids — only the count matters for GET_INFO.features. Upstream
 * KSU_FEATURE_MAX is 6 (SU_COMPAT/KERNEL_UMOUNT/SULOG/ADB_ROOT/
 * SELINUX_HIDE/WEBVIEW_ZYGOTE_UMOUNT). We advertise 0 because none of the
 * feature handlers exist in this backend; advertising a non-zero max would
 * make ksud probe features that always fail. */
#define KSU_FEATURE_MAX 0

/* v3.3.0 feature ids (manager cpp/uapi/feature.h). GET_FEATURE answers
 * supported=false for all of them — allowed by upstream semantics. */
#define KSU_FEATURE_SU_COMPAT             0
#define KSU_FEATURE_KERNEL_UMOUNT         1
#define KSU_FEATURE_SULOG                 2
#define KSU_FEATURE_ADB_ROOT              3
#define KSU_FEATURE_SELINUX_HIDE          4
#define KSU_FEATURE_WEBVIEW_ZYGOTE_UMOUNT 5

/* GET_FEATURE: caller passes an uninitialised struct and we fill value=0 +
 * supported=0; per upstream kernel/policy/feature.c an unregistered feature
 * is not an error, and manager's is_*_enabled() wrappers already treat
 * supported=false as "disabled" (ksu.cc get_feature()). */

struct ksu_get_info_cmd {
	__u32 version;      /* Output: KERNEL_SU_VERSION */
	__u32 flags;        /* Output: KSU_GET_INFO_FLAG_* bits */
	__u32 features;     /* Output: max feature ID supported */
	__u32 uapi_version; /* Output: KERNEL_SU_UAPI_VERSION */
};

struct ksu_get_info_legacy_cmd {
	__u32 version;  /* Output: KERNEL_SU_VERSION */
	__u32 flags;    /* Output: KSU_GET_INFO_FLAG_* bits */
	__u32 features; /* Output: max feature ID supported */
};

/*
 * ---- v3.3.0 uapi structs for the newly implemented commands ----
 *
 * These MUST stay layout-identical to the manager/ksud binaries. Copied
 * verbatim in shape from research/ksu-v330/ksu/uapi/supercall.h + app_profile.h;
 * sizes were verified on host gcc (LP64) and are pinned with _Static_assert.
 */

/* report_event */
struct ksu_report_event_cmd {
	__u32 event; /* Input: EVENT_POST_FS_DATA / BOOT_COMPLETED / MODULE_MOUNTED */
};

struct ksu_check_safemode_cmd {
	__u8 in_safe_mode; /* Output */
};

/* deprecated inline-array variant; wire size 520, count at offset 512 */
struct ksu_get_allow_list_cmd {
	__u32 uids[128]; /* Output */
	__u32 count;     /* Output */
	__u8 allow;      /* Input: true for allow list, false for deny list */
};

/* new flexible-array variant; wire size 4, uids start at offset 4 */
struct ksu_new_get_allow_list_cmd {
	__u16 count;       /* Input/Output: capacity in, filled out */
	__u16 total_count; /* Output: total entries in the requested list */
	__u32 uids[0];     /* Output */
};

struct ksu_uid_granted_root_cmd {
	__u32 uid;    /* Input */
	__u8 granted; /* Output */
};

struct ksu_uid_should_umount_cmd {
	__u32 uid;          /* Input */
	__u8 should_umount; /* Output */
};

struct ksu_get_manager_appid_cmd {
	__u32 appid; /* Output */
};

struct ksu_get_feature_cmd {
	__u32 feature_id; /* Input */
	__u64 value;      /* Output */
	__u8 supported;   /* Output */
};

struct ksu_set_feature_cmd {
	__u32 feature_id; /* Input */
	__u64 value;      /* Input */
};

/*
 * v3.3.0 app_profile as seen on the wire (784 bytes). The driver's internal
 * struct app_profile (ksu.h, v0.9.5) is 776 bytes and the first 776 bytes of
 * both layouts are offset-identical (verified host gcc: key@4, curr_uid@260,
 * allow_su@264, union@272, rp_config.profile@536); the only delta is
 * root_profile.flags (__u64) at profile offset 776 = root_profile offset 240.
 * Handlers translate: GET builds the uapi shape and appends flags, SET reads
 * the uapi shape and drops flags before handing the internal struct to
 * allowlist.c.
 */
#define KSU_UAPI_APP_PROFILE_VER 4
#define KSU_UAPI_FLAG_NO_NEW_PRIVS (1ULL << 0)

struct ksu_uapi_root_profile {
	__s32 uid;
	__s32 gid;
	__u32 groups_count;
	__s32 groups[32];
	struct {
		__u64 effective;
		__u64 permitted;
		__u64 inheritable;
	} capabilities;
	char selinux_domain[64];
	__s32 namespaces;
	__u64 flags;
};

struct ksu_uapi_non_root_profile {
	bool umount_modules;
};

struct ksu_uapi_app_profile {
	__u32 version;
	char key[256];
	__s32 curr_uid;
	bool allow_su;
	union {
		struct {
			bool use_default;
			char template_name[256];
			struct ksu_uapi_root_profile profile;
		} rp_config;
		struct {
			bool use_default;
			struct ksu_uapi_non_root_profile profile;
		} nrp_config;
	};
};

_Static_assert(sizeof(struct ksu_get_allow_list_cmd) == 520,
	       "legacy allow_list ABI size mismatch");
_Static_assert(offsetof(struct ksu_get_allow_list_cmd, count) == 512,
	       "legacy allow_list count offset mismatch");
_Static_assert(sizeof(struct ksu_new_get_allow_list_cmd) == 4,
	       "new allow_list ABI size mismatch");
_Static_assert(offsetof(struct ksu_new_get_allow_list_cmd, uids) == 4,
	       "new allow_list uids offset mismatch");
_Static_assert(sizeof(struct ksu_uapi_app_profile) == 784,
	       "v3.3.0 app_profile wire size mismatch");
_Static_assert(offsetof(struct ksu_uapi_app_profile, curr_uid) == 260,
	       "v3.3.0 app_profile curr_uid offset mismatch");
_Static_assert(offsetof(struct ksu_uapi_root_profile, flags) == 240,
	       "v3.3.0 root_profile flags offset mismatch");

/*
 * ioctl encodings. Kept as explicit hex (not _IOR macros) so a mistake is
 * visible in a diff; each was cross-checked against the _IOC expansion:
 *   _IOC(dir,type,nr,size) = (dir<<30)|(size<<16)|(type<<8)|nr
 * with _IOC_NONE=0, _IOC_READ=2, type='K'=0x4B.
 * All encodings are byte-identical to the manager binary's copy
 * (manager/app/src/main/cpp/uapi/supercall.h) — that file is the final
 * authority since the userspace side is a fixed binary. Note the
 * size=0 quirks kept verbatim: GET_ALLOW_LIST/GET_DENY_LIST share nr 6/7
 * with the NEW_* variants and are told apart ONLY by the size bits
 * (0 vs 4), and GET_MANAGER_APPID carries size 0 despite its 4-byte
 * payload.
 */
#define KSU_IOCTL_GRANT_ROOT      _IOC(_IOC_NONE, 'K', 1, 0) /* 0x00004B01 */
#define KSU_IOCTL_GET_INFO        _IOR('K', 2, struct ksu_get_info_cmd) /* 0x80104B02 */
#define KSU_IOCTL_GET_INFO_LEGACY _IOC(_IOC_READ, 'K', 2, 0) /* 0x80004B02 */
#define KSU_IOCTL_REPORT_EVENT    0x40004B03u /* _IOC(W,'K',3,0)  */
#define KSU_IOCTL_SET_SEPOLICY    0xC0004B04u /* _IOC(RW,'K',4,0) */
#define KSU_IOCTL_CHECK_SAFEMODE  0x80004B05u /* _IOC(R,'K',5,0)  */
#define KSU_IOCTL_GET_ALLOW_LIST  0xC0004B06u /* _IOC(RW,'K',6,0) — deprecated ABI */
#define KSU_IOCTL_GET_DENY_LIST   0xC0004B07u /* _IOC(RW,'K',7,0) — deprecated ABI */
#define KSU_IOCTL_NEW_GET_ALLOW_LIST 0xC0044B06u /* _IOWR('K',6,4) */
#define KSU_IOCTL_NEW_GET_DENY_LIST  0xC0044B07u /* _IOWR('K',7,4) */
#define KSU_IOCTL_UID_GRANTED_ROOT  0xC0004B08u /* _IOC(RW,'K',8,0) */
#define KSU_IOCTL_UID_SHOULD_UMOUNT 0xC0004B09u /* _IOC(RW,'K',9,0) */
#define KSU_IOCTL_GET_MANAGER_APPID 0x80004B0Au /* _IOC(R,'K',10,0) */
#define KSU_IOCTL_GET_APP_PROFILE   0xC0004B0Bu /* _IOC(RW,'K',11,0) */
#define KSU_IOCTL_SET_APP_PROFILE   0x40004B0Cu /* _IOC(W,'K',12,0) */
#define KSU_IOCTL_GET_FEATURE       0xC0004B0Du /* _IOC(RW,'K',13,0) */
#define KSU_IOCTL_SET_FEATURE       0x40004B0Eu /* _IOC(W,'K',14,0) */

#endif /* __KSU_UAPI_SUPERCALL_H */
