package com.ghostlock.m0a;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity {
    static { System.loadLibrary("m0a"); }
    public static native String runNative(String filesDir);

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        Log.i("M0A", "onCreate entered, calling native directly");
        String r = runNative(getFilesDir().getAbsolutePath());
        Log.i("M0A", "native returned: " + r);
    }
}
