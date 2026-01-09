/**
 * FBOSample - OpenGL ES 3.0 FBO (帧缓冲对象) 离屏渲染示例
 *
 * 功能说明:
 * 演示如何使用 FBO (Frame Buffer Object) 进行离屏渲染（Off-Screen Rendering）
 * 实现两遍渲染（Two-Pass Rendering）：第一遍离屏渲染生成灰度图，第二遍渲染到屏幕
 *
 * 技术要点:
 * 1. FBO 的创建和配置 - 离屏渲染的核心
 * 2. 颜色附件（Color Attachment）- 将纹理附加到 FBO
 * 3. 两遍渲染流程 - 第一遍渲染到 FBO，第二遍渲染到默认帧缓冲
 * 4. 纹理坐标系差异 - FBO 纹理坐标原点在左下角
 * 5. 灰度转换算法 - RGB 转灰度的标准亮度公式
 *
 * FBO 的优势:
 * - 支持离屏渲染，不直接显示到屏幕
 * - 可用于后处理效果（模糊、边缘检测、色彩调整等）
 * - 支持多重渲染目标（MRT）
 * - 可用于阴影贴图、反射、环境映射等高级效果
 *
 * 应用场景:
 * 图像滤镜、后处理特效、实时阴影、水面反射、相机预览特效
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include "FBOSample.h"

// ==================== 顶点属性索引定义 ====================
#define VERTEX_POS_INDX  0  // 顶点位置在 shader 中的 location
#define TEXTURE_POS_INDX 1  // 纹理坐标在 shader 中的 location

/**
 * @brief 构造函数
 * @details 初始化所有成员变量为默认值
 */
FBOSample::FBOSample()
{
	m_VaoIds[0] = GL_NONE;        // VAO ID（用于普通渲染和离屏渲染）
	m_VboIds[0] = GL_NONE;        // VBO ID 数组

	m_ImageTextureId = GL_NONE;   // 原始图像纹理 ID
	m_FboTextureId = GL_NONE;     // FBO 颜色附件纹理 ID
	m_SamplerLoc = GL_NONE;       // 普通渲染的采样器位置
	m_FboId = GL_NONE;            // FBO ID
	m_FboProgramObj = GL_NONE;    // 离屏渲染着色器程序 ID
	m_FboVertexShader = GL_NONE;  // 离屏渲染顶点着色器 ID
	m_FboFragmentShader = GL_NONE;// 离屏渲染片段着色器 ID
	m_FboSamplerLoc = GL_NONE;    // 离屏渲染的采样器位置
}

/**
 * @brief 析构函数
 * @details 释放图像内存
 */
FBOSample::~FBOSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

/**
 * @brief 加载图像数据
 * @param pImage 图像数据结构指针
 */
void FBOSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("FBOSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);  // 深拷贝图像数据
	}
}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建两套着色器程序、VAO/VBO、纹理和 FBO
 */
