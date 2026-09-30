package com.ghostlock.m1s;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.text.method.ScrollingMovementMethod;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import rikka.shizuku.Shizuku;

/**
 * m1s — 42 号票 M1(B 形态)一键 root 壳:Shizuku(shell 域)承载现成探针。
 * 状态机:启动 Shizuku → 授权 → 一键跑链。
 * 全链零 native;探针与资产用设备 /data/local/tmp 现成件(跨重启持久)。
 * v4:uptime 门禁删除(2026-09-30 用户拍板;boot36 102s 样本+多次开机即 root
 * 全绿,39 票 4 连 panic 系 buggy-era ko)。失败驱动护栏在脚本侧保留
 * (连 panic≥2 由人工回 4200s 纪律,见 oneclick-root-guide.md §1.2)。
 */
public class MainActivity extends Activity {
    private static final int RC_PERM = 41;

    private TextView statusTv, logTv;
    private Button btn;
    private LinearLayout guidePanel;
    private final Handler ui = new Handler(Looper.getMainLooper());

    private IM1sService svc;
    private Shizuku.UserServiceArgs svcArgs;
    private boolean running;
    private int lastLogHash;

    private final Runnable ticker = new Runnable() {
        @Override public void run() { refreshUi(); ui.postDelayed(this, 1000); }
    };

    private final Shizuku.OnRequestPermissionResultListener permListener =
            new Shizuku.OnRequestPermissionResultListener() {
                @Override public void onRequestPermissionResult(int requestCode, int grantResult) {
                    refreshUi();
                }
            };
    private final Shizuku.OnBinderReceivedListener binderRecvListener =
            new Shizuku.OnBinderReceivedListener() {
                @Override public void onBinderReceived() { refreshUi(); }
            };
    private final Shizuku.OnBinderDeadListener binderDeadListener =
            new Shizuku.OnBinderDeadListener() {
                @Override public void onBinderDead() { refreshUi(); }
            };

    private final ServiceConnection conn = new ServiceConnection() {
        @Override public void onServiceConnected(ComponentName name, IBinder binder) {
            svc = IM1sService.Stub.asInterface(binder);
            // B 形态:UserService 进程无 context,APK 路径经 AIDL 传入才能读内嵌 assets
            try { svc.provisionAssets(getApplicationInfo().sourceDir); }
            catch (Throwable t) { log("provisionAssets 失败: " + t); }
            beginRun();
        }
        @Override public void onServiceDisconnected(ComponentName name) { svc = null; }
    };

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(14);
        root.setPadding(pad, pad, pad, pad);

