package com.ghostlock.m1s;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * M1sService — B 形态:Shizuku UserService。
 * 注意 Shizuku 契约:此类**不是** android.app.Service,而是直接继承 AIDL Stub
 * (starter 进程实例化后强转 IBinder);由 Shizuku 拉起的特权进程(shell uid 2000,
 * root 态启动时 uid 0)里直接 exec 设备上的 t37_dryrun3。链路本体零改动。
 *
 * v5(2026-09-30,B 形态):内嵌资产自愈——资产打进 APK assets,跑链前校验
 * /data/local/tmp 现件 md5,缺失或不符即从 APK ZipFile 解出重写(shell 域直写,
 * 与 adb push 同域),复验仍败则拒跑。md5 常量构建期从 assets 实算生成
 * (AssetHashes,勿手改),与 README/install.sh 资产表同源。
 *
 * v6(2026-10-08,双模):startChain(mode) 选 kernelsu|resukisu。设备侧布局
 * 对齐 A 形态 install.sh:两份 ko 源(ko-kernelsu.ko/ko-resukisu.ko)+ 生效位
 * kernelsu.ko(探针固定读);所选模经源对拷到生效位,语义同 root.sh §3——
 * 生效位≠所选模且本 boot 已驻留模块时拒跑提示重启(finit EEXIST,换了文件
 * 载的还是旧的,44#§3 时序约束)。ARMED 仅 kernelsu 模预期(ReSukiSU 无
 * prctl 通道,缺席=预期,docs/resukisu-bringup-notes.md §3)。
 *
 * 保留(v4):ko 在载拒跑(绝对护栏,防已 root boot 双跑);跑前 force-stop
 * 管理器+安全中心;资产 md5 自证打印;验收行回捞。uptime 门禁已删(用户拍板),
 * 失败驱动护栏(连 panic≥2 回 4200s 冷静纪律)人工执行,见 runbook。
 * v3 完成侦测:探针树按设计驻留(pin 子进程持标记页到重启,41-r127),
 * 故不等进程退出——探针输出甩 t37-host.log,脚本轮询 t37-last.log 的
 * ROUND OK/FATAL 判定完成,进度行增量回显。
 */
public class M1sService extends IM1sService.Stub {

    private static final String DST = "/data/local/tmp";

    /* 与 oneclick-root-guide.md §0 同源的一键脚本(链本体,自愈在 Java 侧先行)。
     * v6:模相关仅验收文案与复打清单(生效位已在 ensureAssets 按所选模对拷就位)。 */
    private static String scriptFor(String mode) {
        boolean resu = "resukisu".equals(mode);
        String okMsg = resu
            ? "[m1s] done ✓ ROUND OK — 开 ReSukiSU 管理器(com.resukisu)应见「工作中 [Built-in] [越狱模式] 35186/4」;ARMED 缺席是预期(无 prctl 通道)"
            : "[m1s] done ✓ ROUND OK — 管理器应见「工作中 v3.3.0 (32601-2)」(pstore 取证约 60s 后由驻留进程落盘)";
        return String.join("\n",
            "echo \"[m1s] uid=$(id -u) selinux=$(cat /proc/self/attr/current 2>/dev/null)\"",
            "up=$(cut -d. -f1 /proc/uptime 2>/dev/null || echo 0)",
            "echo \"[m1s] uptime=${up}s(无门禁,仅供参考)\"",
            "if grep -q '^kernelsu ' /proc/modules 2>/dev/null; then",
            "  echo '[m1s] REFUSE: kernelsu 模块已在载(本 boot 已 root,勿重跑;确需重跑请走 adb 手工)'; exit 5;",
            "fi",
            "echo '[m1s] force-stop 管理器与安全中心…'",
            "am force-stop me.weishu.kernelsu",
            "am force-stop com.resukisu.resukisu",
            "am force-stop com.miui.securitycenter",
            "cd /data/local/tmp || { echo '[m1s] FATAL: /data/local/tmp 不可达'; exit 9; }",
            "echo '[m1s] 资产在位复打(双保险,自愈已在 Java 侧完成):'",
            "md5sum t37_dryrun3 kernelsu.ko ko-kernelsu.ko ko-resukisu.ko 2>&1",
            "echo \"[m1s] ksu-hook-set=$(cat ksu-hook-set 2>/dev/null || echo MISSING)\"",
            "echo '[m1s] === 一键: ./t37_dryrun3 physrw 1 ==='",
            "rm -f /data/local/tmp/t37-last.log",
            "touch /data/local/tmp/t37-teardown",
            "./t37_dryrun3 physrw 1 >>/data/local/tmp/t37-host.log 2>&1 &",
            "P=$!",
            "echo \"[m1s] 探针已启动 pid=$P(进度跟踪中;pin 子进程驻留属设计,不等进程退出)\"",
            "sleep 2",
            "shown=0; term=timeout",
            "for i in $(seq 1 240); do",
            "  cur=$(grep -acE 'slide pinned|UID 0|LIVE AGAIN|ARMED|CROWNED|ROUND OK|FAIL|FATAL|REFUSE|pstore FULL' /data/local/tmp/t37-last.log 2>/dev/null || echo 0)",
            "  if [ \"$cur\" -gt \"$shown\" ]; then",
            "    grep -aE 'slide pinned|UID 0|LIVE AGAIN|ARMED|CROWNED|ROUND OK|FAIL|FATAL|REFUSE|pstore FULL' /data/local/tmp/t37-last.log 2>/dev/null | tail -n +$((shown+1)) | sed 's/^/  /'",
            "    shown=$cur",
            "  fi",
            "  grep -aq 'ROUND OK' /data/local/tmp/t37-last.log 2>/dev/null && { term=ok; break; }",
            "  grep -aqE 'FATAL|REFUSE|GATE FAILED' /data/local/tmp/t37-last.log 2>/dev/null && { term=fail; break; }",
            "  kill -0 $P 2>/dev/null || { term=exit; break; }",
            "  sleep 1",
            "done",
            "echo '[m1s] === 验收(t37-last.log 关键行) ==='",
            "grep -aE 'UID 0|LIVE AGAIN|ARMED|CROWNED|ROUND OK|FAIL|FATAL|REFUSE' /data/local/tmp/t37-last.log 2>/dev/null | tail -8",
            "case \"$term\" in",
            "  ok) echo '" + okMsg + "' ;;",
            "  *) echo \"[m1s] done ✗ ($term) — 未见 ROUND OK;把上面日志发工程侧\" ;;",
            "esac");
    }

