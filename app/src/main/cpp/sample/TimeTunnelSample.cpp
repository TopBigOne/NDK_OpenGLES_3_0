/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include "TimeTunnelSample.h"

// 顶点位置属性索引
#define VERTEX_POS_INDX  0
// 纹理坐标属性索引
#define TEXTURE_POS_INDX 1

// 构造函数：初始化所有成员变量
TimeTunnelSample::TimeTunnelSample()
{
    // 图像纹理 ID
    m_ImageTextureId = GL_NONE;
    // FBO 纹理 ID
	m_FboTextureId = GL_NONE;
	// 采样器 uniform 位置
	m_SamplerLoc = GL_NONE;
	// 帧缓冲对象 ID
	m_FboId = GL_NONE;
	// FBO 着色器程序 ID
	m_FboProgramObj = GL_NONE;
	// FBO 顶点着色器 ID
	m_FboVertexShader = GL_NONE;
	// FBO 片段着色器 ID
	m_FboFragmentShader = GL_NONE;
	// FBO 采样器 uniform 位置
	m_FboSamplerLoc = GL_NONE;
	// MVP 矩阵 uniform 位置
	m_MVPMatrixLoc = GL_NONE;
	// 图像尺寸 uniform 位置
	m_ImgSizeLoc = GL_NONE;
	// 时间 uniform 位置
	m_TimeLoc = GL_NONE;

    // 绕 X 轴的旋转角度
    m_AngleX = 0;
    // 绕 Y 轴的旋转角度
    m_AngleY = 0;

    // X 轴缩放比例
    m_ScaleX = 1.0f;
    // Y 轴缩放比例
    m_ScaleY = 1.0f;

    // 帧索引，用于动画计算
    m_FrameIndex = 0;
}

// 析构函数：释放图像资源
TimeTunnelSample::~TimeTunnelSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

