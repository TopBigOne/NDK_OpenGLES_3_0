/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef BYTEFLOW_LOGUTIL_H
#define BYTEFLOW_LOGUTIL_H

#include<android/log.h>
#include <sys/time.h>

#define  LOG_TAG "ByteFlow"

/*
 * 日志分级约定：
 * - LOGCATE/LOGCATW/LOGCATI：错误、警告与少量关键信息，Release 包中保留；
 * - LOGCATD：一次性的调试信息（初始化、加载图片等），只在 Debug 构建中输出；
 * - LOGCATV：逐帧/高频调用路径上的日志，只在 Debug 构建中输出。
 * 之前所有日志都是 ERROR 级并且逐帧打印，会刷屏 logcat、拖慢渲染线程，也会把真正的错误淹没。
 *
 * Release 构建由 NDK 工具链（以及 CMakeLists.txt 中的兜底）定义 NDEBUG。
 * 关闭时用 if (0) 包裹而不是直接展开为空：参数仍然参与类型检查、仍算“被使用”，
 * 避免只在日志里用到的变量在 Release 下产生 unused 警告，同时编译器会把整个调用优化掉。
 */
#define  LOGCATE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGCATW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGCATI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#ifdef NDEBUG
#define  LOGCATD(...)  do { if (0) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__); } while (0)
#define  LOGCATV(...)  do { if (0) __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__); } while (0)
#else
#define  LOGCATD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGCATV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#endif

static inline long long GetSysCurrentTime()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	long long curTime = ((long long)(time.tv_sec))*1000+time.tv_usec/1000;
	return curTime;
}

/*
 * 耗时统计宏必须成对使用：BEGIN 宏会打开一个作用域，END 宏负责关闭。
 * Release 下只保留花括号，保证调用处的作用域结构不变（宏之间的代码照常执行）。
 */
#ifdef NDEBUG
#define FUN_BEGIN_TIME(FUN) {
#define FUN_END_TIME(FUN) }
#define BEGIN_TIME(FUN) {
#define END_TIME(FUN) }
#else
#define FUN_BEGIN_TIME(FUN) {\
    LOGCATD("%s:%s func start", __FILE__, FUN); \
    long long t0 = GetSysCurrentTime();

#define FUN_END_TIME(FUN) \
    long long t1 = GetSysCurrentTime(); \
    LOGCATD("%s:%s func cost time %ldms", __FILE__, FUN, (long)(t1-t0));}

#define BEGIN_TIME(FUN) {\
    LOGCATD("%s func start", FUN); \
    long long t0 = GetSysCurrentTime();

#define END_TIME(FUN) \
    long long t1 = GetSysCurrentTime(); \
    LOGCATD("%s func cost time %ldms", FUN, (long)(t1-t0));}
#endif

/*
 * 原来的实现无论有没有错误都会打印一条 ERROR 日志，而且在 Draw 里被逐帧调用。
 * 现在只在 glGetError() 确实返回错误时才打印；Release 下整体关闭，
 * 因为 glGetError 在部分驱动上会引起 CPU/GPU 同步，不应留在线上渲染路径里。
 * 使用处必须已经包含 GLES 头文件（与原实现的要求相同）。
 */
#ifdef NDEBUG
#define GO_CHECK_GL_ERROR(...)   do { } while (0)
#else
#define GO_CHECK_GL_ERROR(...)   do { \
    GLenum __glErr = glGetError(); \
    if (__glErr != GL_NO_ERROR) \
        LOGCATE("CHECK_GL_ERROR %s glGetError = 0x%x, line = %d", __FUNCTION__, __glErr, __LINE__); \
    } while (0)
#endif

#define DEBUG_LOGCATE(...) LOGCATV("DEBUG_LOGCATE %s line = %d",  __FUNCTION__, __LINE__)

#endif //BYTEFLOW_LOGUTIL_H
