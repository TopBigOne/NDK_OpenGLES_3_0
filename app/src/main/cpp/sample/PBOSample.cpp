/**
 * PBOSample - OpenGL ES 3.0 像素缓冲对象（Pixel Buffer Object）示例
 *
 * 功能说明:
 * 演示如何使用 PBO 加速像素数据的上传（CPU到GPU）和下载（GPU到CPU）操作
 *
 * 技术要点:
 * 1. PBO Upload（像素上传）：使用 GL_PIXEL_UNPACK_BUFFER 加速纹理数据上传
 * 2. PBO Download（像素下载）：使用 GL_PIXEL_PACK_BUFFER 加速像素读取操作
 * 3. 双缓冲机制：通过双PBO实现异步数据传输，提高性能
 * 4. FBO 离屏渲染：结合 FBO 进行灰度图处理
 * 5. glMapBufferRange：直接映射GPU缓冲区到CPU内存，避免拷贝
 *
 * PBO 优势:
 * - 异步传输：不阻塞渲染流水线
 * - 减少拷贝：通过内存映射减少数据拷贝次数
 * - 提高吞吐：双缓冲机制提高数据传输效率
 *
 * Created by 公众号：字节流动 on 2021/10/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include "PBOSample.h"

// 宏定义开关：控制 PBO 上传和下载的测试
//#define PBO_UPLOAD      // 启用PBO上传模式（注释掉则使用传统方式）
#define PBO_DOWNLOAD      // 启用PBO下载模式（用于对比性能）

// 顶点属性索引
#define VERTEX_POS_INDX  0   // 顶点位置属性索引
#define TEXTURE_POS_INDX 1   // 纹理坐标属性索引

/**
 * @brief 构造函数 - 初始化成员变量
 */
PBOSample::PBOSample()
{
    m_ImageTextureId = GL_NONE;
	m_FboTextureId = GL_NONE;
	m_SamplerLoc = GL_NONE;
	m_FboId = GL_NONE;
	m_FboProgramObj = GL_NONE;
	m_FboVertexShader = GL_NONE;
	m_FboFragmentShader = GL_NONE;
	m_FboSamplerLoc = GL_NONE;
	m_MVPMatrixLoc = GL_NONE;

    m_AngleX = 0;
    m_AngleY = 0;

    m_ScaleX = 1.0f;
    m_ScaleY = 1.0f;

    m_FrameIndex = 0;
}

/**
 * @brief 析构函数 - 释放图像资源
 */
PBOSample::~PBOSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

/**
 * @brief 加载图像数据
 * @param pImage 输入的图像数据指针
 */
void PBOSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("PBOSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建着色器、VBO/VAO、纹理、PBO和FBO
 */
void PBOSample::Init()
{
    if(m_ProgramObj)
        return;

	// ==================== 定义顶点坐标数据 ====================
	// 四个顶点构成矩形（两个三角形），覆盖整个屏幕 [-1, 1]
	GLfloat vVertices[] = {
			-1.0f, -1.0f, 0.0f,  // 左下
			 1.0f, -1.0f, 0.0f,  // 右下
			-1.0f,  1.0f, 0.0f,  // 左上
			 1.0f,  1.0f, 0.0f,  // 右上
	};

	// ==================== 定义正常纹理坐标 ====================
	// 纹理坐标原点在左上角 (0, 0)，用于普通渲染
	GLfloat vTexCoors[] = {
            0.0f, 1.0f,  // 左下 -> 纹理左上
            1.0f, 1.0f,  // 右下 -> 纹理右上
            0.0f, 0.0f,  // 左上 -> 纹理左下
            1.0f, 0.0f,  // 右上 -> 纹理右下
    };

	// ==================== 定义 FBO 纹理坐标 ====================
	// FBO 纹理坐标原点在左下角，与正常纹理方向不同
	GLfloat vFboTexCoors[] = {
			0.0f, 0.0f,  // 左下 -> FBO纹理左下
			1.0f, 0.0f,  // 右下 -> FBO纹理右下
			0.0f, 1.0f,  // 左上 -> FBO纹理左上
			1.0f, 1.0f,  // 右上 -> FBO纹理右上
	};

	// ==================== 定义索引数据 ====================
	// 使用两个三角形构成矩形：三角形1(0,1,2) 和 三角形2(0,2,3)
	GLushort indices[] = { 0, 1, 2, 1, 3, 2 };

	// ==================== 顶点着色器 ====================
	// 支持 MVP 变换矩阵，用于3D变换
	char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"  // 顶点位置
			"layout(location = 1) in vec2 a_texCoord;   \n"  // 纹理坐标
            "uniform mat4 u_MVPMatrix;                  \n"  // MVP变换矩阵
            "out vec2 v_texCoord;                       \n"  // 传递给片段着色器的纹理坐标
			"void main()                                \n"
			"{                                          \n"
			"   gl_Position = u_MVPMatrix * a_position; \n"  // 应用MVP变换
			"   v_texCoord = a_texCoord;                \n"  // 传递纹理坐标
			"}                                          \n";

	// ==================== 片段着色器（普通渲染）====================
	// 用于普通渲染的片段着色器，简单纹理映射
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"                           // 接收纹理坐标
			"layout(location = 0) out vec4 outColor;\n"      // 输出颜色
			"uniform sampler2D s_TextureMap;\n"              // 纹理采样器
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"  // 采样纹理
			"}";

	// ==================== 顶点着色器（FBO离屏渲染）====================
	// 用于离屏渲染的顶点着色器，不使用变换矩阵（直接渲染到FBO）
    char vFboShaderStr[] =
            "#version 300 es                            \n"
            "layout(location = 0) in vec4 a_position;   \n"  // 顶点位置
            "layout(location = 1) in vec2 a_texCoord;   \n"  // 纹理坐标
            "out vec2 v_texCoord;                       \n"  // 传递纹理坐标
            "void main()                                \n"
            "{                                          \n"
            "   gl_Position = a_position;               \n"  // 直接使用顶点位置，无变换
            "   v_texCoord = a_texCoord;                \n"
            "}                                          \n";

	// ==================== 片段着色器（FBO灰度处理）====================
	// 用于离屏渲染的片段着色器，将彩色图像转换为灰度图
	char fFboShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D s_TextureMap;\n"
			"void main()\n"
			"{\n"
			"    vec4 tempColor = texture(s_TextureMap, v_texCoord);\n"
			// 灰度转换公式（基于人眼对不同颜色的敏感度）
			"    float luminance = tempColor.r * 0.299 + tempColor.g * 0.587 + tempColor.b * 0.114;\n"
			"    outColor = vec4(vec3(luminance), tempColor.a);\n"  // 输出灰度图
			"}";

	// ==================== 编译链接着色器程序 ====================
	// 编译链接用于普通渲染的着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// 编译链接用于离屏渲染的着色器程序（灰度转换）
	m_FboProgramObj = GLUtils::CreateProgram(vFboShaderStr, fFboShaderStr, m_FboVertexShader, m_FboFragmentShader);

	if (m_ProgramObj == GL_NONE || m_FboProgramObj == GL_NONE)
	{
		LOGCATE("PBOSample::Init m_ProgramObj == GL_NONE");
		return;
	}

	// ==================== 获取 Uniform 变量位置 ====================
	m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");      // 普通渲染的纹理采样器
	m_MVPMatrixLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");     // MVP矩阵
	m_FboSamplerLoc = glGetUniformLocation(m_FboProgramObj, "s_TextureMap"); // FBO渲染的纹理采样器

	// ==================== 生成 VBO 并加载顶点数据 ====================
	// 创建 4 个 VBO：顶点坐标、纹理坐标、FBO纹理坐标、索引
	glGenBuffers(4, m_VboIds);

	// VBO[0]：顶点位置数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

	// VBO[1]：纹理坐标数据（普通渲染）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoors), vTexCoors, GL_STATIC_DRAW);

	// VBO[2]：纹理坐标数据（FBO渲染）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vFboTexCoors), vFboTexCoors, GL_STATIC_DRAW);

	// VBO[3]：索引数据
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GO_CHECK_GL_ERROR();

	// ==================== 生成 VAO ====================
	// 创建 2 个 VAO：一个用于普通渲染，另一个用于离屏渲染
	glGenVertexArrays(2, m_VaoIds);

    // ==================== 初始化 VAO[0]（普通渲染）====================
	glBindVertexArray(m_VaoIds[0]);

	// 绑定顶点位置属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定纹理坐标属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);


	// ==================== 初始化 VAO[1]（离屏渲染）====================
	glBindVertexArray(m_VaoIds[1]);

	// 绑定顶点位置属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定 FBO 纹理坐标属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);

	// ==================== 创建并初始化图像纹理 ====================
	glGenTextures(1, &m_ImageTextureId);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // S方向边缘夹紧
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // T方向边缘夹紧
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);     // 缩小过滤：线性
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);     // 放大过滤：线性
	// 上传图像数据到纹理
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	GO_CHECK_GL_ERROR();

	// ==================== 初始化 PBO（上传）====================
	// 创建 2 个 PBO 用于上传（双缓冲机制）
    glGenBuffers(2, m_UploadPboIds);
    int imgByteSize = m_RenderImage.width * m_RenderImage.height * 4;  // RGBA格式，每像素4字节

    // 配置 PBO[0]（上传用）
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_UploadPboIds[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, imgByteSize, 0, GL_STREAM_DRAW);  // GL_STREAM_DRAW：数据频繁更新

    // 配置 PBO[1]（上传用）
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_UploadPboIds[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, imgByteSize, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // ==================== 初始化 PBO（下载）====================
    // 创建 2 个 PBO 用于下载（双缓冲机制）
    glGenBuffers(2, m_DownloadPboIds);

    // 配置 PBO[0]（下载用）
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_DownloadPboIds[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, imgByteSize, 0, GL_STREAM_READ);  // GL_STREAM_READ：数据频繁读取

    // 配置 PBO[1]（下载用）
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_DownloadPboIds[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, imgByteSize, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// ==================== 创建 FBO ====================
	if (!CreateFrameBufferObj())
	{
		LOGCATE("PBOSample::Init CreateFrameBufferObj fail");
		return;
	}

}

/**
 * @brief 渲染函数 - 执行离屏渲染和屏幕渲染
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 * @details 演示 PBO 上传和下载流程，结合 FBO 进行灰度图处理
 */
void PBOSample::Draw(int screenW, int screenH)
{
	// ==================== 第一步：离屏渲染（FBO）====================
	// 设置视口为图像尺寸
	glViewport(0, 0, m_RenderImage.width, m_RenderImage.height);

	// 使用 PBO 上传像素数据到纹理（异步优化）
	UploadPixels();
    GO_CHECK_GL_ERROR();

	// 绑定 FBO 进行离屏渲染
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	glUseProgram(m_FboProgramObj);         // 使用灰度转换着色器
	glBindVertexArray(m_VaoIds[1]);        // 绑定 FBO 专用 VAO

	// 绑定并激活源图像纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glUniform1i(m_FboSamplerLoc, 0);
	GO_CHECK_GL_ERROR();

	// 绘制到 FBO（应用灰度转换）
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// 使用 PBO 下载渲染结果（异步优化）
	DownloadPixels();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ==================== 第二步：普通渲染（屏幕）====================
	// 设置视口为屏幕尺寸
	glViewport(0, 0, screenW, screenH);

	// 更新 MVP 矩阵（支持旋转变换）
    UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);
    glUseProgram(m_ProgramObj);          // 使用普通渲染着色器
	GO_CHECK_GL_ERROR();

	glBindVertexArray(m_VaoIds[0]);      // 绑定普通渲染 VAO

	// 绑定 FBO 输出纹理（灰度图）
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
    glUniformMatrix4fv(m_MVPMatrixLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
	glUniform1i(m_SamplerLoc, 0);
	GO_CHECK_GL_ERROR();

	// 绘制到屏幕
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();

	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindVertexArray(GL_NONE);

	m_FrameIndex++;  // 帧计数器（用于双缓冲切换）

}

/**
 * @brief 销毁 OpenGL 资源
 */
void PBOSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
		m_ProgramObj = GL_NONE;
	}

	if (m_FboProgramObj)
	{
		glDeleteProgram(m_FboProgramObj);
		m_FboProgramObj = GL_NONE;
	}

	if (m_ImageTextureId)
	{
		glDeleteTextures(1, &m_ImageTextureId);
	}

	if (m_FboTextureId)
	{
		glDeleteTextures(1, &m_FboTextureId);
	}

	if (m_VboIds[0])
	{
		glDeleteBuffers(4, m_VboIds);
	}

	if (m_VaoIds[0])
	{
		glDeleteVertexArrays(2, m_VaoIds);
	}

	if (m_FboId)
	{
		glDeleteFramebuffers(1, &m_FboId);
	}

    if (m_DownloadPboIds[0]) {
        glDeleteBuffers(2, m_DownloadPboIds);  // 删除下载 PBO
    }

    if (m_UploadPboIds[0]) {
        glDeleteBuffers(2, m_UploadPboIds);    // 删除上传 PBO
    }

}

