# 研究 ticket:t37 owner 线程内核态自旋阻塞 suspend(冻结算不上 freezer)

**状态**:open(修① victim pause 已落地,本 ticket 是 suspend 恢复的最后一里路)
**发现**:2026-10-10,耗电事故调查(用户报 root 后 16h 烧 4350mAh)

## 现象

root 链成功驻留后,round child 进程树里两个线程满转:

| 线程 | 身份 | 状态 | CPU | 性质 |
|---|---|---|---|---|
| victim(t37victim) | 毒载体 | R,100% **utime** | 1 核 | 设计内:纯用户态自旋,零 syscall(悬空 pi_blocked_on 纪律) |
| owner(t37r34c2) | PI 链 owner | R,100% **stime** | 1 核 | **卡在内核 futex/rt_mutex 路径,永不返回** |

owner 的 utime=0 / stime=全部 → 它在 `futex_pi_lock(&f_pi_chain)`(t37_dryrun.c owner_thread)里活锁。机制推断:victim 持有 f_pi_chain 的 rt_mutex 且 100% on-CPU → `owner_running()` 恒真 → owner 的 rt_mutex **osq 乐观自旋永不退出**。

## 后果链

owner 卡内核态自旋/或 D 态 → 永远不到达 freeze 点 → **freezer 冻结失败**(`suspend_stats`: failed_freeze 1051 / fail 1082 / success 939, last_failed_step=freeze, errno=-16 EBUSY)→ 整机无法 suspend → 息屏 16h uptime 占 94.5%,均流 ~270mA。

## 修①已验证的部分(本 ticket 的前提)

42-r129:teardown 默认开(清 victim 悬空 pi_blocked_on)+ victim 停放改 `pause()`。
**预期副作用**:victim 不再 on-CPU → owner 的 osq 因 `owner_running()` 变假而退出自旋,转入 rt_mutex 不可中断睡眠(D 态,0% CPU)。两个核都解放。

**但 D 态 owner 依然过不了 freezer**(不可中断睡眠不进 refrigerator)→ suspend 仍旧失败。本 ticket 就是解决它。

## 攻关方向(按推荐排序)

1. **PI 结构复原后让 owner 的 futex 正常返回**:drain 之后由 root 链(持 configfs 原语)把 f_pi_chain 的 rt_mutex 拓扑复原(waiters 清空/owner 置 victim 后由 victim FUTEX_UNLOCK_PI 级联释放),owner 拿到锁 → 正常 unlock 双 futex → `for(;;) usleep` 轻停放或直接退出。难点:毒化拓扑的逐字段逆推,需论证 unlock 路径不踩 fake_lock 树。
2. **owner 整体退出论证**:如果 do_exit 的 exit_pi_state 对"owner 持有 f_pi_target + 悬在 f_pi_chain 上"的组合安全(r123 后 reboot 已实证干净,但那是整机拆毁路径,不是单线程 exit),则 owner 可直接 return。需要逐行走 4.14 的 exit_pi_state/futex_exit 代码论证。
3. **绕过:f_pi_chain 用非 PI futex 重构形成**?——改动形成结构本身,影响面最大,非必要不动。

## 验收标准

root 链跑完后:`suspend_stats.success` 在息屏 10 分钟内单调上涨、`failed_freeze` 不涨;整机息屏电流回到 MIUI 正常水位(对照基线:<20mA 量级)。

## 现场数据(2026-10-10 采集)

- 卡死 owner:PID 13506 线程 13507,utime=0 stime=5620930(ticks),wchan=0
- golden 日志(修复前成功链):/tmp/t37-golden-dryrun3.log(host 侧,不入库)
- 基线:息屏 16h8m realtime / 15h14m uptime(94.5%),放电 4350mAh
