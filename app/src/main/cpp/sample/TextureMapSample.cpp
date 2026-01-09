/**
 * TextureMapSample - OpenGL ES 3.0 纹理映射示例
 *
 * 功能说明:
 * 演示如何创建纹理对象，加载图像数据到纹理，并在片段着色器中进行纹理采样
 *
 * 技术要点:
 * 1. 纹理对象的创建和配置（glGenTextures, glTexParameter）
 * 2. 纹理坐标的设置和传递
 * 3. 采样器（sampler2D）在着色器中的使用
 * 4. 使用索引缓冲（EBO）绘制矩形
 *
 * 纹理坐标系统:
 * - 原点 (0,0) 在左下角
 * - s 轴（横向）向右，t 轴（纵向）向上
 * - 范围: [0, 1]
 *
 * Created by 公众号：字节流动 on 2021/10/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include <LogUtil.h>
#include "TextureMapSample.h"

/**
 * @brief 构造函数
 */
TextureMapSample::TextureMapSample()
{
	m_TextureId = 0;  // 初始化纹理 ID 为 0

}

/**
 * @brief 析构函数
 * @details 释放图像内存
 */
TextureMapSample::~TextureMapSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建纹理对象、编译着色器程序、获取 uniform 变量位置
 */
void TextureMapSample::Init()
{
	// ==================== 创建并配置纹理 ====================
	glGenTextures(1, &m_TextureId);              // 生成一个纹理对象
	glBindTexture(GL_TEXTURE_2D, m_TextureId);   // 绑定为 2D 纹理

	// 设置纹理环绕方式（超出 [0,1] 范围时的处理）
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // S 轴（横向）钳位到边缘
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // T 轴（纵向）钳位到边缘

	// 设置纹理过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // 缩小时使用线性过滤
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // 放大时使用线性过滤

	glBindTexture(GL_TEXTURE_2D, GL_NONE);       // 解绑纹理

	// ==================== 顶点着色器 ====================
	char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"  // 输入：顶点位置
			"layout(location = 1) in vec2 a_texCoord;   \n"  // 输入：纹理坐标
			"out vec2 v_texCoord;                       \n"  // 输出：传递给片段着色器的纹理坐标
			"void main()                                \n"
			"{                                          \n"
			"   gl_Position = a_position;               \n"  // 设置顶点位置
			"   v_texCoord = a_texCoord;                \n"  // 传递纹理坐标
			"}                                          \n";

	// ==================== 片段着色器 ====================
	char fShaderStr[] =
			"#version 300 es                                     \n"
			"precision mediump float;                            \n"  // 中等精度
			"in vec2 v_texCoord;                                 \n"  // 输入：从顶点着色器传来的纹理坐标
			"layout(location = 0) out vec4 outColor;             \n"  // 输出：片段颜色
			"uniform sampler2D s_TextureMap;                     \n"  // Uniform：纹理采样器
			"void main()                                         \n"
			"{                                                   \n"
			"  outColor = texture(s_TextureMap, v_texCoord);     \n"  // 使用纹理坐标采样纹理
			"}                                                   \n";

	// 创建着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		// 获取采样器 uniform 变量的位置
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
	}
	else
	{
		LOGCATE("TextureMapSample::Init create program fail");
	}

}

/**
 * @brief 渲染函数 - 每帧调用
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void TextureMapSample::Draw(int screenW, int screenH)
{
	LOGCATE("TextureMapSample::Draw()");

	// 确保程序和纹理已创建
	if(m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	// ==================== 清空缓冲区 ====================
	glClearColor(1.0, 1.0, 1.0, 1.0);  // 设置清空颜色为白色
	glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ==================== 顶点坐标（定义一个矩形）====================
	GLfloat verticesCoords[] = {
			-1.0f,  0.5f, 0.0f,  // 顶点 0：左上
			-1.0f, -0.5f, 0.0f,  // 顶点 1：左下
			 1.0f, -0.5f, 0.0f,  // 顶点 2：右下
			 1.0f,  0.5f, 0.0f,  // 顶点 3：右上
	};

	// ==================== 纹理坐标 ====================
	// 纹理坐标系：原点(0,0)在左下角，范围 [0,1]
	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // 纹理坐标 0：左下
			0.0f,  1.0f,        // 纹理坐标 1：左上
			1.0f,  1.0f,        // 纹理坐标 2：右上
			1.0f,  0.0f         // 纹理坐标 3：右下
	};

	// ==================== 索引数组（使用 EBO 绘制两个三角形组成矩形）====================
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };  // 第一个三角形(0,1,2)，第二个三角形(0,2,3)

	// ==================== 上传图像数据到纹理 ====================
	glActiveTexture(GL_TEXTURE0);                // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);   // 绑定纹理对象
	glTexImage2D(
		GL_TEXTURE_2D,            // 纹理类型：2D 纹理
		0,                        // Mipmap 层级：0（基础层）
		GL_RGBA,                  // 内部格式：RGBA
		m_RenderImage.width,      // 图像宽度
		m_RenderImage.height,     // 图像高度
		0,                        // 边框宽度（必须为0）
		GL_RGBA,                  // 像素数据格式：RGBA
		GL_UNSIGNED_BYTE,         // 像素数据类型：无符号字节
		m_RenderImage.ppPlane[0]  // 像素数据指针
	);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);       // 解绑纹理

	// ==================== 使用着色器程序 ====================
	glUseProgram (m_ProgramObj);

	// ==================== 传递顶点属性数据 ====================
	// 设置顶点位置属性指针（location = 0）
	glVertexAttribPointer (
		0,                          // location = 0
		3,                          // 每个顶点 3 个分量（x, y, z）
		GL_FLOAT,                   // 数据类型
		GL_FALSE,                   // 不归一化
		3 * sizeof (GLfloat),       // 步长
		verticesCoords              // 数据指针
	);

	// 设置纹理坐标属性指针（location = 1）
	glVertexAttribPointer (
		1,                          // location = 1
		2,                          // 每个纹理坐标 2 个分量（s, t）
		GL_FLOAT,                   // 数据类型
		GL_FALSE,                   // 不归一化
		2 * sizeof (GLfloat),       // 步长
		textureCoords               // 数据指针
	);

	glEnableVertexAttribArray (0);  // 启用顶点位置属性
	glEnableVertexAttribArray (1);  // 启用纹理坐标属性

	// ==================== 绑定纹理并设置采样器 ====================
	glActiveTexture(GL_TEXTURE0);                // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);   // 绑定纹理

	// 设置采样器 uniform 变量的值为 0（对应 GL_TEXTURE0）
	glUniform1i(m_SamplerLoc, 0);

	// ==================== 使用索引绘制 ====================
	glDrawElements(
		GL_TRIANGLES,       // 图元类型：三角形
		6,                  // 索引数量
		GL_UNSIGNED_SHORT,  // 索引数据类型
		indices             // 索引数组
	);

}

/**
 * @brief 加载图像数据
 * @param pImage 图像数据结构指针
 * @details 将传入的图像数据拷贝到成员变量 m_RenderImage 中
 */
void TextureMapSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("TextureMapSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);  // 深拷贝图像数据
	}
}

/**
 * @brief 销毁 OpenGL 资源
 * @details 删除着色器程序和纹理对象，释放资源
 */
void TextureMapSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);          // 删除着色器程序
		glDeleteTextures(1, &m_TextureId);      // 删除纹理对象
		m_ProgramObj = GL_NONE;
	}

}