/**
 * @brief 创建 FBO（帧缓冲对象）
 * @return 创建成功返回 true，失败返回 false
 * @details 创建 FBO 及其附加纹理，用于离屏渲染
 */
bool PBOSample::CreateFrameBufferObj()
{
	// 创建并初始化 FBO 纹理（用于存储离屏渲染结果）
	glGenTextures(1, &m_FboTextureId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 创建并配置 FBO
	glGenFramebuffers(1, &m_FboId);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);

	// 将纹理附加到 FBO 的颜色附件0
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FboTextureId, 0);

	// 分配纹理存储空间（与原图相同尺寸）
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// 检查 FBO 完整性
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
		LOGCATE("PBOSample::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	return true;

}

/**
 * @brief 更新 MVP 变换矩阵
 * @param mvpMatrix 输出的 MVP 矩阵
 * @param angleX 绕X轴旋转角度（度）
 * @param angleY 绕Y轴旋转角度（度）
 * @param ratio 宽高比
 */
void PBOSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
    LOGCATE("PBOSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
    angleX = angleX % 360;
    angleY = angleY % 360;

    // 转换为弧度
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);

    // 投影矩阵：正交投影
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

    // 视图矩阵：相机位置和朝向
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 1),  // 相机位置：(0,0,1)
            glm::vec3(0, 0, 0),  // 观察目标：原点
            glm::vec3(0, 1, 0)   // 上方向：Y轴正方向
    );

    // 模型矩阵：缩放、旋转、平移变换
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    // 组合为 MVP 矩阵
    mvpMatrix = Projection * View * Model;
}