// 加载图像数据
void TimeTunnelSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("TimeTunnelSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		// 复制图像的宽度、高度、格式和数据
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

// 初始化 OpenGL 资源
void TimeTunnelSample::Init()
{
    // 如果已经初始化过，直接返回
    if(m_ProgramObj)
        return;

	// 顶点坐标数组（NDC 标准化设备坐标，范围 -1 到 1）
	// 四个顶点组成一个矩形平面
	GLfloat vVertices[] = {
			-1.0f, -1.0f, 0.0f,  // 左下角
			 1.0f, -1.0f, 0.0f,  // 右下角
			-1.0f,  1.0f, 0.0f,  // 左上角
			 1.0f,  1.0f, 0.0f,  // 右上角
	};

	// 正常纹理坐标（用于普通渲染）
	// OpenGL 纹理坐标原点在左上角
	GLfloat vTexCoors[] = {
            0.0f, 1.0f,  // 左下角对应纹理的左上角
            1.0f, 1.0f,  // 右下角对应纹理的右上角
            0.0f, 0.0f,  // 左上角对应纹理的左下角
            1.0f, 0.0f,  // 右上角对应纹理的右下角
    };

	// FBO 纹理坐标（用于离屏渲染）
	// FBO 纹理坐标原点位于左下角，与正常纹理方向不同
	GLfloat vFboTexCoors[] = {
			0.0f, 0.0f,  // 左下角
			1.0f, 0.0f,  // 右下角
			0.0f, 1.0f,  // 左上角
			1.0f, 1.0f,  // 右上角
	};

	// 索引数组，定义两个三角形组成矩形
	GLushort indices[] = { 0, 1, 2, 1, 3, 2 };

	// 普通渲染的顶点着色器脚本
	// 使用 MVP 矩阵进行坐标变换
	char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"  // 顶点位置
			"layout(location = 1) in vec2 a_texCoord;   \n"  // 纹理坐标
            "uniform mat4 u_MVPMatrix;                  \n"  // MVP 变换矩阵
            "out vec2 v_texCoord;                       \n"  // 输出给片段着色器的纹理坐标
			"void main()                                \n"
			"{                                          \n"
			"   gl_Position = u_MVPMatrix * a_position; \n"  // 应用 MVP 变换
			"   v_texCoord = a_texCoord;                \n"  // 传递纹理坐标
			"}                                          \n";

	// 普通渲染的片段着色器脚本
	// 简单的纹理采样和输出
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"                           // 从顶点着色器接收的纹理坐标
			"layout(location = 0) out vec4 outColor;\n"      // 输出颜色
			"uniform sampler2D s_TextureMap;\n"              // 纹理采样器
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"  // 采样纹理
			"}";

	// FBO 离屏渲染的顶点着色器脚本
	// 不使用 MVP 变换矩阵，直接使用顶点坐标
    char vFboShaderStr[] =
            "#version 300 es                            \n"
            "layout(location = 0) in vec4 a_position;   \n"  // 顶点位置
            "layout(location = 1) in vec2 a_texCoord;   \n"  // 纹理坐标
            "out vec2 v_texCoord;                       \n"  // 输出给片段着色器的纹理坐标
            "void main()                                \n"
            "{                                          \n"
            "   gl_Position = a_position;               \n"  // 不做变换，直接使用输入坐标
            "   v_texCoord = a_texCoord;                \n"  // 传递纹理坐标
            "}                                          \n";

	// FBO 离屏渲染的片段着色器脚本
	// 实现时空隧道效果的核心算法
	char fFboShaderStr[] =
			"#version 300 es\n"
            "precision highp float;\n"
            "layout(location = 0) out vec4 outColor;\n"
            "uniform float u_time;\n"                         // 时间变量，用于动画
            "uniform vec2 u_imgSize;\n"                       // 图像尺寸
            "void main()\n"
            "{\n"
            "    vec2 fragCoord = gl_FragCoord.xy;\n"         // 当前片段的屏幕坐标
            "\n"
            "    // 输入：像素坐标\n"
            "    // 将坐标转换为以中心为原点的标准化坐标\n"
            "    vec2 p = (-u_imgSize.xy + 2.0*fragCoord)/u_imgSize.y;\n"
            "\n"
            "    // 计算每个像素相对于屏幕中心的角度\n"
            "    float a = atan(p.y,p.x);\n"
            "\n"
            "    // 修改后的距离度量，使用四次幂创建非线性距离场\n"
            "    // 这会产生一种超椭圆形状的扭曲效果\n"
            "    float r = pow( pow(p.x*p.x,4.0) + pow(p.y*p.y,4.0), 1.0/8.0 );\n"
            "\n"
            "    // 通过（动画化的反向）半径和角度来索引纹理\n"
            "    // 1/r 创建隧道效果，u_time 使隧道向内运动\n"
            "    vec2 uv = vec2( 1.0/r + 0.2*u_time, a );\n"
            "\n"
            "    // 图案：使用余弦函数创建条纹图案\n"
            "    // 12.0 和 6.0 控制条纹的密度\n"
            "    float f = cos(12.0*uv.x)*cos(6.0*uv.y);\n"
            "\n"
            "    // 颜色获取：使用调色板映射\n"
            "    // 将余弦值映射到 RGB 颜色，产生彩虹色效果\n"
            "    vec3 col = 0.5 + 0.5*sin( 3.1416*f + vec3(0.0,0.5,1.0) );\n"
            "\n"
            "    // 光照：在中心区域变暗\n"
            "    // 乘以 r 使得靠近中心的区域更暗，形成隧道的深度感\n"
            "    col = col*r;\n"
            "\n"
            "    // 输出：像素颜色\n"
            "    outColor = vec4( col, 1.0 );\n"
            "}";

	// 编译和链接普通渲染的着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// 编译和链接 FBO 离屏渲染的着色器程序
	m_FboProgramObj = GLUtils::CreateProgram(vFboShaderStr, fFboShaderStr, m_FboVertexShader, m_FboFragmentShader);

	// 检查着色器程序是否创建成功
	if (m_ProgramObj == GL_NONE || m_FboProgramObj == GL_NONE)
	{
		LOGCATE("TimeTunnelSample::Init m_ProgramObj == GL_NONE");
		return;
	}
	// 获取普通渲染程序的 uniform 变量位置
	m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
	m_MVPMatrixLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
	// 获取 FBO 渲染程序的 uniform 变量位置
	m_FboSamplerLoc = glGetUniformLocation(m_FboProgramObj, "s_TextureMap");
	m_ImgSizeLoc = glGetUniformLocation(m_FboProgramObj, "u_imgSize");
	m_TimeLoc = glGetUniformLocation(m_FboProgramObj, "u_time");

	// 生成 4 个 VBO（顶点缓冲对象），分别用于存储：
	// VBO[0]: 顶点坐标
	// VBO[1]: 普通纹理坐标
	// VBO[2]: FBO 纹理坐标
	// VBO[3]: 索引数据
	glGenBuffers(4, m_VboIds);

	// 加载顶点坐标到 VBO[0]
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

	// 加载普通纹理坐标到 VBO[1]
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoors), vTexCoors, GL_STATIC_DRAW);

	// 加载 FBO 纹理坐标到 VBO[2]
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vFboTexCoors), vFboTexCoors, GL_STATIC_DRAW);

	// 加载索引数据到 VBO[3]
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GO_CHECK_GL_ERROR();

	// 生成 2 个 VAO（顶点数组对象）
	// VAO[0] 用于普通渲染，VAO[1] 用于 FBO 离屏渲染
	glGenVertexArrays(2, m_VaoIds);

    // 配置普通渲染的 VAO[0]
	glBindVertexArray(m_VaoIds[0]);

	// 绑定顶点位置属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定纹理坐标属性（使用普通纹理坐标）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);


	// 配置 FBO 离屏渲染的 VAO[1]
	glBindVertexArray(m_VaoIds[1]);

	// 绑定顶点位置属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定纹理坐标属性（使用 FBO 纹理坐标）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);

	// 创建并初始化图像纹理
	glGenTextures(1, &m_ImageTextureId);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	// 设置纹理环绕方式为边缘截取
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 设置纹理过滤方式为线性过滤
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// 上传图像数据到纹理
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	GO_CHECK_GL_ERROR();

	// 创建帧缓冲对象（FBO）
	if (!CreateFrameBufferObj())
	{
		LOGCATE("TimeTunnelSample::Init CreateFrameBufferObj fail");
		return;
	}

}

