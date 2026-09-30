package com.ghostlock.m1s;

/** m1s 特权服务(Shizuku UserService,跑在 shell/root 域)的 IPC 面。 */
interface IM1sService {
    /** 传本 app 的 APK 路径:UserService 进程无 context,读内嵌 assets(B 形态自愈)
     *  要靠它打开自身 ZipFile。绑定成功后、startChain 前调用。 */
    void provisionAssets(String apkPath);
    /** 异步启动一键链(内部起线程跑脚本);重复调用安全忽略。
     *  v4:force 参数废除(uptime 门禁已删),签名保留兼容。 */
    void startChain(boolean force);
    /** 取输出缓冲尾部 maxChars 字符(UI 轮询用)。 */
    String tailLog(int maxChars);
    boolean isRunning();
    /** 让特权进程退出(bindUserService 的非 daemon 约定)。 */
    void destroy();
}