/**
 * @brief 上传像素数据到纹理（演示 PBO 上传优势）
 * @details 对比两种方式：
 *   1. 使用 PBO（异步）：先从 PBO 复制到纹理，再更新下一帧数据到 PBO
 *   2. 不使用 PBO（同步）：直接从系统内存复制到纹理
 */
void PBOSample::UploadPixels() {
    LOGCATE("PBOSample::UploadPixels");
	int dataSize = m_RenderImage.width * m_RenderImage.height * 4;  // RGBA 总字节数

#ifdef PBO_UPLOAD
	// ==================== 方式1：使用 PBO 上传（异步优化）====================
	// 双缓冲机制：index 用于当前帧，nextIndex 用于下一帧
	int index = m_FrameIndex % 2;
	int nextIndex = (index + 1) % 2;

	// 步骤1：从 PBO[index] 复制数据到纹理（GPU内部操作，快速）
	BEGIN_TIME("PBOSample::UploadPixels Copy Pixels from PBO to Textrure Obj")
		glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_UploadPboIds[index]);
		// 最后一个参数为 0 表示从绑定的 PBO 读取数据，而非系统内存
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_RenderImage.width, m_RenderImage.height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	END_TIME("PBOSample::UploadPixels Copy Pixels from PBO to Textrure Obj")
#else
	// ==================== 方式2：不使用 PBO（同步传输）====================
	// 直接从系统内存复制到纹理（CPU->GPU，较慢）
	BEGIN_TIME("PBOSample::UploadPixels Copy Pixels from System Mem to Textrure Obj")
    glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_RenderImage.width, m_RenderImage.height, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
    END_TIME("PBOSample::UploadPixels Copy Pixels from System Mem to Textrure Obj")
#endif