void FBOSample::Init()
{
	// ==================== 定义顶点坐标（全屏矩形）====================
	GLfloat vVertices[] = {
			-1.0f, -1.0f, 0.0f,  // 左下
			 1.0f, -1.0f, 0.0f,  // 右下
			-1.0f,  1.0f, 0.0f,  // 左上
			 1.0f,  1.0f, 0.0f,  // 右上
	};

	// ==================== 普通纹理坐标 ====================
	// 用于普通渲染，原点在左上角
	GLfloat vTexCoors[] = {
            0.0f, 1.0f,  // 左上
            1.0f, 1.0f,  // 右上
            0.0f, 0.0f,  // 左下
            1.0f, 0.0f,  // 右下
    };

	// ==================== FBO 纹理坐标 ====================
	// FBO 纹理坐标与普通纹理方向不同，原点位于左下角
	// 这是因为 OpenGL 纹理坐标原点在左下角，而图像通常原点在左上角
	GLfloat vFboTexCoors[] = {
			0.0f, 0.0f,  // 左下
			1.0f, 0.0f,  // 右下
			0.0f, 1.0f,  // 左上
			1.0f, 1.0f,  // 右上
	};

	// ==================== 索引数据（两个三角形组成矩形）====================
	GLushort indices[] = { 0, 1, 2, 1, 3, 2 };

	// ==================== 顶点着色器（普通渲染和离屏渲染共用）====================
	char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"  // 输入：顶点位置
			"layout(location = 1) in vec2 a_texCoord;   \n"  // 输入：纹理坐标
			"out vec2 v_texCoord;                       \n"  // 输出：传递纹理坐标
			"void main()                                \n"
			"{                                          \n"
			"   gl_Position = a_position;               \n"  // 直接传递顶点位置（全屏矩形）
			"   v_texCoord = a_texCoord;                \n"  // 传递纹理坐标
			"}                                          \n";

	// ==================== 片段着色器 1：用于普通渲染 ====================
	// 简单的纹理映射，直接采样纹理颜色
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"                  // 输入：纹理坐标
			"layout(location = 0) out vec4 outColor;\n"  // 输出：片段颜色
			"uniform sampler2D s_TextureMap;\n"      // Uniform：纹理采样器
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"  // 直接采样纹理颜色
			"}";

	// ==================== 片段着色器 2：用于离屏渲染（灰度转换）====================
	// 将彩色图像转换为灰度图像
	char fFboShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"                  // 输入：纹理坐标
			"layout(location = 0) out vec4 outColor;\n"  // 输出：片段颜色
			"uniform sampler2D s_TextureMap;\n"      // Uniform：纹理采样器
			"void main()\n"
			"{\n"
			"    vec4 tempColor = texture(s_TextureMap, v_texCoord);\n"  // 采样原始颜色
			// 使用标准亮度公式计算灰度值（ITU-R BT.601 标准）
			// 人眼对绿色最敏感，红色次之，蓝色最不敏感
			"    float luminance = tempColor.r * 0.299 + tempColor.g * 0.587 + tempColor.b * 0.114;\n"
			"    outColor = vec4(vec3(luminance), tempColor.a);\n"  // 输出灰度图（RGB 相同，保留 Alpha）
			"}";

	// ==================== 编译链接两套着色器程序 ====================
	// 程序 1：用于普通渲染（将 FBO 结果渲染到屏幕）
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// 程序 2：用于离屏渲染（将原始图像渲染为灰度图到 FBO）
	m_FboProgramObj = GLUtils::CreateProgram(vShaderStr, fFboShaderStr, m_FboVertexShader, m_FboFragmentShader);

	if (m_ProgramObj == GL_NONE || m_FboProgramObj == GL_NONE)
	{
		LOGCATE("FBOSample::Init m_ProgramObj == GL_NONE");
		return;
	}

	// 获取 uniform 变量位置
	m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");        // 普通渲染采样器
	m_FboSamplerLoc = glGetUniformLocation(m_FboProgramObj, "s_TextureMap"); // 离屏渲染采样器

	// ==================== 生成 VBO 并加载数据 ====================
	// 创建 4 个 VBO：顶点坐标、普通纹理坐标、FBO 纹理坐标、索引
	glGenBuffers(4, m_VboIds);

	// VBO[0]: 顶点位置数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

	// VBO[1]: 普通纹理坐标数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoors), vTexCoors, GL_STATIC_DRAW);

	// VBO[2]: FBO 纹理坐标数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vFboTexCoors), vFboTexCoors, GL_STATIC_DRAW);

	// VBO[3]: 索引数据（EBO）
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GO_CHECK_GL_ERROR();

	// ==================== 生成 2 个 VAO ====================
	// VAO[0]: 用于普通渲染（渲染到屏幕）
	// VAO[1]: 用于离屏渲染（渲染到 FBO）
	glGenVertexArrays(2, m_VaoIds);

	// ==================== 配置 VAO[0]：普通渲染 ====================
	glBindVertexArray(m_VaoIds[0]);

	// 绑定顶点位置数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定普通纹理坐标数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲区
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);

	// ==================== 配置 VAO[1]：离屏渲染 ====================
	glBindVertexArray(m_VaoIds[1]);

	// 绑定顶点位置数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定 FBO 纹理坐标数据（注意：使用 VBO[2]，纹理坐标不同）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲区
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);

	// ==================== 创建原始图像纹理 ====================
	glGenTextures(1, &m_ImageTextureId);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // S 轴钳位到边缘
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // T 轴钳位到边缘
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);     // 缩小时线性过滤
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);     // 放大时线性过滤
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	GO_CHECK_GL_ERROR();

	// ==================== 创建 FBO 及其颜色附件纹理 ====================
	if (!CreateFrameBufferObj())
	{
		LOGCATE("FBOSample::Init CreateFrameBufferObj fail");
		return;
	}
}

/**
 * @brief 渲染函数 - 两遍渲染
 * @param screenW 屏幕宽度（像素）
 * @param screenH 屏幕高度（像素）
 *
 * @details
 * 渲染流程：
 * 1. 第一遍：离屏渲染到 FBO（将彩色图像转为灰度图）
 * 2. 第二遍：将 FBO 的结果渲染到屏幕
 */
void FBOSample::Draw(int screenW, int screenH)
{
	// ==================== 第一遍渲染：离屏渲染到 FBO ====================
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);  // 设置像素对齐方式
	glViewport(0, 0, m_RenderImage.width, m_RenderImage.height);  // 设置视口为图像尺寸

	// 绑定 FBO（此后的渲染操作将输出到 FBO，而非默认帧缓冲）
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);

	// 使用离屏渲染着色器程序（灰度转换）
	glUseProgram(m_FboProgramObj);
	glBindVertexArray(m_VaoIds[1]);  // 使用离屏渲染 VAO

	// 绑定原始图像纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glUniform1i(m_FboSamplerLoc, 0);

	GO_CHECK_GL_ERROR();

	// 绘制（渲染到 FBO 的颜色附件纹理）
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

	GO_CHECK_GL_ERROR();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// ==================== 可选：读取 FBO 内容到内存（调试用）====================
	// 下面的代码被注释掉了，用于演示如何从 FBO 读取像素数据到内存
	// 可用于保存离屏渲染结果或进一步 CPU 处理
