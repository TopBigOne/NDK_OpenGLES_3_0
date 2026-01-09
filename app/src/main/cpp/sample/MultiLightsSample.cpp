/**
 * MultiLightsSample - OpenGL ES 3.0 多光源渲染示例
 *
 * 功能说明:
 * 实现多光源照明效果，包括聚光灯（Spotlight）的模拟，展示复杂光照场景的渲染
 *
 * 技术要点:
 * 1. 聚光灯光照模型：点光源 + 方向限制 + 边缘衰减
 * 2. 光源参数：位置、方向、颜色、内外切光角
 * 3. 光照衰减计算：常数项、一次项、二次项衰减模型
 * 4. 多物体实例渲染：使用不同的变换矩阵绘制多个立方体
 * 5. 片段着色器中进行光照计算（逐像素光照）
 *
 * 应用场景:
 * - 游戏场景中的手电筒、路灯等聚光灯效果
 * - 多光源复杂光照场景
 * - 光照衰减和强度控制
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "MultiLightsSample.h"
#include "../util/GLUtils.h"

// 定义多个立方体的位置向量数组
glm::vec3 transPositions[] = {
		glm::vec3( 0.0f,  0.0f,  0.0f),
		glm::vec3( 2.0f,  2.0f, -1.0f),
		glm::vec3(-1.5f, -2.2f, -1.5f),
		glm::vec3(-1.8f, -2.0f,  1.3f),
		glm::vec3( 1.4f, -1.4f, -1.5f),
		glm::vec3(-1.7f,  2.0f, -1.5f),
		glm::vec3( 1.3f, -2.0f,  2.5f),
		glm::vec3( 0.5f,  1.3f, -0.1f),
		glm::vec3( 1.5f,  2.2f,  1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f),
		glm::vec3(-1.3f,  0.0f, -1.5f),
		glm::vec3( 0.0f, -1.3f, -0.5f),
		glm::vec3( 0.0f, -1.5f,  1.5f),
};

/**
 * @brief 构造函数 - 初始化所有成员变量
 */
MultiLightsSample::MultiLightsSample()
{
	// 初始化 Uniform 变量位置
	m_SamplerLoc = GL_NONE;
	m_MVPMatLoc = GL_NONE;
	m_ModelMatrixLoc = GL_NONE;

	// 初始化 OpenGL 对象 ID
	m_TextureId = GL_NONE;
	m_VaoId = GL_NONE;

	// 初始化旋转角度
	m_AngleX = 0;
	m_AngleY = 0;

	// 初始化模型矩阵
	m_ModelMatrix = glm::mat4(0.0f);
}

/**
 * @brief 析构函数 - 释放图像资源
 */
MultiLightsSample::~MultiLightsSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

/**
 * @brief 初始化 OpenGL 资源
 * @details 创建着色器程序、纹理对象、VAO 和 VBO，配置聚光灯相关的着色器
 */
