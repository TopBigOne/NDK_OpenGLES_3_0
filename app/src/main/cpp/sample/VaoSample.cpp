/**
 * VaoSample - OpenGL ES 3.0 VAO (顶点数组对象) 示例
 *
 * 功能说明:
 * 演示如何使用 VAO (Vertex Array Object) 和 EBO (Element Buffer Object)
 * 来高效管理顶点数据并使用索引绘制
 *
 * 技术要点:
 * 1. VAO 的创建和配置 - 封装顶点属性状态
 * 2. VBO 的使用 - 存储顶点数据（位置 + 颜色）
 * 3. EBO 的使用 - 存储索引数据，避免重复顶点
 * 4. 交错式顶点数据布局 (Interleaved Vertex Data)
 * 5. Shader 中实现棋盘格效果（灰度/彩色交替）
 *
 * VAO 的优势:
 * - 避免每帧重复设置顶点属性配置
 * - 提高渲染性能和代码可读性
 * - 一次绑定 VAO 即可恢复所有顶点属性状态
 *
 * EBO 的优势:
 * - 减少重复顶点数据（如矩形只需4个顶点而非6个）
 * - 节省内存和带宽
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "VaoSample.h"
#include "../util/GLUtils.h"

// ==================== 顶点数据布局常量 ====================
#define VERTEX_POS_SIZE       3  // 顶点位置分量数: x, y, z
#define VERTEX_COLOR_SIZE     4  // 顶点颜色分量数: r, g, b, a

#define VERTEX_POS_INDX       0  // 顶点位置在 shader 中的 location
#define VERTEX_COLOR_INDX     1  // 顶点颜色在 shader 中的 location

// 顶点步长 = (位置3个float + 颜色4个float) * sizeof(float)
#define VERTEX_STRIDE         (sizeof(GLfloat)*(VERTEX_POS_SIZE+VERTEX_COLOR_SIZE))

/**
 * @brief 构造函数
 */
VaoSample::VaoSample()
{
	m_VaoId = 0;  // 初始化 VAO ID

}

/**
 * @brief 析构函数
 */
VaoSample::~VaoSample()
{

}

/**
 * @brief 加载图像（本示例不使用，空实现）
 * @param pImage 图像数据指针
 */
void VaoSample::LoadImage(NativeImage *pImage)
{
	// 本示例不需要加载图像

}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建 VAO、VBO、EBO，配置顶点属性，编译着色器
 */
