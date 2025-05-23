#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>  // 替代Windows的动态库加载
#include <cstdlib>  // 用于malloc
#include <cstring>  // 用于memcpy, memset, strcat�?
#include <cstddef>  // 用于size_t
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

    //获取sc码时需要取vin码的�?5位做为�?�算
	//老车型算�?
    unsigned char* GetSc(unsigned char* vinCode)
    {
        unsigned char v[6] = {0};
        for (int i = 12; i < 17; i++) //�?取vin码的�?5�?
        {
            v[i - 12] = vinCode[i];
        }
        unsigned char* outputcode = new unsigned char[10];
        memset(outputcode, 0, sizeof(unsigned char) * 10);
        typedef void (*alg1)(unsigned char*, unsigned char*);
        
        // 使用dlopen替代LoadLibrary
        void* handle = dlopen("libG2Encryption.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return outputcode;
        }
        
        // 使用dlsym替代GetProcAddress
        alg1 fun = (alg1)dlsym(handle, "G2_VinToSc");
        if (NULL != fun)
        {
            fun(v, outputcode);
        }
        
        // 使用dlclose替代FreeLibrary
        dlclose(handle);
        return outputcode;
    }

    //获取incode�?
    unsigned short GetIncode(unsigned char* outCode, unsigned char* sc)
    {
        typedef unsigned short (*CalcInCode)(unsigned char*, unsigned char*);
        unsigned short output = 0;
        
        void* handle = dlopen("libG2Encryption.so", RTLD_LAZY);
        if (NULL == handle)
        {
            return output;
        }
        
        CalcInCode fun = (CalcInCode)dlsym(handle, "G2_CalcInCode");
        unsigned char translateOutCode[3] = {0};
        for (int i = 0; i < 3; i++)
        {
            translateOutCode[i] = TranslateChar(outCode[i * 2]) * 16 + TranslateChar(outCode[i * 2 + 1]);
        }
        
        if (NULL != fun)
        {
            output = fun(translateOutCode, sc);
        }
        
        dlclose(handle);
        return output;
    }

    //获取escl�?
    unsigned long GetESCL(unsigned char* vinCode)
    {
        unsigned char v[18] = {0};
        memcpy(v, vinCode, 17);
        unsigned short int ch = 0x3333; //平台掩码，固定取�?3333
        unsigned long key1 = 0x43333033; //车型掩码，固定取�?43333033
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
	    
	    unsigned char* scTemp = GetSc((unsigned char*)vinCodeStr);
	    unsigned short incodeTemp = GetIncode((unsigned char*)outCodeStr, scTemp);
	    
	    for(i=0;i<3;i++) {
	        memset(temp1,0,sizeof(temp1));
	        sprintf(temp1, "%02X", scTemp[i]);
	        strcat(Tsc,temp1);
	    }
	    
	    sprintf(Tincode,"%04X",incodeTemp);
	    
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
		
		//escl - �?复格式化字�?�串�?�?
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

    //新车型算法sc码是16�?字节，outcode码是4�?字节，incode码是16�?字节，vin码算出sc码，sc码和outcode码算出incode�?
    //获取sc码时需要取vin码的�?5位做为�?�算
	//2016年�?�加的新算法
    unsigned char* GetSc_New(unsigned char* vinCode)
    {
        unsigned char v[6] = {0};
        for (int i = 12; i < 17; i++) //�?取vin码的�?5�?
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

	//2016年�?�加的新算法
    //获取incode�?
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

	//2017年�?�加的新算法
	unsigned char* GetSc_2017(unsigned char* vinCode)
    {
        unsigned char v[7] = {0};
        for (int i = 11; i < 17; i++) //�?取vin码的�?6�?
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

	//2017年�?�加的新算法
    //获取incode�?
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

		scTemp = GetSc_New((unsigned char*)Tvincode);//scTemp�?16�?字节
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
		
		//escl - �?复格式化字�?�串�?�?
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
		
		if(1==AlgorType)//2017年添加的X40的防盗算�?
		{
			scTemp = GetSc_2017((unsigned char*)Tvincode);//scTemp�?4�?字节
			incodeTemp = GetIncode_2017((unsigned char*)ToutCode, scTemp);//incodeTemp�?4�?字节
			
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
		
		if(1==AlgorType)//2017年添加的x40的防盗算�?
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
			
			//escl - �?复格式化字�?�串�?�?
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
