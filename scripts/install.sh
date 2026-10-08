#!/usr/bin/env bash
# cannon-onekey 安装脚本:推送资产到设备 /data/local/tmp 并 md5 自证。
# 适用设备:Redmi Note 9 5G (cannon),内核 4.14.186-perf-00243-g9a0d96109bda。
# 探针有 uname 硬门,内核不符会拒跑;本脚本不写任何分区。
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DST=/data/local/tmp

# ---- md5 对拍表(与 docs/oneclick-root-guide.md §3、docs/resukisu-bringup-notes.md §5 一致) ----
MD5_PROBE=eeadf63ac557d5a7c2d946b13de06636   # t37_dryrun3(日常一键二进制,修正 true-restore 版)
MD5_KO=3f6ab77ed7cae2aecb742492e142edef      # kernelsu 模 ko(kernelsu.ko v3,v2 + save 路径 cred 修复)
MD5_RESU=96aa20363a9b6e28bad30192f3a4be84    # resukisu 模 ko(ReSukiSU cannon-port @ ee1cdc53)
MD5_HELLO=f1e1233d                            # hello.ko(探针自检件,host 无副本,仅设备侧条件校验,前 8 位)

PROBE="$REPO_ROOT/assets/probe/bin/t37_dryrun3"
KO="$REPO_ROOT/assets/ko/kernelsu.ko"
RESU="$REPO_ROOT/assets/ko/resukisu.ko"
MASK="$REPO_ROOT/assets/ko/ksu-hook-set"

die() { echo "INSTALL FAIL: $*" >&2; exit 1; }

# ---- 1. repo 内资产自证 ----
for f in "$PROBE" "$KO" "$RESU" "$MASK"; do
  [ -r "$f" ] || die "缺资产 $f"
done
[ "$(md5sum "$PROBE" | awk '{print $1}')" = "$MD5_PROBE" ] || die "探针 md5 不符:$(md5sum "$PROBE")"
[ "$(md5sum "$KO" | awk '{print $1}')" = "$MD5_KO" ] || die "ko md5 不符:$(md5sum "$KO")"
[ "$(md5sum "$RESU" | awk '{print $1}')" = "$MD5_RESU" ] || die "resukisu ko md5 不符:$(md5sum "$RESU")"
grep -q '0x1F' "$MASK" || die "hook 掩码不是 0x1F:$(cat "$MASK")"
echo "repo 资产自证 OK(含 resukisu.ko)"

# ---- 2. 设备检查 ----
adb start-server >/dev/null 2>&1 || true
adb devices | grep -qw 'device$' || die "无设备。usbip 掉线时:sudo usbip attach -r 100.96.123.50 -b 1-1;再 adb kill-server && adb start-server"

# ---- 3. 推送(两份 ko 源 + 掩码;默认生效位=kernelsu 模) ----
adb push "$PROBE" "$DST/" >/dev/null
adb push "$KO" "$DST/ko-kernelsu.ko" >/dev/null
adb push "$RESU" "$DST/ko-resukisu.ko" >/dev/null
adb push "$MASK" "$DST/" >/dev/null
adb shell "cp $DST/ko-kernelsu.ko $DST/kernelsu.ko; chmod 755 $DST/t37_dryrun3; sync"

# ---- 4. 设备侧 md5 回验 ----
[ "$(adb shell md5sum $DST/t37_dryrun3 | awk '{print $1}')" = "$MD5_PROBE" ] || die "设备侧探针 md5 不符"
[ "$(adb shell md5sum $DST/ko-kernelsu.ko | awk '{print $1}')" = "$MD5_KO" ] || die "设备侧 kernelsu 模 ko md5 不符"
[ "$(adb shell md5sum $DST/ko-resukisu.ko | awk '{print $1}')" = "$MD5_RESU" ] || die "设备侧 resukisu 模 ko md5 不符"
[ "$(adb shell md5sum $DST/kernelsu.ko | awk '{print $1}')" = "$MD5_KO" ] || die "设备侧生效位 kernelsu.ko md5 不符"

# hello.ko:探针自检件,host 无副本,设备已有;存在才校验,缺失提示不阻塞(guide §3 同款约定)
DEV_HELLO="$(adb shell "md5sum $DST/hello.ko 2>/dev/null | awk '{print \$1}'" | tr -d '\r' || true)"
if [ -n "$DEV_HELLO" ]; then
  case "$DEV_HELLO" in "$MD5_HELLO"*) echo "hello.ko 在位并吻合 ($DEV_HELLO)";; *) echo "WARN: hello.ko md5=$DEV_HELLO 与对拍表 $MD5_HELLO 不符,请按手册核对" >&2;; esac
else
  echo "WARN: 设备无 hello.ko(探针自检件缺失,首次跑链前请按手册 §3 评估)"
fi

echo "INSTALL OK:t37_dryrun3 / ko-kernelsu.ko / ko-resukisu.ko / ksu-hook-set(0x1F) 已就位于 $DST(生效位 kernelsu.ko=kernelsu 模)"
echo "下一步:./scripts/root.sh [kernelsu|resukisu]"
echo "管理器 APK 在 assets/manager-apk/(两份)——MIUI 拦 adb install(USER_RESTRICTED),需在手机上手动安装(docs/resukisu-bringup-notes.md §4)。"