void MultiLightsSample::Init()
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
	// 将顶点数据和法线变换到世界空间，传递给片段着色器
	char vShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"layout(location = 0) in vec4 a_position;\n"    // 顶点位置
			"layout(location = 1) in vec2 a_texCoord;\n"    // 纹理坐标
			"layout(location = 2) in vec3 a_normal;\n"      // 顶点法线
			"\n"
			"out vec3 normal;\n"                            // 输出法线（世界空间）
			"out vec3 fragPos;\n"                           // 输出片段位置（世界空间）
			"out vec2 v_texCoord;\n"                        // 输出纹理坐标
			"\n"
			"uniform mat4 u_MVPMatrix;\n"                   // MVP 矩阵
			"uniform mat4 u_ModelMatrix;\n"                 // 模型矩阵
			"\n"
			"void main()\n"
			"{\n"
			"    gl_Position = u_MVPMatrix * a_position;\n"                          // 计算裁剪空间坐标
			"    fragPos = vec3(u_ModelMatrix * a_position);\n"                      // 计算片段的世界坐标
			"    normal = mat3(transpose(inverse(u_ModelMatrix))) * a_normal;\n"    // 将法线变换到世界空间（使用法线矩阵）
			"    v_texCoord = a_texCoord;\n"                                         // 传递纹理坐标
			"}";

	// ==================== 片段着色器 ====================
	// 在片段着色器中实现聚光灯光照计算
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			// 定义光源结构体
			"struct Light {\n"
			"    vec3 position;\n"        // 光源位置
			"    vec3 direction;\n"       // 光照方向（聚光灯朝向）
			"    vec3 color;\n"           // 光源颜色
			"    float cutOff;\n"         // 内切光角的余弦值（聚光灯核心区域）
			"    float outerCutOff;\n"    // 外切光角的余弦值（聚光灯边缘区域）
			"\n"
			"    float constant;\n"       // 衰减常数项
			"    float linear;\n"         // 衰减一次项系数
			"    float quadratic;\n"      // 衰减二次项系数
			"};\n"
			"\n"
			"in vec3 normal;\n"           // 输入法线
			"in vec3 fragPos;\n"          // 输入片段位置
			"in vec2 v_texCoord;\n"       // 输入纹理坐标
			"\n"
			"layout(location = 0) out vec4 outColor;\n"   // 输出片段颜色
			"uniform sampler2D s_TextureMap;\n"           // 纹理采样器
			"\n"
			"uniform vec3 viewPos;\n"     // 观察者位置
			"uniform Light light;\n"      // 光源参数
			"\n"
			"void main()\n"
			"{\n"
			"    vec4 objectColor = texture(s_TextureMap, v_texCoord);\n"  // 采样物体颜色
			"\n"
			"    vec3 lightDir = normalize(light.position - fragPos);\n"   // 计算从片段指向光源的方向
			"\n"
			"    // 检查光照是否在聚光灯照射范围内\n"
			"    float theta = dot(lightDir, normalize(-light.direction));\n"  // 计算光线方向与聚光灯方向的夹角余弦值
			"\n"
			"    // 计算边缘过渡强度（平滑过渡）\n"
			"    float epsilon = light.cutOff - light.outerCutOff;\n"                          // 内外圆锥角度差
			"    float intensity = clamp((theta - light.outerCutOff) / epsilon,0.0, 1.0);\n"  // 计算强度衰减（0-1之间平滑过渡）
			"\n"
			"    // Ambient - 环境光\n"
			"    float ambientStrength = 0.25;\n"                          // 环境光强度
			"    vec3 ambient = ambientStrength * light.color;\n"
			"\n"
			"    // Diffuse - 漫反射\n"
			"    vec3 norm = normalize(normal);\n"                         // 归一化法线
			"    float diff = max(dot(norm, lightDir), 0.0);\n"            // 计算漫反射系数
			"    vec3 diffuse = diff * light.color;\n"
			"\n"
			"    // Specular - 镜面反射\n"
			"    vec3 viewDir = normalize(viewPos - fragPos);\n"           // 计算观察方向
			"    vec3 reflectDir = reflect(-lightDir, norm);\n"            // 计算反射方向
			"    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);\n"  // 计算镜面反射系数，高光指数32
			"    vec3 specular = spec * light.color;\n"
			"\n"
			"    // Attenuation - 距离衰减\n"
			"    float distance    = length(light.position - fragPos);\n"  // 计算到光源的距离
			"    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
			"\n"
			"    // 环境光不进行衰减（避免聚光灯范围外过暗）\n"
			"    // ambient  *= attenuation;\n"
			"    diffuse  *= attenuation;\n"                               // 漫反射应用衰减
			"    specular *= attenuation;\n"                               // 镜面反射应用衰减
			"\n"
			"    // 应用聚光灯强度衰减\n"
			"    diffuse *= intensity;\n"
			"    specular*= intensity;\n"
			"\n"
			"    vec3 finalColor = (ambient + diffuse + specular) * vec3(objectColor);\n"  // 合成最终颜色
			"    outColor = vec4(finalColor, 1.0f);\n"
			"}";

	// ==================== 创建着色器程序 ====================
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		// 获取 Uniform 变量位置
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
		GO_CHECK_GL_ERROR();
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
		GO_CHECK_GL_ERROR();
		m_ModelMatrixLoc = glGetUniformLocation(m_ProgramObj, "u_ModelMatrix");
		GO_CHECK_GL_ERROR();
		m_ViewPosLoc = glGetUniformLocation(m_ProgramObj, "viewPos");
		GO_CHECK_GL_ERROR();
	}
	else
	{
		LOGCATE("MultiLightsSample::Init create program fail");
		return;
	}

	// ==================== 定义立方体顶点数据 ====================
	// 每个顶点包含：位置(3) + 纹理坐标(2) + 法线(3) = 8 个浮点数
	GLfloat vertices[] = {
			 //position            //texture coord  //normal
			-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,      0.0f,  0.0f, -1.0f,
			 0.5f, -0.5f, -0.5f,   1.0f, 0.0f,      0.0f,  0.0f, -1.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,      0.0f,  0.0f, -1.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,      0.0f,  0.0f, -1.0f,
			-0.5f,  0.5f, -0.5f,   0.0f, 1.0f,      0.0f,  0.0f, -1.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,      0.0f,  0.0f, -1.0f,

			-0.5f, -0.5f, 0.5f,    0.0f, 0.0f,      0.0f,  0.0f,  1.0f,
			 0.5f, -0.5f, 0.5f,    1.0f, 0.0f,      0.0f,  0.0f,  1.0f,
			 0.5f,  0.5f, 0.5f,    1.0f, 1.0f,      0.0f,  0.0f,  1.0f,
			 0.5f,  0.5f, 0.5f,    1.0f, 1.0f,      0.0f,  0.0f,  1.0f,
			-0.5f,  0.5f, 0.5f,    0.0f, 1.0f,      0.0f,  0.0f,  1.0f,
			-0.5f, -0.5f, 0.5f,    0.0f, 0.0f,      0.0f,  0.0f,  1.0f,

			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f, -0.5f,   1.0f, 1.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,     -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f,     -1.0f,  0.0f,  0.0f,

			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f,      1.0f,  0.0f,  0.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,      1.0f,  0.0f,  0.0f,
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      1.0f,  0.0f,  0.0f,
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      1.0f,  0.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,   0.0f, 0.0f,      1.0f,  0.0f,  0.0f,
			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f,      1.0f,  0.0f,  0.0f,

			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      0.0f, -1.0f,  0.0f,
			 0.5f, -0.5f, -0.5f,   1.0f, 1.0f,      0.0f, -1.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f,      0.0f, -1.0f,  0.0f,
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f,      0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,      0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,      0.0f, -1.0f,  0.0f,

			-0.5f, 0.5f, -0.5f,    0.0f, 1.0f,      0.0f,  1.0f,  0.0f,
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

	// ==================== 上传纹理数据 ====================
	glActiveTexture(GL_TEXTURE0);                                             // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);                                // 绑定纹理对象
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA,
				 GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);                // 上传 RGBA 图像数据
	glBindTexture(GL_TEXTURE_2D, GL_NONE);                                    // 解绑纹理

}