// 绘制函数：执行离屏渲染和普通渲染两个阶段
void TimeTunnelSample::Draw(int screenW, int screenH)
{
	// ==== 第一阶段：离屏渲染（生成时空隧道效果）====

	// 清除颜色、深度和模板缓冲
	glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	// 设置视口大小为图像尺寸
	glViewport(0, 0, m_RenderImage.width, m_RenderImage.height);

	// 更新图像纹理数据
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_RenderImage.width, m_RenderImage.height, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
    GO_CHECK_GL_ERROR();

	// 绑定 FBO，执行离屏渲染
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	// 使用 FBO 着色器程序
	glUseProgram(m_FboProgramObj);
	// 绑定 FBO 专用的 VAO
	glBindVertexArray(m_VaoIds[1]);

	// 激活纹理单元 0 并绑定图像纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glUniform1i(m_FboSamplerLoc, 0);

	// 设置时空隧道效果的参数
	glUniform2f(m_ImgSizeLoc, m_RenderImage.width, m_RenderImage.height);  // 图像尺寸
	glUniform1f(m_TimeLoc, m_FrameIndex * 0.04f);  // 时间参数，控制隧道动画速度
	GO_CHECK_GL_ERROR();
	// 绘制到 FBO
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	// 解绑 VAO 和纹理
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	// 解绑 FBO，恢复到默认帧缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ==== 第二阶段：普通渲染（将 FBO 结果渲染到屏幕）====

	// 设置视口大小为屏幕尺寸
	glViewport(0, 0, screenW, screenH);
	// 更新 MVP 矩阵，支持旋转和缩放
    UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);
	// 使用普通渲染的着色器程序
    glUseProgram(m_ProgramObj);
	GO_CHECK_GL_ERROR();
	// 绑定普通渲染的 VAO
	glBindVertexArray(m_VaoIds[0]);
	// 激活纹理单元 0 并绑定 FBO 纹理（包含时空隧道效果）
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	// 传递 MVP 矩阵到着色器
    glUniformMatrix4fv(m_MVPMatrixLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
	glUniform1i(m_SamplerLoc, 0);
	GO_CHECK_GL_ERROR();
	// 绘制到屏幕
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	// 解绑纹理和 VAO
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindVertexArray(GL_NONE);
	// 帧索引递增，用于动画
	m_FrameIndex++;

}

