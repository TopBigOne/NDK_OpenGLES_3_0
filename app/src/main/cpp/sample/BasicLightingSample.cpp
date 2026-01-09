/**
 * BasicLightingSample - OpenGL ES 3.0 冯氏光照模型示例
 *
 * 功能说明:
 * 实现经典的 Phong（冯氏）光照模型，展示物体在光照作用下的真实感渲染效果
 *
 * 技术要点:
 * 1. 冯氏光照模型三要素：环境光（Ambient）、漫反射（Diffuse）、镜面反射（Specular）
 * 2. 法线向量的变换和应用
 * 3. 光源位置、观察者位置对光照效果的影响
 * 4. 在顶点着色器中计算光照，将结果传递给片段着色器
 *
 * 应用场景:
 * - 3D 物体的基础光照渲染
 * - 实时光照计算
 * - 光照模型学习和演示
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "BasicLightingSample.h"
#include "../util/GLUtils.h"

/**
 * @brief 构造函数 - 初始化所有成员变量
 */
BasicLightingSample::BasicLightingSample()
{
	// 初始化 Uniform 变量位置
	m_SamplerLoc = GL_NONE;
	m_MVPMatLoc = GL_NONE;
	m_ModelMatrixLoc = GL_NONE;
	m_LightPosLoc = GL_NONE;

	// 初始化 OpenGL 对象 ID
	m_TextureId = GL_NONE;
	m_VaoId = GL_NONE;

	// 初始化旋转角度
	m_AngleX = 0;
	m_AngleY = 0;

	// 初始化缩放比例
	m_ScaleX = 1.0f;
	m_ScaleY = 1.0f;

	// 初始化模型矩阵
	m_ModelMatrix = glm::mat4(0.0f);
}

/**
 * @brief 析构函数 - 释放图像资源
 */
BasicLightingSample::~BasicLightingSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建着色器程序、纹理对象、VAO 和 VBO，配置光照相关的着色器
 */
