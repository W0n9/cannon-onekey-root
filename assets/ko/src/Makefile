# External-module build for KernelSU v0.9.5 (table-hook variant, cannon 4.14)
# Usage:
#   make -C <kernel-out> M=$PWD ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
#        CC=clang CLANG_TRIPLE=aarch64-linux-gnu- KSU_VERSION=11872 modules

kernelsu-objs := ksu.o
kernelsu-objs += allowlist.o
kernelsu-objs += apk_sign.o
kernelsu-objs += sucompat.o
kernelsu-objs += throne_tracker.o
kernelsu-objs += core_hook.o
kernelsu-objs += ksud.o
kernelsu-objs += embed_ksud.o
kernelsu-objs += kernel_compat.o
kernelsu-objs += ksu_syscall_hook.o
kernelsu-objs += ksu_supercall.o
kernelsu-objs += ksu_selinux_resolve.o
kernelsu-objs += ksu_selinux_shim.o

kernelsu-objs += selinux/selinux.o
kernelsu-objs += selinux/sepolicy.o
kernelsu-objs += selinux/rules.o
ccflags-y += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
ccflags-y += -I$(objtree)/security/selinux -include $(srctree)/include/uapi/asm-generic/errno.h

obj-m += kernelsu.o

# tiann/KernelSU v0.9.5: rev-list --count = 1672 (GitHub API, 2026-09-07)
# KSU_VERSION = 10000 + 1672 + 200 = 11872
ifndef KSU_VERSION
KSU_VERSION := 11872
endif
ccflags-y += -DKSU_VERSION=$(KSU_VERSION)
ccflags-y += -DKSU_SYSCALL_HOOK=1
# 39-r104: A/B experiment -- disable permissive injection to bisect the
# securitycenter bitmap-blob crash (root-state-only); revert after test
# (39-r108: reverted to ENABLED state = r102 final f96aa996 configuration)
ccflags-y += -DKSU39_SELINUX_PATCH

# 4.14 compat probes against the build kernel tree (same as upstream Makefile)
ifeq ($(shell grep -q " current_sid(void)" $(srctree)/security/selinux/include/objsec.h; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_CURRENT_SID
endif

ifeq ($(shell grep -q "struct selinux_state " $(srctree)/security/selinux/include/security.h; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_SELINUX_STATE
endif

ifdef KSU_MANAGER_PACKAGE
ccflags-y += -DKSU_MANAGER_PACKAGE=\"$(KSU_MANAGER_PACKAGE)\"
endif

ifndef KSU_EXPECTED_SIZE
KSU_EXPECTED_SIZE := 0x033b
endif
ifndef KSU_EXPECTED_HASH
KSU_EXPECTED_HASH := c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6
endif
ccflags-y += -DEXPECTED_SIZE=$(KSU_EXPECTED_SIZE)
ccflags-y += -DEXPECTED_HASH=\"$(KSU_EXPECTED_HASH)\"

ccflags-y += -Wno-implicit-function-declaration -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat
ccflags-y += -Wno-declaration-after-statement -Wno-unused-function

# Keep a new line here!! Because someone may append config
