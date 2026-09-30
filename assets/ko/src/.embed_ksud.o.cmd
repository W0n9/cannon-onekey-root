cmd_/home/w0n9/kernel-src/ksu095-cfi/embed_ksud.o := /home/w0n9/kernel-src/clang-383902/clang-r383902/bin/clang -Wp,-MD,/home/w0n9/kernel-src/ksu095-cfi/.embed_ksud.o.d -nostdinc -isystem /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/include -I/home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include -I./arch/arm64/include/generated  -I/home/w0n9/kernel-src/cannon-r-oss/include -I/home/w0n9/kernel-src/cannon-r-oss/drivers/misc/mediatek/include -I./include -I/home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I/home/w0n9/kernel-src/cannon-r-oss/include/uapi -I./include/generated/uapi -include /home/w0n9/kernel-src/cannon-r-oss/include/linux/kconfig.h  -I/home/w0n9/kernel-src/ksu095-cfi -I/home/w0n9/kernel-src/ksu095-cfi -D__KERNEL__ -Qunused-arguments -mlittle-endian -DKASAN_SHADOW_SCALE_SHIFT=3 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 --target=aarch64-linux-gnu --prefix=/usr/bin/ --gcc-toolchain=/usr -no-integrated-as -Werror=unknown-warning-option -fuse-ld=lld -fno-PIE -mno-implicit-float -DCONFIG_AS_LSE=1 -fno-asynchronous-unwind-tables -fno-pic -Wno-asm-operand-widths -DKASAN_SHADOW_SCALE_SHIFT=3 -fno-delete-null-pointer-checks -Wno-int-in-bool-context -Wno-address-of-packed-member -O2 --param=allow-store-data-races=0 -DCC_HAVE_ASM_GOTO -Wframe-larger-than=3600 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -Wno-duplicate-decl-specifier -Wno-tautological-compare -mno-global-merge -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -flto -fvisibility=default -fsplit-lto-unit -fsanitize=cfi -fno-sanitize-cfi-canonical-jump-tables -fsanitize-cfi-cross-dso -Wdeclaration-after-statement -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-merge-all-constants -fno-stack-check -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time -Werror=incompatible-pointer-types -fmacro-prefix-map=/home/w0n9/kernel-src/cannon-r-oss/= -Wno-initializer-overrides -Wno-unused-value -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-uninitialized  -I/home/w0n9/kernel-src/cannon-r-oss/security/selinux  -I/home/w0n9/kernel-src/cannon-r-oss/security/selinux/include  -I./security/selinux -include /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/errno.h -DKSU_VERSION=32601 -DKSU_SYSCALL_HOOK=1 -DKSU39_SELINUX_PATCH -DKSU_COMPAT_HAS_CURRENT_SID -DKSU_COMPAT_HAS_SELINUX_STATE -DEXPECTED_SIZE=0x033b -DEXPECTED_HASH=\"c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6\" -Wno-implicit-function-declaration -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat -Wno-declaration-after-statement -Wno-unused-function  -DMODULE -mcmodel=large  -DKBUILD_BASENAME='"embed_ksud"'  -DKBUILD_MODNAME='"kernelsu"' -c -o /home/w0n9/kernel-src/ksu095-cfi/.tmp_embed_ksud.o /home/w0n9/kernel-src/ksu095-cfi/embed_ksud.c

source_/home/w0n9/kernel-src/ksu095-cfi/embed_ksud.o := /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/share/cfi_blacklist.txt

deps_/home/w0n9/kernel-src/ksu095-cfi/embed_ksud.o := \
  /home/w0n9/kernel-src/ksu095-cfi/embed_ksud.c \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/compiler_types.h \
    $(wildcard include/config/have/arch/compiler/h.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
    $(wildcard include/config/retpoline.h) \
    $(wildcard include/config/gcov/kernel.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/compiler-clang.h \
    $(wildcard include/config/lto/clang.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/errno.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/errno-base.h \

/home/w0n9/kernel-src/ksu095-cfi/embed_ksud.o: $(deps_/home/w0n9/kernel-src/ksu095-cfi/embed_ksud.o)

$(deps_/home/w0n9/kernel-src/ksu095-cfi/embed_ksud.o):
