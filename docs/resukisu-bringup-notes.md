# handoff:ReSukiSU 分支 ko bring-up 研究成果(44 号票精编)

- 日期:2026-10-01 完证,2026-10-08 抽取进本 repo
- 源票:`ghostlock-app/.scratch/cannon-root/issues/44-resukisu-cannon-bringup.md`(全量证据链)
- worktree:`~/workspaces/resukisu-cannon`(branch `cannon-port`,修复 commit `ee1cdc53`)

## 0. 一句话

ReSukiSU v4.2.0-rc3 的 cannon-port 分支 ko 经三批次修复后**上设备全绿**:finit 成功、管理器识别(官方 ReSukiSU app 与主线 KernelSU app **双管理器共存识别**)、appinfo 判据双绿、零 panic。本 repo 据此提供双模一键(见 §4)。

## 1. 两次 panic 的根因(通用教训,换内核/换 ko 都适用)

| # | 炸点 | 根因 | 修复 |
|---|---|---|---|
| 1 | finit 期间 `__cfi_check_fail` target=task_work_add | task_work_add 未导出且无 `.cfi_jt`;4.14 CFI 检查在**调用方**([target-8] 哈希比对),bare 真地址必 fail → BUG;探针的 122 站 NOP 在 finit **返回后**才生效,init 在 finit **期间**调用即炸 | init 期 deferred 工作全部改 `schedule_delayed_work`(5s 后 kworker,NOP 后) |
| 2 | 同型,finit 期间 policydb_write | base 修好后 init 期 `apply_kernelsu_rules` 的裸调用不再被 vmalloc 失败短路 | `apply_kernelsu_rules` 同样挪 dwork |

**教训(通用)**:修好数据通路=打开裸调用的闸门;每批修复后必须以「修复后的新状态」重跑审计,不能沿用修复前的短路结论。凡经 kallsyms 裸地址的间接调用,一律推到探针 NOP 之后。

## 2. 真机结构布局定谳(MT6853 stock 4.14.186-perf,与 OSS 源码树的差异)

- `selinux_ss` 内 **policydb base = ss+0x10**,不是头算的 `&ss->policydb`(=ss+8;sidtab 后多 8 字节)。结构内 delta(头编译 offsetof)经实证正确:nprim=3553 落位精确、permissive_map 位写生效。
- `selinux_state->ss@+0x18`、`avc@+0x10`(与头一致,双实证)。
- policydb 在 `len` 之后**再次**分叉:内核侧 `policyvers` 位落在 db+0x1E0 且活值为位域形态(`0x800002`),头算偏移 db+0x1D8 恰读真值 30——**因此内核 `policydb_write` 拒绝(-EINVAL),policy 规则注入(write 链)在真机不可用**;方案 A 拍板:保持安全失败,permissive-only 对齐现役(链后恒 permissive 已覆盖全部实际 root 需求)。
- CONFIG 维度零差异(IKCFG 解 stock .config 对拍);差异全在结构布局(MTK 魔改)。

## 3. 双管理器支持矩阵(实测)

| ko \ 管理器 | 主线 KernelSU 3.3.0(me.weishu,uapi 2) | ReSukiSU 官方 v4.2.0-rc3(com.resukisu,uapi 4) |
|---|---|---|
| **kernelsu.ko v3**(现役,3f6ab77e) | ✅ 绿卡「工作中 v3.3.0 (32601-2)」 | ⚠️ 未验证(理论兼容:基础 cmd 编号同源) |
| **ReSukiSU ko**(96aa2036=ee1cdc53) | ⚠️ 可识别(红卡版本提示)但「工作中 [越狱模式] 35186-4」 | ✅ 全匹配绿卡「工作中 [Built-in] [越狱模式] 35186/4」 |

- **throne 扫包时序约束(必读)**:ReSukiSU ko 的签名扫包只在 finit+5s 的 dwork 跑一次——**跑链之后新装的管理器 app 必须重启+重跑链才会被识别**(fd 不装,supercall 不通,app 显示「不支持」)。两个管理器 app 都已装时,一次扫包同时识别两个 uid(10239/10208 双管理器共存实证)。
- **切换 ko 必须重启后首次跑链才生效**:探针 finit 对本 boot 已驻留的模块报 EEXIST 并按幂等继续——换了 kernelsu.ko 文件但内核里还是旧模块(root.sh 已内置该保护)。
- 主线 ko(v3)读 `ksu-hook-set` 掩码(0x1F);ReSukiSU ko **不读**(表钩 6 站固定:execve/execveat/faccessat/newfstatat/reboot/setresuid,无 prctl)。文件两模共用,保留即可。
- 加冕:主线 ko=ksu_manager_uid 直写;ReSukiSU ko=探针 P4.5 签名扫描(模块镜像内 candidate,写入读回验证)。两模探针日志都出 `MANAGER CROWNED`。
- 探针 ARMED 标记:主线 ko 有(prctl 通道);**ReSukiSU ko 缺席是预期**(无 prctl 通道,litmus ret=-1)。`ARMED` 不在验收绿清单的强制项。

## 4. 本 repo 的双模用法

```sh
./scripts/install.sh          # 推两份 ko 资产(默认生效模=kernelsu)
./scripts/root.sh             # 默认 kernelsu 模(与旧版行为一致)
./scripts/root.sh resukisu    # 切 ReSukiSU ko(若本 boot 已载另一模,提示重启)
```

- 切换=设备侧把 `ko-kernelsu.ko` / `ko-resukisu.ko` 对拷到探针固定读取的 `kernelsu.ko`,md5 复验后跑链。
- 验收:两模绿标记同为 `UID 0 ACHIEVED` / `ASHMEM LIVE AGAIN` / `MANAGER CROWNED` / `ROUND OK`;`ARMED` 仅 kernelsu 模出现。
- 管理器 app 安装:MIUI 拦一切 adb install(`USER_RESTRICTED`,`adb_install_need_confirm=0` 也拦)——**手动安装**。APK 已备 `assets/manager-apk/`。

## 5. 资产 md5(ReSukiSU 新增件)

| 文件 | md5 |
|---|---|
| `assets/ko/resukisu.ko`(ee1cdc53 编译产物) | `96aa20363a9b6e28bad30192f3a4be84` |
| `assets/manager-apk/ReSukiSU_v4.2.0-rc3_35171-arm64-v8a-release.apk` | `bc324b81f47f9087317582657e4e922f` |

## 6. 纪律提醒(两模通用)

- 探针前必停管理器(me.weishu.kernelsu + com.resukisu.resukisu + com.miui.securitycenter;root.sh 已内置三停)。
- 连续 panic≥2 → 冷静 70 分钟(uptime≥4200s)再单次试;仍炸先读 pstore 现场别再试。
- dmesg ring 只存 ~250s(wlan 刷屏),抓内核日志要趁跑链后立即,或用设备侧后台 grep 循环写文件。
- MIUI 上 `pm install` 直调与 adb install 全路被拦,别浪费时间,直接手动装 APK。