    private final StringBuilder buf = new StringBuilder();
    private volatile boolean running = false;
    private volatile String apkPath;

    /** Shizuku starter 要求:public 无参构造。 */
    public M1sService() { }

    private synchronized void append(String s) {
        if (buf.length() > 262144) buf.delete(0, 131072);
        buf.append(s).append('\n');
    }

    @Override
    public void provisionAssets(String apkPath) {
        this.apkPath = apkPath;
    }

    // ---- B 形态自愈:内嵌资产校验/解出/复验(shell 域,与 adb push 同域) ----

    /** 跑链前置:探针+hook-set+两份 ko 源在位且 md5 吻合、生效位=所选模才放行;
     *  不符即从 APK assets 解出重写,生效位经源对拷(同 root.sh §3);
     *  生效位≠所选模且本 boot 已驻留模块 → 拒跑提示重启(EEXIST 时序约束)。 */
    private boolean ensureAssets(String mode) {
        boolean resu = "resukisu".equals(mode);
        String wantKo = resu ? AssetHashes.KO_RESUKISU : AssetHashes.KO;
        append("[m1s] 资产自愈检查(内嵌自举,mode=" + mode + ")…");
        String dProbe = devMd5(DST + "/t37_dryrun3");
        String dKsu = devMd5(DST + "/ko-kernelsu.ko");
        String dResu = devMd5(DST + "/ko-resukisu.ko");
        String dActive = devMd5(DST + "/kernelsu.ko");
        boolean probeOk = AssetHashes.PROBE.equals(dProbe) && hookSetOk();
        boolean srcsOk = AssetHashes.KO.equals(dKsu) && AssetHashes.KO_RESUKISU.equals(dResu);
        boolean activeOk = wantKo.equals(dActive);
        append("[m1s] probe=" + short8(dProbe) + " ko源ksu=" + short8(dKsu)
                + " ko源resu=" + short8(dResu) + " 生效位=" + short8(dActive)
                + "(期望 " + (resu ? "resukisu" : "kernelsu") + ")");
        if (probeOk && srcsOk && activeOk) {
            append("[m1s] 资产在位且 md5 吻合,跳过解出(生效位=" + mode + ")");
            return true;
        }
        if (!activeOk && moduleResident()) {
            append("[m1s] REFUSE: 生效位不是所选模(" + (resu ? "resukisu" : "kernelsu")
                    + ")且本 boot 已驻留模块——切换须重启后首次跑链才生效(finit EEXIST,44#§3)。"
                    + "请重启设备后重试,或保持现模。");
            return false;
        }
        append("[m1s] 资产缺失或不符 → 从 APK assets 解出…");
        if (apkPath == null) {
            append("[m1s] FATAL: 无 APK 路径(未 provision),无法自愈——拒跑");
            return false;
        }
        String[][] plan = {
            {"assets/t37_dryrun3", DST + "/t37_dryrun3"},
            {"assets/kernelsu.ko", DST + "/ko-kernelsu.ko"},
            {"assets/resukisu.ko", DST + "/ko-resukisu.ko"},
            {"assets/ksu-hook-set", DST + "/ksu-hook-set"},
        };
        try (java.util.zip.ZipFile zf = new java.util.zip.ZipFile(apkPath)) {
            for (String[] p : plan) {
                java.util.zip.ZipEntry e = zf.getEntry(p[0]);
                if (e == null) { append("[m1s] FATAL: APK 内无 " + p[0]); return false; }
                if (!writeDeviceFile(zf.getInputStream(e), p[1])) {
                    append("[m1s] FATAL: 写 " + p[1] + " 失败"); return false;
                }
            }
        } catch (Throwable th) { append("[m1s] FATAL 解资产: " + th); return false; }
        sh("chmod 755 " + DST + "/t37_dryrun3; sync");
        // 生效位:所选模源对拷(设备侧本地 cp,不经 APK 流)
        String srcKo = resu ? DST + "/ko-resukisu.ko" : DST + "/ko-kernelsu.ko";
        sh("cp '" + srcKo + "' '" + DST + "/kernelsu.ko'; sync");
        dProbe = devMd5(DST + "/t37_dryrun3");
        dKsu = devMd5(DST + "/ko-kernelsu.ko");
        dResu = devMd5(DST + "/ko-resukisu.ko");
        dActive = devMd5(DST + "/kernelsu.ko");
        boolean ok = AssetHashes.PROBE.equals(dProbe) && hookSetOk()
                && AssetHashes.KO.equals(dKsu) && AssetHashes.KO_RESUKISU.equals(dResu)
                && wantKo.equals(dActive);
        if (ok) append("[m1s] 自愈复验 OK(生效位=" + mode + ",md5=" + short8(dActive) + ")");
        else append("[m1s] FATAL: 自愈复验仍不符(probe=" + short8(dProbe)
                + " ko源ksu=" + short8(dKsu) + " ko源resu=" + short8(dResu)
                + " 生效位=" + short8(dActive) + ")——拒跑,把日志发工程侧");
        return ok;
    }

