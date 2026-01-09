/**
 * TriangleSample - OpenGL ES 3.0 三角形绘制示例
 *
 * 功能说明:
 * 这是 OpenGL ES 最基础的示例，演示如何使用顶点着色器和片段着色器绘制一个简单的三角形
 *
 * 技术要点:
 * 1. 着色器程序的创建和编译（顶点着色器 + 片段着色器）
 * 2. 顶点数据的定义和传递
 * 3. 基础的 OpenGL 渲染流程
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "TriangleSample.h"
#include "../util/GLUtils.h"
#include "../util/LogUtil.h"

/**
 * @brief 构造函数
 */
TriangleSample::TriangleSample()
{

}

/**
 * @brief 析构函数
 */
TriangleSample::~TriangleSample()
{
}

/**
 * @brief 加载图像数据（本示例不使用图像，空实现）
 * @param pImage 图像数据指针
 */
void TriangleSample::LoadImage(NativeImage *pImage)
{
	// 本示例不需要加载图像，空实现

}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建并编译着色器程序，包括顶点着色器和片段着色器
 */
void TriangleSample::Init()
{
	// 避免重复初始化
	if(m_ProgramObj != 0)
		return;

	// ==================== 顶点着色器 ====================
	// 顶点着色器负责处理每个顶点的位置变换
	char vShaderStr[] =
			"#version 300 es                          \n"  // 指定 GLSL 版本为 3.00 ES
			"layout(location = 0) in vec4 vPosition;  \n"  // 输入：顶点位置（location=0 对应 glVertexAttribPointer 的第一个参数）
			"void main()                              \n"
			"{                                        \n"
			"   gl_Position = vPosition;              \n"  // 直接将输入位置赋值给内置变量 gl_Position（无变换）
			"}                                        \n";

	// ==================== 片段着色器 ====================
	// 片段着色器负责计算每个像素（片段）的颜色
	char fShaderStr[] =
			"#version 300 es                              \n"  // 指定 GLSL 版本为 3.00 ES
			"precision mediump float;                     \n"  // 指定浮点精度为中等精度（移动设备上的性能与精度平衡）
			"out vec4 fragColor;                          \n"  // 输出：片段颜色
			"void main()                                  \n"
			"{                                            \n"
			"   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"  // 输出红色 (R=1.0, G=0.0, B=0.0, A=1.0)
			"}                                            \n";

	// 创建着色器程序：编译顶点着色器和片段着色器，并链接成一个完整的程序
	// m_VertexShader 和 m_FragmentShader 会存储编译后的着色器 ID
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

}

/**
 * @brief 渲染函数 - 每帧调用一次
 * @param screenW 屏幕宽度（像素）
 * @param screenH 屏幕高度（像素）
 */
void TriangleSample::Draw(int screenW, int screenH)
{
	LOGCATE("TriangleSample::Draw");

	// ==================== 定义顶点数据 ====================
	// 三角形的三个顶点坐标（x, y, z）
	// OpenGL 的坐标系：中心为 (0, 0)，x 轴向右，y 轴向上，范围 [-1, 1]
	GLfloat vVertices[] = {
			 0.0f,  0.5f, 0.0f,  // 顶点 0：顶部
			-0.5f, -0.5f, 0.0f,  // 顶点 1：左下
			 0.5f, -0.5f, 0.0f,  // 顶点 2：右下
	};

	// 确保着色器程序已创建
	if(m_ProgramObj == 0)
		return;

	// ==================== 清空缓冲区 ====================
	glClear(GL_COLOR_BUFFER_BIT);           // 清空颜色缓冲区
	glClearColor(1.0, 1.0, 1.0, 1.0);       // 设置清空颜色为白色 (R=1, G=1, B=1, A=1)

	// ==================== 使用着色器程序 ====================
	glUseProgram (m_ProgramObj);            // 激活我们创建的着色器程序

	// ==================== 传递顶点数据 ====================
	// 配置顶点属性指针，告诉 OpenGL 如何解析顶点数据
	glVertexAttribPointer (
		0,                // location = 0（对应顶点着色器中的 layout(location = 0)）
		3,                // 每个顶点包含 3 个分量（x, y, z）
		GL_FLOAT,         // 数据类型为浮点型
		GL_FALSE,         // 不需要归一化（数据已经在 [-1, 1] 范围内）
		0,                // 步长为 0（表示顶点数据紧密排列，无间隔）
		vVertices         // 顶点数据数组的指针
	);
	glEnableVertexAttribArray (0);          // 启用顶点属性数组（location = 0）

	// ==================== 绘制三角形 ====================
	glDrawArrays (
		GL_TRIANGLES,     // 图元类型：三角形
		0,                // 起始顶点索引
		3                 // 绘制的顶点数量
	);

	// ==================== 取消激活程序 ====================
	glUseProgram (GL_NONE);                 // 解绑着色器程序

}

/**
 * @brief 销毁 OpenGL 资源
 * @details 释放着色器程序，防止内存泄漏
 */
void TriangleSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);      // 删除着色器程序对象
		m_ProgramObj = GL_NONE;             // 重置为 GL_NONE，避免野指针
	}

}
