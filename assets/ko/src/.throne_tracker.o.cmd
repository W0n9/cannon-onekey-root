cmd_/home/w0n9/kernel-src/ksu095-cfi/throne_tracker.o := /home/w0n9/kernel-src/clang-383902/clang-r383902/bin/clang -Wp,-MD,/home/w0n9/kernel-src/ksu095-cfi/.throne_tracker.o.d -nostdinc -isystem /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/include -I/home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include -I./arch/arm64/include/generated  -I/home/w0n9/kernel-src/cannon-r-oss/include -I/home/w0n9/kernel-src/cannon-r-oss/drivers/misc/mediatek/include -I./include -I/home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I/home/w0n9/kernel-src/cannon-r-oss/include/uapi -I./include/generated/uapi -include /home/w0n9/kernel-src/cannon-r-oss/include/linux/kconfig.h  -I/home/w0n9/kernel-src/ksu095-cfi -I/home/w0n9/kernel-src/ksu095-cfi -D__KERNEL__ -Qunused-arguments -mlittle-endian -DKASAN_SHADOW_SCALE_SHIFT=3 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 --target=aarch64-linux-gnu --prefix=/usr/bin/ --gcc-toolchain=/usr -no-integrated-as -Werror=unknown-warning-option -fuse-ld=lld -fno-PIE -mno-implicit-float -DCONFIG_AS_LSE=1 -fno-asynchronous-unwind-tables -fno-pic -Wno-asm-operand-widths -DKASAN_SHADOW_SCALE_SHIFT=3 -fno-delete-null-pointer-checks -Wno-int-in-bool-context -Wno-address-of-packed-member -O2 --param=allow-store-data-races=0 -DCC_HAVE_ASM_GOTO -Wframe-larger-than=3600 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -Wno-duplicate-decl-specifier -Wno-tautological-compare -mno-global-merge -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -flto -fvisibility=default -fsplit-lto-unit -fsanitize=cfi -fno-sanitize-cfi-canonical-jump-tables -fsanitize-cfi-cross-dso -Wdeclaration-after-statement -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-merge-all-constants -fno-stack-check -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time -Werror=incompatible-pointer-types -fmacro-prefix-map=/home/w0n9/kernel-src/cannon-r-oss/= -Wno-initializer-overrides -Wno-unused-value -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-uninitialized  -I/home/w0n9/kernel-src/cannon-r-oss/security/selinux  -I/home/w0n9/kernel-src/cannon-r-oss/security/selinux/include  -I./security/selinux -include /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/errno.h -DKSU_VERSION=32601 -DKSU_SYSCALL_HOOK=1 -DKSU39_SELINUX_PATCH -DKSU_COMPAT_HAS_CURRENT_SID -DKSU_COMPAT_HAS_SELINUX_STATE -DEXPECTED_SIZE=0x033b -DEXPECTED_HASH=\"c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6\" -Wno-implicit-function-declaration -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat -Wno-declaration-after-statement -Wno-unused-function  -DMODULE -mcmodel=large  -DKBUILD_BASENAME='"throne_tracker"'  -DKBUILD_MODNAME='"kernelsu"' -c -o /home/w0n9/kernel-src/ksu095-cfi/.tmp_throne_tracker.o /home/w0n9/kernel-src/ksu095-cfi/throne_tracker.c

source_/home/w0n9/kernel-src/ksu095-cfi/throne_tracker.o := /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/share/cfi_blacklist.txt

