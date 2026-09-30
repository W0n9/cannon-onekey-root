package com.ghostlock.m1s;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * M1sService — 42 号票 M1(B 形态):Shizuku UserService。
 * 注意 Shizuku 契约:此类**不是** android.app.Service,而是直接继承 AIDL Stub
 * (starter 进程实例化后强转 IBinder);由 Shizuku 拉起的特权进程(shell uid 2000,
 * root 态启动时 uid 0)里直接 exec 设备上现成的 t37_dryrun3。链路本体零改动。
 */
public class M1sService extends IM1sService.Stub {

    /* 与 oneclick-root-guide.md §0 同源的一键脚本。
     * v4(2026-09-30):uptime 门禁删除(用户拍板;boot36 102s 样本+多次开机即
     * root 全绿,39 票 4 连 panic 系 buggy-era ko,F1+blob 修复后零复发)。
     * 保留:ko 在载拒跑(绝对护栏,防已 root boot 双跑);跑前 force-stop
     * 管理器+安全中心;资产 md5 自证打印;验收行回捞。
     * 失败驱动护栏(人工):连 panic≥2 回 4200s 冷静纪律,见 runbook §1.2。
     * v3 完成侦测:探针树按设计驻留(pin 子进程持标记页到重启,41-r127),
     * 故不等进程退出——探针输出甩 t37-host.log,脚本轮询 t37-last.log 的
     * ROUND OK/FATAL 判定完成,进度行增量回显。 */
    private static final String SCRIPT = String.join("\n",
        "echo \"[m1s] uid=$(id -u) selinux=$(cat /proc/self/attr/current 2>/dev/null)\"",
        "up=$(cut -d. -f1 /proc/uptime 2>/dev/null || echo 0)",
        "echo \"[m1s] uptime=${up}s(无门禁,仅供参考)\"",
        "if grep -q kernelsu /proc/modules 2>/dev/null; then",
        "  echo '[m1s] REFUSE: kernelsu 模块已在载(本 boot 已 root,勿重跑;确需重跑请走 adb 手工)'; exit 5;",
        "fi",
        "echo '[m1s] force-stop 管理器与安全中心…'",
        "am force-stop me.weishu.kernelsu",
        "am force-stop com.miui.securitycenter",
        "cd /data/local/tmp || { echo '[m1s] FATAL: /data/local/tmp 不可达'; exit 9; }",
        "echo '[m1s] 资产校验(须 t37_dryrun3=eeadf63a.. / kernelsu.ko=3f6ab77e.. / mask=0x1F):'",
        "md5sum t37_dryrun3 kernelsu.ko 2>&1",
        "echo \"[m1s] ksu-hook-set=$(cat ksu-hook-set 2>/dev/null || echo MISSING)\"",
        "echo '[m1s] === 一键: ./t37_dryrun3 physrw 1 ==='",
        "rm -f /data/local/tmp/t37-last.log",
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
        "  ok) echo '[m1s] done ✓ ROUND OK — 管理器应见「工作中 v3.3.0 (32601-2)」(pstore 取证约 60s 后由驻留进程落盘)' ;;",
        "  *) echo \"[m1s] done ✗ ($term) — 未见 ROUND OK;把上面日志发工程侧\" ;;",
        "esac"
    );

    private final StringBuilder buf = new StringBuilder();
    private volatile boolean running = false;

    /** Shizuku starter 要求:public 无参构造。 */
    public M1sService() { }

    private synchronized void append(String s) {
        if (buf.length() > 262144) buf.delete(0, 131072);
        buf.append(s).append('\n');
    }

    @Override
    public void startChain(boolean force) {
        // v4:force 参数废除(uptime 门禁已删),AIDL 签名保留以兼容旧客户端
        if (running) { append("[m1s] 已在跑,忽略重复点击"); return; }
        running = true;
        final String script = SCRIPT;
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
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
