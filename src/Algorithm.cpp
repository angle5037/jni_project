#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>  // 鏇夸唬Windows鐨勫姩鎬佸簱鍔犺浇
#include <cstdlib>  // 鐢ㄤ簬malloc
#include <cstring>  // 鐢ㄤ簬memcpy, memset, strcat绛?
#include <cstddef>  // 鐢ㄤ簬size_t
#include "../include/com_launch_chyc_jni_JniAlgorithm.h"

// Add at the top of your file
#include <fstream>
#include <ctime>
#include <string>
#include <sstream>

// Global log file
std::ofstream jniLogFile;

// Function to initialize logging
void jniInitLogging() {
    if (!jniLogFile.is_open()) {
        jniLogFile.open("/tmp/jni_algorithm.log", std::ios::app);
        
        // Add timestamp
        time_t now = time(0);
        char* dt = ctime(&now);
        jniLogFile << "\n[" << dt << "] libJniAlgorithm.so loaded" << std::endl;
    }
}

// Function to log messages
void jniLogMessage(const std::string& message) {
    if (jniLogFile.is_open()) {
        jniLogFile << message << std::endl;
    }
}

// Function to close logging
void jniCloseLogging() {
    if (jniLogFile.is_open()) {
        jniLogFile << "libJniAlgorithm.so unloaded" << std::endl;
        jniLogFile.close();
    }
}

// Add this to your JNI_OnLoad function (if you have one)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    jniInitLogging();
    jniLogMessage("JNI_OnLoad called");
    return JNI_VERSION_1_6;
}

// Add this to your JNI_OnUnload function (if you have one)
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    jniLogMessage("JNI_OnUnload called");
    jniCloseLogging();
}


JavaVM *jvm;
JNIEnv *static_env;

#ifdef __cplusplus
extern "C" {
#endif
	//jstring to char
	char* jstringToChar(JNIEnv* env, jstring jstr)
	{
		char* rtn = NULL;
		jclass strClass = env->FindClass("java/lang/String");
		jstring strencode = env->NewStringUTF("utf-8");
		jmethodID mid = env->GetMethodID(strClass, "getBytes","(Ljava/lang/String;)[B");
		jbyteArray barr = (jbyteArray)(env)->CallObjectMethod(jstr, mid,strencode);
		jsize alen = env->GetArrayLength(barr);
		jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
		if (alen > 0) {
			rtn = (char*) malloc(alen + 1);
			memcpy(rtn, ba, alen);
			rtn[alen] = '\0';
		}
		env->ReleaseByteArrayElements(barr, ba, 0);
		env->DeleteLocalRef(strClass);
		env->DeleteLocalRef(strencode);
		env->DeleteLocalRef(barr);
		return rtn;
	}
	
	//char* to jstring
	jstring stoJstring( JNIEnv* env, const char* pat )
	{
		jstring result;
		jclass strClass = env->FindClass("java/lang/String");
		jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
		jbyteArray bytes = env->NewByteArray(strlen(pat));
		env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*)pat);
		jstring encoding = env->NewStringUTF("utf-8");
		result = (jstring)(env)->NewObject(strClass, ctorID, bytes, encoding);
		
		(env)->DeleteLocalRef(strClass);
		(env)->DeleteLocalRef(bytes);
		(env)->DeleteLocalRef(encoding);
		return result;
	}

    int TranslateChar(unsigned char v)
    {
        int i = 0;
        if (v >= '0' && v <= '9')
        {
            i = v - '0';
        }
        if (v >= 'A' && v < 'a')
        {
            i =  v - 'A' + 10;
        }
        if (v >= 'a')
        {
            i =  v - 'a' + 10;
        }
        return i;
    }

    //鑾峰彇sc鐮佹椂闇�瑕佸彇vin鐮佺殑鍚?5浣嶅仛涓鸿?＄畻
	//鑰佽溅鍨嬬畻娉?
    unsigned char* GetSc(unsigned char* vinCode)
    {
        unsigned char v[6] = {0};
        for (int i = 12; i < 17; i++) //鎴?鍙杤in鐮佺殑鍚?5浣?
        {
            v[i - 12] = vinCode[i];
        }
        unsigned char* outputcode = new unsigned char[10];
        memset(outputcode, 0, sizeof(unsigned char) * 10);
        typedef void (*alg1)(unsigned char*, unsigned char*);
        
        // 浣跨敤dlopen鏇夸唬LoadLibrary
        void* handle = dlopen("libG2Encryption.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return outputcode;
        }
        
        // 浣跨敤dlsym鏇夸唬GetProcAddress
        alg1 fun = (alg1)dlsym(handle, "G2_VinToSc");
        if (NULL != fun)
        {
            fun(v, outputcode);
        }
        
        // 浣跨敤dlclose鏇夸唬FreeLibrary
        dlclose(handle);
        return outputcode;
    }

    //鑾峰彇incode鐮?