/**
 * @brief 加载图像数据
 * @param pImage 图像数据指针
 */
void MultiLightsSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("MultiLightsSample::LoadImage pImage = %p", pImage->ppPlane[0]);
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
void MultiLightsSample::Draw(int screenW, int screenH)
{
	LOGCATE("MultiLightsSample::Draw()");

	// 确保着色器程序和纹理已创建
	if (m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	// ==================== 启用深度测试 ====================
	glEnable(GL_DEPTH_TEST);                                                  // 启用深度测试，确保立方体前后关系正确

	float ratio = (float)screenW / screenH;

	// ==================== 使用着色器程序 ====================
	glUseProgram(m_ProgramObj);                                               // 激活着色器程序

	glBindVertexArray(m_VaoId);                                               // 绑定 VAO

	glUniform3f(m_ViewPosLoc,     0.0f, 0.0f, 3.0f);                          // 设置观察者位置

	// ==================== 绑定纹理 ====================
	glActiveTexture(GL_TEXTURE0);                                             // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, m_TextureId);                                // 绑定纹理
	glUniform1i(m_SamplerLoc, 0);                                             // 设置采样器使用纹理单元 0

	// ==================== 设置聚光灯参数 ====================
	glUniform3f(glGetUniformLocation(m_ProgramObj, "light.position"), 0.0f, 0.0f, 3.0f);     // 光源位置（与相机位置相同，模拟手电筒）
	glUniform3f(glGetUniformLocation(m_ProgramObj, "light.color"), 1.0f, 1.0f, 1.0f);        // 光源颜色为白色
	glUniform3f(glGetUniformLocation(m_ProgramObj, "light.direction"), 0.0f, 0.0f, -1.0f);   // 光照方向（沿 -Z 轴）

	// 设置聚光灯内外切光角（用余弦值表示）
	glUniform1f(glGetUniformLocation(m_ProgramObj, "light.cutOff"), glm::cos(glm::radians(10.5f)));        // 内切光角 10.5 度
	glUniform1f(glGetUniformLocation(m_ProgramObj, "light.outerCutOff"), glm::cos(glm::radians(11.5f)));   // 外切光角 11.5 度

	// 设置衰减系数（模拟距离衰减效果）
	glUniform1f(glGetUniformLocation(m_ProgramObj, "light.constant"),  1.0f);    // 常数项（保证分母不为0）
	glUniform1f(glGetUniformLocation(m_ProgramObj, "light.linear"),    0.09);    // 一次项（线性衰减）
	glUniform1f(glGetUniformLocation(m_ProgramObj, "light.quadratic"), 0.032);   // 二次项（二次衰减）

	// ==================== 绘制多个立方体 ====================
	// 使用不同的位置和旋转角度绘制多个立方体
	for(int i = 0; i < sizeof(transPositions)/ sizeof(transPositions[0]); i++)
	{
		// 更新变换矩阵（每个立方体使用不同的位置）
		UpdateMatrix(m_MVPMatrix, m_ModelMatrix, m_AngleX + 10, m_AngleY + 10, 0.4, transPositions[i], ratio);
		glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);        // 设置 MVP 矩阵
		glUniformMatrix4fv(m_ModelMatrixLoc, 1, GL_FALSE, &m_ModelMatrix[0][0]); // 设置模型矩阵
		glDrawArrays(GL_TRIANGLES, 0, 36);                                        // 绘制立方体
	}

}

