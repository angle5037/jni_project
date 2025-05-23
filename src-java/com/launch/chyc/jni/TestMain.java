package com.launch.chyc.jni;

import com.launch.core.jni.StringWraper;  
import java.util.Arrays;

public class TestMain {

    public static void main(String[] args) {


        String vin  = "LSJW26N93JS123456";   //
        String outCode = "A1B2C3";           //

        if (args.length >= 2) {              //
            vin     = args[0];
            outCode = args[1];
        }

     
        byte[] sc16     = new byte[16];
        byte[] incode16 = new byte[16];


        JniAlgorithm jni = new JniAlgorithm();
        jni.getScAndIncodeG2(vin, outCode, sc16, incode16);


        System.out.printf("VIN    = %s%n", vin);
        System.out.printf("OUT    = %s%n", outCode);
        System.out.printf("SC     = %02X%02X%02X%n", sc16[0], sc16[1], sc16[2]);
        System.out.printf("Incode = %02X%02X%n", incode16[0], incode16[1]);

	System.out.print("SC16     = ");
	for (byte b : sc16) System.out.printf("%02X", b);  System.out.println();

	System.out.print("Incode16 = ");
	for (byte b : incode16) System.out.printf("%02X", b);  System.out.println();

        // System.out.println("SC16     = " + Arrays.toString(sc16));
        // System.out.println("Incode16 = " + Arrays.toString(incode16));
    }
}
