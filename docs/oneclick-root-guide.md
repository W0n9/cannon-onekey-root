# cannon 一键 root 日常手册

- 设备：Redmi Note 9 5G(cannon),MIUI V14.0.6.0.SJECNXM,内核 `4.14.186-perf-00243-g9a0d96109bda`
- 版本:v1.1(2026-09-30)——配套日常态:**t37_dryrun3(eeadf63a)+ kernelsu.ko=8dd5a9ad + ksu-hook-set=0x1F**;★uptime 门禁取消(用户拍板:boot36 102s 样本 + 多次 m1s 开机即 root 全绿;39 票 4 连 panic 系 buggy-era ko,F1+blob 修复后零复发)★
- 原理一句话：运行时漏洞链(免解 BL、零分区写入)拿到内核读写 → 加载 KernelSU ko 并接管 syscall 表 → 管理器加冕。**root 不随重启保留,每次开机后重跑一次。**

---

## 0. TL;DR(每次重启后)

```bash
# 1. 停管理器(开着跑探针是历史事故源)——开机即可,无需等 uptime
adb shell 'am force-stop me.weishu.kernelsu; am force-stop com.miui.securitycenter'

# 2. 一键
adb shell "cd /data/local/tmp && ./t37_dryrun3 physrw 1"

# 3. 验收(约 10 秒跑完)
adb shell "tail -5 /data/local/tmp/t37-last.log"
# 看到 ROUND OK 即成;开管理器应显示「工作中 v3.3.0 (32601-2)」
```

---

## 1. 分步详解

### 1.1 连接

设备经 usbip 进来。`adb devices` 没设备时:

```bash
sudo usbip attach -r 100.96.123.50 -b 1-1
# 还不行就:adb kill-server && adb start-server,再看 adb devices
```

### 1.2 窗口纪律(已取消,留档)

~~uptime≥2500s 门禁~~:**2026-09-30 起取消**(用户拍板)。实证:boot36 于 uptime 102s 硬跑全链全绿零 panic(42 号票 c6),其后用户多次 m1s 开机即 root 成功。

> 来历(留档勿删):门禁系 39 号票时代事故——boot 早期(155–200s)syscall 流量大,撞 **buggy-era ko** 新装 faccessat hook 的 CFI 边缘 → 4 连 panic。该 ko 早已退役(F1+blob 修复后零复发),门禁失去保护对象。slide 双采门/page oracle 双跑门「不稳即拒跑」仍在(那是探针内生保护,与 uptime 无关)。
>
> **保留唯一失败驱动护栏**:若连续 panic≥2,回 uptime≥4200s(70 分钟)冷静后再单次试;仍炸先读 pstore 现场别再试。

### 1.3 跑前必停管理器

```bash
adb shell 'am force-stop me.weishu.kernelsu; am force-stop com.miui.securitycenter'
```

### 1.4 一键执行

```bash
adb shell "cd /data/local/tmp && ./t37_dryrun3 physrw 1"
```

- 全自动:uname 硬门(内核版本不符直接拒跑)→ slide 自采(median 投票+双采同值门,采不一致会拒跑,重跑即可)→ root 链 → ashmem 真表回写(blob 修复)→ finit 载 ko → 48/48 NOP → ARM → 加冕管理器 → 自清场退出。
- 全程约 10 秒;终端无输出属正常(日志在设备 `/data/local/tmp/t37-last.log`)。

### 1.5 验收

```bash
adb shell "grep -E 'UID 0|LIVE AGAIN|ARMED|CROWNED|ROUND OK|FAIL' /data/local/tmp/t37-last.log | tail -6"
```

全链绿标记应齐:`ASHMEM LIVE AGAIN` / `UID 0 ACHIEVED` / `ARMED` / `MANAGER CROWNED` / `ROUND OK`。

然后开 KernelSU 管理器:主页卡应显示 **「工作中」+「v3.3.0 (32601-2)」**。dmesg 应有 `hooked nr` 五行(execve/execveat/faccessat/newfstatat/prctl 全装)+ ARM 段 nr 142/147。

### 1.6 成功后可以

- 管理器里给 app 授 root(超级用户页签);
- **应用信息页随便开、装 app 随便装**——blob 崩(ashmem 毒化)已修复(r142/r143/r145,0/25 实测零崩);
- 设备可长期带 root 跑(已实测 5 小时+ 零 panic 零崩)。

---

## 2. 常见问题与自救

| 症状 | 原因/处置 |
|---|---|
| `adb devices` 空 | usbip 掉线:`sudo usbip attach -r 100.96.123.50 -b 1-1`;再不行 `adb kill-server && adb start-server` |
| 探针拒跑/双采不同值 | 系统忙或采样抖动,直接重跑(拒跑是保护,不是故障) |
| 跑完管理器「获取 Root 失败!」 | ko/掩码不对:`md5sum` 对表(见 §3)——须 kernelsu.ko=8dd5a9ad 且 ksu-hook-set=0x1F(只 hook prctl 的旧实验态 0x10 没 su) |
| 设备突然自己重启 | 先别动,连上后 `getprop ro.boot.bootreason`:`kernel_panic`=内核炸(读 t37-last.log 的 pstore-tail 段,现场会自动归档);`rescueparty`=用户态连环崩(blob 修复后不应再出现,若出现=资产回潮,查探针 md5) |
| 跑完后又 panic | 罕见(r143 起三跑三绿)。等自动重启,uptime≥4200s 再单次重试;仍炸先读现场别再试 |
| 应用信息页崩/装 app 连环崩重启 | **你跑了 t37_dryrun2 或资产回潮**——t37_dryrun2 无 blob 修复。改跑 t37_dryrun3,md5 对表 |