void VaoSample::Init()
{
	// ==================== 顶点着色器 ====================
	const char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"  // 输入：顶点位置
			"layout(location = 1) in vec4 a_color;      \n"  // 输入：顶点颜色
			"out vec4 v_color;                          \n"  // 输出：传递给片段着色器的颜色
			"out vec4 v_position;                       \n"  // 输出：传递给片段着色器的位置（用于棋盘格计算）
			"void main()                                \n"
			"{                                          \n"
			"    v_color = a_color;                     \n"  // 传递颜色
			"    gl_Position = a_position;              \n"  // 设置顶点位置
			"    v_position = a_position;               \n"  // 传递位置用于片段着色器计算
			"}";


	// ==================== 片段着色器 ====================
	// 实现棋盘格效果：奇数格显示灰度，偶数格显示原色
	const char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec4 v_color;\n"                  // 从顶点着色器传来的颜色
			"in vec4 v_position;\n"               // 从顶点着色器传来的位置
			"out vec4 o_fragColor;\n"             // 输出：片段颜色
			"void main()\n"
			"{\n"
			"    float n = 10.0;\n"               // 棋盘格数量
			"    float span = 1.0 / n;\n"         // 每个格子的跨度
			"    int i = int((v_position.x + 0.5)/span);\n"  // 计算X方向的格子索引
			"    int j = int((v_position.y + 0.5)/span);\n"  // 计算Y方向的格子索引
			"\n"
			"    int grayColor = int(mod(float(i+j), 2.0));\n"  // 判断奇偶（棋盘格效果）
			"    if(grayColor == 1)\n"
			"    {\n"
			"        // 奇数格：转换为灰度（使用标准亮度公式）\n"
			"        float luminance = v_color.r*0.299 + v_color.g*0.587 + v_color.b*0.114;\n"
			"        o_fragColor = vec4(vec3(luminance), v_color.a);\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        // 偶数格：保持原色\n"
			"        o_fragColor = v_color;\n"
			"    }\n"
			"}";

	// ==================== 定义顶点数据（交错式布局）====================
	// 4个顶点组成矩形，每个顶点包含：位置(x,y,z) + 颜色(r,g,b,a)
	GLfloat vertices[4 *(VERTEX_POS_SIZE + VERTEX_COLOR_SIZE )] =
			{
					-0.5f,  0.5f, 0.0f,       // v0 位置：左上
					1.0f,  0.0f, 0.0f, 1.0f,  // c0 颜色：红色
					-0.5f, -0.5f, 0.0f,       // v1 位置：左下
					0.0f,  1.0f, 0.0f, 1.0f,  // c1 颜色：绿色
					0.5f, -0.5f, 0.0f,        // v2 位置：右下
					0.0f,  0.0f, 1.0f, 1.0f,  // c2 颜色：蓝色
					0.5f,  0.5f, 0.0f,        // v3 位置：右上
					0.5f,  1.0f, 1.0f, 1.0f,  // c3 颜色：青色
			};

	// ==================== 索引数据 ====================
	// 使用索引绘制两个三角形组成矩形（避免重复顶点）
	// 第一个三角形: v0-v1-v2，第二个三角形: v0-v2-v3
	GLushort indices[6] = { 0, 1, 2, 0, 2, 3};

	// ==================== 编译着色器程序 ====================
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// ==================== 生成并配置 VBO ====================
	// 生成2个 VBO：一个存顶点数据，一个存索引数据
	glGenBuffers(2, m_VboIds);

	// VBO[0]: 存储顶点数据（位置 + 颜色）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// VBO[1]: 存储索引数据（作为 EBO 使用）
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// ==================== 生成并配置 VAO ====================
	glGenVertexArrays(1, &m_VaoId);      // 生成 VAO
	glBindVertexArray(m_VaoId);          // 绑定 VAO，开始记录顶点属性配置

	// 绑定 VBO 和 EBO（这些绑定会被 VAO 记录）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[1]);

	// 启用顶点属性数组
	glEnableVertexAttribArray(VERTEX_POS_INDX);    // 启用位置属性
	glEnableVertexAttribArray(VERTEX_COLOR_INDX);  // 启用颜色属性

	// 配置顶点位置属性指针（location = 0）
	glVertexAttribPointer(
		VERTEX_POS_INDX,           // location = 0
		VERTEX_POS_SIZE,           // 3 个分量 (x, y, z)
		GL_FLOAT,                  // 数据类型
		GL_FALSE,                  // 不归一化
		VERTEX_STRIDE,             // 步长：7个float（3位置 + 4颜色）
		(const void *)0            // 偏移：从0开始
	);

	// 配置顶点颜色属性指针（location = 1）
	glVertexAttribPointer(
		VERTEX_COLOR_INDX,                              // location = 1
		VERTEX_COLOR_SIZE,                              // 4 个分量 (r, g, b, a)
		GL_FLOAT,                                       // 数据类型
		GL_FALSE,                                       // 不归一化
		VERTEX_STRIDE,                                  // 步长：7个float
		(const void *)(VERTEX_POS_SIZE *sizeof(GLfloat))  // 偏移：跳过3个float（位置数据）
	);

	// 解绑 VAO（保存当前配置）
	glBindVertexArray(GL_NONE);
}

/**
 * @brief 渲染函数 - 每帧调用
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void VaoSample::Draw(int screenW, int screenH)
{
	if(m_ProgramObj == 0) return;

	// ==================== 使用着色器程序 ====================
	glUseProgram(m_ProgramObj);

	// ==================== 绑定 VAO ====================
	// 绑定 VAO 后，所有顶点属性配置自动恢复（无需重复设置）
	glBindVertexArray(m_VaoId);

	// ==================== 使用索引绘制 ====================
	glDrawElements(
		GL_TRIANGLES,         // 图元类型：三角形
		6,                    // 索引数量
		GL_UNSIGNED_SHORT,    // 索引数据类型
		(const void *)0       // 偏移为0（从EBO起始位置读取）
	);

	// ==================== 解绑 VAO ====================
	glBindVertexArray(GL_NONE);
}

/**
 * @brief 销毁 OpenGL 资源
 * @details 删除着色器程序、VBO 和 VAO
 */
void VaoSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);      // 删除着色器程序
		glDeleteBuffers(2, m_VboIds);       // 删除2个VBO
		glDeleteVertexArrays(1, &m_VaoId);  // 删除VAO
		m_ProgramObj = GL_NONE;
	}

}
