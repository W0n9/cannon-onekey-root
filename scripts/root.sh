#!/usr/bin/env bash
# cannon-onekey 一键 root:停管理器 → 跑探针 → 验收。
# 纪律(2026-09-30 拍板):uptime 门禁已取消,开机即可跑;
# 唯一保留失败驱动护栏——连续 panic≥2 后冷静 70 分钟(uptime≥4200s)再单次试,仍炸先读 pstore 现场别再试。
set -euo pipefail

DST=/data/local/tmp

die() { echo "ROOT FAIL: $*" >&2; exit 1; }

adb start-server >/dev/null 2>&1 || true
adb devices | grep -qw 'device$' || die "无设备。usbip 掉线时:sudo usbip attach -r 100.96.123.50 -b 1-1;再 adb kill-server && adb start-server"

# ---- 1. 跑前必停管理器(历史事故源;guide §1.3) ----
adb shell 'am force-stop me.weishu.kernelsu; am force-stop com.miui.securitycenter'

# ---- 2. 资产在位自证(拒装就先 install.sh) ----
adb shell "test -x $DST/t37_dryrun3" || die "设备无探针,先跑 scripts/install.sh"

# ---- 3. 一键(全自动:uname 硬门 → slide 自采双采门 → root 链 → ashmem blob 修复 → finit 载 ko → 48/48 NOP → ARM → 加冕 → 自清场) ----
adb shell "cd $DST && ./t37_dryrun3 physrw 1"

# ---- 4. 验收(约 10 秒;终端无输出属正常,日志在设备 t37-last.log) ----
echo '---- 日志尾 5 行 ----'
adb shell "tail -5 $DST/t37-last.log" | tr -d '\r'
echo '---- 绿标记验收 ----'
adb shell "grep -E 'UID 0|LIVE AGAIN|ARMED|CROWNED|ROUND OK|FAIL' $DST/t37-last.log | tail -6" | tr -d '\r'

echo 'ROOT DONE。开 KernelSU 管理器应显示「工作中 v3.3.0 (32601-2)」。'
echo '若见 REFUSE/双采不同值:系统忙,直接重跑一次(拒跑是保护不是故障)。'
echo '若设备自己重启:等自动重启,连 panic≥2 则冷静 70 分钟后再单次试;仍炸先读 pstore 现场别再试。'