// 销毁函数：释放所有 OpenGL 资源
void TimeTunnelSample::Destroy()
{
	// 删除普通渲染着色器程序
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
		m_ProgramObj = GL_NONE;
	}

	// 删除 FBO 渲染着色器程序
	if (m_FboProgramObj)
	{
		glDeleteProgram(m_FboProgramObj);
		m_FboProgramObj = GL_NONE;
	}

	// 删除图像纹理
	if (m_ImageTextureId)
	{
		glDeleteTextures(1, &m_ImageTextureId);
	}

	// 删除 FBO 纹理
	if (m_FboTextureId)
	{
		glDeleteTextures(1, &m_FboTextureId);
	}

	// 删除所有 VBO
	if (m_VboIds[0])
	{
		glDeleteBuffers(4, m_VboIds);
	}

	// 删除所有 VAO
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

// 创建帧缓冲对象（FBO）
bool TimeTunnelSample::CreateFrameBufferObj()
{
	// 创建并初始化 FBO 纹理
	glGenTextures(1, &m_FboTextureId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	// 设置纹理环绕方式
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 设置纹理过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 创建并初始化 FBO
	glGenFramebuffers(1, &m_FboId);
	// 绑定 FBO
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	// 绑定 FBO 纹理
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	// 将纹理附加到 FBO 的颜色附件
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FboTextureId, 0);
	// 分配纹理存储空间（与图像尺寸相同）
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	// 检查 FBO 完整性
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
		LOGCATE("TimeTunnelSample::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
		return false;
	}
	// 解绑纹理和 FBO
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	return true;

}

/**
 * 更新 MVP 矩阵
 * @param mvpMatrix 输出的 MVP 矩阵
 * @param angleX 绕 X 轴旋转角度（度）
 * @param angleY 绕 Y 轴旋转角度（度）
 * @param ratio 屏幕宽高比
 */
void TimeTunnelSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
    LOGCATE("TimeTunnelSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
    // 将角度限制在 0-360 度范围内
    angleX = angleX % 360;
    angleY = angleY % 360;

    // 将角度转换为弧度
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);

    // 投影矩阵：使用正交投影
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    // 其他投影方式（已注释）：
    // 透视投影（视锥体）：glm::frustum(-ratio, ratio, -1.0f, 1.0f, 1.0f, 100);
    // 透视投影：glm::perspective(45.0f,ratio, 0.1f,100.f);

    // 视图矩阵：定义相机位置和方向
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 1), // 相机位于世界坐标 (0,0,1)
            glm::vec3(0, 0, 0), // 相机朝向原点
            glm::vec3(0, 1, 0)  // 向上方向为 Y 轴正方向
    );

    // 模型矩阵：定义物体的变换（缩放、旋转、平移）
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));       // 缩放
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));    // 绕 X 轴旋转
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));    // 绕 Y 轴旋转
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));           // 平移（这里不做平移）

    // 计算最终的 MVP 矩阵（顺序：投影 * 视图 * 模型）
    mvpMatrix = Projection * View * Model;
}