deps_/home/w0n9/kernel-src/ksu095-cfi/throne_tracker.o := \
  /home/w0n9/kernel-src/ksu095-cfi/throne_tracker.c \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/err.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/stack/validation.h) \
    $(wildcard include/config/kasan.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/types.h \
  arch/arm64/include/generated/uapi/asm/types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/int-ll64.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/int-ll64.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/bitsperlong.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitsperlong.h \
    $(wildcard include/config/64bit.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/bitsperlong.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/posix_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stddef.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/stddef.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/posix_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/posix_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/barrier.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/barrier.h \
    $(wildcard include/config/smp.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kasan-checks.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
  arch/arm64/include/generated/uapi/asm/errno.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/fs.h \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/fs/posix/acl.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/cgroup/writeback.h) \
    $(wildcard include/config/ima.h) \
    $(wildcard include/config/fsnotify.h) \
    $(wildcard include/config/fs/encryption.h) \
    $(wildcard include/config/fs/verity.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/epoll.h) \
    $(wildcard include/config/file/locking.h) \
    $(wildcard include/config/unicode.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/quota.h) \
    $(wildcard include/config/fs/dax.h) \
    $(wildcard include/config/mandatory/file/locking.h) \
    $(wildcard include/config/block.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
    $(wildcard include/config/migration.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/linkage.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stringify.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/rel/crcs.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/linkage.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/wait_bit.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/wait.h \
    $(wildcard include/config/lockdep.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
    $(wildcard include/config/page/poisoning/zero.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/const.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/arch/has/refcount.h) \
    $(wildcard include/config/panic/timeout.h) \
    $(wildcard include/config/tracing.h) \
  /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/include/stdarg.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bitops.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bits.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/bitops.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/builtin-__ffs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/builtin-ffs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/builtin-__fls.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/builtin-fls.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/ffz.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/fls64.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/sched.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/hweight.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/arch_hweight.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/const_hweight.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/lock.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/non-atomic.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitops/le.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/byteorder.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/byteorder/little_endian.h \
    $(wildcard include/config/cpu/big/endian.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/byteorder/little_endian.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/swab.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/swab.h \
  arch/arm64/include/generated/uapi/asm/swab.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/swab.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/byteorder/generic.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/typecheck.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/printk.h \
    $(wildcard include/config/mtk/aee/feature.h) \
    $(wildcard include/config/mtk/eng/build.h) \
    $(wildcard include/config/printk/mt/prefix.h) \
    $(wildcard include/config/message/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/default.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk/nmi.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/init.h \
    $(wildcard include/config/strict/kernel/rwx.h) \
    $(wildcard include/config/strict/module/rwx.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kern_levels.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/kernel.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/sysinfo.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cache.h \
    $(wildcard include/config/kasan/sw/tags.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cputype.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/sysreg.h \
    $(wildcard include/config/broken/gas/inst.h) \
    $(wildcard include/config/arm64/4k/pages.h) \
    $(wildcard include/config/arm64/16k/pages.h) \
    $(wildcard include/config/arm64/64k/pages.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/compiler.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/build_bug.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dynamic_debug.h \
    $(wildcard include/config/jump/label.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/preempt.h \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  arch/arm64/include/generated/asm/preempt.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/preempt.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/thread_info.h \
    $(wildcard include/config/thread/info/in/task.h) \
    $(wildcard include/config/have/arch/within/stack/frames.h) \
    $(wildcard include/config/hardened/usercopy.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/bug/on/data/corruption.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/bug.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/asm-bug.h \
    $(wildcard include/config/debug/bugverbose.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/brk-imm.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/restart_block.h \
    $(wildcard include/config/compat.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/current.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/thread_info.h \
    $(wildcard include/config/arm64/sw/ttbr0/pan.h) \
    $(wildcard include/config/shadow/call/stack.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/memory.h \
    $(wildcard include/config/arm64/va/bits.h) \
    $(wildcard include/config/vmap/stack.h) \
    $(wildcard include/config/debug/align/rodata.h) \
    $(wildcard include/config/blk/dev/initrd.h) \
    $(wildcard include/config/debug/virtual.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/page-def.h \
    $(wildcard include/config/arm64/page/shift.h) \
    $(wildcard include/config/arm64/cont/shift.h) \
  arch/arm64/include/generated/asm/sizes.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/sizes.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sizes.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/vm/pgflags.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pfn.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/stack_pointer.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/irqflags.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/ptrace.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/ptrace.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/hwcap.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/hwcap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/ptrace.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bottom_half.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/spinlock_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/spinlock_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/lockdep.h \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/lockdep/crossrelease.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rwlock_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/spinlock.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/lse.h \
    $(wildcard include/config/as/lse.h) \
    $(wildcard include/config/arm64/lse/atomics.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/processor.h \
    $(wildcard include/config/have/hw/breakpoint.h) \
    $(wildcard include/config/arm64/tagged/addr/abi.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
    $(wildcard include/config/fortify/source.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/string.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/string.h \
    $(wildcard include/config/arch/has/uaccess/flushcache.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/alternative.h \
    $(wildcard include/config/arm64/uao.h) \
    $(wildcard include/config/foo.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cpucaps.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/insn.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cpufeature.h \
    $(wildcard include/config/arm64/ssbd.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/jump_label.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/atomic.h \
    $(wildcard include/config/generic/atomic64.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/atomic.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/atomic_ll_sc.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cmpxchg.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/atomic-long.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/fpsimd.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/hw_breakpoint.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/virt.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/sections.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/sections.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable-hwdef.h \
    $(wildcard include/config/pgtable/levels.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rwlock.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/spinlock_api_smp.h \
    $(wildcard include/config/inline/spin/lock.h) \
    $(wildcard include/config/inline/spin/lock/bh.h) \
    $(wildcard include/config/inline/spin/lock/irq.h) \
    $(wildcard include/config/inline/spin/lock/irqsave.h) \
    $(wildcard include/config/inline/spin/trylock.h) \
    $(wildcard include/config/inline/spin/trylock/bh.h) \
    $(wildcard include/config/uninline/spin/unlock.h) \
    $(wildcard include/config/inline/spin/unlock/bh.h) \
    $(wildcard include/config/inline/spin/unlock/irq.h) \
    $(wildcard include/config/inline/spin/unlock/irqrestore.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rwlock_api_smp.h \
    $(wildcard include/config/inline/read/lock.h) \
    $(wildcard include/config/inline/write/lock.h) \
    $(wildcard include/config/inline/read/lock/bh.h) \
    $(wildcard include/config/inline/write/lock/bh.h) \
    $(wildcard include/config/inline/read/lock/irq.h) \
    $(wildcard include/config/inline/write/lock/irq.h) \
    $(wildcard include/config/inline/read/lock/irqsave.h) \
    $(wildcard include/config/inline/write/lock/irqsave.h) \
    $(wildcard include/config/inline/read/trylock.h) \
    $(wildcard include/config/inline/write/trylock.h) \
    $(wildcard include/config/inline/read/unlock.h) \
    $(wildcard include/config/inline/write/unlock.h) \
    $(wildcard include/config/inline/read/unlock/bh.h) \
    $(wildcard include/config/inline/write/unlock/bh.h) \
    $(wildcard include/config/inline/read/unlock/irq.h) \
    $(wildcard include/config/inline/write/unlock/irq.h) \
    $(wildcard include/config/inline/read/unlock/irqrestore.h) \
    $(wildcard include/config/inline/write/unlock/irqrestore.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/wait.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kdev_t.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/kdev_t.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dcache.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rculist.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcupdate.h \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/rcu/stall/common.h) \
    $(wildcard include/config/no/hz/full.h) \
    $(wildcard include/config/rcu/nocb/cpu.h) \
    $(wildcard include/config/tasks/rcu.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/prove/rcu.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/arch/weak/release/acquire.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bitmap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcutree.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rculist_bl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/list_bl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bit_spinlock.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/seqlock.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/lockref.h \
    $(wildcard include/config/arch/use/cmpxchg/lockref.h) \
  include/generated/bounds.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stringhash.h \
    $(wildcard include/config/dcache/word/access.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/hash.h \
    $(wildcard include/config/have/arch/hash.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/path.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stat.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/stat.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/stat.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/stat.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/compat.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched.h \
    $(wildcard include/config/virt/cpu/accounting/native.h) \
    $(wildcard include/config/sched/info.h) \
    $(wildcard include/config/sched/hmp.h) \
    $(wildcard include/config/schedstats.h) \
    $(wildcard include/config/sched/hmp/prio/filter.h) \
    $(wildcard include/config/fair/group/sched.h) \
    $(wildcard include/config/mtk/rt/throttle/mon.h) \
    $(wildcard include/config/sched/walt.h) \
    $(wildcard include/config/rt/group/sched.h) \
    $(wildcard include/config/uclamp/task.h) \
    $(wildcard include/config/uclamp/groups/count.h) \
    $(wildcard include/config/mtk/sched/boost.h) \
    $(wildcard include/config/cgroup/sched.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/psi.h) \
    $(wildcard include/config/memcg.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/compat/brk.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/arch/has/scaled/cputime.h) \
    $(wildcard include/config/cpu/freq/times.h) \
    $(wildcard include/config/virt/cpu/accounting/gen.h) \
    $(wildcard include/config/mtk/mlog.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/posix/timers.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/detect/hung/task.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/ubsan.h) \
    $(wildcard include/config/task/xacct.h) \
    $(wildcard include/config/cpusets.h) \
    $(wildcard include/config/intel/rdt.h) \
    $(wildcard include/config/futex.h) \
    $(wildcard include/config/perf/events.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/numa/balancing.h) \
    $(wildcard include/config/task/delay/acct.h) \
    $(wildcard include/config/fault/injection.h) \
    $(wildcard include/config/latencytop.h) \
    $(wildcard include/config/function/graph/tracer.h) \
    $(wildcard include/config/kcov.h) \
    $(wildcard include/config/uprobes.h) \
    $(wildcard include/config/bcache.h) \
    $(wildcard include/config/livepatch.h) \
    $(wildcard include/config/mtk/task/turbo.h) \
    $(wildcard include/config/sched/tune.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/sched.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pid.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/time64.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/time.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/math64.h \
    $(wildcard include/config/arch/supports/int128.h) \
  arch/arm64/include/generated/asm/div64.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/div64.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/sem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ipc.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uidgid.h \
    $(wildcard include/config/multiuser.h) \
    $(wildcard include/config/user/ns.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/highuid.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rhashtable.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/errno.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/errno.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/jhash.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/unaligned/packed_struct.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/list_nulls.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
    $(wildcard include/config/wq/watchdog.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/timer.h \
    $(wildcard include/config/cfi/clang.h) \
    $(wildcard include/config/debug/objects/timers.h) \
    $(wildcard include/config/no/hz/common.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ktime.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/jiffies.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/timex.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/timex.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/param.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/param.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/param.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/timex.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/arch_timer.h \
    $(wildcard include/config/arm/arch/timer/ool/workaround.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/smp.h \
    $(wildcard include/config/up/late/init.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/llist.h \
    $(wildcard include/config/arch/have/nmi/safe/cmpxchg.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/smp.h \
    $(wildcard include/config/arm64/acpi/parking/protocol.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/percpu.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/percpu.h \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
    $(wildcard include/config/mtk/sched/monitor.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/clocksource/arm_arch_timer.h \
    $(wildcard include/config/arm/arch/timer.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/timecounter.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/timex.h \
  include/generated/timeconst.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/timekeeping.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/debug/objects/free.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mutex.h \
    $(wildcard include/config/mutex/spin/on/owner.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/osq_lock.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/debug_locks.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/ipc.h \
  arch/arm64/include/generated/uapi/asm/ipcbuf.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/ipcbuf.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/refcount.h \
    $(wildcard include/config/refcount/full.h) \
  arch/arm64/include/generated/uapi/asm/sembuf.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/sembuf.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/shm.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/page.h \
    $(wildcard include/config/have/arch/pfn/valid.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/personality.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/personality.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable-types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/pgtable-nopud.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/pgtable-nop4d-hack.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/5level-fixup.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/getorder.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/shm.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/hugetlb_encode.h \
  arch/arm64/include/generated/uapi/asm/shmbuf.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/shmbuf.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/shmparam.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/shmparam.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kcov.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/kcov.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/plist.h \
    $(wildcard include/config/debug/pi/list.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/hrtimer.h \
    $(wildcard include/config/high/res/timers.h) \
    $(wildcard include/config/time/low/res.h) \
    $(wildcard include/config/timerfd.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rbtree.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/timerqueue.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/seccomp.h \
    $(wildcard include/config/seccomp.h) \
    $(wildcard include/config/have/arch/seccomp/filter.h) \
    $(wildcard include/config/seccomp/filter.h) \
    $(wildcard include/config/checkpoint/restore.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/seccomp.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/seccomp.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/unistd.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/unistd.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/unistd.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/unistd.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/seccomp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/unistd.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/nodemask.h \
    $(wildcard include/config/highmem.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/resource.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/resource.h \
  arch/arm64/include/generated/uapi/asm/resource.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/resource.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/resource.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/latencytop.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched/prio.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/signal_types.h \
    $(wildcard include/config/old/sigaction.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/signal.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/signal.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/signal.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/signal.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/signal-defs.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/sigcontext.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/siginfo.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/siginfo.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mm_types_task.h \
    $(wildcard include/config/arch/want/batched/unmap/tlb/flush.h) \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/arch/enable/split/pmd/ptlock.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/task_io_accounting.h \
    $(wildcard include/config/task/io/accounting.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched/sched.h \
    $(wildcard include/config/mtk/sched/trace.h) \
    $(wildcard include/config/mtk/sched/debug.h) \
    $(wildcard include/config/mtk/sched/eas/power/support.h) \
    $(wildcard include/config/mach/mt6873.h) \
    $(wildcard include/config/mtk/sched/interop.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched/task_stack.h \
    $(wildcard include/config/stack/growsup.h) \
    $(wildcard include/config/debug/stack/usage.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/magic.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/stat.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/list_lru.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/shrinker.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/radix-tree.h \
    $(wildcard include/config/radix/tree/multiorder.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/spin/on/owner.h) \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  arch/arm64/include/generated/asm/rwsem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/rwsem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mm_types.h \
    $(wildcard include/config/have/cmpxchg/double.h) \
    $(wildcard include/config/have/aligned/struct/page.h) \
    $(wildcard include/config/transparent/hugepage.h) \
    $(wildcard include/config/userfaultfd.h) \
    $(wildcard include/config/speculative/page/fault.h) \
    $(wildcard include/config/have/arch/compat/mmap/bases.h) \
    $(wildcard include/config/membarrier.h) \
    $(wildcard include/config/aio.h) \
    $(wildcard include/config/mmu/notifier.h) \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hmm.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/auxvec.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/auxvec.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/auxvec.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/completion.h \
    $(wildcard include/config/lockdep/completions.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uprobes.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/uprobes.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/debug-monitors.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/esr.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/probes.h \
    $(wildcard include/config/kprobes.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/page-flags-layout.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/sparsemem.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/mmu.h \
    $(wildcard include/config/unmap/kernel/at/el0.h) \
    $(wildcard include/config/harden/branch/predictor.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/capability.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/capability.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/semaphore.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/fcntl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/fcntl.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/fcntl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/fcntl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/fiemap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/migrate_mode.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/percpu-rwsem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcuwait.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcu_sync.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/delayed_call.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uuid.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/uuid.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/errseq.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/fs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/limits.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/ioctl.h \
  arch/arm64/include/generated/uapi/asm/ioctl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/ioctl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/ioctl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/quota.h \
    $(wildcard include/config/quota/netlink/interface.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/percpu_counter.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/gfp.h \
    $(wildcard include/config/dmauser/pages.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/zone/device.h) \
    $(wildcard include/config/zone/movable/cma.h) \
    $(wildcard include/config/pm/sleep.h) \
    $(wildcard include/config/memory/isolation.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/cma.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/zsmalloc.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/page/extension.h) \
    $(wildcard include/config/no/bootmem.h) \
    $(wildcard include/config/deferred/struct/page/init.h) \
    $(wildcard include/config/have/memory/present.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/node/memmap/size.h) \
    $(wildcard include/config/have/memblock/node/map.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/have/arch/early/pfn/to/nid.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/memory_hotplug.h \
    $(wildcard include/config/arch/has/add/pages.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
    $(wildcard include/config/have/bootmem/info/node.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/notifier.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/srcu.h \
    $(wildcard include/config/tiny/srcu.h) \
    $(wildcard include/config/tree/srcu.h) \
    $(wildcard include/config/srcu.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcu_segcblist.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/srcutree.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcu_node_tree.h \
    $(wildcard include/config/rcu/fanout.h) \
    $(wildcard include/config/rcu/fanout/leaf.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/topology.h \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
    $(wildcard include/config/sched/smt.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/topology.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/arch_topology.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/topology.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/dqblk_xfs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dqblk_v1.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dqblk_v2.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dqblk_qtree.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/projid.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/quota.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/nfs_fs_i.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/slab.h \
    $(wildcard include/config/debug/slab.h) \
    $(wildcard include/config/failslab.h) \
    $(wildcard include/config/have/hardened/usercopy/allocator.h) \
    $(wildcard include/config/slab.h) \
    $(wildcard include/config/slub.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kmemleak.h \
    $(wildcard include/config/debug/kmemleak.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/vmalloc.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kasan.h \
    $(wildcard include/config/kasan/generic.h) \
  include/generated/uapi/linux/version.h \
  /home/w0n9/kernel-src/ksu095-cfi/allowlist.h \
  /home/w0n9/kernel-src/ksu095-cfi/ksu.h \
  /home/w0n9/kernel-src/ksu095-cfi/klog.h \
  /home/w0n9/kernel-src/ksu095-cfi/manager.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cred.h \
    $(wildcard include/config/debug/credentials.h) \
    $(wildcard include/config/keys.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/key.h \
    $(wildcard include/config/sysctl.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sysctl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/sysctl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/assoc_array.h \
    $(wildcard include/config/associative/array.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/selinux.h \
    $(wildcard include/config/security/selinux.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched/user.h \
    $(wildcard include/config/fanotify.h) \
    $(wildcard include/config/posix/mqueue.h) \
    $(wildcard include/config/bpf/syscall.h) \
    $(wildcard include/config/net.h) \
  /home/w0n9/kernel-src/ksu095-cfi/throne_tracker.h \
  /home/w0n9/kernel-src/ksu095-cfi/kernel_compat.h \
    $(wildcard include/config/is/hw/hisi.h) \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/policydb.h \
    $(wildcard include/config/mls.h) \
    $(wildcard include/config/android/netlink/route.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/flex_array.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/reciprocal_div.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/symtab.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/hashtab.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/avtab.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/include/security.h \
    $(wildcard include/config/security/selinux/develop.h) \
    $(wildcard include/config/netlabel.h) \
  security/selinux/flask.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/sidtab.h \
    $(wildcard include/config/security/selinux/sidtab/hash/bits.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/hashtable.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/context.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/ebitmap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netlabel.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/net.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/random.h \
    $(wildcard include/config/gcc/plugin/latent/entropy.h) \
    $(wildcard include/config/arch/random.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/once.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/random.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/irqnr.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/irqnr.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/net.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/socket.h \
    $(wildcard include/config/proc/fs.h) \
  arch/arm64/include/generated/uapi/asm/socket.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/socket.h \
  arch/arm64/include/generated/uapi/asm/sockios.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/sockios.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/sockios.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uio.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/uio.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/socket.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/skbuff.h \
    $(wildcard include/config/nf/conntrack.h) \
    $(wildcard include/config/bridge/netfilter.h) \
    $(wildcard include/config/xfrm.h) \
    $(wildcard include/config/ipv6/ndisc/nodetype.h) \
    $(wildcard include/config/net/switchdev.h) \
    $(wildcard include/config/net/cls/act.h) \
    $(wildcard include/config/net/sched.h) \
    $(wildcard include/config/net/rx/busy/poll.h) \
    $(wildcard include/config/xps.h) \
    $(wildcard include/config/network/secmark.h) \
    $(wildcard include/config/network/phy/timestamping.h) \
    $(wildcard include/config/netfilter/xt/target/trace.h) \
    $(wildcard include/config/nf/tables.h) \
    $(wildcard include/config/ip/vs.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/textsearch.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/checksum.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uaccess.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/uaccess.h \
    $(wildcard include/config/arm64/pan.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/kernel-pgtable.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/proc-fns.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable-prot.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/fixmap.h \
    $(wildcard include/config/acpi/apei/ghes.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/boot.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/fixmap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/pgtable.h \
    $(wildcard include/config/have/arch/transparent/hugepage/pud.h) \
    $(wildcard include/config/have/arch/soft/dirty.h) \
    $(wildcard include/config/arch/enable/thp/migration.h) \
    $(wildcard include/config/have/arch/huge/vmap.h) \
    $(wildcard include/config/x86/espfix64.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/extable.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/checksum.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/checksum.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dma-mapping.h \
    $(wildcard include/config/have/generic/dma/coherent.h) \
    $(wildcard include/config/has/dma.h) \
    $(wildcard include/config/arch/has/dma/set/coherent/mask.h) \
    $(wildcard include/config/need/dma/map/state.h) \
    $(wildcard include/config/dma/api/debug.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/device.h \
    $(wildcard include/config/debug/devres.h) \
    $(wildcard include/config/generic/msi/irq/domain.h) \
    $(wildcard include/config/pinctrl.h) \
    $(wildcard include/config/generic/msi/irq.h) \
    $(wildcard include/config/dma/cma.h) \
    $(wildcard include/config/of.h) \
    $(wildcard include/config/devtmpfs.h) \
    $(wildcard include/config/sysfs/deprecated.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ioport.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kobject.h \
    $(wildcard include/config/uevent/helper.h) \
    $(wildcard include/config/debug/kobject/release.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sysfs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kernfs.h \
    $(wildcard include/config/kernfs.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/idr.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kobject_ns.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kref.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/klist.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pinctrl/devinfo.h \
    $(wildcard include/config/pm.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pinctrl/consumer.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/seq_file.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pinctrl/pinctrl-state.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pm.h \
    $(wildcard include/config/vt/console/sleep.h) \
    $(wildcard include/config/pm/clk.h) \
    $(wildcard include/config/pm/generic/domains.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ratelimit.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/device.h \
    $(wildcard include/config/iommu/api.h) \
    $(wildcard include/config/xen.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pm_wakeup.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dma-debug.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dma-direction.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/scatterlist.h \
    $(wildcard include/config/debug/sg.h) \
    $(wildcard include/config/need/sg/dma/length.h) \
    $(wildcard include/config/sgl/alloc.h) \
    $(wildcard include/config/arch/has/sg/chain.h) \
    $(wildcard include/config/sg/pool.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mm.h \
    $(wildcard include/config/have/arch/mmap/rnd/bits.h) \
    $(wildcard include/config/have/arch/mmap/rnd/compat/bits.h) \
    $(wildcard include/config/mem/soft/dirty.h) \
    $(wildcard include/config/arch/uses/high/vma/flags.h) \
    $(wildcard include/config/x86.h) \
    $(wildcard include/config/x86/intel/memory/protection/keys.h) \
    $(wildcard include/config/ppc.h) \
    $(wildcard include/config/parisc.h) \
    $(wildcard include/config/metag.h) \
    $(wildcard include/config/ia64.h) \
    $(wildcard include/config/x86/intel/mpx.h) \
    $(wildcard include/config/device/private.h) \
    $(wildcard include/config/device/public.h) \
    $(wildcard include/config/shmem.h) \
    $(wildcard include/config/debug/vm/rb.h) \
    $(wildcard include/config/page/poisoning.h) \
    $(wildcard include/config/init/on/alloc/default/on.h) \
    $(wildcard include/config/init/on/free/default/on.h) \
    $(wildcard include/config/debug/pagealloc.h) \
    $(wildcard include/config/hibernation.h) \
    $(wildcard include/config/hugetlbfs.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/range.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/percpu-refcount.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/page_ext.h \
    $(wildcard include/config/idle/page/tracking.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stacktrace.h \
    $(wildcard include/config/stacktrace.h) \
    $(wildcard include/config/user/stacktrace/support.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stackdepot.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/page_ref.h \
    $(wildcard include/config/debug/page/ref.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/page-flags.h \
    $(wildcard include/config/arch/uses/pg/uncached.h) \
    $(wildcard include/config/memory/failure.h) \
    $(wildcard include/config/thp/swap.h) \
    $(wildcard include/config/ksm.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/tracepoint-defs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/static_key.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/memremap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/huge_mm.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched/coredump.h \
    $(wildcard include/config/core/dump/default/elf/headers.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/vmstat.h \
    $(wildcard include/config/vm/event/counters.h) \
    $(wildcard include/config/debug/tlbflush.h) \
    $(wildcard include/config/debug/vm/vmacache.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/vm_event_item.h \
    $(wildcard include/config/memory/balloon.h) \
    $(wildcard include/config/balloon/compaction.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/io.h \
  arch/arm64/include/generated/asm/early_ioremap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/early_ioremap.h \
    $(wildcard include/config/generic/early/ioremap.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/xen/xen.h \
    $(wildcard include/config/xen/pvh.h) \
    $(wildcard include/config/xen/dom0.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/io.h \
    $(wildcard include/config/generic/iomap.h) \
    $(wildcard include/config/has/ioport/map.h) \
    $(wildcard include/config/virt/to/bus.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/pci_iomap.h \
    $(wildcard include/config/pci.h) \
    $(wildcard include/config/no/generic/pci/ioport/map.h) \
    $(wildcard include/config/generic/pci/iomap.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mem_encrypt.h \
    $(wildcard include/config/arch/has/mem/encrypt.h) \
    $(wildcard include/config/amd/mem/encrypt.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/dma-mapping.h \
    $(wildcard include/config/mtk/bouncing/check.h) \
    $(wildcard include/config/iommu/dma.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/xen/hypervisor.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/xen/arm/hypervisor.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/../../../../drivers/misc/mediatek/include/mt-plat/aee.h \
    $(wildcard include/config/mtk/printk/uart/console.h) \
    $(wildcard include/config/console/lock/duration/detect.h) \
    $(wildcard include/config/mtk/ram/console.h) \
    $(wildcard include/config/mtk/aee/dram/console.h) \
    $(wildcard include/config/mach/mt6763.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/platform_device.h \
    $(wildcard include/config/suspend.h) \
    $(wildcard include/config/hibernate/callbacks.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mod_devicetable.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netdev_features.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched/clock.h \
    $(wildcard include/config/have/unstable/sched/clock.h) \
    $(wildcard include/config/irq/time/accounting.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/flow_dissector.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/in6.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/in6.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/libc-compat.h \
    $(wildcard include/config/data.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/siphash.h \
    $(wildcard include/config/have/efficient/unaligned/access.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/if_ether.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/splice.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pipe_fs_i.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/if_packet.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/flow.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/in.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/in.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netlink.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netlink.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/scm.h \
    $(wildcard include/config/security/network.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/security.h \
    $(wildcard include/config/security/infiniband.h) \
    $(wildcard include/config/security/network/xfrm.h) \
    $(wildcard include/config/security/path.h) \
    $(wildcard include/config/audit.h) \
    $(wildcard include/config/securityfs.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/nsproxy.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netlink.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/request_sock.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/sock.h \
    $(wildcard include/config/ipv6.h) \
    $(wildcard include/config/rps.h) \
    $(wildcard include/config/sock/cgroup/data.h) \
    $(wildcard include/config/inet.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/hardirq.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ftrace_irq.h \
    $(wildcard include/config/ftrace/nmi/enter.h) \
    $(wildcard include/config/hwlat/tracer.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/vtime.h \
    $(wildcard include/config/virt/cpu/accounting.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/context_tracking_state.h \
    $(wildcard include/config/context/tracking.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/hardirq.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/irq.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/irq.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/kvm_arm.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/irq_cpustat.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netdevice.h \
    $(wildcard include/config/dcb.h) \
    $(wildcard include/config/hyperv/net.h) \
    $(wildcard include/config/wlan.h) \
    $(wildcard include/config/ax25.h) \
    $(wildcard include/config/mac80211/mesh.h) \
    $(wildcard include/config/net/ipip.h) \
    $(wildcard include/config/net/ipgre.h) \
    $(wildcard include/config/ipv6/sit.h) \
    $(wildcard include/config/ipv6/tunnel.h) \
    $(wildcard include/config/netpoll.h) \
    $(wildcard include/config/bql.h) \
    $(wildcard include/config/rfs/accel.h) \
    $(wildcard include/config/fcoe.h) \
    $(wildcard include/config/xfrm/offload.h) \
    $(wildcard include/config/net/poll/controller.h) \
    $(wildcard include/config/libfcoe.h) \
    $(wildcard include/config/wireless/ext.h) \
    $(wildcard include/config/net/l3/master/dev.h) \
    $(wildcard include/config/vlan/8021q.h) \
    $(wildcard include/config/net/dsa.h) \
    $(wildcard include/config/tipc.h) \
    $(wildcard include/config/mpls/routing.h) \
    $(wildcard include/config/netfilter/ingress.h) \
    $(wildcard include/config/garp.h) \
    $(wildcard include/config/mrp.h) \
    $(wildcard include/config/cgroup/net/prio.h) \
    $(wildcard include/config/net/flow/limit.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/delay.h \
  arch/arm64/include/generated/asm/delay.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/delay.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/prefetch.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/dynamic_queue_limits.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ethtool.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/compat.h \
    $(wildcard include/config/compat/old/sigaction.h) \
    $(wildcard include/config/odd/rt/sigaction.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/if.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/hdlc/ioctl.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/aio_abi.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/ethtool.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/if_ether.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/net_namespace.h \
    $(wildcard include/config/ieee802154/6lowpan.h) \
    $(wildcard include/config/ip/sctp.h) \
    $(wildcard include/config/ip/dccp.h) \
    $(wildcard include/config/netfilter.h) \
    $(wildcard include/config/nf/defrag/ipv6.h) \
    $(wildcard include/config/netfilter/netlink/acct.h) \
    $(wildcard include/config/nf/ct/netlink/timeout.h) \
    $(wildcard include/config/wext/core.h) \
    $(wildcard include/config/mpls.h) \
    $(wildcard include/config/can.h) \
    $(wildcard include/config/net/ns.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/core.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/mib.h \
    $(wildcard include/config/xfrm/statistics.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/snmp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/snmp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/u64_stats_sync.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/unix.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/packet.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/ipv4.h \
    $(wildcard include/config/ip/multiple/tables.h) \
    $(wildcard include/config/ip/route/classid.h) \
    $(wildcard include/config/ip/mroute.h) \
    $(wildcard include/config/ip/mroute/multiple/tables.h) \
    $(wildcard include/config/ip/route/multipath.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/inet_frag.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/ipv6.h \
    $(wildcard include/config/ipv6/multiple/tables.h) \
    $(wildcard include/config/ipv6/mroute.h) \
    $(wildcard include/config/ipv6/mroute/multiple/tables.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/dst_ops.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/ieee802154_6lowpan.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/sctp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/dccp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/netfilter.h \
    $(wildcard include/config/nf/defrag/ipv4.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netfilter_defs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netfilter.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/x_tables.h \
    $(wildcard include/config/bridge/nf/ebtables.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/conntrack.h \
    $(wildcard include/config/nf/ct/proto/dccp.h) \
    $(wildcard include/config/nf/ct/proto/sctp.h) \
    $(wildcard include/config/nf/conntrack/events.h) \
    $(wildcard include/config/nf/conntrack/labels.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netfilter/nf_conntrack_tcp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netfilter/nf_conntrack_tcp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netfilter/nf_conntrack_dccp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netfilter/nf_conntrack_tuple_common.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netfilter/nf_conntrack_common.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netfilter/nf_conntrack_common.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/netfilter/nf_conntrack_sctp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netfilter/nf_conntrack_sctp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/nftables.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/xfrm.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/xfrm.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/mpls.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netns/can.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ns_common.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/seq_file_net.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/netprio_cgroup.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cgroup.h \
    $(wildcard include/config/cgroup/net/classid.h) \
    $(wildcard include/config/cgroup/data.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/cgroupstats.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/taskstats.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/user_namespace.h \
    $(wildcard include/config/inotify/user.h) \
    $(wildcard include/config/persistent/keyrings.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cgroup-defs.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bpf-cgroup.h \
    $(wildcard include/config/cgroup/bpf.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/bpf.h \
    $(wildcard include/config/efficient/unaligned/access.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/bpf_common.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/psi_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kthread.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cgroup_subsys.h \
    $(wildcard include/config/cgroup/cpuacct.h) \
    $(wildcard include/config/blk/cgroup.h) \
    $(wildcard include/config/cgroup/device.h) \
    $(wildcard include/config/cgroup/freezer.h) \
    $(wildcard include/config/cgroup/perf.h) \
    $(wildcard include/config/cgroup/hugetlb.h) \
    $(wildcard include/config/cgroup/pids.h) \
    $(wildcard include/config/cgroup/rdma.h) \
    $(wildcard include/config/cgroup/debug.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/neighbour.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/netdevice.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/if_link.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/if_link.h \
    $(wildcard include/config/pending.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/if_bonding.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/pkt_cls.h \
    $(wildcard include/config/net/cls/ind.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/pkt_sched.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/page_counter.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/memcontrol.h \
    $(wildcard include/config/memcg/swap.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/vmpressure.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/eventfd.h \
    $(wildcard include/config/eventfd.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/writeback.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/flex_proportions.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/backing-dev-defs.h \
    $(wildcard include/config/debug/fs.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/blk_types.h \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/blk/dev/throttling/low.h) \
    $(wildcard include/config/blk/inline/encryption.h) \
    $(wildcard include/config/dm/default/key.h) \
    $(wildcard include/config/blk/dev/integrity.h) \
    $(wildcard include/config/mtk/hw/fde.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bvec.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bio.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/highmem.h \
    $(wildcard include/config/x86/32.h) \
    $(wildcard include/config/debug/highmem.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cacheflush.h \
  arch/arm64/include/generated/asm/kmap_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/kmap_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mempool.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ioprio.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/iocontext.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bio-crypt-ctx.h \
    $(wildcard include/config/mmc/crypto/legacy.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/filter.h \
    $(wildcard include/config/bpf/jit.h) \
    $(wildcard include/config/bpf/jit/always/on.h) \
    $(wildcard include/config/arch/has/set/memory.h) \
    $(wildcard include/config/have/ebpf/jit.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cryptohash.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/set_memory.h \
  arch/arm64/include/generated/asm/set_memory.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/set_memory.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/sch_generic.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/gen_stats.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/gen_stats.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rtnetlink.h \
    $(wildcard include/config/net/ingress.h) \
    $(wildcard include/config/net/egress.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/rtnetlink.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/if_addr.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/rtnetlink.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/filter.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rculist_nulls.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/poll.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/poll.h \
  arch/arm64/include/generated/uapi/asm/poll.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/poll.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/dst.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/neighbour.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/tcp_states.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/net_tstamp.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/net/smc.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/mls_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/security/selinux/ss/constraint.h \

/home/w0n9/kernel-src/ksu095-cfi/throne_tracker.o: $(deps_/home/w0n9/kernel-src/ksu095-cfi/throne_tracker.o)

$(deps_/home/w0n9/kernel-src/ksu095-cfi/throne_tracker.o):
