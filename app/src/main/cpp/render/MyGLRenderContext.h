/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef OPENGLES_3_0_MYGLRENDERCONTEXT_H
#define OPENGLES_3_0_MYGLRENDERCONTEXT_H

#include "stdint.h"
#include <mutex>
#include <vector>
#include <GLES3/gl3.h>
#include "TextureMapSample.h"
#include "NV21TextureMapSample.h"
#include "TriangleSample.h"

class MyGLRenderContext
{
	MyGLRenderContext();

	~MyGLRenderContext();

public:
	void SetImageData(int format, int width, int height, uint8_t *pData);

	void SetImageDataWithIndex(int index, int format, int width, int height, uint8_t *pData);

	void SetParamsInt(int paramType, int value0, int value1);

	void SetParamsFloat(int paramType, float value0, float value1);

	void SetParamsShortArr(short *const pShortArr, int arrSize);

	void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY);

	void OnSurfaceCreated();

	void OnSurfaceChanged(int width, int height);

	void OnDrawFrame();

	static MyGLRenderContext* GetInstance();
	static void DestroyInstance();

private:
	static MyGLRenderContext *m_pContext;
	// 保护 m_pContext 的创建与销毁
	static std::mutex s_InstanceMutex;

	// Java 层在 UI 线程调用 SetXXX，GLSurfaceView 在 GL 线程调用 OnDrawFrame，
	// 两边都会访问 m_pCurSample，必须用同一把锁保护。
	std::mutex m_Mutex;

	// 切换示例后被替换下来的旧示例。它们持有 GL 资源，只能在 GL 线程里 Destroy，
	// 所以先放进这个列表，等下一次 OnDrawFrame 统一释放。
	// 用列表而不是单个指针：两帧之间连续切换多次时，旧示例不会被覆盖而泄漏。
	std::vector<GLSampleBase *> m_PendingDestroySamples;
	GLSampleBase *m_pCurSample;
	int m_ScreenW;
	int m_ScreenH;

};


#endif //NDK_OPENGLES_3_0_MYGLRENDERCONTEXT_H