#ifdef PBO_UPLOAD
	// 步骤2：更新下一帧数据到 PBO[nextIndex]（与步骤1异步执行）
	BEGIN_TIME("PBOSample::UploadPixels Update Image data")
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_UploadPboIds[nextIndex]);
	// 先调用 glBufferData 使旧数据失效（提高性能）
	glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);

	// 映射 PBO 到 CPU 内存空间（零拷贝方式）
	GLubyte *bufPtr = (GLubyte *) glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0,
												   dataSize,
												   GL_MAP_WRITE_BIT |            // 写权限
												   GL_MAP_INVALIDATE_BUFFER_BIT); // 丢弃旧数据
	GO_CHECK_GL_ERROR();
	LOGCATE("PBOSample::UploadPixels bufPtr=%p",bufPtr);

	if(bufPtr)
	{
		GO_CHECK_GL_ERROR();
		// 将图像数据复制到 PBO
		memcpy(bufPtr, m_RenderImage.ppPlane[0], static_cast<size_t>(dataSize));

		// 模拟修改：随机位置绘制5行灰色条纹（用于观察效果）
		int randomRow = rand() % (m_RenderImage.height - 5);
		memset(bufPtr + randomRow * m_RenderImage.width * 4, 188,
        static_cast<size_t>(m_RenderImage.width * 4 * 5));

		// 取消映射，数据将自动传输到 GPU
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	END_TIME("PBOSample::UploadPixels Update Image data")

#else
	// 方式2：模拟更新图像数据（用于性能对比）
    NativeImage nativeImage = m_RenderImage;
	NativeImageUtil::AllocNativeImage(&nativeImage);
	BEGIN_TIME("PBOSample::UploadPixels Update Image data")
		// 随机位置绘制5行灰色条纹
		int randomRow = rand() % (m_RenderImage.height - 5);
		memset(m_RenderImage.ppPlane[0] + randomRow * m_RenderImage.width * 4, 188,
		static_cast<size_t>(m_RenderImage.width * 4 * 5));
        NativeImageUtil::CopyNativeImage(&m_RenderImage, &nativeImage);
	END_TIME("PBOSample::UploadPixels Update Image data")
	NativeImageUtil::FreeNativeImage(&nativeImage);
#endif

}

/**
 * @brief 从 GPU 下载像素数据（演示 PBO 下载优势）
 * @details 对比两种方式：
 *   1. 使用 PBO（异步）：glReadPixels 写入 PBO，下一帧再读取
 *   2. 不使用 PBO（同步）：glReadPixels 直接写入系统内存，阻塞渲染
 */
void PBOSample::DownloadPixels() {
    int dataSize = m_RenderImage.width * m_RenderImage.height * 4;
	NativeImage nativeImage = m_RenderImage;
	nativeImage.format = IMAGE_FORMAT_RGBA;


	uint8_t *pBuffer = new uint8_t[dataSize];
	nativeImage.ppPlane[0] = pBuffer;
	BEGIN_TIME("DownloadPixels glReadPixels without PBO")
		glReadPixels(0, 0, nativeImage.width, nativeImage.height, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
	//NativeImageUtil::DumpNativeImage(&nativeImage, "/sdcard/DCIM", "Normal");
	END_TIME("DownloadPixels glReadPixels without PBO")
    delete []pBuffer;

    int index = m_FrameIndex % 2;
    int nextIndex = (index + 1) % 2;

    BEGIN_TIME("DownloadPixels glReadPixels with PBO")
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_DownloadPboIds[index]);
    glReadPixels(0, 0, m_RenderImage.width, m_RenderImage.height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    END_TIME("DownloadPixels glReadPixels with PBO")

    if(m_DownloadImages[nextIndex].ppPlane[0] == nullptr)
    {
		m_DownloadImages[nextIndex] = m_RenderImage;
		m_DownloadImages[nextIndex].format = IMAGE_FORMAT_RGBA;

		BEGIN_TIME("DownloadPixels PBO glMapBufferRange")
			glBindBuffer(GL_PIXEL_PACK_BUFFER, m_DownloadPboIds[nextIndex]);
			GLubyte *bufPtr = static_cast<GLubyte *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0,
																	  dataSize,
																	  GL_MAP_READ_BIT));

			if (bufPtr) {
				m_DownloadImages[nextIndex].ppPlane[0] = bufPtr;
				//NativeImageUtil::DumpNativeImage(&nativeImage, "/sdcard/DCIM", "PBO");
			}
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		END_TIME("DownloadPixels PBO glMapBufferRange")
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

	if(m_DownloadImages[nextIndex].ppPlane[0] != nullptr) {
		char key[] = "PBO";
		char fileName[64] = {0};
		sprintf(fileName, "%s_%d", key, nextIndex);
		std::string path(DEFAULT_OGL_ASSETS_DIR);
		NativeImageUtil::DumpNativeImage(&m_DownloadImages[nextIndex], path.c_str(), fileName);
	}
}
