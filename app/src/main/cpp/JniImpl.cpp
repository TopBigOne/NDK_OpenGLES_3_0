//
// Created by ByteFlow on 2019/7/9.
//
#include "util/LogUtil.h"
#include <MyGLRenderContext.h>
#include <EGLRender.h>
#include "jni.h"
#include <new>
#include <AssetsDir.h>

/*
 * 把 Java byte[] 拷贝成 native 内存。
 * - 数组为 null、长度不足 minSize 时返回 nullptr：原来直接按 Java 传入的宽高解释数据，
 *   数组比宽高声明的小时，示例里 memcpy/glTexImage2D 会越界读；
 * - 用 nothrow new：几十 MB 的 YUV 图分配失败时不应让 bad_alloc 直接终止进程。
 * 这里用 GetByteArrayRegion 拷贝而不用 GetPrimitiveArrayCritical：拷贝之后要去拿渲染上下文的锁，
 * critical 区内不能阻塞等待其它线程。
 */
static uint8_t *CopyByteArray(JNIEnv *env, jbyteArray array, size_t minSize, jsize *pLen)
{
	if (array == nullptr) return nullptr;
	jsize len = env->GetArrayLength(array);
	if (len <= 0 || static_cast<size_t>(len) < minSize)
	{
		LOGCATE("CopyByteArray invalid array length=%d, expect>=%zu", len, minSize);
		return nullptr;
	}
	uint8_t *buf = new (std::nothrow) uint8_t[len];
	if (buf == nullptr)
	{
		LOGCATE("CopyByteArray alloc %d bytes fail", len);
		return nullptr;
	}
	env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte*>(buf));
	if (env->ExceptionCheck())
	{
		// GetByteArrayRegion 失败会挂起 ArrayIndexOutOfBoundsException，交给 Java 层处理
		delete[] buf;
		return nullptr;
	}
	if (pLen) *pLen = len;
	return buf;
}

#define NATIVE_RENDER_CLASS_NAME "com/byteflow/app/MyNativeRender"
#define NATIVE_BG_RENDER_CLASS_NAME "com/byteflow/app/egl/NativeEglRender"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_Init
 * Signature: ()V
 */
