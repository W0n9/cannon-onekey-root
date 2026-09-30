# cannon-onekey

[English](README.md) | [简体中文](README_ZH.md)

Redmi Note 9 5G(cannon)专用免解 BL 一键 root:运行时内核漏洞链,不解锁 bootloader、零分区写入。目标内核 `4.14.186-perf-00243-g9a0d96109bda`,探针对内核版本硬门拒跑,天然限定设备范围。root 不落盘,每次开机重跑一次。

## 快速开始(adb shell)

```sh
./scripts/install.sh   # 推资产到 /data/local/tmp + md5 自证(不写任何分区)
./scripts/root.sh      # 停管理器 → 跑探针 → 验收,约 10 秒
```

验收绿标记(设备日志 `/data/local/tmp/t37-last.log`):`ASHMEM LIVE AGAIN` / `UID 0 ACHIEVED` / `ARMED` / `MANAGER CROWNED` / `ROUND OK`;KernelSU 管理器应显示「工作中 (v3.3.0, 32601-2)」。

## 资产 md5 对拍表

| 文件 | md5 / 内容 |
|---|---|
| `assets/probe/bin/t37_dryrun3` | `eeadf63ac557d5a7c2d946b13de06636` |
| `assets/ko/kernelsu.ko`(v3) | `3f6ab77ed7cae2aecb742492e142edef` |
| `assets/ko/ksu-hook-set` | 内容 `0x1F` |
| `assets/manager-apk/KernelSU_v3.3.0_32601-release.apk` | `b1b4b931209c2b1215914f85d9741aba` |
| `hello.ko`(探针自检件,host 无副本,设备侧条件校验) | `f1e1233d`(前 8 位) |

## 纪律(必读)

- 跑探针前必停管理器(`me.weishu.kernelsu` + `com.miui.securitycenter`),两个脚本均已内置。
- uptime 门禁已取消(2026-09-30 拍板),开机即可跑。唯一保留失败驱动护栏:连续 panic≥2 → 冷静 70 分钟(uptime≥4200s)再单次试;仍炸先读 pstore 现场别再试。
- 别用 `t37_dryrun2` 当日常件(无 ashmem blob 修复,应用信息页崩会回潮);别动任何 `*.bak`;别删改 `ksu-hook-set`。
- 模块未载时别开管理器闲逛(历史 MIUI rescueparty 连环崩诱因)。
- root 不落盘,每次重启重跑;OTA/换内核后 uname 门会拒跑(安全特性)。

## 目录结构

```
assets/probe/    探针源码 + 日常件 t37_dryrun3
assets/ko/       kernelsu.ko v3 + 模块源码(4.14 非 GKI 自改版)+ hook 掩码
assets/manager-apk/  KernelSU manager v3.3.0(上游 release)
assets/m0a/      M0a 能力探针(诊断工具,可选)
app/m1s/         B 形态壳 app 源码(Shizuku;本期只入库未重构)
manager/KernelSU/    管理器上游源码树(许可与出处参考)
scripts/         install.sh / root.sh
docs/            oneclick-root-guide.md(A 形态手册)/ m1s-shizuku-validation.md(B 形态 runbook)
```

## 许可

- `assets/ko/`:**GPL-2.0**,继承上游 KernelSU 内核侧许可;本 ko 为 4.14 非 GKI 自改版(allowlist 持久化等)。
- `manager/KernelSU/`:**GPL-3.0**,上游原树,出处 [tiann/KernelSU](https://github.com/tiann/KernelSU)。
- `assets/probe/`、`app/m1s/`、`scripts/`:本项目自研。

## 研究票档

cannon-root 工程的研究票档/issues/handoff 留在原私有工作区,**不在本仓库发布**;本 repo 只收产品资产。`docs/` 两份手册内的票记索引仍指向原处。

## 规划中(本期未做)

- B 形态独立 app:资产内嵌自愈(Shizuku shell 域)、正式 gradle 构建管线、Shizuku 授权引导页、keystore 管理。
- 「重启后自动提醒/自动 root」增强(2026-09-30 拍板暂缓)。
