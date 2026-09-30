#!/usr/bin/env python3
"""Median-distance slide pinner (37-r11 method).
True slide minimizes the median distance from (ip - s) to the nearest
function-start. Validated on r10 boot: true 0x13e9a00000 -> 0x68;
wrong 0x13e8800000 -> 0x1b8de4 (3 orders of magnitude)."""
import struct, sys, bisect

def load_starts(tbl):
    starts = []
    for line in open(tbl):
        p = line.split()
        if len(p) >= 3 and p[1] in 'TtWw':
            starts.append(int(p[0], 16))
    starts.sort()
    return starts

def load_ips(path):
    """Top-512 kernel-VA u64s by frequency (samples dominate; file-header
    noise u64s are singletons and get outvoted — full-distinct scan failed
    on r11 because noise diluted the median)."""
    from collections import Counter
    cnt = Counter()
    d = open(path, 'rb').read()
    for off in range(0, len(d) - 8):
        v = struct.unpack_from('<Q', d, off)[0]
        if (v >> 40) == 0xffffff:
            cnt[v] += 1
    return [ip for ip, c in cnt.most_common(512)]

def median_dist(ips, starts, s):
    ds = []
    for ip in ips:
        a = ip - s
        i = bisect.bisect_left(starts, a)
        best = 1 << 62
        for j in (i - 1, i):
            if 0 <= j < len(starts):
                best = min(best, abs(a - starts[j]))
        ds.append(best)
    ds.sort()
    return ds[len(ds) // 2]

if __name__ == '__main__':
    perf = sys.argv[1]
    tbl = sys.argv[2] if len(sys.argv) > 2 else \
        '/home/w0n9/workspaces/ghostlock-app/.scratch/cannon-root/research/kallsyms-addresses-corrected.txt'
    ips = load_ips(perf)
    starts = load_starts(tbl)
    best = (0, 1 << 62)
    for slot in range(0, 0x20000):
        s = slot * 0x200000
        md = median_dist(ips, starts, s)
        if md < best[1]:
            best = (s, md)
    s, md = best
    print(f'BEST {hex(s)} median={hex(md)}')
    for dd in (-0x400000, -0x200000, 0x200000, 0x400000):
        print(f'  neigh {hex(s+dd)}: {hex(median_dist(ips, starts, s+dd))}')
