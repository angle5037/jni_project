package com.launch.chyc.jni;

import com.launch.core.jni.StringWraper;

public class JniAlgorithmTest {

    static {                       // 告诉 JVM 去 bin 目录找 libJniAlgorithm.so
        System.load(System.getProperty("user.dir") + "/bin/libJniAlgorithm.so");
    }

    // 声明 native 方法（签名必须与头文件一致）
    private native void getScAndIncode(String vin, String out,
                                       StringWraper sc, StringWraper in);

    public static void main(String[] args) {
        String vin  = "LFPHC7CC6L1B75928";
        String out  = "0A0E0C";

        StringWraper sc  = new StringWraper();
        StringWraper inc = new StringWraper();

        new JniAlgorithmTest().getScAndIncode(vin, out, sc, inc);

        System.out.println("SC     = " + sc.value);
        System.out.println("Incode = " + inc.value);
    }
}
