# m1s(Shizuku 壳一键 root)验证 runbook —— 每次重启后

- 版本:v4.0(2026-09-30):**B 形态 m1s v5(gradle 构建,release 签名)——资产内嵌自愈上线**,三件套(探针/ko/hook-set)打进 APK assets,跑链前自动校验 + 缺损自愈,**不再依赖 adb 预置 /data/local/tmp**;Shizuku 授权引导页内化(直达按钮 + MIUI 坑文案);ko v3=3f6ab77e(allowlist save 修复)不变
- 前置资产:**仅 APK 本身**(内嵌自愈)。Shizuku 13.6.0 需已装并随每次重启启动

---

## 0. TL;DR

**开机 → 开无线调试 → Shizuku「通过无线调试启动」→ 开 m1s → 点「一键 ROOT」→ 见 ROUND OK → 管理器「工作中」。无门禁,开机即可跑;资产缺损自动从 APK 自愈,零 adb 依赖。**

## 1. 首装(一次)

1. **卸载旧 m1s(v4)**:旧版 debug 签名与 release 不同,无法覆盖(v5 起固定 release 签名,今后升级可覆盖)。
2. **安装 m1s release APK v5.0**(md5 见 README 资产表;`adb install` 或设备上直接装)。
3. 打开 m1s → 首次 Shizuku 授权(下面 §2 步骤走一遍)。

## 2. 每次重启后(约 2 分钟)

1. **开机**。
2. **开无线调试**:设置 → 更多设置 → 开发者选项 → **无线调试** → 开。(WiFi 须连接;MIUI 弹网络确认请允许。)
3. **启动 Shizuku**:开 Shizuku →「**通过无线调试启动**」→ 配对(下拉通知栏输 6 位配对码)→ 启动。
4. **开 m1s**:状态行应显示 `Shizuku v13 uid=2000(shell) …`;「Shizuku 未运行」时 app 显示**引导面板**(三直达按钮:打开/装 Shizuku、跳无线调试、跳开发者选项),按按钮走上面步骤即可。
   - MIUI 杀后台就把 Shizuku 锁定(最近任务卡片下拉加锁)。
5. **点「一键 ROOT」→「跑」**:跑链前 app 自动校验三件套 md5,缺失/损坏自动从 APK 重解(日志见 `[m1s] 资产自愈检查`);随后探针约 15 秒。
6. 日志尾 `ROUND OK` + 状态行变绿即成(探针进程驻留是设计行为,不是卡死)。
7. **开 KernelSU 管理器**:「**工作中 v3.3.0 (32601-2)**」。授权跨 boot 留存(ko v3)。

## 3. 异常表

| 现象 | 处置 |
|---|---|
| Shizuku 配对失败/找不到配对入口 | 截屏发我;备选 LADB 路线(走电脑装) |
| m1s 里 `REFUSE: kernelsu 模块已在载` | 本 boot 已 root,直接用,别再点 |
| m1s 里 `双采不同值/DOUBLE-SAMPLE GATE FAILED` | 系统没稳,再点一次即可(拒跑是保护) |
| m1s 里 `FATAL: 自愈复验仍不符` | 资产自愈失败(罕见);把日志发工程侧,勿反复点 |
| 跑着跑着设备自己重启 | **别动**,告诉我(我读 pstore 现场);纪律:连续两次 panic 后冷静 70 分钟 |
| 无线调试开关重启后没了 | 正常(MIUI 变量),每次重启重新开+重新配对(实测全程<2 分钟) |

## 4. uptime 门禁(已删除,留档)

门禁来历(留档):boot 早期(155–200s)系统流量撞新装 faccessat hook 曾 **4 连 panic**(39 号票血泪,**buggy-era ko**);修复态 ko 有 102s 硬跑零 panic 样本(boot36)+ 用户多次开机即 root 成功。**2026-09-30 起纪律取消,且 m1s v4 起已从 app 里整个删除**(force 勾选/等待按钮/脚本 REFUSE 全清)。保留唯一失败驱动护栏:连 panic≥2 次就回 70 分钟(uptime≥4200s)冷静纪律,并先报工程侧读 pstore。ko 在载拒跑是绝对护栏,不变。

## 5. 背景一句话

m1s 本体不含任何漏洞代码——它只是通过 Shizuku 在 shell 域执行设备上的 `t37_dryrun3 physrw 1`(与 adb 跑完全同域同权),外加自愈护栏(跑链前三件套 md5 校验,缺损从 APK assets 经 shell 域解出重写并复验,复验败拒跑)、纪律护栏(ko 在载拒跑/跑前停管理器;uptime 门禁已删除,见 §4)。root 不落盘,每次重启都要重跑;但 **root 授权(管理器里的 app 列表)跨 boot 留存**(save 修复在 ko v3=3f6ab77e 落地,已端到端验证)。
