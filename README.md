# cannon-onekey

[English](README.md) | [简体中文](README_ZH.md)

One-click root for the **Redmi Note 9 5G (cannon)** via a runtime kernel exploit chain — no bootloader unlock, zero partition writes. Targets kernel `4.14.186-perf-00243-g9a0d96109bda`; the exploit binary hard-refuses any other kernel build, which naturally bounds the device scope. Root does not persist across reboots — re-run the chain after every boot.

## Quick start (adb shell)

```sh
./scripts/install.sh   # push assets to /data/local/tmp + md5 self-check (no partition writes)
./scripts/root.sh      # stop the manager apps → run the exploit probe → verify, ~10 s
```

Success marks in the device log `/data/local/tmp/t37-last.log`: `ASHMEM LIVE AGAIN` / `UID 0 ACHIEVED` / `ARMED` / `MANAGER CROWNED` / `ROUND OK`. The KernelSU manager app should then report **working (v3.3.0, 32601-2)**.

## Asset checksums

| File | md5 / content |
|---|---|
| `assets/probe/bin/t37_dryrun3` | `eeadf63ac557d5a7c2d946b13de06636` |
| `assets/ko/kernelsu.ko` (v3) | `3f6ab77ed7cae2aecb742492e142edef` |
| `assets/ko/ksu-hook-set` | content `0x1F` |
| `assets/manager-apk/KernelSU_v3.3.0_32601-release.apk` | `b1b4b931209c2b1215914f85d9741aba` |
| `hello.ko` (probe self-test module; no host copy, device-side conditional check) | `f1e1233d` (first 8 hex) |

## Safety rules (must read)

- Always stop the manager apps (`me.weishu.kernelsu` + `com.miui.securitycenter`) before running the probe; both scripts do this for you.
- The uptime gate was lifted by decision (2026-09-30): the chain may run right after boot. The one remaining failure-driven guard: after ≥ 2 consecutive panics, cool down for 70 minutes (uptime ≥ 4200 s) and retry once; if it still panics, read the pstore forensics before trying again.
- Never use `t37_dryrun2` as the daily driver (it lacks the ashmem blob fix; app-info crashes resurface). Do not touch any `*.bak` files; never delete or edit `ksu-hook-set`.
- Do not open the manager app while the module is not loaded (a historic trigger of MIUI rescueparty crash loops).
- Root is not persistent: re-run after every reboot. After an OTA or kernel change the uname gate refuses to run (by design).

## Directory layout

```
assets/probe/        exploit probe sources + daily binary t37_dryrun3
assets/ko/           kernelsu.ko v3 + module sources (4.14 non-GKI variant) + hook mask
assets/manager-apk/  KernelSU manager v3.3.0 (upstream release)
assets/m0a/          M0a capability probe (optional diagnostic tool)
app/m1s/             form-B shell app sources (Shizuku; imported as-is, not reworked in this release)
manager/KernelSU/    upstream manager source tree (license / provenance reference)
scripts/             install.sh / root.sh
docs/                oneclick-root-guide.md (form-A manual) / m1s-shizuku-validation.md (form-B runbook)
```

## Licensing

- `assets/ko/`: **GPL-2.0**, inherited from the upstream KernelSU kernel-side license; the module is a self-maintained 4.14 non-GKI variant (allowlist persistence, etc.).
- `manager/KernelSU/`: **GPL-3.0**, upstream tree, provenance: [tiann/KernelSU](https://github.com/tiann/KernelSU).
- `assets/probe/`, `app/m1s/`, `scripts/`: original work of this project.

## Research archive

The research tickets, issues and handoffs of the cannon-root engineering effort stay in the original private workspace and are **not** published in this repo; product assets only. The ticket indexes inside the two `docs/` manuals still point there.

## Roadmap (not in this release)

- Form B: standalone app with embedded, self-healing assets (Shizuku shell domain), a Gradle-based build pipeline, guided Shizuku authorization, and keystore management.
- Auto-remind / auto-root after reboot (deferred by decision, 2026-09-30).