JNIEXPORT void JNICALL native_Init(JNIEnv *env, jobject instance)
{
	MyGLRenderContext::GetInstance();

}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_UnInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL native_UnInit(JNIEnv *env, jobject instance)
{
	MyGLRenderContext::DestroyInstance();
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_SetImageData
 * Signature: (III[B)V
 */
JNIEXPORT void JNICALL native_SetImageData
(JNIEnv *env, jobject instance, jint format, jint width, jint height, jbyteArray imageData)
{
	size_t expect = NativeImageUtil::GetImageSize(format, width, height);
	if (expect == 0)
	{
		LOGCATE("native_SetImageData invalid image format=%d [w,h]=[%d,%d]", format, width, height);
		return;
	}
	uint8_t* buf = CopyByteArray(env, imageData, expect, nullptr);
	if (buf == nullptr) return;
	MyGLRenderContext::GetInstance()->SetImageData(format, width, height, buf);
	delete[] buf;
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_SetImageDataWithIndex
 * Signature: (IIII[B)V
 */
JNIEXPORT void JNICALL native_SetImageDataWithIndex
		(JNIEnv *env, jobject instance, jint index, jint format, jint width, jint height, jbyteArray imageData)
{
	size_t expect = NativeImageUtil::GetImageSize(format, width, height);
	if (expect == 0)
	{
		LOGCATE("native_SetImageDataWithIndex invalid image format=%d [w,h]=[%d,%d]", format, width, height);
		return;
	}
	uint8_t* buf = CopyByteArray(env, imageData, expect, nullptr);
	if (buf == nullptr) return;
	MyGLRenderContext::GetInstance()->SetImageDataWithIndex(index, format, width, height, buf);
	delete[] buf;
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_SetParamsInt
 * Signature: (III)V
 */
JNIEXPORT void JNICALL native_SetParamsInt
		(JNIEnv *env, jobject instance, jint paramType, jint value0, jint value1)
{
	MyGLRenderContext::GetInstance()->SetParamsInt(paramType, value0, value1);
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_SetParamsFloat
 * Signature: (IFF)V
 */
JNIEXPORT void JNICALL native_SetParamsFloat
		(JNIEnv *env, jobject instance, jint paramType, jfloat value0, jfloat value1)
{
	MyGLRenderContext::GetInstance()->SetParamsFloat(paramType, value0, value1);
}


/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_SetAudioData
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL native_SetAudioData
		(JNIEnv *env, jobject instance, jshortArray data)
{
	if (data == nullptr) return;
	jsize len = env->GetArrayLength(data);
	// 空数组时原来会在 SetParamsShortArr 的日志里读 pShortArr[0]，越界
	if (len <= 0) return;
	short *pShortBuf = new (std::nothrow) short[len];
	if (pShortBuf == nullptr) return;
	env->GetShortArrayRegion(data, 0, len, reinterpret_cast<jshort*>(pShortBuf));
	if (!env->ExceptionCheck())
	{
		MyGLRenderContext::GetInstance()->SetParamsShortArr(pShortBuf, len);
	}
	delete[] pShortBuf;
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_UpdateTransformMatrix
 * Signature: (FFFF)V
 */
JNIEXPORT void JNICALL native_UpdateTransformMatrix(JNIEnv *env, jobject instance, jfloat rotateX, jfloat rotateY, jfloat scaleX, jfloat scaleY)
{
	MyGLRenderContext::GetInstance()->UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_OnSurfaceCreated
 * Signature: ()V
 */
JNIEXPORT void JNICALL native_OnSurfaceCreated(JNIEnv *env, jobject instance)
{
	MyGLRenderContext::GetInstance()->OnSurfaceCreated();
}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_OnSurfaceChanged
 * Signature: (II)V
 */
JNIEXPORT void JNICALL native_OnSurfaceChanged
(JNIEnv *env, jobject instance, jint width, jint height)
{
	MyGLRenderContext::GetInstance()->OnSurfaceChanged(width, height);

}

/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_OnDrawFrame
 * Signature: ()V
 */
JNIEXPORT void JNICALL native_OnDrawFrame(JNIEnv *env, jobject instance)
{
	MyGLRenderContext::GetInstance()->OnDrawFrame();

}


/*
 * Class:     com_byteflow_app_MyNativeRender
 * Method:    native_SetAssetsDir
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL native_SetAssetsDir(JNIEnv *env, jobject instance, jstring dir)
{
	if (dir == nullptr) return;
	const char *pDir = env->GetStringUTFChars(dir, nullptr);
	if (pDir == nullptr) return;  // OOM，异常已挂起
	LOGCATD("native_SetAssetsDir dir=%s", pDir);
	AssetsDir::Set(pDir);
	env->ReleaseStringUTFChars(dir, pDir);
}

/*
 * Class:     com_byteflow_app_egl_NativeBgRender
 * Method:    native_EglRenderInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL native_EglRenderInit(JNIEnv *env, jobject instance)
{
	EGLRender::GetInstance()->Init();

}

/*
 * Class:     com_byteflow_app_egl_NativeBgRender
 * Method:    native_EglRenderSetImageData
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL native_EglRenderSetImageData(JNIEnv *env, jobject instance, jbyteArray data, jint width, jint height)
{
	size_t expect = NativeImageUtil::GetImageSize(IMAGE_FORMAT_RGBA, width, height);
	if (expect == 0) return;
	uint8_t* buf = CopyByteArray(env, data, expect, nullptr);
	if (buf == nullptr) return;
	EGLRender::GetInstance()->SetImageData(buf, width, height);
	delete[] buf;
}

/*
 * Class:     com_byteflow_app_egl_NativeBgRender
 * Method:    native_EglRenderSetIntParams
 * Signature: (II)V
 */
JNIEXPORT void JNICALL native_EglRenderSetIntParams(JNIEnv *env, jobject instance, jint type, jint param)
{
	EGLRender::GetInstance()->SetIntParams(type, param);

}

/*
 * Class:     com_byteflow_app_egl_NativeBgRender
 * Method:    native_EglRenderDraw
 * Signature: ()V
 */
JNIEXPORT void JNICALL native_EglRenderDraw(JNIEnv *env, jobject instance)
{
	EGLRender::GetInstance()->Draw();
}

/*
 * Class:     com_byteflow_app_egl_NativeBgRender
 * Method:    natuve_BgRenderUnInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL natuve_BgRenderUnInit(JNIEnv *env, jobject instance)
{
	EGLRender::GetInstance()->UnInit();
}

#ifdef __cplusplus
}
#endif

static JNINativeMethod g_RenderMethods[] = {
		{"native_Init",                      "()V",       (void *)(native_Init)},
		{"native_UnInit",                    "()V",       (void *)(native_UnInit)},
		{"native_SetImageData",              "(III[B)V",  (void *)(native_SetImageData)},
		{"native_SetImageDataWithIndex",     "(IIII[B)V", (void *)(native_SetImageDataWithIndex)},
		{"native_SetParamsInt",              "(III)V",    (void *)(native_SetParamsInt)},
		{"native_SetParamsFloat",            "(IFF)V",    (void *)(native_SetParamsFloat)},
		{"native_SetAudioData",              "([S)V",     (void *)(native_SetAudioData)},
		{"native_UpdateTransformMatrix",     "(FFFF)V",   (void *)(native_UpdateTransformMatrix)},
		{"native_OnSurfaceCreated",          "()V",       (void *)(native_OnSurfaceCreated)},
		{"native_OnSurfaceChanged",          "(II)V",     (void *)(native_OnSurfaceChanged)},
		{"native_OnDrawFrame",               "()V",       (void *)(native_OnDrawFrame)},
		{"native_SetAssetsDir",              "(Ljava/lang/String;)V", (void *)(native_SetAssetsDir)},
};

static JNINativeMethod g_BgRenderMethods[] = {
		{"native_EglRenderInit",          "()V",       (void *)(native_EglRenderInit)},
		{"native_EglRenderSetImageData",  "([BII)V",   (void *)(native_EglRenderSetImageData)},
		{"native_EglRenderSetIntParams",  "(II)V",     (void *)(native_EglRenderSetIntParams)},
		{"native_EglRenderDraw",          "()V",       (void *)(native_EglRenderDraw)},
		{"native_EglRenderUnInit",        "()V",       (void *)(natuve_BgRenderUnInit)},
};

static int RegisterNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *methods, int methodNum)
{
	LOGCATD("RegisterNativeMethods");
	jclass clazz = env->FindClass(className);
	if (clazz == NULL)
	{
		LOGCATE("RegisterNativeMethods fail. clazz == NULL");
		return JNI_FALSE;
	}
	jint ret = env->RegisterNatives(clazz, methods, methodNum);
	env->DeleteLocalRef(clazz);
	if (ret < 0)
	{
		LOGCATE("RegisterNativeMethods fail");
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

static void UnregisterNativeMethods(JNIEnv *env, const char *className)
{
	LOGCATD("UnregisterNativeMethods");
	jclass clazz = env->FindClass(className);
	if (clazz == NULL)
	{
		LOGCATE("UnregisterNativeMethods fail. clazz == NULL");
		env->ExceptionClear();
		return;
	}
	env->UnregisterNatives(clazz);
	env->DeleteLocalRef(clazz);
}

// call this func when loading lib
extern "C" jint JNI_OnLoad(JavaVM *jvm, void *p)
{
	LOGCATD("===== JNI_OnLoad =====");
	jint jniRet = JNI_ERR;
	JNIEnv *env = NULL;
	if (jvm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK)
	{
		return jniRet;
	}

	jint regRet = RegisterNativeMethods(env, NATIVE_RENDER_CLASS_NAME, g_RenderMethods,
										sizeof(g_RenderMethods) /
										sizeof(g_RenderMethods[0]));
	if (regRet != JNI_TRUE)
	{
		return JNI_ERR;
	}

	regRet = RegisterNativeMethods(env, NATIVE_BG_RENDER_CLASS_NAME, g_BgRenderMethods,
										sizeof(g_BgRenderMethods) /
										sizeof(g_BgRenderMethods[0]));
	if (regRet != JNI_TRUE)
	{
		return JNI_ERR;
	}

	return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *jvm, void *p)
{
	JNIEnv *env = NULL;
	if (jvm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK)
	{
		return;
	}

	UnregisterNativeMethods(env, NATIVE_RENDER_CLASS_NAME);

	UnregisterNativeMethods(env, NATIVE_BG_RENDER_CLASS_NAME);
}
