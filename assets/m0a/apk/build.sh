#!/bin/bash
# build.sh — m0a 探针 APK 手工构建(无 gradle:aapt2+d8+apksigner+NDK clang)
set -euo pipefail
cd "$(dirname "$0")"
SDK=$HOME/Android/Sdk
BT=$SDK/build-tools/36.0.0
NDKCC=$SDK/ndk/27.2.12479018/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang
AJAR=$SDK/platforms/android-36/android.jar

echo "[1/6] native lib"
$NDKCC -O2 -fPIC -shared -Wall -Wextra m0a_jni.c -llog -o lib/arm64-v8a/libm0a.so

echo "[2/6] javac"
rm -rf gen dex out && mkdir -p gen dex out
javac --release 8 -cp "$AJAR" -d gen src/com/ghostlock/m0a/MainActivity.java

echo "[3/6] d8"
$BT/d8 --min-api 26 --lib "$AJAR" --output dex gen/com/ghostlock/m0a/*.class 2>&1 | grep -v "^Warning" || true

echo "[4/6] aapt2 link"
$BT/aapt2 link -o out/base.apk -I "$AJAR" --manifest AndroidManifest.xml --java /tmp/m0a-gen --min-sdk-version 26 --target-sdk-version 34

echo "[5/6] assemble"
cp out/base.apk out/app-unsigned.apk
( cd dex && zip -q ../out/app-unsigned.apk classes.dex )
zip -0 -q out/app-unsigned.apk lib/arm64-v8a/libm0a.so

echo "[6/6] align+sign"
$BT/zipalign -f -p 4 out/app-unsigned.apk out/app-aligned.apk
[ -f debug.keystore ] || keytool -genkeypair -keystore debug.keystore -alias m0a -keyalg RSA -keysize 2048 -validity 10000 -storepass android -keypass android -dname "CN=M0A Probe" 2>/dev/null
$BT/apksigner sign --ks debug.keystore --ks-pass pass:android --key-pass pass:android --out out/m0a.apk out/app-aligned.apk
ls -la out/m0a.apk && md5sum out/m0a.apk
