#!/usr/bin/env bash
# cannon-onekey 一键 root:停管理器 → 跑探针 → 验收。
# 用法:./root.sh [kernelsu|resukisu]   (缺省 kernelsu,与旧版行为一致)
# 纪律(2026-09-30 拍板):uptime 门禁已取消,开机即可跑;
# 唯一保留失败驱动护栏——连续 panic≥2 后冷静 70 分钟(uptime≥4200s)再单次试,仍炸先读 pstore 现场别再试。
# 双模说明与支持矩阵:docs/resukisu-bringup-notes.md
set -euo pipefail

DST=/data/local/tmp

# ---- 资产 md5 对拍表(与 docs/resukisu-bringup-notes.md §5 一致) ----
MD5_KO_KSU=3f6ab77ed7cae2aecb742492e142edef      # kernelsu 模:v3(v2 + save cred 修复)
MD5_KO_RESU=96aa20363a9b6e28bad30192f3a4be84     # resukisu 模:ReSukiSU cannon-port @ ee1cdc53

MODE="${1:-kernelsu}"
case "$MODE" in
  kernelsu) MD5_WANT="$MD5_KO_KSU"; SRC="$DST/ko-kernelsu.ko";;
  resukisu) MD5_WANT="$MD5_KO_RESU"; SRC="$DST/ko-resukisu.ko";;
  *) echo "用法:./root.sh [kernelsu|resukisu]" >&2; exit 1;;
esac

die() { echo "ROOT FAIL: $*" >&2; exit 1; }
md5_of() { adb shell "md5sum $1 2>/dev/null" | awk '{print $1}' | tr -d '\r'; }

adb start-server >/dev/null 2>&1 || true
adb devices | grep -qw 'device$' || die "无设备。usbip 掉线时:sudo usbip attach -r 100.96.123.50 -b 1-1;再 adb kill-server && adb start-server"

# ---- 1. 跑前必停管理器(两模统一三停;历史事故源) ----
adb shell 'am force-stop me.weishu.kernelsu; am force-stop com.resukisu.resukisu; am force-stop com.miui.securitycenter'

# ---- 2. 资产在位自证 ----
adb shell "test -x $DST/t37_dryrun3" || die "设备无探针,先跑 scripts/install.sh"

# ---- 3. 切换生效 ko(探针固定读 kernelsu.ko;cp 幂等) ----
CUR="$(md5_of "$DST/kernelsu.ko")"
if [ "$CUR" != "$MD5_WANT" ]; then
  # 生效位不是请求的模——需要换文件;但若本 boot 已驻留模块,换了也白换(finit EEXIST 幂等,载的还是旧的)
  if adb shell "grep -q '^kernelsu ' /proc/modules 2>/dev/null"; then
    die "本 boot 已驻留另一份 ko,切换须重启后首次跑链才生效(44#§3 时序约束):adb reboot 后重跑本脚本 $MODE"
  fi
  adb shell "test -r $SRC" || die "设备缺 $SRC,先跑 scripts/install.sh(新版会推两份 ko 源)"
  adb shell "cp $SRC $DST/kernelsu.ko"
  CUR="$(md5_of "$DST/kernelsu.ko")"
  [ "$CUR" = "$MD5_WANT" ] || die "切换后 md5 不符:got=$CUR want=$MD5_WANT"
fi
echo "生效 ko: $MODE ($CUR)"

# ---- 4. 一键(全自动:uname 硬门 → slide 自采双采门 → root 链 → ashmem blob 修复 → finit 载 ko → de-CFI → ARM → 加冕 → 自清场) ----
adb shell "cd $DST && ./t37_dryrun3 physrw 1"

# ---- 5. 验收(约 10 秒;终端无输出属正常,日志在设备 t37-last.log) ----
echo '---- 日志尾 5 行 ----'
adb shell "tail -5 $DST/t37-last.log" | tr -d '\r'
echo '---- 绿标记验收 ----'
adb shell "grep -E 'UID 0|LIVE AGAIN|ARMED|CROWNED|ROUND OK|FAIL' $DST/t37-last.log | tail -6" | tr -d '\r'

case "$MODE" in
  kernelsu)
    if ! adb shell "grep -q 'ARMED' $DST/t37-last.log"; then
      echo 'WARN: ARMED 缺席——本 boot 实际驻留的可能不是 kernelsu ko(切换须重启后首次跑链才生效),建议 adb reboot 后重跑。' >&2
    fi
    echo 'ROOT DONE。开 KernelSU 管理器(me.weishu)应显示「工作中 v3.3.0 (32601-2)」。';;
  resukisu)
    echo 'ROOT DONE。开 ReSukiSU 管理器(com.resukisu)应显示绿卡「工作中 [Built-in] [越狱模式] 35186/4」。'
    echo '注:ARMED 标记缺席是预期(ReSukiSU 无 prctl 通道);跑链后新装/重装的管理器 app 需重启+重跑链才被识别(throne 扫包时序,44#§3)。';;
esac
echo '若见 REFUSE/双采不同值:系统忙,直接重跑一次(拒跑是保护不是故障)。'
echo '若设备自己重启:等自动重启,连 panic≥2 则冷静 70 分钟后再单次试;仍炸先读 pstore 现场别再试。'
