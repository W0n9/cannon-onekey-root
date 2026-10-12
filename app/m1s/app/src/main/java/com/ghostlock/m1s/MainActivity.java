package com.ghostlock.m1s;

import android.animation.ValueAnimator;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Outline;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewOutlineProvider;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import rikka.shizuku.Shizuku;

/**
 * m1s — 42 号票 M1(B 形态)一键 root 壳:Shizuku(shell 域)承载现成探针。
 * 状态机:启动 Shizuku → 授权 → 一键跑链。
 * 全链零 native;探针与资产用设备 /data/local/tmp 现成件(跨重启持久)。
 *
 * v7(2026-10-12,UI 重构):界面换「驾驶舱」形态——状态 Hero 卡 + 主按钮 +
 * 分段选模 + 引导卡 + 日志底部抽屉。设计来源:prototype/m1s-ui 分支
 * (app/m1s/prototype/ui-variants.html,三变体交互稿)变体 A,用户选定;
 * B(大圆钮)/C(终端流)废弃。状态机与跑链语义一行未动。
 *
 * v6(2026-10-08,双模):分段选 kernelsu|resukisu(记住上次选择),传给
 * startChain(mode);验收文案按模——kernelsu 应见「工作中 v3.3.0 (32601-2)」
 * 且 ARMED 出现,resukisu 应见「工作中 [Built-in] [越狱模式] 35186/4」且
 * ARMED 缺席是预期(无 prctl 通道,docs/resukisu-bringup-notes.md §3)。
 * 切换 ko 须重启后首跑生效(EEXIST),服务侧有守卫,UI 提示说明。
 * v4:uptime 门禁删除(2026-09-30 用户拍板)。失败驱动护栏在脚本侧保留
 * (连 panic≥2 由人工回 4200s 纪律,见 oneclick-root-guide.md §1.2)。
 */
public class MainActivity extends Activity {
    private static final int RC_PERM = 41;
    private static final String PREFS = "m1s";
    private static final String KEY_MODE = "mode";
    private static final String MODE_KSU = "kernelsu";
    private static final String MODE_RESU = "resukisu";

    // ---- 驾驶舱色板(对齐 prototype/m1s-ui 变体 A;零 androidx,手绘 GradientDrawable)----
    private static final int C_BG        = Color.rgb(0xF6, 0xF7, 0xFB);
    private static final int C_CARD      = Color.rgb(0xFF, 0xFF, 0xFF);
    private static final int C_INK       = Color.rgb(0x19, 0x1C, 0x22);
    private static final int C_INK_SUB   = Color.rgb(0x5D, 0x62, 0x70);
    private static final int C_INK_MUTE  = Color.rgb(0x8A, 0x8F, 0x9C);
    private static final int C_HERO      = Color.rgb(0xE7, 0xEC, 0xF7);  // 默认(运行中)
    private static final int C_OK_BG     = Color.rgb(0xDD, 0xF2, 0xE3);
    private static final int C_OK_DOT    = Color.rgb(0x1C, 0x8A, 0x44);
    private static final int C_OK_INK    = Color.rgb(0x14, 0x53, 0x2D);
    private static final int C_WARN_BG   = Color.rgb(0xFD, 0xEE, 0xDA);
    private static final int C_WARN_DOT  = Color.rgb(0xD0, 0x7A, 0x1E);
    private static final int C_WARN_INK  = Color.rgb(0x7C, 0x4A, 0x03);
    private static final int C_BAD_BG    = Color.rgb(0xFB, 0xE3, 0xE3);
    private static final int C_BAD_DOT   = Color.rgb(0xC3, 0x33, 0x27);
    private static final int C_BAD_INK   = Color.rgb(0x8F, 0x1D, 0x12);
    private static final int C_IDLE_DOT  = Color.rgb(0x5B, 0x6B, 0x8C);
    private static final int C_PRIMARY   = Color.rgb(0x14, 0x6C, 0x2E);
    private static final int C_PRIM_WARN = Color.rgb(0xC9, 0x6A, 0x10);
    private static final int C_PRIM_OFF  = Color.rgb(0x9A, 0xA1, 0xAF);
    private static final int C_SEG_BG    = Color.rgb(0xEC, 0xEF, 0xF5);
    private static final int C_DRAWER    = Color.rgb(0x10, 0x13, 0x1A);
    private static final int C_LOG_INK   = Color.rgb(0xB9, 0xC2, 0xD4);
    private static final int C_PEEK_INK  = Color.rgb(0x8A, 0x93, 0xA8);
    private static final int C_GRAB      = Color.rgb(0x3A, 0x41, 0x52);
    private static final int C_GUIDE_BTN = Color.rgb(0x33, 0x41, 0x5E);
    private static final int C_GUIDE_BR  = Color.rgb(0xD4, 0xD9, 0xE4);
    private static final int C_NUM_BG    = Color.rgb(0xE7, 0xEC, 0xF7);
    private static final int C_NUM_INK   = Color.rgb(0x42, 0x55, 0x7A);
    private static final int C_STEP_INK  = Color.rgb(0x3A, 0x3F, 0x4B);