// -------- 修正版：旧车型算法 GetIncode --------
unsigned short GetIncode(unsigned char* outCode, unsigned char* sc)
{
    // ?? 三参数：outCode(3B)、sc(16B)、retBuf(2B)
    typedef void (*CalcInCode)(unsigned char*, unsigned char*, unsigned char*);

    /* ---------- ① 解析 outCode（ASCII HEX → 3 字节） ---------- */
    unsigned char translateOutCode[3] = {0};
    for (int i = 0; i < 3; ++i) {
        translateOutCode[i] = TranslateChar(outCode[i * 2]) * 16
                            + TranslateChar(outCode[i * 2 + 1]);
    }

    /* ---------- ② 动态加载算法库 ---------- */
    void* handle = dlopen("libG2Encryption.so", RTLD_LAZY);
    if (handle == NULL) {         // 加载失败，直接返回 0
        return 0;
    }

    CalcInCode fun = (CalcInCode)dlsym(handle, "G2_CalcInCode");

    /* ---------- ③ 调用算法 ---------- */
    unsigned char retBuf[2] = {0};
    unsigned short output   = 0;

    if (fun) {
        fun(translateOutCode, sc, retBuf);          // 第 3 个参数是输出缓冲
        output = (retBuf[0] << 8) | retBuf[1];      // 组装成 unsigned short
    }

    dlclose(handle);
    return output;
}

/* ========== 1. 计算 16 字节 SC ========== */
unsigned char* GetSc_G2(const char* vin17)
{
    // 取 VIN 后 5 字符，按 ASCII 逐字节传给算法
    unsigned char vin5[5];
    memcpy(vin5, vin17 + 12, 5);           // VIN[12]..VIN[16]

    unsigned char* sc16 = new unsigned char[16]{0};

    void* h = dlopen("libG2Encryption.so", RTLD_LAZY);
    if (h) {
        typedef void (*VinToSc)(const unsigned char*, unsigned char*);
        VinToSc fun = (VinToSc)dlsym(h, "G2_VinToSc");
        if (fun) fun(vin5, sc16);          // 填满 16 字节
        dlclose(h);
    }
    return sc16;                           // 始终返回 16 B 缓冲
}

/* ========== 2. 计算 16 字节 Incode ========== */
unsigned char* GetIncode_G2(const char* outAscii /*6/8 HEX*/,
                            const unsigned char* sc16)
{
    /* 解析 outAscii 为 4 字节二进制 ------------------------- */
    unsigned char out4[4] = {0};
    size_t hexLen = strlen(outAscii);      // 6(旧车) 或 8(新车)
    size_t need   = hexLen / 2;            // 3 或 4
/* -------- 旧代码（左补 0） --------
    // 若只有 3 字节，左补 0；确保 out4[0]..out4[3] 已填
    for (size_t i = 0; i < need; ++i) {
        out4[4 - need + i] =
            TranslateChar(outAscii[i*2]) * 16 +
            TranslateChar(outAscii[i*2+1]);
    }
*/

/* -------- 新代码（右补 0） -------- */
for (size_t i = 0; i < need; ++i) {
    out4[i] = TranslateChar(outAscii[i*2]) * 16
            + TranslateChar(outAscii[i*2+1]);      // 依次填 0,1,2
}
/* 如果只有 3 byte，留出4号位默认 0 */

    /* 调用算法 --------------------------------------------- */
    unsigned char* in16 = new unsigned char[16]{0};

    void* h = dlopen("libG2Encryption.so", RTLD_LAZY);
    if (h) {
        typedef void (*CalcInCode)(const unsigned char*,
                                   const unsigned char*,
                                   unsigned char*);
        CalcInCode fun = (CalcInCode)dlsym(h, "G2_CalcInCode");
        if (fun) fun(out4, sc16, in16);
        dlclose(h);
    }
    return in16;                           // 16 字节结果
}

    //鑾峰彇escl鐮?
    unsigned long GetESCL(unsigned char* vinCode)
    {
        unsigned char v[18] = {0};
        memcpy(v, vinCode, 17);
        unsigned short int ch = 0x3333; //骞冲彴鎺╃爜锛屽浐瀹氬彇鍊?3333
        unsigned long key1 = 0x43333033; //杞﹀瀷鎺╃爜锛屽浐瀹氬彇鍊?43333033
        unsigned long escl = 0;
        typedef unsigned long (*alg)(unsigned char*, unsigned short int, unsigned long);
        
        void* handle = dlopen("./lib/libC303.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return escl;
        }
        
        alg fun = (alg)dlsym(handle, "alg");
        if (NULL != fun)
        {
            escl = fun(v, ch, key1);
        }
        
        dlclose(handle);
        return escl;
    }
