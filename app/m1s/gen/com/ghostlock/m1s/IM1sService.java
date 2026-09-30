/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Using: /home/w0n9/Android/Sdk/build-tools/36.0.0/aidl -I aidl -o gen aidl/com/ghostlock/m1s/IM1sService.aidl
 *
 * DO NOT CHECK THIS FILE INTO A CODE TREE (e.g. git, etc..).
 * ALWAYS GENERATE THIS FILE FROM UPDATED AIDL COMPILER
 * AS A BUILD INTERMEDIATE ONLY. THIS IS NOT SOURCE CODE.
 */
package com.ghostlock.m1s;
/** m1s 特权服务(Shizuku UserService,跑在 shell/root 域)的 IPC 面。 */
public interface IM1sService extends android.os.IInterface
{
  /** Default implementation for IM1sService. */
  public static class Default implements com.ghostlock.m1s.IM1sService
  {
    /**
     * 异步启动一键链(内部起线程跑脚本);重复调用安全忽略。
     *  force=true 时无视 uptime<2500s 门禁(实验选项,panic 风险自负)。
     */
    @Override public void startChain(boolean force) throws android.os.RemoteException
    {
    }
    /** 取输出缓冲尾部 maxChars 字符(UI 轮询用)。 */
    @Override public java.lang.String tailLog(int maxChars) throws android.os.RemoteException
    {
      return null;
    }
    @Override public boolean isRunning() throws android.os.RemoteException
    {
      return false;
    }
    /** 让特权进程退出(bindUserService 的非 daemon 约定)。 */
    @Override public void destroy() throws android.os.RemoteException
    {
    }
    @Override
    public android.os.IBinder asBinder() {
      return null;
    }
  }
  /** Local-side IPC implementation stub class. */
  public static abstract class Stub extends android.os.Binder implements com.ghostlock.m1s.IM1sService
  {
    /** Construct the stub and attach it to the interface. */
    @SuppressWarnings("this-escape")
    public Stub()
    {
      this.attachInterface(this, DESCRIPTOR);
    }
    /**
     * Cast an IBinder object into an com.ghostlock.m1s.IM1sService interface,
     * generating a proxy if needed.
     */
    public static com.ghostlock.m1s.IM1sService asInterface(android.os.IBinder obj)
    {
      if ((obj==null)) {
        return null;
      }
      android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
      if (((iin!=null)&&(iin instanceof com.ghostlock.m1s.IM1sService))) {
        return ((com.ghostlock.m1s.IM1sService)iin);
      }
      return new com.ghostlock.m1s.IM1sService.Stub.Proxy(obj);
    }
    @Override public android.os.IBinder asBinder()
    {
      return this;
    }
    @Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
    {
      java.lang.String descriptor = DESCRIPTOR;
      if (code >= android.os.IBinder.FIRST_CALL_TRANSACTION && code <= android.os.IBinder.LAST_CALL_TRANSACTION) {
        data.enforceInterface(descriptor);
      }
      if (code == INTERFACE_TRANSACTION) {
        reply.writeString(descriptor);
        return true;
      }
      switch (code)
      {
        case TRANSACTION_startChain:
        {
          boolean _arg0;
          _arg0 = (0!=data.readInt());
          this.startChain(_arg0);
          reply.writeNoException();
          break;
        }
        case TRANSACTION_tailLog:
        {
          int _arg0;
          _arg0 = data.readInt();
          java.lang.String _result = this.tailLog(_arg0);
          reply.writeNoException();
          reply.writeString(_result);
          break;
        }
        case TRANSACTION_isRunning:
        {
          boolean _result = this.isRunning();
          reply.writeNoException();
          reply.writeInt(((_result)?(1):(0)));
          break;
        }
        case TRANSACTION_destroy:
        {
          this.destroy();
          reply.writeNoException();
          break;
        }
        default:
        {
          return super.onTransact(code, data, reply, flags);
        }
      }
      return true;
    }
    private static class Proxy implements com.ghostlock.m1s.IM1sService
    {
      private android.os.IBinder mRemote;
      Proxy(android.os.IBinder remote)
      {
        mRemote = remote;
      }
      @Override public android.os.IBinder asBinder()
      {
        return mRemote;
      }
      public java.lang.String getInterfaceDescriptor()
      {
        return DESCRIPTOR;
      }
      /**
       * 异步启动一键链(内部起线程跑脚本);重复调用安全忽略。
       *  force=true 时无视 uptime<2500s 门禁(实验选项,panic 风险自负)。
       */
      @Override public void startChain(boolean force) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(((force)?(1):(0)));
          boolean _status = mRemote.transact(Stub.TRANSACTION_startChain, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
      /** 取输出缓冲尾部 maxChars 字符(UI 轮询用)。 */
      @Override public java.lang.String tailLog(int maxChars) throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        java.lang.String _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          _data.writeInt(maxChars);
          boolean _status = mRemote.transact(Stub.TRANSACTION_tailLog, _data, _reply, 0);
          _reply.readException();
          _result = _reply.readString();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      @Override public boolean isRunning() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        boolean _result;
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_isRunning, _data, _reply, 0);
          _reply.readException();
          _result = (0!=_reply.readInt());
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
        return _result;
      }
      /** 让特权进程退出(bindUserService 的非 daemon 约定)。 */
      @Override public void destroy() throws android.os.RemoteException
      {
        android.os.Parcel _data = android.os.Parcel.obtain();
        android.os.Parcel _reply = android.os.Parcel.obtain();
        try {
          _data.writeInterfaceToken(DESCRIPTOR);
          boolean _status = mRemote.transact(Stub.TRANSACTION_destroy, _data, _reply, 0);
          _reply.readException();
        }
        finally {
          _reply.recycle();
          _data.recycle();
        }
      }
    }
    static final int TRANSACTION_startChain = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
    static final int TRANSACTION_tailLog = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
    static final int TRANSACTION_isRunning = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
    static final int TRANSACTION_destroy = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
  }
  /** @hide */
  public static final java.lang.String DESCRIPTOR = "com.ghostlock.m1s.IM1sService";
  /**
   * 异步启动一键链(内部起线程跑脚本);重复调用安全忽略。
   *  force=true 时无视 uptime<2500s 门禁(实验选项,panic 风险自负)。
   */
  public void startChain(boolean force) throws android.os.RemoteException;
  /** 取输出缓冲尾部 maxChars 字符(UI 轮询用)。 */
  public java.lang.String tailLog(int maxChars) throws android.os.RemoteException;
  public boolean isRunning() throws android.os.RemoteException;
  /** 让特权进程退出(bindUserService 的非 daemon 约定)。 */
  public void destroy() throws android.os.RemoteException;
}