        statusTv = new TextView(this);
        statusTv.setTextSize(14);
        root.addView(statusTv, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        btn = new Button(this);
        btn.setTextSize(18);
        root.addView(btn, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        guidePanel = buildGuidePanel();
        root.addView(guidePanel, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        logTv = new TextView(this);
        logTv.setTextSize(11);
        logTv.setTypeface(android.graphics.Typeface.MONOSPACE);
        logTv.setMovementMethod(ScrollingMovementMethod.getInstance());
        ScrollView sv = new ScrollView(this);
        sv.addView(logTv);
        root.addView(sv, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));
        setContentView(root);

        Shizuku.addBinderReceivedListenerSticky(binderRecvListener);
        Shizuku.addBinderDeadListener(binderDeadListener);
        Shizuku.addRequestPermissionResultListener(permListener);

        btn.setOnClickListener(v -> onButton());

        log("m1s 就绪。链=现成 /data/local/tmp/t37_dryrun3 physrw 1(shell 域,零改动)。");
        ticker.run();
    }

    @Override
    protected void onDestroy() {
        ui.removeCallbacksAndMessages(null);
        Shizuku.removeRequestPermissionResultListener(permListener);
        Shizuku.removeBinderReceivedListener(binderRecvListener);
        Shizuku.removeBinderDeadListener(binderDeadListener);
        super.onDestroy();
    }

    private enum State { NO_SHIZUKU, NO_PERM, READY }

    private State currentState() {
        try {
            if (!Shizuku.pingBinder()) return State.NO_SHIZUKU;
            if (Shizuku.checkSelfPermission() != PackageManager.PERMISSION_GRANTED)
                return State.NO_PERM;
        } catch (Throwable t) {
            return State.NO_SHIZUKU;
        }
        return State.READY;
    }

    private void refreshUi() {
        if (isFinishing()) return;
        State st = currentState();
        long upSec = SystemClock.uptimeMillis() / 1000;
        String server;
        try {
            server = Shizuku.pingBinder()
                    ? ("Shizuku v" + Shizuku.getVersion()
                       + " uid=" + Shizuku.getUid() + (Shizuku.getUid() == 0 ? "(root)" : "(shell)")
                       + " " + Shizuku.getSELinuxContext())
                    : "Shizuku 未运行";
        } catch (Throwable t) {
            server = "Shizuku 状态异常";
        }
        statusTv.setText(String.format(
                "%s\nuptime=%ds%s",
                server, upSec, running ? "  [链运行中…]" : ""));

        switch (st) {
            case NO_SHIZUKU:
                guidePanel.setVisibility(android.view.View.VISIBLE);
                btn.setEnabled(true);
                btn.setText("Shizuku 未运行 → 打开启动它");
                statusTv.setTextColor(Color.rgb(0xC0, 0x50, 0x00));
                break;
            case NO_PERM:
                guidePanel.setVisibility(android.view.View.GONE);
                btn.setEnabled(true);
                btn.setText("授予 m1s Shizuku 权限");
                statusTv.setTextColor(Color.rgb(0xC0, 0x50, 0x00));
                break;
            case READY:
                guidePanel.setVisibility(android.view.View.GONE);
                btn.setEnabled(!running);
                btn.setText(running ? "链运行中…" : "一键 ROOT");
                statusTv.setTextColor(Color.rgb(0x00, 0x70, 0x00));
                break;
        }
    }

    private void onButton() {
        switch (currentState()) {
            case NO_SHIZUKU: {
                Intent i = getPackageManager().getLaunchIntentForPackage(
                        "moe.shizuku.privileged.api");
                if (i != null) startActivity(i);
                else log("未找到 Shizuku 应用");
                return;
            }
            case NO_PERM:
                try { Shizuku.requestPermission(RC_PERM); }
                catch (Throwable t) { log("requestPermission 失败: " + t); }
                return;
            case READY:
                if (running) return;
                new AlertDialog.Builder(this)
                        .setTitle("一键 root")
                        .setMessage("将 force-stop 管理器/安全中心,并在 Shizuku shell 域运行 "
                          + "t37_dryrun3 physrw 1。继续?")
                        .setPositiveButton("跑", (d, w) -> startChainViaShizuku())
                        .setNegativeButton("取消", null)
                        .show();
        }
    }

    private void startChainViaShizuku() {
        if (svc != null) { beginRun(); return; }
        svcArgs = new Shizuku.UserServiceArgs(new ComponentName(this, M1sService.class))
                .daemon(false).processNameSuffix("shell").debuggable(true).version(2);
        try {
            Shizuku.bindUserService(svcArgs, conn);
            log("[m1s] 拉起特权服务…");
        } catch (Throwable t) {
            log("bindUserService 失败: " + t);
        }
    }

    private void beginRun() {
        try {
            running = true;
            lastLogHash = 0;
            svc.startChain(false); // v4:force 参数废除(门禁已删),保留 AIDL 签名兼容
            refreshUi();
            pollLog();
        } catch (Throwable t) {
            running = false;
            log("startChain 失败: " + t);
        }
    }

    private void pollLog() {
        if (!running) return;
        ui.postDelayed(new Runnable() {
            @Override public void run() {
                try {
                    if (svc != null) {
                        String t = svc.tailLog(6000);
                        if (t.hashCode() != lastLogHash) {
                            lastLogHash = t.hashCode();
                            logTv.setText(t);
                            scrollDown();
                        }
                        if (!svc.isRunning()) { finishRun(); return; }
                    }
                } catch (Throwable t) {
                    log("poll 失败: " + t);
                }
                pollLog();
            }
        }, 700);
    }

    private void finishRun() {
        running = false;
        try {
            if (svc != null) {
                String t = svc.tailLog(6000);
                logTv.setText(t);
                scrollDown();
                if (t.contains("ROUND OK")) {
                    statusTv.setText("✓ ROUND OK — 开管理器应见「工作中 v3.3.0 (32601-2)」");
                    statusTv.setTextColor(Color.rgb(0x00, 0x70, 0x00));
                } else if (t.contains("REFUSE")) {
                    statusTv.setText("⊘ 探针拒跑(见日志;保护性拒绝,不是故障)");
                    statusTv.setTextColor(Color.rgb(0xC0, 0x50, 0x00));
                } else {
                    statusTv.setText("✗ 未见 ROUND OK — 把日志发给工程侧");
                    statusTv.setTextColor(Color.rgb(0xB0, 0x00, 0x00));
                }
                svc.destroy();
            }
        } catch (Throwable ignored) { }
        try {
            if (svcArgs != null) Shizuku.unbindUserService(svcArgs, conn, false);
        } catch (Throwable ignored) { }
        svc = null;
        refreshUi();
    }

    // ---- B 形态:Shizuku 授权引导(静态指引 + 直达按钮;runbook §1 步骤内化) ----

    private LinearLayout buildGuidePanel() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(10);
        panel.setPadding(pad, pad, pad, pad);
        panel.setBackgroundColor(Color.rgb(0x22, 0x24, 0x28));

        TextView title = new TextView(this);
        title.setText("首次使用:启动 Shizuku(每次重启后)");
        title.setTextSize(14);
        title.setTextColor(Color.rgb(0xE0, 0xE0, 0xE0));
        panel.addView(title);

        TextView steps = new TextView(this);
        steps.setTextSize(12);
        steps.setTextColor(Color.rgb(0xB8, 0xBC, 0xC2));
        steps.setText(
            "① 跳「开发者选项」→ 无线调试 → 开(WiFi 须连接;MIUI 弹网络确认请允许)\n"
          + "② 开 Shizuku →「通过无线调试启动」→ 配对(下拉通知栏输 6 位码)\n"
          + "③ 启动成功后回本 app,状态行变绿即可一键\n"
          + "提示:MIUI 杀后台请把 Shizuku 锁定(最近任务卡片下拉加锁)");
        panel.addView(steps);

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        Button bShizuku = mkGuideBtn("打开/装 Shizuku", row);
        bShizuku.setOnClickListener(v -> openShizukuOrInstall());
        Button bWireless = mkGuideBtn("跳无线调试", row);
        bWireless.setOnClickListener(v -> openWirelessDebugging());
        Button bDevOpts = mkGuideBtn("跳开发者选项", row);
        bDevOpts.setOnClickListener(v -> {
            try {
                startActivity(new Intent(
                        android.provider.Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS));
            } catch (Throwable t) { log("跳设置失败: " + t); }
        });

        panel.addView(row, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        panel.setVisibility(android.view.View.GONE);
        return panel;
    }

    private Button mkGuideBtn(String text, LinearLayout row) {
        Button b = new Button(this);
        b.setText(text);
        b.setTextSize(12);
        row.addView(b, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        return b;
    }

    private void openShizukuOrInstall() {
        Intent i = getPackageManager().getLaunchIntentForPackage("moe.shizuku.privileged.api");
        if (i != null) { startActivity(i); return; }
        // 未装:Shizuku 不在国内应用商店,给官方 GitHub release
        try {
            startActivity(new Intent(Intent.ACTION_VIEW,
                    android.net.Uri.parse("https://github.com/RikkaApps/Shizuku/releases")));
        } catch (Throwable t) { log("无浏览器可打开;请手动安装 Shizuku(13.x)"); }
    }

    private void openWirelessDebugging() {
        // AOSP 11+ 子页直跳;MIUI 定制无此 Activity 时回退开发者选项
        try {
            startActivity(new Intent().setClassName("com.android.settings",
                    "com.android.settings.Settings$WirelessDebuggingActivity"));
            return;
        } catch (Throwable ignored) { }
        try {
            startActivity(new Intent(
                    android.provider.Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS));
        } catch (Throwable t) { log("跳设置失败: " + t); }
    }

    private void scrollDown() {
        // logTv 在 ScrollView 里;滚到底
        final ScrollView sv = (ScrollView) logTv.getParent();
        sv.post(new Runnable() {
            @Override public void run() { sv.fullScroll(ScrollView.FOCUS_DOWN); }
        });
    }

    private void log(String s) {
        logTv.append(s + "\n");
    }

    private int dp(int v) {
        return (int) (v * getResources().getDisplayMetrics().density + 0.5f);
    }
}
