/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include <EGL/egl.h>
#include "SharedEGLContextSample.h"

#define VERTEX_POS_INDX  0
#define TEXTURE_POS_INDX 1

SharedEGLContextSample::SharedEGLContextSample()
{
	m_VaoId = GL_NONE;
	for (int i = 0; i < 4; ++i) m_VboIds[i] = GL_NONE;
	m_LooperStarted = false;
	m_AsyncFrameCount = 0;
	m_ImageTextureId = GL_NONE;
	m_FboTextureId = GL_NONE;
	m_FboProgramObj = GL_NONE;
}

SharedEGLContextSample::~SharedEGLContextSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

void SharedEGLContextSample::LoadImage(NativeImage *pImage)
{
	if (pImage)
	{
		LOGCATD("SharedEGLContextSample::LoadImage pImage = %p", pImage->ppPlane[0]);
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

bool SharedEGLContextSample::Init()
{
	// 工作线程按图像尺寸创建离屏 FBO，没有图像时尺寸为 0，FBO 不完整且不会有任何反馈
	if (m_RenderImage.ppPlane[0] == nullptr || m_RenderImage.width <= 0 || m_RenderImage.height <= 0)
	{
		LOGCATE("SharedEGLContextSample::Init no image loaded");
		return false;
	}

	//顶点坐标
	GLfloat vVertices[] = {
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,
	};

	//正常纹理坐标
	GLfloat vTexCoors[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
    };

	//fbo 纹理坐标与正常纹理方向不同，原点位于左下角
	GLfloat vFboTexCoors[] = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 1.0f,
	};

	GLushort indices[] = { 0, 1, 2, 1, 3, 2 };

	char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"
			"layout(location = 1) in vec2 a_texCoord;   \n"
			"out vec2 v_texCoord;                       \n"
			"void main()                                \n"
			"{                                          \n"
			"   gl_Position = a_position;               \n"
			"   v_texCoord = a_texCoord;                \n"
			"}                                          \n";

	// 用于普通渲染的片段着色器脚本，简单纹理映射
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D s_TextureMap;\n"
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"
			"}";

	// 用于离屏渲染的片段着色器脚本，作为另一个线程的着色器程序
	char fFboShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D s_TextureMap;\n"
			"uniform float u_Offset;\n"
			"void main()\n"
			"{\n"
			"    vec4 tempColor = texture(s_TextureMap, v_texCoord);\n"
			"    if(v_texCoord.x < u_Offset) {\n"
			"        outColor = vec4(1.0 - tempColor.r, 1.0 - tempColor.g, 1.0 - tempColor.b, tempColor.a);\n"
			"    }\n"
			"    else if(v_texCoord.x < u_Offset + 0.005) {\n"
			"        outColor = vec4(1.0);\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        outColor = tempColor;\n"
			"    }\n"
			"}";

	// 编译链接用于普通渲染的着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);

	// 编译链接用于离屏渲染的着色器程序
	m_FboProgramObj = GLUtils::CreateProgram(vShaderStr, fFboShaderStr);

	if (m_ProgramObj == GL_NONE || m_FboProgramObj == GL_NONE)
	{
		LOGCATE("SharedEGLContextSample::Init create program fail");
		return false;
	}

	// 生成 VBO ，加载顶点数据和索引数据
	glGenBuffers(4, m_VboIds);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoors), vTexCoors, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vFboTexCoors), vFboTexCoors, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &m_VaoId);
    // 初始化用于普通渲染的 VAO
	glBindVertexArray(m_VaoId);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);

	// 创建并初始化图像纹理
	glGenTextures(1, &m_ImageTextureId);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	GO_CHECK_GL_ERROR();
	SizeF imgSize;
	imgSize.width  = m_RenderImage.width;
	imgSize.height = m_RenderImage.height;

	m_GLEnv.sharedCtx     = eglGetCurrentContext();
	m_GLEnv.program       = m_FboProgramObj;
	m_GLEnv.inputTexId    = m_ImageTextureId;
	m_GLEnv.vboIds[0]     = m_VboIds[0];
	m_GLEnv.vboIds[1]     = m_VboIds[2];
	m_GLEnv.vboIds[2]     = m_VboIds[3];
	m_GLEnv.imgSize       = imgSize;
	m_GLEnv.renderDone    = OnAsyncRenderDone;
	m_GLEnv.callbackCtx   = this;
    LOGCATD("SharedEGLContextSample::Init sharedCtx=%p", m_GLEnv.sharedCtx);

	// 上下文重建后会再次走到这里：此前 Destroy 已经让旧的工作线程释放资源并退出，
	// GetInstance 会创建新的 looper，以新的当前上下文作为共享上下文重新初始化
    GLRenderLooper::GetInstance()->postMessage(MSG_SurfaceCreated, &m_GLEnv);

	GLRenderLooper::GetInstance()->postMessage(MSG_SurfaceChanged, m_RenderImage.width, m_RenderImage.height);
	m_LooperStarted = true;
	return true;
}

