package com.ghostlock.m1s;

/** m1s 特权服务(Shizuku UserService,跑在 shell/root 域)的 IPC 面。 */
interface IM1sService {
    /** 异步启动一键链(内部起线程跑脚本);重复调用安全忽略。
     *  force=true 时无视 uptime<2500s 门禁(实验选项,panic 风险自负)。 */
    void startChain(boolean force);
    /** 取输出缓冲尾部 maxChars 字符(UI 轮询用)。 */
    String tailLog(int maxChars);
    boolean isRunning();
    /** 让特权进程退出(bindUserService 的非 daemon 约定)。 */
    void destroy();
}
