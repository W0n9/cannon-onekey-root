#!/usr/bin/env python3
"""39-r108: vermagic equal-length rebuild for cannon kernelsu.ko.

This tree's kbuild emits UTS_RELEASE = 4.14.186-g30fa96c92 (62-char vermagic
value) but the device runs 4.14.186-perf-00243-g9a0d96109bda (76 chars,
+14). .modinfo is a fixed 150-byte section on both variants, so the +14
vermagic growth is paid for by shrinking description "Android KernelSU"
(16 chars) -> "KS" (2 chars), exactly the layout the loaded ref ko uses.
Replaces the whole .modinfo section content in place (same size, same
offsets) from a verified-loadable ref ko. Zero tail slack, single string
copy, no other section touched.
"""
import struct, sys

def find_section(d, want):
    e_shoff, = struct.unpack_from('<Q', d, 0x28)
    e_shentsize, e_shnum = struct.unpack_from('<HH', d, 0x3a)
    e_shstrndx, = struct.unpack_from('<H', d, 0x3e)
    shs = [struct.unpack_from('<IIQQQQIIQQ', d, e_shoff + i * e_shentsize)
           for i in range(e_shnum)]
    strtab_off = shs[e_shstrndx][4]
    for sh in shs:
        o = strtab_off + sh[0]
        if d[o:d.index(b'\0', o)].decode() == want:
            return sh[4], sh[5]
    raise SystemExit(f'section {want} not found')

ref, target = sys.argv[1], sys.argv[2]
rd = open(ref, 'rb').read()
td = bytearray(open(target, 'rb').read())
r_off, r_size = find_section(rd, '.modinfo')
t_off, t_size = find_section(td, '.modinfo')
assert r_size == t_size, f'.modinfo size mismatch: ref={r_size} target={t_size}'
rm = rd[r_off:r_off + r_size]
td[t_off:t_off + t_size] = rm
open(target, 'wb').write(td)
print(f'.modinfo replaced: {r_size}B from {ref} -> {target}')
