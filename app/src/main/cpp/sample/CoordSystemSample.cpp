/**
 * CoordSystemSample - OpenGL ES 3.0 坐标系统与 MVP 变换示例
 *
 * 功能说明:
 * 演示 3D 图形渲染中最核心的概念 - MVP (Model-View-Projection) 矩阵变换
 * 通过这三个矩阵的组合，将 3D 物体从模型空间变换到屏幕空间
 *
 * 技术要点:
 * 1. Model 矩阵（模型矩阵）- 物体在世界空间中的变换（旋转、缩放、平移）
 * 2. View 矩阵（观察矩阵）- 定义相机的位置和朝向
 * 3. Projection 矩阵（投影矩阵）- 定义透视投影（近大远小）或正交投影
 * 4. GLM 库的使用 - 强大的数学库，提供矩阵运算函数
 *
 * 坐标空间变换流程:
 * 局部空间 -> 世界空间 -> 观察空间 -> 裁剪空间 -> 屏幕空间
 *   (Model)      (View)     (Projection)   (透视除法)
 *
 * 应用场景:
 * 所有 3D 图形渲染的基础，必须掌握的核心概念
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "CoordSystemSample.h"
#include "../util/GLUtils.h"

/**
 * @brief 构造函数
 * @details 初始化成员变量
 */
CoordSystemSample::CoordSystemSample()
{
	m_SamplerLoc = GL_NONE;     // 采样器 uniform 变量位置
	m_MVPMatLoc = GL_NONE;      // MVP 矩阵 uniform 变量位置

	m_TextureId = GL_NONE;      // 纹理 ID
	m_VaoId = GL_NONE;          // VAO ID

	m_AngleX = 0;               // 绕 X 轴旋转角度（度数）
	m_AngleY = 0;               // 绕 Y 轴旋转角度（度数）

	m_ScaleX = 1.0f;            // X 轴缩放比例
	m_ScaleY = 1.0f;            // Y 轴缩放比例
}

/**
 * @brief 析构函数
 * @details 释放图像内存
 */
CoordSystemSample::~CoordSystemSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建纹理、着色器程序、VAO/VBO，并上传数据
 */
void CoordSystemSample::Init()
{
	// 避免重复初始化
	if(m_ProgramObj)
		return;

	// ==================== 创建并配置纹理 ====================
	glGenTextures(1, &m_TextureId);              // 生成纹理对象
	glBindTexture(GL_TEXTURE_2D, m_TextureId);   // 绑定纹理

	// 设置纹理环绕方式（钳位到边缘）
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// 设置纹理过滤方式（线性过滤）
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, GL_NONE);       // 解绑纹理

	// ==================== 顶点着色器 ====================
	// 关键点：引入 MVP 矩阵变换
	char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;   \n"  // 输入：顶点位置
            "layout(location = 1) in vec2 a_texCoord;   \n"  // 输入：纹理坐标
            "uniform mat4 u_MVPMatrix;                  \n"  // Uniform：MVP 变换矩阵（4x4 矩阵）
            "out vec2 v_texCoord;                       \n"  // 输出：传递给片段着色器的纹理坐标
            "void main()                                \n"
            "{                                          \n"
            "    gl_Position = u_MVPMatrix * a_position;\n"  // 应用 MVP 变换：从模型空间变换到裁剪空间
            "    v_texCoord = a_texCoord;               \n"  // 直接传递纹理坐标
            "}";

	// ==================== 片段着色器 ====================
	char fShaderStr[] =
			"#version 300 es                                     \n"
			"precision mediump float;                            \n"
			"in vec2 v_texCoord;                                 \n"  // 输入：纹理坐标
			"layout(location = 0) out vec4 outColor;             \n"  // 输出：片段颜色
			"uniform sampler2D s_TextureMap;                     \n"  // Uniform：纹理采样器
			"void main()                                         \n"
			"{                                                   \n"
			"  outColor = texture(s_TextureMap, v_texCoord);     \n"  // 采样纹理颜色
			"}                                                   \n";

	// ==================== 编译着色器程序 ====================
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		// 获取 uniform 变量位置
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");  // 纹理采样器
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");    // MVP 矩阵
	}
	else
	{
		LOGCATE("CoordSystemSample::Init create program fail");
	}

	// ==================== 定义顶点坐标（矩形的 4 个顶点）====================
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // 顶点 0：左上
			-1.0f, -1.0f, 0.0f,  // 顶点 1：左下
			 1.0f, -1.0f, 0.0f,  // 顶点 2：右下
			 1.0f,  1.0f, 0.0f,  // 顶点 3：右上
	};

	// ==================== 定义纹理坐标 ====================
	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // 纹理坐标 0：左下
			0.0f,  1.0f,        // 纹理坐标 1：左上
			1.0f,  1.0f,        // 纹理坐标 2：右上
			1.0f,  0.0f         // 纹理坐标 3：右下
	};

	// ==================== 索引数据（EBO）====================
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };  // 两个三角形组成矩形

	// ==================== 生成 VBO 并上传数据 ====================
	glGenBuffers(3, m_VboIds);  // 生成 3 个缓冲区：顶点位置、纹理坐标、索引

	// VBO[0]: 顶点位置数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

	// VBO[1]: 纹理坐标数据
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	// VBO[2]: 索引数据（EBO）
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// ==================== 生成并配置 VAO ====================
	glGenVertexArrays(1, &m_VaoId);    // 生成 VAO
	glBindVertexArray(m_VaoId);        // 绑定 VAO，开始记录顶点属性配置

	// 配置顶点位置属性（location = 0）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);      // 启用顶点属性数组
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);  // 解绑 VBO

	// 配置纹理坐标属性（location = 1）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(1);      // 启用顶点属性数组
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);  // 解绑 VBO

	// 绑定索引缓冲区（EBO）
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

	glBindVertexArray(GL_NONE);        // 解绑 VAO

	// ==================== 上传图像数据到纹理 ====================
	glActiveTexture(GL_TEXTURE0);                // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);   // 绑定纹理
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);       // 解绑纹理
}