void SharedEGLContextSample::Draw(int screenW, int screenH)
{
	LOGCATV("SharedEGLContextSample::Draw");
	{
		unique_lock<mutex> lock(m_Mutex);
		const uint64_t lastFrame = m_AsyncFrameCount;
		GLRenderLooper::GetInstance()->postMessage(MSG_DrawFrame);
		// 原来无谓词、无超时地 wait：虚假唤醒会画出未更新的纹理；工作线程初始化失败或卡住时，
		// GL 线程会被永久阻塞（界面冻结、无法暂停）。改为等待帧计数变化，超时则跳过本帧。
		bool done = m_Cond.wait_for(lock, std::chrono::milliseconds(100),
									[this, lastFrame] { return m_AsyncFrameCount != lastFrame; });
		if (!done || m_FboTextureId == GL_NONE)
		{
			LOGCATE("SharedEGLContextSample::Draw wait async render timeout, skip this frame");
			return;
		}
	}

	glViewport(0, 0, screenW, screenH);
	glUseProgram(m_ProgramObj);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(m_VaoId);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	GLUtils::setInt(m_ProgramObj, "s_TextureMap", 0);
	GO_CHECK_GL_ERROR();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindVertexArray(GL_NONE);

}

void SharedEGLContextSample::OnAsyncRenderDone(void *callback, int fboTexId) {
	LOGCATV("SharedEGLContextSample::OnAsyncRenderDone callback=%p, fboTexId=%d", callback, fboTexId);
	SharedEGLContextSample *ctx = static_cast<SharedEGLContextSample *>(callback);
	unique_lock<mutex> lock(ctx->m_Mutex);
	ctx->m_FboTextureId = fboTexId;
	ctx->m_AsyncFrameCount++;
	ctx->m_Cond.notify_all();
}

void SharedEGLContextSample::Destroy()
{
	// 工作线程持有自己的共享 EGLContext 和其中创建的 FBO/纹理/VAO，无论主上下文是否已丢失
	// （丢弃模式下也一样）都必须通知它在自己的线程里释放并退出。
	// ReleaseInstance 会等待线程处理完队列中的消息后再返回。
	if (m_LooperStarted)
	{
		GLRenderLooper::GetInstance()->postMessage(MSG_SurfaceDestroyed);
		GLRenderLooper::ReleaseInstance();
		m_LooperStarted = false;
	}
	// FBO 纹理属于工作线程，由它负责删除；这里只清掉引用
	m_FboTextureId = GL_NONE;

	GLUtils::DeleteProgram(m_ProgramObj);
	GLUtils::DeleteProgram(m_FboProgramObj);
	GLUtils::DeleteTexture(m_ImageTextureId);
	GLUtils::DeleteBuffers(4, m_VboIds);
	GLUtils::DeleteVertexArray(m_VaoId);
}
