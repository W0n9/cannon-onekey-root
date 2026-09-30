# m1s(Shizuku 壳一键 root)验证 runbook —— 每次重启后

- 版本:v3.0(2026-09-30):**m1s v4(46a81cdd)——uptime 门禁从 app 里整个删除**(force 勾选/REFUSE/等待按钮全清;AIDL 签名保留兼容);ko v3=3f6ab77e(allowlist save 修复)已上线;M0b 已采(m0a 步骤退役)
- 前置资产(已就位,无需操作):Shizuku 13.6.0;m1s v4(Shizuku 授权已持久);/data/local/tmp(t37_dryrun3=eeadf63a、**kernelsu.ko=3f6ab77e**、ksu-hook-set=0x1F)

---

## 0. TL;DR

**开机 → 开无线调试 → Shizuku「通过无线调试启动」→ 开 m1s → 点「一键 ROOT」→ 见 ROUND OK → 管理器「工作中」。无门禁,开机即可跑(boot36 102s 硬跑全绿 + 多次开机即 root 成功实证)。**

## 1. 分步

1. **开机**。
2. **开无线调试**:设置 → 更多设置 → 开发者选项 → **无线调试** → 开。(MIUI 可能弹网络确认,允许;需 WiFi 开着。boot36 实测:开机 100s 内走完配对全流程,摩擦可接受。)
3. **启动 Shizuku**:开 Shizuku →「**通过无线调试启动**」→ 配对(通知栏输 6 位配对码)→ 启动。
   - 若配对码输入框找不到:下拉通知栏找 Shizuku 的通知,直接在通知里回复配对码。
4. **开 m1s**:状态行应显示 `Shizuku v13 uid=2000(shell) u:r:shell:s0`。
   - 显示「Shizuku 未运行」→ 回第 3 步;MIUI 杀后台就把 Shizuku 锁定(最近任务卡片下拉加锁)。
5. **点「一键 ROOT」**(m1s v4 起无门禁,开机即可跑;39 号票的 boot 早期 panic 系 buggy-era ko,F1+blob 修复后零复发)。
6. **点「一键 ROOT」→「跑」**:约 15 秒。日志尾 `ROUND OK` + 状态行变绿即成(v3 起 app 会正常显示完成,不再卡在 pstore dump——探针进程驻留是设计行为,不是卡死)。
7. **开 KernelSU 管理器**:「**工作中 v3.3.0 (32601-2)**」。**授权现在跨 boot 留存**(ko v2):授权过的 app 重启重跑链后仍在列表里。
8. 跑完成败都说一声,我远程收 `/data/local/tmp/t37-last.log` 对拍。

## 2. 异常表

| 现象 | 处置 |
|---|---|
| Shizuku 配对失败/找不到配对入口 | 截屏发我;备选 LADB 路线(走电脑装) |
| m1s 里 `REFUSE: kernelsu 模块已在载` | 本 boot 已 root,直接用,别再点 |
| m1s 里 `双采不同值/DOUBLE-SAMPLE GATE FAILED` | 系统没稳,再点一次即可(拒跑是保护) |
| 跑着跑着设备自己重启 | **别动**,告诉我(我读 pstore 现场);纪律:连续两次 panic 后冷静 70 分钟 |
| 无线调试开关重启后没了 | 正常(MIUI 变量),每次重启重新开+重新配对(boot36 实测全程<2 分钟) |

## 2.1 uptime 门禁(已删除,留档)

门禁来历(留档):boot 早期(155–200s)系统流量撞新装 faccessat hook 曾 **4 连 panic**(39 号票血泪,**buggy-era ko**);修复态 ko 有 102s 硬跑零 panic 样本(boot36)+ 用户多次开机即 root 成功。**2026-09-30 起纪律取消,且 m1s v4(46a81cdd)已从 app 里整个删除**(force 勾选/等待按钮/脚本 REFUSE 全清)。保留唯一失败驱动护栏:连 panic≥2 次就回 70 分钟(uptime≥4200s)冷静纪律,并先报工程侧读 pstore。ko 在载拒跑是绝对护栏,不变。

## 3. 背景一句话

m1s 本体不含任何漏洞代码——它只是通过 Shizuku 在 shell 域执行设备上现成的 `t37_dryrun3 physrw 1`(与 adb 跑完全同域同权),外加纪律护栏(ko 在载拒跑/跑前停管理器/资产 md5 自证;uptime 门禁已删除,见 §2.1)。root 不落盘,每次重启都要重跑;但 **root 授权(管理器里的 app 列表)从 ko v2 起跨 boot 留存**(save 修复在 ko v3=3f6ab77e 落地,已端到端验证)。
