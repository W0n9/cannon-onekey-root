cmd_/home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.o := /home/w0n9/kernel-src/clang-383902/clang-r383902/bin/clang -Wp,-MD,/home/w0n9/kernel-src/ksu095-cfi/.ksu_selinux_resolve.o.d -nostdinc -isystem /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/include -I/home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include -I./arch/arm64/include/generated  -I/home/w0n9/kernel-src/cannon-r-oss/include -I/home/w0n9/kernel-src/cannon-r-oss/drivers/misc/mediatek/include -I./include -I/home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I/home/w0n9/kernel-src/cannon-r-oss/include/uapi -I./include/generated/uapi -include /home/w0n9/kernel-src/cannon-r-oss/include/linux/kconfig.h  -I/home/w0n9/kernel-src/ksu095-cfi -I/home/w0n9/kernel-src/ksu095-cfi -D__KERNEL__ -Qunused-arguments -mlittle-endian -DKASAN_SHADOW_SCALE_SHIFT=3 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 --target=aarch64-linux-gnu --prefix=/usr/bin/ --gcc-toolchain=/usr -no-integrated-as -Werror=unknown-warning-option -fuse-ld=lld -fno-PIE -mno-implicit-float -DCONFIG_AS_LSE=1 -fno-asynchronous-unwind-tables -fno-pic -Wno-asm-operand-widths -DKASAN_SHADOW_SCALE_SHIFT=3 -fno-delete-null-pointer-checks -Wno-int-in-bool-context -Wno-address-of-packed-member -O2 --param=allow-store-data-races=0 -DCC_HAVE_ASM_GOTO -Wframe-larger-than=3600 -fstack-protector-strong -Wno-format-invalid-specifier -Wno-gnu -Wno-duplicate-decl-specifier -Wno-tautological-compare -mno-global-merge -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -flto -fvisibility=default -fsplit-lto-unit -fsanitize=cfi -fno-sanitize-cfi-canonical-jump-tables -fsanitize-cfi-cross-dso -Wdeclaration-after-statement -Wno-pointer-sign -Wno-array-bounds -fno-strict-overflow -fno-merge-all-constants -fno-stack-check -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time -Werror=incompatible-pointer-types -fmacro-prefix-map=/home/w0n9/kernel-src/cannon-r-oss/= -Wno-initializer-overrides -Wno-unused-value -Wno-format -Wno-sign-compare -Wno-format-zero-length -Wno-uninitialized  -I/home/w0n9/kernel-src/cannon-r-oss/security/selinux  -I/home/w0n9/kernel-src/cannon-r-oss/security/selinux/include  -I./security/selinux -include /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/errno.h -DKSU_VERSION=32601 -DKSU_SYSCALL_HOOK=1 -DKSU39_SELINUX_PATCH -DKSU_COMPAT_HAS_CURRENT_SID -DKSU_COMPAT_HAS_SELINUX_STATE -DEXPECTED_SIZE=0x033b -DEXPECTED_HASH=\"c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6\" -Wno-implicit-function-declaration -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat -Wno-declaration-after-statement -Wno-unused-function  -DMODULE -mcmodel=large  -DKBUILD_BASENAME='"ksu_selinux_resolve"'  -DKBUILD_MODNAME='"kernelsu"' -c -o /home/w0n9/kernel-src/ksu095-cfi/.tmp_ksu_selinux_resolve.o /home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.c

source_/home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.o := /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/share/cfi_blacklist.txt