/* ---------- JNI: VIN+OutCode → 16B SC / 16B Incode ---------- */
JNIEXPORT void JNICALL
Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeG2
  (JNIEnv* env, jobject /*this*/,
   jstring jVin, jstring jOut,
   jbyteArray jSc, jbyteArray jIncode)
{
    /* 1. 把 Java 字符串转成 C 字符串 -------------------------- */
    char* vinStr  = jstringToChar(env, jVin);    // 17 字节 VIN
    char* outStr  = jstringToChar(env, jOut);    // 6/8 HEX OutCode

    /* 2. 计算 16 字节 SC / Incode ----------------------------- */
    unsigned char* sc16 = GetSc_G2(vinStr);                 // new 函数
    unsigned char* in16 = GetIncode_G2(outStr, sc16);       // new 函数

    /* 3. 拷贝结果回 Java 字节数组 ----------------------------- */
    env->SetByteArrayRegion(jSc,     0, 16, (jbyte*)sc16);
    env->SetByteArrayRegion(jIncode, 0, 16, (jbyte*)in16);

    /* 4. 清理 ------------------------------------------------- */
    delete[] sc16;
    delete[] in16;
    free(vinStr);
    free(outStr);
}

	// Modify each of your JNI functions to add logging
	JNIEXPORT void JNICALL Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncode(JNIEnv * env, jobject job, jstring vinCode, jstring outCode, jobject sc, jobject incode) {
	    jniInitLogging();
	    jniLogMessage("Calling Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncode");
	    
	    // Log input parameters
	    char* vinCodeStr = jstringToChar(env, vinCode);
	    char* outCodeStr = jstringToChar(env, outCode);
	    std::stringstream ss;
	    ss << "Parameters: vinCode=" << (vinCodeStr ? vinCodeStr : "null") 
	       << ", outCode=" << (outCodeStr ? outCodeStr : "null");
	    jniLogMessage(ss.str());
	    
	    // Original function code...
	    char temp1[10] = {0};
	    int i = 0;
	    char Tsc[60] = {0};
	    char Tincode[60] = {0};
	    
	    unsigned char* scTemp = GetSc_G2(vinCodeStr);
	    unsigned char* incodeTemp = GetIncode_G2(outCodeStr, scTemp);
	    
	    for(i=0;i<3;i++) {
	        memset(temp1,0,sizeof(temp1));
	        sprintf(temp1, "%02X", scTemp[i]);
	        strcat(Tsc,temp1);
	    }
	    
	    sprintf(Tincode,"%02X%02X", incodeTemp[0], incodeTemp[1]);
	    
	    // Log results
	    ss.str("");
	    ss << "Results: SC=" << Tsc << ", Incode=" << Tincode;
	    jniLogMessage(ss.str());
	    
	    jstring scStr = stoJstring(env,Tsc);
	    jclass classSC = env->GetObjectClass(sc);
	    jfieldID scValue = env->GetFieldID(classSC,"value","Ljava/lang/String;");
	    env->SetObjectField(sc, scValue, scStr);
	    
	    jstring incodeStr = stoJstring(env,Tincode);
	    jclass classIncode = env->GetObjectClass(incode);
	    jfieldID incodeValue = env->GetFieldID(classIncode,"value","Ljava/lang/String;");
	    env->SetObjectField(incode, incodeValue, incodeStr);
	    
	    delete[] scTemp;
	    delete[] incodeTemp;
	    free(vinCodeStr);
	    free(outCodeStr);
	    
	    jniLogMessage("Completed Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncode");
	}