/**
 * @brief 销毁 OpenGL 资源
 */
void MultiLightsSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);       // 删除着色器程序
		glDeleteBuffers(1, m_VboIds);        // 删除 VBO
		glDeleteVertexArrays(1, &m_VaoId);   // 删除 VAO
		glDeleteTextures(1, &m_TextureId);   // 删除纹理
		m_ProgramObj = GL_NONE;
		m_VaoId = GL_NONE;
		m_TextureId = GL_NONE;
	}

}

/**
 * @brief 更新 MVP 矩阵（本示例中未实现，使用 UpdateMatrix 代替）
 */
void MultiLightsSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	// No implement
}

/**
 * @brief 更新变换矩阵（包括 MVP 和 Model 矩阵）
 * @param mvpMatrix MVP 矩阵引用
 * @param modelMatrix 模型矩阵引用
 * @param angleXRotate 绕 X 轴旋转角度
 * @param angleYRotate 绕 Y 轴旋转角度
 * @param scale 缩放比例
 * @param transVec3 平移向量
 * @param ratio 宽高比
 */
void MultiLightsSample::UpdateMatrix(glm::mat4 &mvpMatrix, glm::mat4 &modelMatrix, int angleXRotate, int angleYRotate, float scale, glm::vec3 transVec3, float ratio)
{
	LOGCATE("MultiLightsSample::UpdateMatrix angleX = %d, angleY = %d, ratio = %f", angleXRotate,
			angleYRotate, ratio);
	angleXRotate = angleXRotate % 360;                                        // 限制角度在 0-360 度范围内
	angleYRotate = angleYRotate % 360;

	// 转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleXRotate);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleYRotate);

	// ==================== 投影矩阵 ====================
	// 使用透视投影，FOV=45度，近裁剪面=0.1，远裁剪面=100
	glm::mat4 Projection = glm::perspective(45.0f, ratio, 0.1f, 100.f);

	// ==================== 视图矩阵 ====================
	// 相机位置在 (0, 0, 3)，观察原点，Y 轴向上
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 3),                                               // 相机位置（世界坐标）
			glm::vec3(0, 0, 0),                                               // 观察目标（原点）
			glm::vec3(0, 1, 0)                                                // 上方向向量（Y 轴向上）
	);

	// ==================== 模型矩阵 ====================
	glm::mat4 Model = glm::mat4(1.0f);                                        // 初始化为单位矩阵
	Model = glm::scale(Model, glm::vec3(scale, scale, scale));               // 缩放
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));       // 绕 X 轴旋转
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));       // 绕 Y 轴旋转
	Model = glm::translate(Model, transVec3);                                 // 平移到指定位置

	modelMatrix = Model;                                                      // 保存模型矩阵（用于光照计算）

	mvpMatrix = Projection * View * Model;                                    // 计算 MVP 矩阵
}

/**
 * @brief 更新变换矩阵参数
 * @param rotateX 绕 X 轴旋转角度
 * @param rotateY 绕 Y 轴旋转角度
 * @param scaleX X 轴缩放比例（未使用）
 * @param scaleY Y 轴缩放比例（未使用）
 */
void MultiLightsSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);                                     // 更新旋转角度
	m_AngleY = static_cast<int>(rotateY);
}