deps_/home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.o := \
  /home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.c \
    $(wildcard include/config/kallsyms/all.h) \
    $(wildcard include/config/security/selinux/develop.h) \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kernel.h \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/arch/has/refcount.h) \
    $(wildcard include/config/panic/timeout.h) \
    $(wildcard include/config/tracing.h) \
  /home/w0n9/kernel-src/clang-383902/clang-r383902/lib64/clang/11.0.1/include/stdarg.h \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/stddef.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/stddef.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/types.h \
  arch/arm64/include/generated/uapi/asm/types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/int-ll64.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/int-ll64.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/bitsperlong.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/bitsperlong.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/bitsperlong.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/posix_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/posix_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/asm-generic/posix_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/stack/validation.h) \
    $(wildcard include/config/kasan.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/barrier.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/barrier.h \
    $(wildcard include/config/smp.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kasan-checks.h \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/kallsyms.h \
    $(wildcard include/config/kallsyms.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/errno.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/errno.h \
  arch/arm64/include/generated/uapi/asm/errno.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uaccess.h \
    $(wildcard include/config/compat.h) \
    $(wildcard include/config/preempt/count.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/sched.h \
    $(wildcard include/config/debug/preempt.h) \
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
    $(wildcard include/config/thread/info/in/task.h) \
    $(wildcard include/config/mtk/sched/boost.h) \
    $(wildcard include/config/cgroup/sched.h) \
    $(wildcard include/config/preempt/notifiers.h) \
    $(wildcard include/config/blk/dev/io/trace.h) \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/tasks/rcu.h) \
    $(wildcard include/config/psi.h) \
    $(wildcard include/config/memcg.h) \
    $(wildcard include/config/slob.h) \
    $(wildcard include/config/compat/brk.h) \
    $(wildcard include/config/cgroups.h) \
    $(wildcard include/config/cc/stackprotector.h) \
    $(wildcard include/config/arch/has/scaled/cputime.h) \
    $(wildcard include/config/cpu/freq/times.h) \
    $(wildcard include/config/virt/cpu/accounting/gen.h) \
    $(wildcard include/config/no/hz/full.h) \
    $(wildcard include/config/mtk/mlog.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/posix/timers.h) \
    $(wildcard include/config/sysvipc.h) \
    $(wildcard include/config/detect/hung/task.h) \
    $(wildcard include/config/auditsyscall.h) \
    $(wildcard include/config/rt/mutexes.h) \
    $(wildcard include/config/debug/mutexes.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lockdep/crossrelease.h) \
    $(wildcard include/config/ubsan.h) \
    $(wildcard include/config/block.h) \
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
    $(wildcard include/config/vmap/stack.h) \
    $(wildcard include/config/livepatch.h) \
    $(wildcard include/config/security.h) \
    $(wildcard include/config/mtk/task/turbo.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/sched/tune.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/sched.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/current.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/pid.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rculist.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
    $(wildcard include/config/page/poisoning/zero.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/uapi/linux/const.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcupdate.h \
    $(wildcard include/config/rcu/stall/common.h) \
    $(wildcard include/config/rcu/nocb/cpu.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/prove/rcu.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/arch/weak/release/acquire.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/atomic.h \
    $(wildcard include/config/generic/atomic64.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/atomic.h \
    $(wildcard include/config/arm64/lse/atomics.h) \
    $(wildcard include/config/as/lse.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/lse.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/atomic_ll_sc.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/cmpxchg.h \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/atomic-long.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/irqflags.h \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/irqflags.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/ptrace.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/ptrace.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/hwcap.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/uapi/asm/hwcap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/ptrace.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/preempt.h \
  arch/arm64/include/generated/asm/preempt.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/preempt.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/thread_info.h \
    $(wildcard include/config/have/arch/within/stack/frames.h) \
    $(wildcard include/config/hardened/usercopy.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/restart_block.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/thread_info.h \
    $(wildcard include/config/arm64/sw/ttbr0/pan.h) \
    $(wildcard include/config/shadow/call/stack.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/memory.h \
    $(wildcard include/config/arm64/va/bits.h) \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bottom_half.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/lockdep.h \
    $(wildcard include/config/lock/stat.h) \
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
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/fpsimd.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/hw_breakpoint.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/virt.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/sections.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/sections.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable-hwdef.h \
    $(wildcard include/config/pgtable/levels.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/bitmap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rcutree.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/wait.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/spinlock_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/spinlock_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rwlock_types.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/spinlock.h \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/err.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/jhash.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/unaligned/packed_struct.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/list_nulls.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/wq/watchdog.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/timer.h \
    $(wildcard include/config/cfi/clang.h) \
    $(wildcard include/config/debug/objects/timers.h) \
    $(wildcard include/config/no/hz/common.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/ktime.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/seqlock.h \
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
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/uaccess.h \
    $(wildcard include/config/arm64/pan.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/kernel-pgtable.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable.h \
    $(wildcard include/config/transparent/hugepage.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/proc-fns.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/pgtable-prot.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/fixmap.h \
    $(wildcard include/config/acpi/apei/ghes.h) \
    $(wildcard include/config/unmap/kernel/at/el0.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/boot.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/fixmap.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/pgtable.h \
    $(wildcard include/config/have/arch/transparent/hugepage/pud.h) \
    $(wildcard include/config/have/arch/soft/dirty.h) \
    $(wildcard include/config/arch/enable/thp/migration.h) \
    $(wildcard include/config/have/arch/huge/vmap.h) \
    $(wildcard include/config/x86/espfix64.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/mm_types.h \
    $(wildcard include/config/have/cmpxchg/double.h) \
    $(wildcard include/config/have/aligned/struct/page.h) \
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
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/spin/on/owner.h) \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  arch/arm64/include/generated/asm/rwsem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/asm-generic/rwsem.h \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/completion.h \
    $(wildcard include/config/lockdep/completions.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/uprobes.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/uprobes.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/debug-monitors.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/esr.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/probes.h \
    $(wildcard include/config/kprobes.h) \
  /home/w0n9/kernel-src/cannon-r-oss/include/linux/page-flags-layout.h \
  include/generated/bounds.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/sparsemem.h \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/mmu.h \
    $(wildcard include/config/harden/branch/predictor.h) \
  /home/w0n9/kernel-src/cannon-r-oss/arch/arm64/include/asm/extable.h \
  /home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.h \

/home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.o: $(deps_/home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.o)

$(deps_/home/w0n9/kernel-src/ksu095-cfi/ksu_selinux_resolve.o):