    private TextView heroDot, heroBig, heroSub, banner, segKsu, segResu, drawerPeek, logTv;
    private LinearLayout heroCard, guideCard;
    private Button btnPrimary;
    private View drawer;
    private ScrollView logScroll;
    private String versionName = "?";

    private final Handler ui = new Handler(Looper.getMainLooper());
    private IM1sService svc;
    private Shizuku.UserServiceArgs svcArgs;
    private boolean running;
    private boolean drawerOpen;
    private int lastLogHash;

    private enum State { NO_SHIZUKU, NO_PERM, READY }
    private enum Result { NONE, OK, REFUSE, FAIL }
    private Result lastResult = Result.NONE;

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
        try {
            versionName = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (Throwable ignored) { }

        // 状态栏融入浅色驾驶舱
        getWindow().setStatusBarColor(C_BG);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR);

        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(C_BG);
        root.addView(buildContent(), new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        drawer = buildDrawer();
        FrameLayout.LayoutParams dlp = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(46));
        dlp.gravity = Gravity.BOTTOM;
        root.addView(drawer, dlp);
        setContentView(root);

        Shizuku.addBinderReceivedListenerSticky(binderRecvListener);
        Shizuku.addBinderDeadListener(binderDeadListener);
        Shizuku.addRequestPermissionResultListener(permListener);

        btnPrimary.setOnClickListener(v -> onButton());

        log("m1s v" + versionName + " 就绪。双模:KernelSU(主线)/ ReSukiSU,选模后一键(shell 域,零改动)。");
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

    // ================= 视图构建 =================

    private View buildContent() {
        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(16);
        col.setPadding(pad, dp(10), pad, dp(46 + 16)); // 底部留抽屉 peek 高度

        // App 栏:m1s + 版本
        LinearLayout bar = new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        TextView brand = new TextView(this);
        brand.setText("m1s");
        brand.setTextSize(20);
        brand.setTypeface(Typeface.DEFAULT_BOLD);
        brand.setTextColor(C_INK);
        bar.addView(brand);
        TextView ver = new TextView(this);
        ver.setText("  v" + versionName + " · 一键 root(B 形态)");
        ver.setTextSize(11);
        ver.setTextColor(C_INK_MUTE);
        bar.addView(ver);
        LinearLayout.LayoutParams barLp = margins(col, bar, 0, 0, 0, 12);
        barLp.leftMargin = dp(4);

        buildHero(col);
        buildBanner(col);
        buildPrimary(col);
        buildModeCard(col);
        buildGuideCard(col);

        ScrollView sv = new ScrollView(this);
        sv.addView(col);
        return sv;
    }