void BasicLightingSample::Init()
{
	// 避免重复初始化
	if (m_ProgramObj)
	{
		return;
	}

	// ==================== 创建纹理对象 ====================
	glGenTextures(1, &m_TextureId);                                        // 生成纹理 ID
	glBindTexture(GL_TEXTURE_2D, m_TextureId);                             // 绑定纹理对象
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // S 轴（横向）使用边缘夹持模式
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // T 轴（纵向）使用边缘夹持模式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);     // 缩小时使用线性插值
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);     // 放大时使用线性插值
	glBindTexture(GL_TEXTURE_2D, GL_NONE);                                 // 解绑纹理

	// ==================== 顶点着色器 ====================
	// 在顶点着色器中计算冯氏光照模型的三个分量
	char vShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"layout(location = 0) in vec4 a_position;\n"    // 顶点位置
			"layout(location = 1) in vec2 a_texCoord;\n"    // 纹理坐标
			"layout(location = 2) in vec3 a_normal;\n"      // 顶点法线
			"uniform mat4 u_MVPMatrix;\n"                   // MVP 矩阵
			"uniform mat4 u_ModelMatrix;\n"                 // 模型矩阵（用于变换法线和片段位置）
			"uniform vec3 lightPos;\n"                      // 光源位置（世界坐标）
			"uniform vec3 lightColor;\n"                    // 光源颜色
			"uniform vec3 viewPos;\n"                       // 观察者位置（世界坐标）
			"out vec2 v_texCoord;\n"                        // 输出纹理坐标
			"out vec3 ambient;\n"                           // 输出环境光分量
			"out vec3 diffuse;\n"                           // 输出漫反射分量
			"out vec3 specular;\n"                          // 输出镜面反射分量
			"void main()\n"
			"{\n"
			"    gl_Position = u_MVPMatrix * a_position;\n"                                    // 计算裁剪空间坐标
			"    vec3 fragPos = vec3(u_ModelMatrix * a_position);\n"                           // 计算片段的世界坐标\n"
			"\n"
			"    // Ambient - 环境光：模拟场景中的基础光照，与光源方向无关\n"
			"    float ambientStrength = 0.1;\n"                                               // 环境光强度系数
			"    ambient = ambientStrength * lightColor;\n"                                    // 计算环境光分量
			"\n"
			"    // Diffuse - 漫反射：模拟粗糙表面的散射，强度与入射角度相关\n"
            "    float diffuseStrength = 0.5;\n"                                               // 漫反射强度系数
			"    vec3 unitNormal = normalize(vec3(u_ModelMatrix * vec4(a_normal, 1.0)));\n"   // 将法线变换到世界空间并归一化
			"    vec3 lightDir = normalize(lightPos - fragPos);\n"                             // 计算从片段指向光源的单位向量
			"    float diff = max(dot(unitNormal, lightDir), 0.0);\n"                         // 计算法线和光线方向的点积（余弦值），取非负值
			"    diffuse = diffuseStrength * diff * lightColor;\n"                            // 计算漫反射分量
			"\n"
			"    // Specular - 镜面反射：模拟光滑表面的高光，强度与观察角度相关\n"
			"    float specularStrength = 0.9;\n"                                              // 镜面反射强度系数
			"    vec3 viewDir = normalize(viewPos - fragPos);\n"                               // 计算从片段指向观察者的单位向量
			"    vec3 reflectDir = reflect(-lightDir, unitNormal);\n"                          // 计算光线的反射方向
			"    float spec = pow(max(dot(unitNormal, reflectDir), 0.0), 16.0);\n"            // 计算镜面反射系数，16.0 是高光指数（控制高光范围）
			"    specular = specularStrength * spec * lightColor;\n"                          // 计算镜面反射分量
			"    v_texCoord = a_texCoord;\n"                                                   // 传递纹理坐标
			"}";

	// ==================== 片段着色器 ====================
	// 将光照分量与纹理颜色结合，输出最终颜色
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"                        // 输入纹理坐标
			"in vec3 ambient;\n"                           // 输入环境光分量
			"in vec3 diffuse;\n"                           // 输入漫反射分量
			"in vec3 specular;\n"                          // 输入镜面反射分量
			"layout(location = 0) out vec4 outColor;\n"   // 输出片段颜色
			"uniform sampler2D s_TextureMap;\n"           // 纹理采样器
			"void main()\n"
			"{\n"
			"    vec4 objectColor = texture(s_TextureMap, v_texCoord);\n"           // 从纹理采样物体颜色
			"    vec3 finalColor = (ambient + diffuse + specular) * vec3(objectColor);\n"  // 将三个光照分量相加并与物体颜色相乘
			"    outColor = vec4(finalColor, 1.0);\n"                               // 输出最终颜色
			"}";

	// ==================== 创建着色器程序 ====================
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		// 获取 Uniform 变量位置
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");      // 纹理采样器
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");        // MVP 矩阵
		m_ModelMatrixLoc = glGetUniformLocation(m_ProgramObj, "u_ModelMatrix"); // 模型矩阵
		m_LightPosLoc = glGetUniformLocation(m_ProgramObj, "lightPos");         // 光源位置
		m_LightColorLoc = glGetUniformLocation(m_ProgramObj, "lightColor");     // 光源颜色
		m_ViewPosLoc = glGetUniformLocation(m_ProgramObj, "viewPos");           // 观察者位置
	}
	else
	{
		LOGCATE("BasicLightingSample::Init create program fail");
		return;
	}

	// ==================== 定义立方体顶点数据 ====================
	// 每个顶点包含：位置(3) + 纹理坐标(2) + 法线(3) = 8 个浮点数
	GLfloat vertices[] = {
			 //position            //texture coord  //normal
			-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,      0.0f,  0.0f, -1.0f,  // 背面
			 0.5f, -0.5f, -0.5f,   1.0f, 0.0f,      0.0f,  0.0f, -1.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,      0.0f,  0.0f, -1.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,      0.0f,  0.0f, -1.0f,
			-0.5f,  0.5f, -0.5f,   0.0f, 1.0f,      0.0f,  0.0f, -1.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,      0.0f,  0.0f, -1.0f,

			-0.5f, -0.5f, 0.5f,    0.0f, 0.0f,      0.0f,  0.0f,  1.0f,  // 正面
			 0.5f, -0.5f, 0.5f,    1.0f, 0.0f,      0.0f,  0.0f,  1.0f,
			 0.5f,  0.5f, 0.5f,    1.0f, 1.0f,      0.0f,  0.0f,  1.0f,
			 0.5f,  0.5f, 0.5f,    1.0f, 1.0f,      0.0f,  0.0f,  1.0f,
			-0.5f,  0.5f, 0.5f,    0.0f, 1.0f,      0.0f,  0.0f,  1.0f,
			-0.5f, -0.5f, 0.5f,    0.0f, 0.0f,      0.0f,  0.0f,  1.0f,

			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f,     -1.0f,  0.0f,  0.0f,  // 左侧面
			-0.5f,  0.5f, -0.5f,   1.0f, 1.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f,     -1.0f,  0.0f,  0.0f,

			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f,      1.0f,  0.0f,  0.0f,  // 右侧面
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,      1.0f,  0.0f,  0.0f,
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      1.0f,  0.0f,  0.0f,
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      1.0f,  0.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,   0.0f, 0.0f,      1.0f,  0.0f,  0.0f,
			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f,      1.0f,  0.0f,  0.0f,

			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      0.0f, -1.0f,  0.0f,  // 底面
			 0.5f, -0.5f, -0.5f,   1.0f, 1.0f,      0.0f, -1.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f,      0.0f, -1.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f,      0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,      0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      0.0f, -1.0f,  0.0f,

			-0.5f, 0.5f, -0.5f,    0.0f, 1.0f,      0.0f,  1.0f,  0.0f,  // 顶面
			 0.5f, 0.5f, -0.5f,    1.0f, 1.0f,      0.0f,  1.0f,  0.0f,
			 0.5f, 0.5f,  0.5f,    1.0f, 0.0f,      0.0f,  1.0f,  0.0f,
			 0.5f, 0.5f,  0.5f,    1.0f, 0.0f,      0.0f,  1.0f,  0.0f,
			-0.5f, 0.5f,  0.5f,    0.0f, 0.0f,      0.0f,  1.0f,  0.0f,
			-0.5f, 0.5f, -0.5f,    0.0f, 1.0f,      0.0f,  1.0f,  0.0f,
	};

	// ==================== 创建 VBO 并上传顶点数据 ====================
	glGenBuffers(1, m_VboIds);                                                // 生成 VBO ID
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);                               // 绑定 VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 上传顶点数据（静态绘制）

	// ==================== 创建并配置 VAO ====================
	glGenVertexArrays(1, &m_VaoId);                                           // 生成 VAO ID
	glBindVertexArray(m_VaoId);                                               // 绑定 VAO

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);                               // 绑定 VBO
	// 配置顶点位置属性（location = 0）
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const void *) 0);
	// 配置纹理坐标属性（location = 1）
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const void *) (3* sizeof(GLfloat)));
	// 配置法线属性（location = 2）
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (const void *) (5* sizeof(GLfloat)));
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);                                   // 解绑 VBO

	glBindVertexArray(GL_NONE);                                               // 解绑 VAO

}

