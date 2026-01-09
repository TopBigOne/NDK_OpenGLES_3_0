/**
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * FBO Blit（位块传输）示例
 * 本示例演示了如何使用 glBlitFramebuffer 函数进行帧缓冲区之间的高效数据复制
 * 主要特性：
 * 1. 多渲染目标（MRT）：同时渲染到4个颜色附件
 * 2. FBO Blit：使用 glBlitFramebuffer 将 FBO 内容快速传输到屏幕
 * 3. 分屏显示：将4个颜色附件分别显示在屏幕的4个区域
 * */

#include <gtc/matrix_transform.hpp>
#include "FBOBlitSample.h"
#include "../util/GLUtils.h"

// 多渲染目标的颜色附件数组，定义了4个颜色附件
// 这些附件将在渲染时同时被写入，用于实现一次绘制输出多个结果
const GLenum attachments[ATTACHMENT_NUM] = {
		GL_COLOR_ATTACHMENT0,  // 第0个颜色附件，存储完整的RGBA图像
		GL_COLOR_ATTACHMENT1,  // 第1个颜色附件，存储红色通道
		GL_COLOR_ATTACHMENT2,  // 第2个颜色附件，存储绿色通道
		GL_COLOR_ATTACHMENT3   // 第3个颜色附件，存储蓝色通道
};

// 构造函数：初始化所有成员变量
FBOBlitSample::FBOBlitSample()
{
	// 着色器变量位置初始化
	m_SamplerLoc = GL_NONE;   // 纹理采样器的uniform位置
	m_MVPMatLoc = GL_NONE;    // MVP矩阵的uniform位置

	// OpenGL对象ID初始化
	m_TextureId = GL_NONE;    // 输入图像纹理ID
	m_VaoId = GL_NONE;        // 顶点数组对象ID

	// 变换参数初始化
	m_AngleX = 0;             // 绕X轴旋转角度
	m_AngleY = 0;             // 绕Y轴旋转角度

	m_ScaleX = 1.0f;          // X轴缩放因子
	m_ScaleY = 1.0f;          // Y轴缩放因子

    m_ProgramObj = GL_NONE;   // 着色器程序对象
}