    private void buildHero(LinearLayout col) {
        heroCard = new LinearLayout(this);
        heroCard.setOrientation(LinearLayout.HORIZONTAL);
        heroCard.setGravity(Gravity.CENTER_VERTICAL);
        int pad = dp(18);
        heroCard.setPadding(pad, pad, pad, pad);
        heroCard.setBackground(round(24, C_HERO));
        clipRound(heroCard, 24);

        heroDot = new TextView(this);
        heroDot.setGravity(Gravity.CENTER);
        heroDot.setTextColor(Color.WHITE);
        heroDot.setTextSize(20);
        heroDot.setTypeface(Typeface.DEFAULT_BOLD);
        LinearLayout.LayoutParams dlp = new LinearLayout.LayoutParams(dp(44), dp(44));
        dlp.rightMargin = dp(14);
        heroCard.addView(heroDot, dlp);

        LinearLayout texts = new LinearLayout(this);
        texts.setOrientation(LinearLayout.VERTICAL);
        heroBig = new TextView(this);
        heroBig.setTextSize(17);
        heroBig.setTypeface(Typeface.DEFAULT_BOLD);
        heroBig.setTextColor(C_INK);
        texts.addView(heroBig);
        heroSub = new TextView(this);
        heroSub.setTextSize(12);
        heroSub.setTypeface(Typeface.MONOSPACE);
        heroSub.setTextColor(C_INK_SUB);
        LinearLayout.LayoutParams slp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        slp.topMargin = dp(3);
        texts.addView(heroSub, slp);
        heroCard.addView(texts, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        margins(col, heroCard, 0, 0, 0, 12);
    }

    private void buildBanner(LinearLayout col) {
        banner = new TextView(this);
        banner.setTextSize(13);
        banner.setLineSpacing(0, 1.2f);
        int ph = dp(14), pv = dp(12);
        banner.setPadding(ph, pv, ph, pv);
        banner.setVisibility(View.GONE);
        margins(col, banner, 0, 0, 0, 12);
    }

    private void buildPrimary(LinearLayout col) {
        btnPrimary = new Button(this);
        btnPrimary.setTextSize(17);
        btnPrimary.setTypeface(Typeface.DEFAULT_BOLD);
        btnPrimary.setTextColor(Color.WHITE);
        btnPrimary.setAllCaps(false);
        btnPrimary.setMinHeight(0);
        btnPrimary.setMinimumHeight(0);
        int pv = dp(18);
        btnPrimary.setPadding(0, pv, 0, pv);
        btnPrimary.setStateListAnimator(null);
        clipRound(btnPrimary, 20);
        margins(col, btnPrimary, 0, 0, 0, 12);
    }

    private void buildModeCard(LinearLayout col) {
        LinearLayout card = card();
        card.addView(cardTitle("生效位(切换 ko 须重启后首跑生效)"));

        LinearLayout seg = new LinearLayout(this);
        seg.setOrientation(LinearLayout.HORIZONTAL);
        seg.setBackground(round(14, C_SEG_BG));
        int p = dp(4);
        seg.setPadding(p, p, p, p);
        clipRound(seg, 14);
        segKsu = segItem(seg, "KernelSU(主线)");
        segResu = segItem(seg, "ReSukiSU");
        card.addView(seg, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        segKsu.setOnClickListener(v -> selectMode(MODE_KSU));
        segResu.setOnClickListener(v -> selectMode(MODE_RESU));

        TextView hint = new TextView(this);
        hint.setText("若本 boot 已驻留另一模,服务侧 EEXIST 守卫会拒跑——重启后首跑才切换生效。选择会记住。");
        hint.setTextSize(11);
        hint.setTextColor(C_INK_MUTE);
        hint.setLineSpacing(0, 1.3f);
        LinearLayout.LayoutParams hlp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        hlp.topMargin = dp(8);
        card.addView(hint, hlp);

        margins(col, card, 0, 0, 0, 12);
        updateSeg();
    }

    private TextView segItem(LinearLayout seg, String label) {
        TextView tv = new TextView(this);
        tv.setText(label);
        tv.setTextSize(13);
        tv.setGravity(Gravity.CENTER);
        int pv = dp(10);
        tv.setPadding(0, pv, 0, pv);
        seg.addView(tv, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        return tv;
    }

    // ---- 引导卡(仅 NO_SHIZUKU 可见;runbook §1 步骤内化,直达按钮保留) ----

    private void buildGuideCard(LinearLayout col) {
        guideCard = card();
        guideCard.addView(cardTitle("首次使用:启动 Shizuku(每次重启后)"));

        String[] steps = {
            "跳「开发者选项」→ 无线调试 → 开(WiFi 须连接;MIUI 弹网络确认请允许)",
            "开 Shizuku →「通过无线调试启动」→ 配对(下拉通知栏输 6 位码)",
            "启动成功后回本 app,状态行变绿即可一键",
        };
        for (int i = 0; i < steps.length; i++) {
            LinearLayout row = new LinearLayout(this);
            row.setOrientation(LinearLayout.HORIZONTAL);
            TextView n = new TextView(this);
            n.setText(String.valueOf(i + 1));
            n.setTextSize(11);
            n.setTypeface(Typeface.DEFAULT_BOLD);
            n.setTextColor(C_NUM_INK);
            n.setGravity(Gravity.CENTER);
            n.setBackground(circle(C_NUM_BG));
            LinearLayout.LayoutParams nlp = new LinearLayout.LayoutParams(dp(20), dp(20));
            nlp.rightMargin = dp(10);
            nlp.topMargin = dp(1);
            row.addView(n, nlp);
            TextView t = new TextView(this);
            t.setText(steps[i]);
            t.setTextSize(13);
            t.setTextColor(C_STEP_INK);
            t.setLineSpacing(0, 1.3f);
            row.addView(t, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            LinearLayout.LayoutParams rlp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            rlp.topMargin = dp(8);
            guideCard.addView(row, rlp);
        }

        TextView tip = new TextView(this);
        tip.setText("提示:MIUI 杀后台请把 Shizuku 锁定(最近任务卡片下拉加锁)");
        tip.setTextSize(11);
        tip.setTextColor(C_INK_MUTE);
        LinearLayout.LayoutParams tlp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        tlp.topMargin = dp(8);
        guideCard.addView(tip, tlp);

        LinearLayout btns = new LinearLayout(this);
        btns.setOrientation(LinearLayout.HORIZONTAL);
        guideBtn(btns, "打开/装 Shizuku").setOnClickListener(v -> openShizukuOrInstall());
        guideBtn(btns, "跳无线调试").setOnClickListener(v -> openWirelessDebugging());
        guideBtn(btns, "跳开发者选项").setOnClickListener(v -> {
            try {
                startActivity(new Intent(
                        android.provider.Settings.ACTION_APPLICATION_DEVELOPMENT_SETTINGS));
            } catch (Throwable t) { log("跳设置失败: " + t); }
        });
        LinearLayout.LayoutParams blp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        blp.topMargin = dp(10);
        guideCard.addView(btns, blp);

        guideCard.setVisibility(View.GONE);
        margins(col, guideCard, 0, 0, 0, 12);
    }

    private Button guideBtn(LinearLayout row, String text) {
        Button b = new Button(this);
        b.setText(text);
        b.setTextSize(12);
        b.setAllCaps(false);
        b.setTextColor(C_GUIDE_BTN);
        b.setMinHeight(0);
        b.setMinimumHeight(0);
        int pv = dp(9);
        b.setPadding(dp(4), pv, dp(4), pv);
        GradientDrawable g = round(12, C_BG);
        g.setStroke(dp(1), C_GUIDE_BR);
        b.setBackground(g);
        b.setStateListAnimator(null);
        clipRound(b, 12);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        lp.rightMargin = dp(8);
        row.addView(b, lp);
        return b;
    }

    private View buildDrawer() {
        // 深色底部抽屉:peek 抓手行 + 等宽日志体;点 peek 开合,跑链时自动展开
        LinearLayout d = new LinearLayout(this);
        d.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(C_DRAWER);
        float r = dp(22);
        bg.setCornerRadii(new float[]{r, r, r, r, 0, 0, 0, 0});
        d.setBackground(bg);
        d.setElevation(dp(8));

        FrameLayout peek = new FrameLayout(this);
        peek.setOnClickListener(v -> setDrawer(!drawerOpen));
        View grab = new View(this);
        grab.setBackground(round(2, C_GRAB));
        FrameLayout.LayoutParams glp = new FrameLayout.LayoutParams(dp(36), dp(4));
        glp.gravity = Gravity.CENTER_HORIZONTAL | Gravity.TOP;
        glp.topMargin = dp(7);
        peek.addView(grab, glp);
        drawerPeek = new TextView(this);
        drawerPeek.setText("▴ 运行日志");
        drawerPeek.setTextSize(12);
        drawerPeek.setTextColor(C_PEEK_INK);
        drawerPeek.setGravity(Gravity.CENTER);
        peek.addView(drawerPeek, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        d.addView(peek, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(46)));

        logTv = new TextView(this);
        logTv.setTextSize(11);
        logTv.setTextColor(C_LOG_INK);
        logTv.setTypeface(Typeface.MONOSPACE);
        logTv.setLineSpacing(0, 1.5f);
        int ph = dp(16);
        logTv.setPadding(ph, dp(4), ph, dp(20));
        logScroll = new ScrollView(this);
        logScroll.addView(logTv);
        d.addView(logScroll, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));
        return d;
    }

    // ================= 状态渲染 =================

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

    private String serverLine() {
        try {
            return Shizuku.pingBinder()
                    ? ("Shizuku v" + Shizuku.getVersion()
                       + " · uid=" + Shizuku.getUid() + (Shizuku.getUid() == 0 ? "(root)" : "(shell)")
                       + " · " + Shizuku.getSELinuxContext())
                    : "Shizuku 未运行";
        } catch (Throwable t) {
            return "Shizuku 状态异常";
        }
    }

    private void hero(String glyph, int dotColor, int cardColor, String big, String sub) {
        heroDot.setText(glyph);
        heroDot.setBackground(circle(dotColor));
        heroCard.setBackground(round(24, cardColor));
        heroBig.setText(big);
        heroSub.setText(sub);
    }

    private void refreshUi() {
        if (isFinishing()) return;
        State st = currentState();
        long upSec = SystemClock.uptimeMillis() / 1000;
        String srv = serverLine();

        guideCard.setVisibility(st == State.NO_SHIZUKU ? View.VISIBLE : View.GONE);
        banner.setVisibility(View.GONE);

        if (running) {
            hero("…", C_IDLE_DOT, C_HERO, "链运行中", srv + " · [链运行中…]");
            primary("链运行中…", C_PRIM_OFF, false);
            return;
        }
        switch (st) {
            case NO_SHIZUKU:
                hero("!", C_WARN_DOT, C_WARN_BG, "Shizuku 未运行", "需先启动 Shizuku(每次重启后)");
                primary("Shizuku 未运行 → 打开启动它", C_PRIM_WARN, true);
                break;
            case NO_PERM:
                hero("🔑", C_WARN_DOT, C_WARN_BG, "等待 Shizuku 授权", srv);
                primary("授予 m1s Shizuku 权限", C_PRIM_WARN, true);
                break;
            case READY:
                switch (lastResult) {
                    case OK:
                        hero("✓", C_OK_DOT, C_OK_BG, "ROUND OK", srv);
                        showBanner(C_OK_BG, C_OK_INK, MODE_RESU.equals(savedMode())
                                ? "✓ ROUND OK — 开 ReSukiSU 管理器应见「工作中 [Built-in] [越狱模式] 35186/4」(ARMED 缺席是预期)"
                                : "✓ ROUND OK — 开 KernelSU 管理器应见「工作中 v3.3.0 (32601-2)」,且 ARMED 出现");
                        break;
                    case REFUSE:
                        hero("⊘", C_WARN_DOT, C_WARN_BG, "探针拒跑", srv);
                        showBanner(C_WARN_BG, C_WARN_INK,
                                "⊘ 探针拒跑(见日志;保护性拒绝,不是故障;切换模须重启后首跑)");
                        break;
                    case FAIL:
                        hero("✗", C_BAD_DOT, C_BAD_BG, "未见 ROUND OK", srv);
                        showBanner(C_BAD_BG, C_BAD_INK, "✗ 未见 ROUND OK — 把日志发给工程侧");
                        break;
                    default:
                        hero("✓", C_OK_DOT, C_OK_BG, "就绪 · 可一键 root", srv + " · uptime=" + upSec + "s");
                        break;
                }
                primary("一键 ROOT", C_PRIMARY, true);
                break;
        }
    }

    private void showBanner(int bg, int ink, String text) {
        banner.setBackground(round(16, bg));
        banner.setTextColor(ink);
        banner.setText(text);
        banner.setVisibility(View.VISIBLE);
    }

    private void primary(String label, int color, boolean enabled) {
        btnPrimary.setText(label);
        btnPrimary.setEnabled(enabled);
        btnPrimary.setBackground(round(20, enabled ? color : C_PRIM_OFF));
    }

    private void setDrawer(boolean open) {
        if (drawerOpen == open) return;
        drawerOpen = open;
        drawerPeek.setText(open ? "▾ 运行日志" : "▴ 运行日志");
        int from = drawer.getHeight() > 0 ? drawer.getHeight() : dp(46);
        int to = open ? (int) (getResources().getDisplayMetrics().heightPixels * 0.46f) : dp(46);
        ValueAnimator anim = ValueAnimator.ofInt(from, to);
        anim.setDuration(280);
        anim.addUpdateListener(a -> {
            ViewGroup.LayoutParams lp = drawer.getLayoutParams();
            lp.height = (Integer) a.getAnimatedValue();
            drawer.setLayoutParams(lp);
        });
        anim.start();
    }

    // ---- 双模:生效位选择(记住上次选择;切换须重启后首跑生效,服务侧守卫) ----

    private void selectMode(String m) {
        if (!m.equals(savedMode())) {
            getSharedPreferences(PREFS, MODE_PRIVATE).edit().putString(KEY_MODE, m).apply();
        }
        updateSeg();
    }

    private void updateSeg() {
        boolean resu = MODE_RESU.equals(savedMode());
        segKsu.setBackground(round(11, resu ? Color.TRANSPARENT : C_CARD));
        segKsu.setTextColor(resu ? C_INK_SUB : C_OK_INK);
        segKsu.setTypeface(resu ? Typeface.DEFAULT : Typeface.DEFAULT_BOLD);
        segResu.setBackground(round(11, resu ? C_CARD : Color.TRANSPARENT));
        segResu.setTextColor(resu ? C_OK_INK : C_INK_SUB);
        segResu.setTypeface(resu ? Typeface.DEFAULT_BOLD : Typeface.DEFAULT);
    }

    private String savedMode() {
        return getSharedPreferences(PREFS, MODE_PRIVATE).getString(KEY_MODE, MODE_KSU);
    }

    // ================= 跑链(语义与 v6 一致) =================

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
                final String mode = savedMode();
                String managerHint = MODE_RESU.equals(mode)
                        ? "ReSukiSU 管理器(com.resukisu)应见「工作中 [Built-in] [越狱模式] 35186/4」"
                        : "KernelSU 管理器(me.weishu)应见「工作中 v3.3.0 (32601-2)」";
                new AlertDialog.Builder(this)
                        .setTitle("一键 root(" + mode + ")")
                        .setMessage("将 force-stop 管理器/安全中心,并在 Shizuku shell 域运行 "
                          + "t37_dryrun3 physrw 1(生效位=" + mode + ")。\n"
                          + "成功后 " + managerHint + "\n"
                          + "注意:若本 boot 已驻留另一模,会被守卫拒跑——重启后首跑才切换生效。继续?")
                        .setPositiveButton("跑", (d, w) -> startChainViaShizuku())
                        .setNegativeButton("取消", null)
                        .show();
        }
    }

    private void startChainViaShizuku() {
        if (svc != null) { beginRun(); return; }
        svcArgs = new Shizuku.UserServiceArgs(new ComponentName(this, M1sService.class))
                .daemon(false).processNameSuffix("shell").debuggable(true).version(3);
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
            lastResult = Result.NONE;
            lastLogHash = 0;
            svc.startChain(savedMode()); // v6:模参数(kernelsu|resukisu)
            refreshUi();
            setDrawer(true); // 跑链自动展开日志抽屉
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
                if (t.contains("ROUND OK"))      lastResult = Result.OK;
                else if (t.contains("REFUSE"))   lastResult = Result.REFUSE;
                else                             lastResult = Result.FAIL;
                svc.destroy();
            }
        } catch (Throwable ignored) { }
        try {
            if (svcArgs != null) Shizuku.unbindUserService(svcArgs, conn, false);
        } catch (Throwable ignored) { }
        svc = null;
        refreshUi();
    }

    // ---- 引导卡直达(与 v6 一致) ----

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

    // ================= 小工具 =================

    private LinearLayout card() {
        LinearLayout c = new LinearLayout(this);
        c.setOrientation(LinearLayout.VERTICAL);
        int ph = dp(16), pv = dp(14);
        c.setPadding(ph, pv, ph, pv);
        c.setBackground(round(20, C_CARD));
        c.setElevation(dp(1));
        clipRound(c, 20);
        return c;
    }

    private TextView cardTitle(String text) {
        TextView t = new TextView(this);
        t.setText(text);
        t.setTextSize(13);
        t.setTextColor(C_INK_SUB);
        t.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        ((LinearLayout.LayoutParams) t.getLayoutParams()).bottomMargin = dp(10);
        return t;
    }

    private LinearLayout.LayoutParams margins(LinearLayout col, View v, int l, int t, int r, int b) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.leftMargin = dp(l); lp.topMargin = dp(t); lp.rightMargin = dp(r); lp.bottomMargin = dp(b);
        col.addView(v, lp);
        return lp;
    }

    private GradientDrawable round(float rDp, int color) {
        GradientDrawable g = new GradientDrawable();
        g.setCornerRadius(dp(rDp));
        g.setColor(color);
        return g;
    }

    private GradientDrawable circle(int color) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(GradientDrawable.OVAL);
        g.setColor(color);
        return g;
    }

    private void clipRound(View v, final float rDp) {
        v.setOutlineProvider(new ViewOutlineProvider() {
            @Override public void getOutline(View view, Outline o) {
                o.setRoundRect(0, 0, view.getWidth(), view.getHeight(), dp(rDp));
            }
        });
        v.setClipToOutline(true);
    }

    private void scrollDown() {
        logScroll.post(() -> logScroll.fullScroll(ScrollView.FOCUS_DOWN));
    }

    private void log(String s) {
        logTv.append(s + "\n");
        scrollDown();
    }

    private int dp(float v) {
        return (int) (v * getResources().getDisplayMetrics().density + 0.5f);
    }
}