/**
 * @brief 加载图像数据
 * @param pImage 图像数据指针
 */
void BasicLightingSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("BasicLightingSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);  // 拷贝图像数据
	}

}

/**
 * @brief 渲染函数 - 每帧调用一次
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void BasicLightingSample::Draw(int screenW, int screenH)
{
	LOGCATE("BasicLightingSample::Draw()");

	// 确保着色器程序和纹理已创建
	if (m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	// ==================== 启用深度测试 ====================
	glEnable(GL_DEPTH_TEST);                                                  // 启用深度测试，确保立方体前后关系正确

	// ==================== 更新 MVP 矩阵 ====================
	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float) screenW / screenH);

	// ==================== 上传纹理数据 ====================
	glActiveTexture(GL_TEXTURE0);                                             // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);                                // 绑定纹理对象
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA,
				 GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);                // 上传 RGBA 图像数据
	glBindTexture(GL_TEXTURE_2D, GL_NONE);                                    // 解绑纹理

	// ==================== 使用着色器程序并设置 Uniform 变量 ====================
	glUseProgram(m_ProgramObj);                                               // 激活着色器程序

	glBindVertexArray(m_VaoId);                                               // 绑定 VAO

	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);         // 设置 MVP 矩阵
	glUniformMatrix4fv(m_ModelMatrixLoc, 1, GL_FALSE, &m_ModelMatrix[0][0]);  // 设置模型矩阵

	glUniform3f(m_LightColorLoc,  1.0f, 1.0f, 1.0f);                          // 设置光源颜色为白色
	glUniform3f(m_LightPosLoc,    -2.0f, 0.0f, 2.0f);                         // 设置光源位置
	glUniform3f(m_ViewPosLoc,     -3.0f, 0.0f, 3.0f);                         // 设置观察者位置

	// ==================== 绑定纹理并绘制 ====================
	glActiveTexture(GL_TEXTURE0);                                             // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);                                // 绑定纹理
	glUniform1i(m_SamplerLoc, 0);                                             // 设置采样器使用纹理单元 0

	glDrawArrays(GL_TRIANGLES, 0, 36);                                        // 绘制立方体（36 个顶点，6 个面 * 2 个三角形 * 3 个顶点）
	GO_CHECK_GL_ERROR();
}

/**
 * @brief 销毁 OpenGL 资源
 */
void BasicLightingSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);       // 删除着色器程序
		glDeleteBuffers(1, m_VboIds);        // 删除 VBO
		glDeleteVertexArrays(1, &m_VaoId);   // 删除 VAO
		glDeleteTextures(1, &m_TextureId);   // 删除纹理
	}
}


/**
 * @brief 更新 MVP 矩阵
 * @param mvpMatrix MVP 矩阵引用
 * @param angleX 绕X轴旋转度数
 * @param angleY 绕Y轴旋转度数
 * @param ratio 宽高比
 * */
void BasicLightingSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("BasicLightingSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX,
			angleY, ratio);
	angleX = angleX % 360;                                                    // 限制角度在 0-360 度范围内
	angleY = angleY % 360;

	// 转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);

	// ==================== 投影矩阵 ====================
	// 使用透视投影，FOV=45度，近裁剪面=0.1，远裁剪面=100
	glm::mat4 Projection = glm::perspective(45.0f, ratio, 0.1f, 100.f);

	// ==================== 视图矩阵 ====================
	// 相机位置在 (-3, 0, 3)，观察原点，Y 轴向上
	glm::mat4 View = glm::lookAt(
			glm::vec3(-3, 0, 3),                                              // 相机位置（世界坐标）
			glm::vec3(0, 0, 0),                                               // 观察目标（原点）
			glm::vec3(0, 1, 0)                                                // 上方向向量（Y 轴向上）
	);

	// ==================== 模型矩阵 ====================
	glm::mat4 Model = glm::mat4(1.0f);                                        // 初始化为单位矩阵
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleX, m_ScaleX));      // 缩放
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));       // 绕 X 轴旋转
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));       // 绕 Y 轴旋转
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));              // 平移

	m_ModelMatrix = Model;                                                    // 保存模型矩阵（用于光照计算）

	mvpMatrix = Projection * View * Model;                                    // 计算 MVP 矩阵

}

/**
 * @brief 更新变换矩阵参数
 * @param rotateX 绕 X 轴旋转角度
 * @param rotateY 绕 Y 轴旋转角度
 * @param scaleX X 轴缩放比例
 * @param scaleY Y 轴缩放比例
 */
void BasicLightingSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);                                     // 更新旋转角度
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;                                                        // 更新缩放比例
	m_ScaleY = scaleY;
}
