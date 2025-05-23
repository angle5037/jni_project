package com.launch.chyc.jni;

//import com.launch.core.jni.StringWraper;

public class JniAlgorithm {

    static {
        System.load(System.getProperty("user.dir") + "/bin/libJniAlgorithm.so");
    }

    public native void getScAndIncodeG2(
            String vin, String out,
            byte[] sc16, byte[] incode16);
}
