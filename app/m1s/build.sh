#!/bin/bash
# build.sh — m1s(42 号票 M1-B:Shizuku 壳)手工构建(无 gradle:aidl+javac+d8+aapt2+apksigner)
# 依赖:m0a-apk 同款 host 链 + 同目录下 aar-x/(shizuku api)+ prov-x/(shizuku provider)已解包
set -euo pipefail
cd "$(dirname "$0")"
SDK=$HOME/Android/Sdk
BT=$SDK/build-tools/36.0.0
AJAR=$SDK/platforms/android-36/android.jar
API_JAR=aar-x/classes.jar
PROV_JAR=prov-x/classes.jar
AIDL_JAR=aidl-x/classes.jar
SHARED_JAR=shared-x/classes.jar

[ -f "$API_JAR" ] || { echo "缺 $API_JAR(shizuku api aar 解包)"; exit 1; }
[ -f "$PROV_JAR" ] || { echo "缺 $PROV_JAR(shizuku provider aar 解包)"; exit 1; }
[ -f "$AIDL_JAR" ] || { echo "缺 $AIDL_JAR(shizuku aidl aar 解包)"; exit 1; }
[ -f "$SHARED_JAR" ] || { echo "缺 $SHARED_JAR(shizuku shared aar 解包)"; exit 1; }

echo "[1/6] aidl"
rm -rf gen classes dex out && mkdir -p gen classes dex out
$BT/aidl -I aidl -o gen aidl/com/ghostlock/m1s/IM1sService.aidl

echo "[2/6] javac"
javac --release 8 -cp "$AJAR:$API_JAR:$PROV_JAR:$AIDL_JAR:$SHARED_JAR" -d classes \
    gen/com/ghostlock/m1s/IM1sService.java \
    src/com/ghostlock/m1s/*.java

echo "[3/6] d8(我方类 + shizuku api + provider 合并成单个 classes.dex)"
$BT/d8 --min-api 26 --lib "$AJAR" --output dex \
    $(find classes -name '*.class') "$API_JAR" "$PROV_JAR" "$AIDL_JAR" "$SHARED_JAR" 2>&1 | grep -v "^Warning" || true

echo "[4/6] aapt2 link"
$BT/aapt2 link -o out/base.apk -I "$AJAR" --manifest AndroidManifest.xml \
    --java gen-res --min-sdk-version 26 --target-sdk-version 34

echo "[5/6] assemble"
cp out/base.apk out/app-unsigned.apk
( cd dex && zip -q ../out/app-unsigned.apk classes.dex )

echo "[6/6] align+sign"
$BT/zipalign -f -p 4 out/app-unsigned.apk out/app-aligned.apk
[ -f debug.keystore ] || cp ../m0a-apk/debug.keystore debug.keystore
$BT/apksigner sign --ks debug.keystore --ks-pass pass:android --key-pass pass:android \
    --out out/m1s.apk out/app-aligned.apk
ls -la out/m1s.apk && md5sum out/m1s.apk