//	uint8_t *pBuffer = new uint8_t[m_RenderImage.width * m_RenderImage.height * 4];
//
//	NativeImage nativeImage = m_RenderImage;
//	nativeImage.format = IMAGE_FORMAT_RGBA;
//	nativeImage.ppPlane[0] = pBuffer;
//	FUN_BEGIN_TIME("FBO glReadPixels")
//		glReadPixels(0, 0, nativeImage.width, nativeImage.height, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
//	FUN_END_TIME("FBO cost glReadPixels")
//
//	NativeImageUtil::DumpNativeImage(&nativeImage, "/sdcard/DCIM", "NDK");
//	delete []pBuffer;

	// 解绑 FBO，恢复默认帧缓冲（接下来渲染到屏幕）
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ==================== 第二遍渲染：将 FBO 结果渲染到屏幕 ====================
	glViewport(0, 0, screenW, screenH);  // 设置视口为屏幕尺寸

	// 使用普通渲染着色器程序（简单纹理映射）
	glUseProgram(m_ProgramObj);
	GO_CHECK_GL_ERROR();

	glBindVertexArray(m_VaoIds[0]);  // 使用普通渲染 VAO

	// 绑定 FBO 的颜色附件纹理（包含第一遍渲染的灰度图结果）
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glUniform1i(m_SamplerLoc, 0);

	GO_CHECK_GL_ERROR();

	// 绘制（渲染到默认帧缓冲，即屏幕）
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

	GO_CHECK_GL_ERROR();
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindVertexArray(GL_NONE);
}

/**
 * @brief 销毁 OpenGL 资源
 * @details 删除着色器程序、纹理、缓冲区、VAO 和 FBO
 */
void FBOSample::Destroy()
{
	// 删除普通渲染着色器程序
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
	}

	// 删除离屏渲染着色器程序
	if (m_FboProgramObj)
	{
		glDeleteProgram(m_FboProgramObj);
	}

	// 删除原始图像纹理
	if (m_ImageTextureId)
	{
		glDeleteTextures(1, &m_ImageTextureId);
	}

	// 删除 FBO 颜色附件纹理
	if (m_FboTextureId)
	{
		glDeleteTextures(1, &m_FboTextureId);
	}

	// 删除 VBO
	if (m_VboIds[0])
	{
		glDeleteBuffers(4, m_VboIds);
	}

	// 删除 VAO
	if (m_VaoIds[0])
	{
		glDeleteVertexArrays(2, m_VaoIds);
	}

	// 删除 FBO
	if (m_FboId)
	{
		glDeleteFramebuffers(1, &m_FboId);
	}
}

/**
 * @brief 创建帧缓冲对象（FBO）
 * @return true 创建成功，false 创建失败
 *
 * @details
 * FBO 创建步骤：
 * 1. 创建 FBO 颜色附件纹理（用于存储渲染结果）
 * 2. 创建 FBO 对象
 * 3. 将纹理附加到 FBO 的颜色附件 0
 * 4. 检查 FBO 完整性状态
 */
bool FBOSample::CreateFrameBufferObj()
{
	// ==================== 创建并配置 FBO 颜色附件纹理 ====================
	glGenTextures(1, &m_FboTextureId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);

	// 设置纹理参数（与普通纹理相同）
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// ==================== 创建并配置 FBO ====================
	glGenFramebuffers(1, &m_FboId);                // 生成 FBO 对象
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);    // 绑定 FBO

	// 将纹理附加到 FBO 的颜色附件 0
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,           // 目标：帧缓冲
		GL_COLOR_ATTACHMENT0,     // 附件点：颜色附件 0（可以有多个颜色附件，用于 MRT）
		GL_TEXTURE_2D,            // 纹理类型：2D 纹理
		m_FboTextureId,           // 纹理 ID
		0                         // Mipmap 层级：0（基础层）
	);

	// 为纹理分配存储空间（不上传数据，只分配空间）
	glTexImage2D(
		GL_TEXTURE_2D,            // 纹理类型
		0,                        // Mipmap 层级
		GL_RGBA,                  // 内部格式
		m_RenderImage.width,      // 宽度
		m_RenderImage.height,     // 高度
		0,                        // 边框宽度（必须为 0）
		GL_RGBA,                  // 像素数据格式
		GL_UNSIGNED_BYTE,         // 像素数据类型
		nullptr                   // 数据指针（nullptr 表示只分配空间，不上传数据）
	);

	// ==================== 检查 FBO 完整性 ====================
	// FBO 必须处于完整状态才能正常使用
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
		LOGCATE("FBOSample::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
		return false;
	}

	// 解绑纹理和 FBO
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

	return true;
}