// 析构函数：释放图像资源
FBOBlitSample::~FBOBlitSample()
{
	// 释放Native图像内存
	NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

// 初始化OpenGL资源
void FBOBlitSample::Init()
{
	// 防止重复初始化
	if(m_ProgramObj)
		return;

	// 创建RGBA纹理对象，用于存储输入图像
	glGenTextures(1, &m_TextureId);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	// 设置纹理环绕方式为边缘截取，防止采样越界
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 设置纹理过滤方式为线性插值，使图像更平滑
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 顶点着色器：处理顶点变换
	char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;\n"  // 顶点位置属性
            "layout(location = 1) in vec2 a_texCoord;\n"  // 纹理坐标属性
            "uniform mat4 u_MVPMatrix;\n"                 // MVP变换矩阵
            "out vec2 v_texCoord;\n"                      // 传递给片段着色器的纹理坐标
            "void main()\n"
            "{\n"
            "    gl_Position = u_MVPMatrix * a_position;\n"  // 顶点位置变换
            "    v_texCoord = a_texCoord;\n"                 // 直接传递纹理坐标
            "}";

	// 片段着色器：使用多渲染目标（MRT）同时输出到4个颜色附件
	// 这是MRT的核心实现，一次绘制可以输出多个不同的结果
	char fMRTShaderStr[] =
			"#version 300 es\n"
            "precision mediump float;\n"
            "in vec2 v_texCoord;\n"                         // 从顶点着色器接收的纹理坐标
            "layout(location = 0) out vec4 outColor0;\n"    // 输出到颜色附件0（完整图像）
            "layout(location = 1) out vec4 outColor1;\n"    // 输出到颜色附件1（仅红色通道）
            "layout(location = 2) out vec4 outColor2;\n"    // 输出到颜色附件2（仅绿色通道）
            "layout(location = 3) out vec4 outColor3;\n"    // 输出到颜色附件3（仅蓝色通道）
            "uniform sampler2D s_Texture;\n"                // 输入纹理采样器
            "void main()\n"
            "{\n"
            "    vec4 outputColor = texture(s_Texture, v_texCoord);\n"  // 采样输入纹理
            "    outColor0 = outputColor;\n"                            // 附件0：完整的RGBA图像
            "    outColor1 = vec4(outputColor.r, 0.0, 0.0, 1.0);\n"     // 附件1：只保留红色分量
            "    outColor2 = vec4(0.0, outputColor.g, 0.0, 1.0);\n"     // 附件2：只保留绿色分量
            "    outColor3 = vec4(0.0, 0.0, outputColor.b, 1.0);\n"     // 附件3：只保留蓝色分量
            "}";

	// 创建着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fMRTShaderStr);
	if (m_ProgramObj)
	{
		// 获取uniform变量的位置
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_Texture");  // 纹理采样器位置
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix"); // MVP矩阵位置
	}
	else
	{
		LOGCATE("FBOBlitSample::Init create program fail");
	}

	// 定义矩形的顶点坐标（NDC坐标系，范围-1到1）
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // Position 0: 左上角
			-1.0f, -1.0f, 0.0f,  // Position 1: 左下角
			1.0f,  -1.0f, 0.0f,  // Position 2: 右下角
			1.0f,   1.0f, 0.0f,  // Position 3: 右上角
	};

	// 定义纹理坐标（标准纹理坐标系，范围0到1）
	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // TexCoord 0: 左下角
			0.0f,  1.0f,        // TexCoord 1: 左上角
			1.0f,  1.0f,        // TexCoord 2: 右上角
			1.0f,  0.0f         // TexCoord 3: 右下角
	};

	// 定义索引数组，使用两个三角形组成矩形
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// 生成VBO并加载顶点数据
	glGenBuffers(3, m_VboIds);
	// VBO 0: 顶点坐标缓冲
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

	// VBO 1: 纹理坐标缓冲
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	// VBO 2: 索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// 生成并配置VAO（顶点数组对象）
	// VAO保存了顶点属性配置状态，避免重复设置
	glGenVertexArrays(1, &m_VaoId);
	glBindVertexArray(m_VaoId);

	// 配置顶点位置属性（location = 0）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 配置纹理坐标属性（location = 1）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

	glBindVertexArray(GL_NONE);

	// 上传RGBA图像数据到纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 初始化帧缓冲对象（FBO），用于离屏渲染
	LOGCATE("FBOBlitSample::Init InitFBO = %d", InitFBO());

}

// 加载图像数据
void FBOBlitSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("FBOBlitSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		// 复制图像元数据
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		// 复制图像数据
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

// 渲染函数：执行离屏渲染和屏幕显示
void FBOBlitSample::Draw(int screenW, int screenH)
{
	LOGCATE("FBOBlitSample::Draw()");
	m_SurfaceWidth = screenW;
	m_SurfaceHeight = screenH;

	if(m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	// 获取默认帧缓冲（屏幕缓冲）
	GLint defaultFrameBuffer = GL_NONE;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFrameBuffer);

	// 第一步：绑定FBO进行离屏渲染
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glViewport ( 0, 0, m_RenderImage.width, m_RenderImage.height);
	glClear(GL_COLOR_BUFFER_BIT);
	// 指定渲染到多个颜色附件（MRT的关键设置）
	glDrawBuffers(ATTACHMENT_NUM, attachments);

	glUseProgram (m_ProgramObj);

	glBindVertexArray(m_VaoId);
	// 更新MVP矩阵
	UpdateMVPMatrix(m_MVPMatrix, 0, m_AngleY, (float)screenW / screenH);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);

	// 绑定输入纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glUniform1i(m_SamplerLoc, 0);

	// 绘制矩形，同时输出到4个颜色附件
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

	// 第二步：切换到默认帧缓冲准备显示到屏幕
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFrameBuffer);
	glViewport ( 0, 0, m_SurfaceWidth, m_SurfaceHeight);
	glClear(GL_COLOR_BUFFER_BIT);

	// 使用Blit操作将FBO内容快速传输到屏幕
	BlitTextures();
}

void FBOBlitSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
        glDeleteProgram(m_ProgramObj);
		glDeleteBuffers(3, m_VboIds);
		glDeleteVertexArrays(1, &m_VaoId);
		glDeleteTextures(1, &m_TextureId);
	}
}

/**
 * @param angleX 绕X轴旋转度数
 * @param angleY 绕Y轴旋转度数
 * @param ratio 宽高比
 * */
void FBOBlitSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("FBOBlitSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	angleX = angleX % 360;
	angleY = angleY % 360;

	//转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


	// Projection matrix
	glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	//glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
	//glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

	// View matrix
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
			glm::vec3(0, 0, 0), // and looks at the origin
			glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

	mvpMatrix = Projection * View * Model;

}

void FBOBlitSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}

// 初始化帧缓冲对象（FBO）及其多个颜色附件
bool FBOBlitSample::InitFBO() {
	// 保存当前帧缓冲绑定
	GLint defaultFrameBuffer = GL_NONE;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFrameBuffer);

	// 创建FBO对象
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// 创建4个颜色附件纹理，用于实现多渲染目标（MRT）
	glGenTextures(ATTACHMENT_NUM, m_AttachTexIds);
	for (int i = 0; i < ATTACHMENT_NUM; ++i) {
		glBindTexture(GL_TEXTURE_2D, m_AttachTexIds[i]);
		// 设置纹理参数
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// 分配纹理存储空间（不传入数据，用nullptr）
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		// 将纹理附加到FBO的颜色附件上
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachments[i], GL_TEXTURE_2D, m_AttachTexIds[i], 0);
	}

	// 指定MRT输出的颜色附件
	glDrawBuffers(ATTACHMENT_NUM, attachments);

	// 检查FBO完整性
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		return false;
	}
	// 恢复默认帧缓冲绑定
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
    return true;
}

// 使用 glBlitFramebuffer 将FBO中的4个颜色附件分别传输到屏幕的4个区域
// glBlitFramebuffer 是一种高效的位块传输（Blit）操作，可以快速复制帧缓冲区内容
// 相比使用着色器渲染，Blit操作通常更快，因为它可以利用GPU的专门硬件加速
void FBOBlitSample::BlitTextures() {
	// 绑定FBO为读取帧缓冲
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);

	// Blit颜色附件0（完整图像）到屏幕左下角
	glReadBuffer(GL_COLOR_ATTACHMENT0);  // 指定从哪个颜色附件读取
	// glBlitFramebuffer 参数说明：
	// srcX0, srcY0, srcX1, srcY1: 源矩形区域（从FBO读取的区域）
	// dstX0, dstY0, dstX1, dstY1: 目标矩形区域（写入屏幕的区域）
	// mask: 要复制的缓冲位（这里只复制颜色缓冲）
	// filter: 过滤方式（GL_LINEAR 或 GL_NEAREST）
	glBlitFramebuffer(0, 0, m_RenderImage.width, m_RenderImage.height,
						0, 0, m_SurfaceWidth/2, m_SurfaceHeight/2,
						GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// Blit颜色附件1（红色通道）到屏幕右下角
	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glBlitFramebuffer(0, 0, m_RenderImage.width, m_RenderImage.height,
						m_SurfaceWidth/2, 0, m_SurfaceWidth, m_SurfaceHeight/2,
						GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// Blit颜色附件2（绿色通道）到屏幕左上角
	glReadBuffer(GL_COLOR_ATTACHMENT2);
	glBlitFramebuffer(0, 0, m_RenderImage.width, m_RenderImage.height,
						0, m_SurfaceHeight/2, m_SurfaceWidth/2, m_SurfaceHeight,
						GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// Blit颜色附件3（蓝色通道）到屏幕右上角
	glReadBuffer(GL_COLOR_ATTACHMENT3);
	glBlitFramebuffer(0, 0, m_RenderImage.width, m_RenderImage.height,
						m_SurfaceWidth/2, m_SurfaceHeight/2, m_SurfaceWidth, m_SurfaceHeight,
						GL_COLOR_BUFFER_BIT, GL_LINEAR);
}