## 3. 资产清单(md5 对拍用)

设备 `/data/local/tmp/`:

| 文件 | md5(前 8 位) | 说明 |
|---|---|---|
| `t37_dryrun3` | `eeadf63a` | **日常一键二进制**(修正 true-restore 版) |
| `kernelsu.ko` | `3f6ab77e` | **v3(2026-09-30 起)= v2 + save 路径 cred 修复**(inline save 原以 manager uid 跑,/data/adb 0700 必拒 → 授权从不落盘;现 override 到 kernel-root cred,授权跨 boot 留存) |
| `ksu-hook-set` | 内容=`0x1F` | hook 掩码(0x1F=全 5 项;勿删勿改) |
| `hello.ko` | `f1e1233d` | 探针自检件 |
| `t37_dryrun3.d236c6a9.bak` | — | 旧探针备份(勿删) |
| `kernelsu.ko.8dd5a9ad.bak` | `8dd5a9ad` | v1 ko 备份(无 allowlist 持久化;回滚用) |
| `kernelsu.ko.51103394.bak` | `51103394` | v2 ko 备份(init-load 有、save 仍坏;已被 v3 取代) |
| `kernelsu.ko.f96aa996.bak` | `f96aa996` | 旧 ko 正本备份(无掩码无 F1,已退役) |

**警示:`t37_dryrun2` 是 r110 冻结件,没有 blob 修复——跑了 root 照常成,但应用信息页崩/装 app 重启会回潮。日常别用它。**

host 侧源码/备份:`/home/w0n9/kernel-src/ksu095-cfi/`(ko 源码,bak/ 下有 f96aa996)、`.scratch/cannon-root/prototype/t37-dryrun/`(探针源码 `t37_dryrun.c` + 备份 `t37_dryrun3.d236c6a9.bak`)。

资产重建(`/data/local/tmp` 被清时):

```bash
adb push /home/w0n9/workspaces/ghostlock-app/.scratch/cannon-root/prototype/t37-dryrun/t37_dryrun3 /data/local/tmp/
adb push /home/w0n9/kernel-src/ksu095-cfi/kernelsu.ko /data/local/tmp/
adb shell 'chmod 755 /data/local/tmp/t37_dryrun3; printf 0x1F > /data/local/tmp/ksu-hook-set; sync'
# hello.ko(探针自检小模块)host 无副本,靠设备自带 hello.ko.bak 恢复;双失再议
```

## 4. 禁忌(血泪浓缩)

1. ~~uptime<2500s 别跑探针~~(2026-09-30 取消);唯一保留:连续 panic≥2 后冷静 70 分钟(uptime≥4200s)再单次试。
2. **跑探针前必停管理器**(me.weishu.kernelsu + securitycenter)。
3. **别用 t37_dryrun2 当日常件**(blob 崩回潮);别动任何 `*.bak`。
4. **别删/改 `ksu-hook-set`**(删掉=8dd5a9ad 默认全装,行为等价;但留着 0x1F 是自证文件)。
5. 模块未载时别开管理器闲逛(历史 rescueparty 诱因;r145 后未再验证该路径,保持纪律)。
6. root 不落盘:**每次重启都要重跑**;OTA/换内核后 uname 门会拒跑(安全特性)。

## 5. 脱离电脑:手机上本地一键(无线调试自连)

**为什么需要 shell 域**:探针自采 slide 用 `perf_event_open`、physrw 校准用 `/proc/self/pagemap`——普通 App 域(untrusted_app)被 SELinux 挡死,**Termux 直接跑二进制不可行**。adb 的 shell 域与传输介质无关(USB/usbip/网络等价),所以让手机自连自己的 adbd 即可。

**一次性准备**:
1. 开发者选项里开「无线调试」;
2. 装 [LADB](https://github.com/tytydraco/LADB)(自带 adb 客户端的本地终端)或 Shizuku+终端;
3. 按 LADB 引导配对(配对码/端口在无线调试页里;MIUI 上端口每次开关会变)。

**每次开机后**:
1. 确认无线调试开着(MIUI 重启后可能自动关,需手动再开并重新配对——这是 MIUI 的 UX 变量,实测后再固化);
2. LADB 里执行(开机即可,无需等 uptime):
   ```sh
   am force-stop me.weishu.kernelsu; am force-stop com.miui.securitycenter
   cd /data/local/tmp && ./t37_dryrun3 physrw 1
   tail -5 /data/local/tmp/t37-last.log   # 见 ROUND OK 即成
   ```

**注意**:①安全上别用 `adb tcpip 5555` 裸开端口(无认证,同 WiFi 任何人可连)——无线调试(配对+TLS)才是正路;②本路径**未实测**(shell 域等价性是 adb 协议层事实,变数在 MIUI 无线调试的重启保持性);③进阶:Shizuku 配开机自启+延时任务可逼近「开机 45 分钟后自动 root,零操作」,同样待实测。

## 6. 票记索引(想深挖的看这里)

- 37 号票(`.scratch/cannon-root/issues/37-tier2-dryrun.md`):r130 CFI 机理定谳 / r140 掩码谜底 / **r142-r143 blob 炸弹定谳+修复 / r144 全链终验 / r145 日常态终版**
- 41 号票(`issues/41-f1-direct-bl.md`):F1 票收官(追记 E/F)
- 38/39 号票收官见 memory:`redmi-note9-5g-unsupported` / `cannon-39-supercall-progress`
