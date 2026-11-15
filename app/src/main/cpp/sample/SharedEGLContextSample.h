/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_SHAREDEGLCTXSAMPLE_H
#define NDK_OPENGLES_3_0_SHAREDEGLCTXSAMPLE_H

#include "GLSampleBase.h"
#include "../util/ImageDef.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <GLRenderLooper.h>

using namespace std;

class SharedEGLContextSample : public GLSampleBase
{
public:
	SharedEGLContextSample();

	virtual ~SharedEGLContextSample();

	virtual void LoadImage(NativeImage *pImage);

	virtual bool Init();
	virtual void Draw(int screenW, int screenH);

	virtual void Destroy();

	static void OnAsyncRenderDone(void* callback, int fboTexId);

private:
	GLuint m_ImageTextureId;
	GLuint m_FboTextureId;
	GLuint m_VaoId;
	GLuint m_VboIds[4];
	NativeImage m_RenderImage;
	GLuint m_FboProgramObj;

	mutex m_Mutex;
	condition_variable m_Cond;
	GLEnv m_GLEnv;
	// 是否已经向 GLRenderLooper 投递过 MSG_SurfaceCreated（即工作线程持有共享上下文）。
	// Destroy 只在为 true 时才通知工作线程销毁，避免 Init 失败后的 Destroy 去创建一个
	// 从未初始化过 m_GLEnv 的 looper 并让它处理 MSG_SurfaceDestroyed。
	bool m_LooperStarted;
	// 工作线程完成的帧计数（受 m_Mutex 保护），作为 Draw 等待的谓词，防止虚假唤醒
	uint64_t m_AsyncFrameCount;
};


#endif //NDK_OPENGLES_3_0_SHAREDEGLCTXSAMPLE_H