/**
 * @brief 加载图像数据
 * @param pImage 图像数据结构指针
 */
void CoordSystemSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("CoordSystemSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);  // 深拷贝图像数据
	}
}

/**
 * @brief 渲染函数 - 每帧调用
 * @param screenW 屏幕宽度（像素）
 * @param screenH 屏幕高度（像素）
 */
void CoordSystemSample::Draw(int screenW, int screenH)
{
	LOGCATE("CoordSystemSample::Draw()");

	// 确保程序和纹理已创建
	if(m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	// ==================== 更新 MVP 矩阵 ====================
	// 根据当前旋转角度、屏幕宽高比计算新的 MVP 矩阵
	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);

	// ==================== 使用着色器程序 ====================
	glUseProgram (m_ProgramObj);

	// ==================== 绑定 VAO ====================
	glBindVertexArray(m_VaoId);  // 绑定 VAO，自动恢复所有顶点属性配置

	// ==================== 传递 MVP 矩阵到着色器 ====================
	// glUniformMatrix4fv: 传递 4x4 矩阵
	// 参数: location, 矩阵数量, 是否转置, 矩阵数据指针
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);

	// ==================== 绑定纹理并设置采样器 ====================
	glActiveTexture(GL_TEXTURE0);                // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);   // 绑定纹理
	glUniform1i(m_SamplerLoc, 0);                // 设置采样器为纹理单元 0

	// ==================== 使用索引绘制 ====================
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
}

/**
 * @brief 销毁 OpenGL 资源
 */
void CoordSystemSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);          // 删除着色器程序
		glDeleteBuffers(3, m_VboIds);           // 删除 3 个 VBO
		glDeleteVertexArrays(1, &m_VaoId);      // 删除 VAO
		glDeleteTextures(1, &m_TextureId);      // 删除纹理
	}
}

/**
 * @brief 更新 MVP 矩阵
 * @param mvpMatrix 输出的 MVP 矩阵
 * @param angleX 绕 X 轴旋转角度（度数）
 * @param angleY 绕 Y 轴旋转角度（度数）
 * @param ratio 屏幕宽高比（width / height）
 *
 * @details
 * MVP 矩阵计算过程：
 * 1. Projection 矩阵：定义投影方式（透视/正交）
 * 2. View 矩阵：定义相机的位置、朝向、上方向
 * 3. Model 矩阵：定义物体的缩放、旋转、平移
 * 4. 最终 MVP = Projection * View * Model
 *
 * 注意：矩阵乘法顺序是从右往左应用的！
 */
void CoordSystemSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("CoordSystemSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);

	// 将角度限制在 [0, 360) 范围内
	angleX = angleX % 360;
	angleY = angleY % 360;

	// 转换为弧度（OpenGL 的三角函数使用弧度）
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);

	// ==================== 1. Projection 矩阵（投影矩阵）====================
	// 选项 1：正交投影（无透视效果，远近物体大小相同）
	// glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

	// 选项 2：视锥体投影（透视效果，但参数较复杂）
	// glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);

	// 选项 3：透视投影（最常用，符合人眼视觉）✅
	// 参数：视场角（FOV）45度，宽高比，近裁剪面，远裁剪面
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 100.f);

	// ==================== 2. View 矩阵（观察矩阵）====================
	// glm::lookAt 定义相机的位置和朝向
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 4),  // 相机位置：在世界空间的 (0, 0, 4) 位置（Z轴正方向）
			glm::vec3(0, 0, 0),  // 目标位置：相机看向原点 (0, 0, 0)
			glm::vec3(0, 1, 0)   // 上方向：Y 轴正方向为"上"（设置为 (0, -1, 0) 则上下颠倒）
	);

	// ==================== 3. Model 矩阵（模型矩阵）====================
	glm::mat4 Model = glm::mat4(1.0f);  // 初始化为单位矩阵（无变换）

	// 应用变换（注意顺序：先缩放，再旋转，最后平移）
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));   // 缩放
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));  // 绕 X 轴旋转
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));  // 绕 Y 轴旋转
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));        // 平移（这里无平移）

	// ==================== 4. 计算最终 MVP 矩阵 ====================
	// 矩阵乘法顺序：Projection * View * Model
	// 变换应用顺序（从右往左）：顶点先经过 Model 变换，再经过 View 变换，最后经过 Projection 变换
	mvpMatrix = Projection * View * Model;
}

/**
 * @brief 更新变换参数（旋转和缩放）
 * @param rotateX 绕 X 轴旋转角度
 * @param rotateY 绕 Y 轴旋转角度
 * @param scaleX X 轴缩放比例
 * @param scaleY Y 轴缩放比例
 */
void CoordSystemSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}