// Similarly modify all other JNI functions...

	JNIEXPORT void JNICALL Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCL(JNIEnv * env, jobject job, jstring vinCode, jstring outCode, jobject sc, jobject incode, jobject escl)
    {
    	jniInitLogging();
	    jniLogMessage("Calling Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCL");
	    
		char temp1[10] = {0};
		int i = 0;
		
		char *Tvincode = NULL;
		char *ToutCode = NULL;
		char Tsc[60] = {0};
		char Tincode[60] = {0};
		char TempEscl[20] = {0};
		
		Tvincode = jstringToChar(env,vinCode);
		ToutCode = jstringToChar(env,outCode);
		
        unsigned char* scTemp = GetSc((unsigned char*)Tvincode);
		unsigned short incodeTemp = GetIncode((unsigned char*)ToutCode, scTemp);
        unsigned long esclTemp = GetESCL((unsigned char*)Tvincode);
		
		for(i=0;i<3;i++)
		{
			memset(temp1,0,sizeof(temp1));
			sprintf(temp1, "%02X", scTemp[i]);
			strcat(Tsc,temp1);
		}
		
		sprintf(Tincode,"%04X",incodeTemp);

		//sc
		jstring scStr = stoJstring(env,Tsc);
		jclass classSC = env->GetObjectClass(sc);
		jfieldID scValue = env->GetFieldID(classSC,"value","Ljava/lang/String;");
		env->SetObjectField(sc ,scValue,scStr);
		
		//incode
		jstring incodeStr = stoJstring(env,Tincode);
		jclass classIncode = env->GetObjectClass(incode);
		jfieldID incodeValue = env->GetFieldID(classIncode,"value","Ljava/lang/String;");
		env->SetObjectField(incode ,incodeValue,incodeStr);
		
		//escl - 淇?澶嶆牸寮忓寲瀛楃?︿覆闂?棰?
		sprintf(TempEscl, "%08lX", esclTemp);
		jstring esclStr = stoJstring(env,TempEscl);
		jclass classEscl = env->GetObjectClass(escl);
		jfieldID esclValue = env->GetFieldID(classEscl,"value","Ljava/lang/String;");
		env->SetObjectField(escl ,esclValue,esclStr);
		
		delete []scTemp;
		free(Tvincode);
		free(ToutCode);
		
		 jniLogMessage("Completed Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCL");
    }

    //鏂拌溅鍨嬬畻娉晄c鐮佹槸16涓?瀛楄妭锛宱utcode鐮佹槸4涓?瀛楄妭锛宨ncode鐮佹槸16涓?瀛楄妭锛寁in鐮佺畻鍑簊c鐮侊紝sc鐮佸拰outcode鐮佺畻鍑篿ncode鐮?
    //鑾峰彇sc鐮佹椂闇�瑕佸彇vin鐮佺殑鍚?5浣嶅仛涓鸿?＄畻
	//2016骞村?炲姞鐨勬柊绠楁硶
    unsigned char* GetSc_New(unsigned char* vinCode)
    {
        unsigned char v[6] = {0};
        for (int i = 12; i < 17; i++) //鎴?鍙杤in鐮佺殑鍚?5浣?
        {
            v[i - 12] = vinCode[i];
        }
		unsigned char* outputcode = new unsigned char[18];
        memset(outputcode, 0, sizeof(unsigned char) * 18);
        typedef void (*alg1)(unsigned char*, unsigned char*);
		
		void* handle = dlopen("./lib/libG2alg2dll.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return outputcode;
        }
		
		alg1 fun = (alg1)dlsym(handle, "G2alg2");
        if (NULL != fun)
        {
            fun(v, outputcode);
        }
        
        dlclose(handle);
        return outputcode;
    }

	//2016骞村?炲姞鐨勬柊绠楁硶
    //鑾峰彇incode鐮?
	unsigned char* GetIncode_New(unsigned char* outCode, unsigned char* sc)
    {
		typedef void (*CalcInCode)(unsigned char*, unsigned char*, unsigned char*);

		unsigned char* output = new unsigned char[18];
		memset(output, 0, sizeof(unsigned char) * 18);
		
		void* handle = dlopen("./lib/libG2alg3dll.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return output;
        }
		
		CalcInCode fun = (CalcInCode)dlsym(handle, "G2alg3");

        unsigned char translateOutCode[5] = {0};
        for (int i = 0; i < 4; i++)
        {
            translateOutCode[i] = TranslateChar(outCode[i * 2]) * 16 + TranslateChar(outCode[i * 2 + 1]);
        }

        if (NULL != fun)
        {
            fun(translateOutCode, sc, output);
        }
        
        dlclose(handle);
        return output;
    }

	//2017骞村?炲姞鐨勬柊绠楁硶
	unsigned char* GetSc_2017(unsigned char* vinCode)
    {
        unsigned char v[7] = {0};
        for (int i = 11; i < 17; i++) //鎴?鍙杤in鐮佺殑鍚?6浣?
        {
            v[i - 11] = vinCode[i];
        }
		unsigned char* outputcode = new unsigned char[18];
        memset(outputcode, 0, sizeof(unsigned char) * 18);
        typedef void (*alg1)(unsigned char*, unsigned char*);
		
		void* handle = dlopen("./lib/libencryptalg1.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return outputcode;
        }
		
		alg1 fun = (alg1)dlsym(handle, "EncryptAlg1");
        if (NULL != fun)
        {
            fun(v, outputcode);
        }
        
        dlclose(handle);
        return outputcode;
    }

	//2017骞村?炲姞鐨勬柊绠楁硶
    //鑾峰彇incode鐮?
	unsigned char* GetIncode_2017(unsigned char* outCode, unsigned char* sc)
    {
		typedef void (*CalcInCode)(unsigned char*, unsigned char*, unsigned char*);

		unsigned char* output = new unsigned char[18];
		memset(output, 0, sizeof(unsigned char) * 18);
		
		void* handle = dlopen("./lib/libencryptalg2.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return output;
        }
		
		CalcInCode fun = (CalcInCode)dlsym(handle, "EncryptAlg2");

        unsigned char translateOutCode[5] = {0};
        for (int i = 0; i < 4; i++)
        {
            translateOutCode[i] = TranslateChar(outCode[i * 2]) * 16 + TranslateChar(outCode[i * 2 + 1]);
        }

        if (NULL != fun)
        {
            fun(translateOutCode, sc, output);
        }
        
        dlclose(handle);
        return output;
    }

	JNIEXPORT void JNICALL Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeNew(JNIEnv * env, jobject job, jstring vinCode, jstring outCode, jobject sc, jobject incode)
    {
    	jniInitLogging();
	    jniLogMessage("Calling Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeNew");
	    
		char temp1[10] = {0};
		int i = 0;
		char *Tvincode = NULL;
		char *ToutCode = NULL;
		char Tsc[60] = {0};
		char Tincode[60] = {0};
		unsigned char* scTemp = NULL;
		unsigned char* incodeTemp = NULL;
		
		Tvincode = jstringToChar(env,vinCode);
		ToutCode = jstringToChar(env,outCode);

		scTemp = GetSc_New((unsigned char*)Tvincode);//scTemp鏄?16涓?瀛楄妭
		incodeTemp = GetIncode_New((unsigned char*)ToutCode, scTemp);
		
		for(i=0;i<16;i++)
		{
			memset(temp1,0,sizeof(temp1));
			sprintf(temp1, "%02X", scTemp[i]);
			strcat(Tsc,temp1);
		}
		
		for(i=0;i<16;i++)
		{
			memset(temp1,0,sizeof(temp1));
			sprintf(temp1, "%02X", incodeTemp[i]);
			strcat(Tincode,temp1);
		}
		
		jstring scStr = stoJstring(env,Tsc);
		jclass classSC = env->GetObjectClass(sc);
		jfieldID scValue = env->GetFieldID(classSC,"value","Ljava/lang/String;");
		env->SetObjectField(sc ,scValue,scStr);
		
		jstring incodeStr = stoJstring(env,Tincode);
		jclass classIncode = env->GetObjectClass(incode);
		jfieldID incodeValue = env->GetFieldID(classIncode,"value","Ljava/lang/String;");
		env->SetObjectField(incode ,incodeValue,incodeStr);
		
		delete []scTemp;
		delete []incodeTemp;
		free(Tvincode);
		free(ToutCode);
		 jniLogMessage("Completed Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeNew");
    }

	JNIEXPORT void JNICALL Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCLNew(JNIEnv * env, jobject job, jstring vinCode, jstring outCode, jobject sc, jobject incode, jobject escl)
	{
		  jniInitLogging();
	    jniLogMessage("Calling Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCLNew");
	    
		char temp1[10] = {0};
		int i = 0;
		
		char *Tvincode = NULL;
		char *ToutCode = NULL;
		char Tsc[60] = {0};
		char Tincode[60] = {0};
		char TempEscl[20] = {0};
		unsigned char* scTemp=NULL;
		unsigned char* incodeTemp=NULL;
		unsigned long esclTemp=0;
		
		Tvincode = jstringToChar(env,vinCode);
		ToutCode = jstringToChar(env,outCode);
		
		scTemp = GetSc_New((unsigned char*)Tvincode);
		incodeTemp = GetIncode_New((unsigned char*)ToutCode, scTemp);
		esclTemp = GetESCL((unsigned char*)Tvincode);
		
		for(i=0;i<16;i++)
		{
			memset(temp1,0,sizeof(temp1));
			sprintf(temp1, "%02X", scTemp[i]);
			strcat(Tsc,temp1);
		}
		for(i=0;i<16;i++)
		{
			memset(temp1,0,sizeof(temp1));
			sprintf(temp1, "%02X", incodeTemp[i]);
			strcat(Tincode,temp1);
		}
		
		//sc
		jstring scStr = stoJstring(env,Tsc);
		jclass classSC = env->GetObjectClass(sc);
		jfieldID scValue = env->GetFieldID(classSC,"value","Ljava/lang/String;");
		env->SetObjectField(sc ,scValue,scStr);
		
		//incode
		jstring incodeStr = stoJstring(env,Tincode);
		jclass classIncode = env->GetObjectClass(incode);
		jfieldID incodeValue = env->GetFieldID(classIncode,"value","Ljava/lang/String;");
		env->SetObjectField(incode ,incodeValue,incodeStr);
		
		//escl - 淇?澶嶆牸寮忓寲瀛楃?︿覆闂?棰?
		sprintf(TempEscl, "%08lX", esclTemp);
		jstring esclStr = stoJstring(env,TempEscl);
		jclass classEscl = env->GetObjectClass(escl);
		jfieldID esclValue = env->GetFieldID(classEscl,"value","Ljava/lang/String;");
		env->SetObjectField(escl ,esclValue,esclStr);
		
		delete []scTemp;
		delete []incodeTemp;
		free(Tvincode);
		free(ToutCode);
		
		jniLogMessage("Completed Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCLNew");
    }

	JNIEXPORT void JNICALL Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeMain(JNIEnv * env, jobject job, jstring vinCode, jstring outCode, jobject sc, jobject incode,jint AlgorType)
    {
    	jniInitLogging();
	    jniLogMessage("Calling Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeMain");
		char temp1[10] = {0};
		int i = 0;
		char *Tvincode = NULL;
		char *ToutCode = NULL;
		char Tsc[60] = {0};
		char Tincode[60] = {0};
		unsigned char* scTemp = NULL;
		unsigned char* incodeTemp = NULL;
		
		Tvincode = jstringToChar(env,vinCode);
		ToutCode = jstringToChar(env,outCode);
		
		if(1==AlgorType)//2017骞存坊鍔犵殑X40鐨勯槻鐩楃畻娉?
		{
			scTemp = GetSc_2017((unsigned char*)Tvincode);//scTemp鏄?4涓?瀛楄妭
			incodeTemp = GetIncode_2017((unsigned char*)ToutCode, scTemp);//incodeTemp鏄?4涓?瀛楄妭
			
			for(i=0;i<4;i++)
			{
				memset(temp1,0,sizeof(temp1));
				sprintf(temp1, "%02X", scTemp[i]);
				strcat(Tsc,temp1);
			}
			
			for(i=0;i<4;i++)
			{
				memset(temp1,0,sizeof(temp1));
				sprintf(temp1, "%02X", incodeTemp[i]);
				strcat(Tincode,temp1);
			}
			
			jstring scStr = stoJstring(env,Tsc);
			jclass classSC = env->GetObjectClass(sc);
			jfieldID scValue = env->GetFieldID(classSC,"value","Ljava/lang/String;");
			env->SetObjectField(sc ,scValue,scStr);
			
			jstring incodeStr = stoJstring(env,Tincode);
			jclass classIncode = env->GetObjectClass(incode);
			jfieldID incodeValue = env->GetFieldID(classIncode,"value","Ljava/lang/String;");
			env->SetObjectField(incode ,incodeValue,incodeStr);
		}
		
		delete []scTemp;
		delete []incodeTemp;
		free(Tvincode);
		free(ToutCode);
		
			jniLogMessage("Completed Java_com_launch_chyc_jni_JniAlgorithm_getScAndIncodeMain");
    }

	JNIEXPORT void JNICALL Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCLMain(JNIEnv * env, jobject job, jstring vinCode, jstring outCode, jobject sc, jobject incode, jobject escl,jint AlgorType)
	{
		  jniInitLogging();
	    jniLogMessage("Calling Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCLMain");
	    
		char temp1[10] = {0};
		int i = 0;
		
		char *Tvincode = NULL;
		char *ToutCode = NULL;
		char Tsc[60] = {0};
		char Tincode[60] = {0};
		char TempEscl[20] = {0};
		unsigned char* scTemp=NULL;
		unsigned char* incodeTemp=NULL;
		unsigned long esclTemp=0;
		
		Tvincode = jstringToChar(env,vinCode);
		ToutCode = jstringToChar(env,outCode);
		
		if(1==AlgorType)//2017骞存坊鍔犵殑x40鐨勯槻鐩楃畻娉?
		{
			scTemp = GetSc_2017((unsigned char*)Tvincode);
			incodeTemp = GetIncode_2017((unsigned char*)ToutCode, scTemp);
			esclTemp = GetESCL((unsigned char*)Tvincode);
			
			for(i=0;i<4;i++)
			{
				memset(temp1,0,sizeof(temp1));
				sprintf(temp1, "%02X", scTemp[i]);
				strcat(Tsc,temp1);
			}
			for(i=0;i<4;i++)
			{
				memset(temp1,0,sizeof(temp1));
				sprintf(temp1, "%02X", incodeTemp[i]);
				strcat(Tincode,temp1);
			}
			
			//sc
			jstring scStr = stoJstring(env,Tsc);
			jclass classSC = env->GetObjectClass(sc);
			jfieldID scValue = env->GetFieldID(classSC,"value","Ljava/lang/String;");
			env->SetObjectField(sc ,scValue,scStr);
			
			//incode
			jstring incodeStr = stoJstring(env,Tincode);
			jclass classIncode = env->GetObjectClass(incode);
			jfieldID incodeValue = env->GetFieldID(classIncode,"value","Ljava/lang/String;");
			env->SetObjectField(incode ,incodeValue,incodeStr);
			
			//escl - 淇?澶嶆牸寮忓寲瀛楃?︿覆闂?棰?
			sprintf(TempEscl, "%08lX", esclTemp);
			jstring esclStr = stoJstring(env,TempEscl);
			jclass classEscl = env->GetObjectClass(escl);
			jfieldID esclValue = env->GetFieldID(classEscl,"value","Ljava/lang/String;");
			env->SetObjectField(escl ,esclValue,esclStr);
		}
		
		delete []scTemp;
		delete []incodeTemp;
		free(Tvincode);
		free(ToutCode);
		
		jniLogMessage("Completed Java_com_launch_chyc_jni_JniAlgorithm_getScIncodeAndESCLMain");
    }
	
#ifdef __cplusplus
}
#endif