    /** 本 boot 是否已驻留 kernelsu 模块(两模模块名同源,互为 EEXIST)。
     *  用 grep -q 的 exit code(grep -c 无匹配也会打印 0,不能拿输出当布尔)。 */
    private boolean moduleResident() {
        return sh("grep -q '^kernelsu ' /proc/modules 2>/dev/null && echo yes || echo no")
                .equals("yes");
    }

    private boolean hookSetOk() {
        return "0x1F".equals(sh("cat " + DST + "/ksu-hook-set 2>/dev/null").trim());
    }

    private static String devMd5(String path) {
        String out = sh("md5sum " + path);
        return out.startsWith("[ERR]") || out.isEmpty() ? "" : out.split("\\s+")[0];
    }

    private static String short8(String s) {
        return (s == null || s.isEmpty()) ? "MISSING"
                : s.substring(0, Math.min(8, s.length())) + "…";
    }

    private static String sh(String cmd) {
        try {
            Process p = new ProcessBuilder("/system/bin/sh", "-c", cmd)
                    .redirectErrorStream(true).start();
            String out = new String(readAll(p.getInputStream()));
            p.waitFor();
            return out.trim();
        } catch (Throwable th) { return "[ERR] " + th; }
    }

    /** 二进制直写设备文件(探针 1.2MB / ko 3.7MB,经 stdin 一次流式拷贝)。 */
    private boolean writeDeviceFile(java.io.InputStream in, String devPath) {
        Process p = null;
        try {
            p = new ProcessBuilder("/system/bin/sh", "-c", "cat > '" + devPath + "'").start();
            java.io.OutputStream os = p.getOutputStream();
            byte[] b = new byte[65536];
            int n;
            while ((n = in.read(b)) > 0) os.write(b, 0, n);
            os.close();
            return p.waitFor() == 0;
        } catch (Throwable th) {
            append("[m1s] ERR writeDeviceFile(" + devPath + "): " + th);
            return false;
        } finally {
            if (p != null) p.destroy();
        }
    }

    private static byte[] readAll(java.io.InputStream in) throws Exception {
        java.io.ByteArrayOutputStream bo = new java.io.ByteArrayOutputStream();
        byte[] b = new byte[65536];
        int n;
        while ((n = in.read(b)) > 0) bo.write(b, 0, n);
        return bo.toByteArray();
    }

    // ---- 一键链(v4 语义不变,前置加自愈;v6 增模参数) ----

    @Override
    public void startChain(String mode) {
        final String m = "resukisu".equals(mode) ? "resukisu" : "kernelsu";
        if (running) { append("[m1s] 已在跑,忽略重复点击"); return; }
        running = true;
        final String script = scriptFor(m);
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (!ensureAssets(m)) { running = false; return; }
                    Process p = new ProcessBuilder("/system/bin/sh", "-c", script)
                            .redirectErrorStream(true).start();
                    BufferedReader r = new BufferedReader(
                            new InputStreamReader(p.getInputStream()));
                    String line;
                    while ((line = r.readLine()) != null) append(line);
                    append("[m1s] 进程退出 rc=" + p.waitFor());
                } catch (Throwable th) {
                    append("[m1s] ERR " + th);
                }
                running = false;
            }
        }, "m1s-chain");
        t.start();
    }

    @Override
    public String tailLog(int maxChars) {
        synchronized (this) {
            int n = buf.length();
            return buf.substring(Math.max(0, n - Math.max(0, maxChars)));
        }
    }

    @Override
    public boolean isRunning() { return running; }

    @Override
    public void destroy() { System.exit(0); }
}
