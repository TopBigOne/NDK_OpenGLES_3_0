/**
 * 资源目录配置。
 *
 * 原来所有路径都硬编码为 "/sdcard/Android/data/com.byteflow.app/files/Download"：
 * - 多用户/工作资料下实际路径是 /storage/emulated/<userId>/...，硬编码路径不可用；
 * - 修改 applicationId 后路径失效；
 * - Android 11+ 分区存储下只能访问应用自己的目录，路径必须与 Java 层拷贝资源的目录完全一致。
 * 现在由 Java 层在初始化时把 Context.getExternalFilesDir(DIRECTORY_DOWNLOADS) 的实际路径
 * 传进来（native_SetAssetsDir），native 侧所有资源/输出路径都从这里取。
 */

#ifndef NDK_OPENGLES_3_0_ASSETSDIR_H
#define NDK_OPENGLES_3_0_ASSETSDIR_H

#include <mutex>
#include <string.h>
#include <limits.h>

class AssetsDir
{
public:
	static void Set(const char *pDir)
	{
		if (pDir == nullptr || pDir[0] == '\0') return;
		std::lock_guard<std::mutex> lock(Mutex());
		strncpy(Buffer(), pDir, PATH_MAX - 1);
		Buffer()[PATH_MAX - 1] = '\0';
	}

	// 返回的指针指向静态缓冲区，在进程生命周期内有效。
	// Set 只在 Activity 初始化时调用一次，之后各线程只读。
	static const char *Get()
	{
		std::lock_guard<std::mutex> lock(Mutex());
		return Buffer();
	}

private:
	static std::mutex &Mutex()
	{
		static std::mutex s_Mutex;
		return s_Mutex;
	}

	static char *Buffer()
	{
		// 未设置时回退到旧的默认路径，保证行为与原来一致
		static char s_Dir[PATH_MAX] = "/sdcard/Android/data/com.byteflow.app/files/Download";
		return s_Dir;
	}
};

#endif //NDK_OPENGLES_3_0_ASSETSDIR_H
