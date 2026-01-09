/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * 3D 模型加载示例
 *
 * 功能说明：
 * - 使用 Assimp 库加载 3D 模型文件 (.obj 格式)
 * - 支持模型纹理贴图
 * - 实现 Phong 光照模型 (环境光 + 漫反射 + 镜面反射)
 * - 支持模型旋转和缩放变换
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "Model3DSample.h"
#include "../util/GLUtils.h"

/**
 * 构造函数
 * 初始化模型的旋转角度、缩放参数和指针
 */
Model3DSample::Model3DSample()
{
	m_AngleX = 0;  // X 轴旋转角度
	m_AngleY = 0;  // Y 轴旋转角度

	m_ScaleX = 1.0f;  // X 轴缩放比例
	m_ScaleY = 1.0f;  // Y 轴缩放比例

	m_pModel = nullptr;   // 3D 模型对象指针
	m_pShader = nullptr;  // 着色器程序对象指针
}

/**
 * 析构函数
 */
Model3DSample::~Model3DSample()
{

}

/**
 * 初始化函数
 * 功能：创建着色器程序并使用 Assimp 加载 3D 模型
 */
void Model3DSample::Init()
{
	// 防止重复初始化
	if(m_pModel != nullptr && m_pShader != nullptr)
		return;

	// ========== 顶点着色器 ==========
	// 实现 Phong 光照模型的顶点着色器
	char vShaderStr[] =
			"#version 300 es\n"
            "precision mediump float;\n"
			"layout (location = 0) in vec3 a_position;\n"    // 顶点位置
			"layout (location = 1) in vec3 a_normal;\n"      // 顶点法线
			"layout (location = 2) in vec2 a_texCoord;\n"    // 纹理坐标
			"out vec2 v_texCoord;\n"                          // 传递到片段着色器的纹理坐标
			"uniform mat4 u_MVPMatrix;\n"                     // MVP 矩阵（模型-视图-投影）
            "uniform mat4 u_ModelMatrix;\n"                   // 模型矩阵
            "uniform vec3 lightPos;\n"                        // 光源位置
            "uniform vec3 lightColor;\n"                      // 光源颜色
            "uniform vec3 viewPos;\n"                         // 观察者（相机）位置
            "out vec3 ambient;\n"                             // 环境光分量
            "out vec3 diffuse;\n"                             // 漫反射光分量
            "out vec3 specular;\n"                            // 镜面反射光分量
			"void main()\n"
			"{\n"
			"    v_texCoord = a_texCoord;    \n"
            "    vec4 position = vec4(a_position, 1.0);\n"
            "    gl_Position = u_MVPMatrix * position;\n"     // 变换到裁剪空间
            "    vec3 fragPos = vec3(u_ModelMatrix * position);\n"  // 世界空间中的片段位置
            "\n"
            "    // 环境光分量 (Ambient)\n"
            "    // 模拟场景中的间接光照，为所有片段提供基础亮度\n"
            "    float ambientStrength = 0.25;\n"             // 环境光强度
            "    ambient = ambientStrength * lightColor;\n"
            "\n"
            "    // 漫反射光分量 (Diffuse)\n"
            "    // 根据光线入射角度计算漫反射效果\n"
            "    float diffuseStrength = 0.5;\n"              // 漫反射强度
            "    vec3 unitNormal = normalize(vec3(u_ModelMatrix * vec4(a_normal, 1.0)));\n"  // 世界空间中的法线
            "    vec3 lightDir = normalize(lightPos - fragPos);\n"  // 光源方向向量
            "    float diff = max(dot(unitNormal, lightDir), 0.0);\n"  // 漫反射因子（兰伯特余弦定律）
            "    diffuse = diffuseStrength * diff * lightColor;\n"
            "\n"
            "    // 镜面反射光分量 (Specular)\n"
            "    // 模拟光滑表面的高光效果\n"
            "    float specularStrength = 0.3;\n"             // 镜面反射强度
            "    vec3 viewDir = normalize(viewPos - fragPos);\n"  // 视线方向向量
            "    vec3 reflectDir = reflect(-lightDir, unitNormal);\n"  // 光线反射方向
            "    float spec = pow(max(dot(unitNormal, reflectDir), 0.0), 16.0);\n"  // 镜面反射因子（16.0 是光泽度）
            "    specular = specularStrength * spec * lightColor;\n"
			"}";

	// ========== 片段着色器（带纹理） ==========
	char fShaderStr[] =
			"#version 300 es\n"
            "precision mediump float;"
			"out vec4 outColor;\n"
			"in vec2 v_texCoord;\n"                       // 纹理坐标
            "in vec3 ambient;\n"                          // 环境光
            "in vec3 diffuse;\n"                          // 漫反射光
            "in vec3 specular;\n"                         // 镜面反射光
			"uniform sampler2D texture_diffuse1;\n"       // 漫反射纹理采样器
			"void main()\n"
			"{    \n"
            "    vec4 objectColor = texture(texture_diffuse1, v_texCoord);\n"  // 采样纹理颜色
            "    vec3 finalColor = (ambient + diffuse + specular) * vec3(objectColor);\n"  // Phong 光照模型
            "    outColor = vec4(finalColor, 1.0);\n"
			"}";

    // ========== 片段着色器（无纹理） ==========
    // 当模型不包含纹理时使用纯色渲染
    char fNoTextureShaderStr[] =
            "#version 300 es\n"
            "precision highp float;\n"
            "out vec4 outColor;\n"
            "in vec3 ambient;\n"
            "in vec3 diffuse;\n"
            "in vec3 specular;\n"
            "void main()\n"
            "{    \n"
            "    vec4 objectColor = vec4(0.6, 0.6, 0.6, 1.0);\n"  // 灰色物体
            "    vec3 finalColor = (ambient + diffuse + specular) * vec3(objectColor);\n"
            "    outColor = vec4(finalColor, 1.0);\n"
            "}";

    // ========== 使用 Assimp 加载 3D 模型 ==========
    // TODO: 先把 model 文件夹拷贝到 /sdcard/Android/data/com.byteflow.app/files/Download 路径下
    // 然后可以选择你要加载的模型
	//m_pModel = new Model(path + "/model/nanosuit/nanosuit.obj");  // 纳米服模型
	std::string path(DEFAULT_OGL_ASSETS_DIR);
    m_pModel = new Model(path + "/model/poly/Apricot_02_hi_poly.obj");  // 当前使用的模型
    //m_pModel = new Model(path + "/model/tank/Abrams_BF3.obj");        // 坦克模型
    //m_pModel = new Model(path + "/model/girl/091_W_Aya_10K.obj");     // 人物模型
    //m_pModel = new Model(path + "/model/new/camaro.obj");             // 汽车模型
    //m_pModel = new Model(path + "/model/bird/12214_Bird_v1max_l3.obj");  // 鸟类模型

    // ========== 根据模型是否包含纹理选择对应的着色器 ==========
    if (m_pModel->ContainsTextures())
    {
        m_pShader = new Shader(vShaderStr, fShaderStr);  // 使用带纹理的着色器
    }
    else
    {
        m_pShader = new Shader(vShaderStr, fNoTextureShaderStr);  // 使用无纹理的着色器
    }
}

/**
 * 加载图像（本示例中未使用）
 * @param pImage 图像数据指针
 */
void Model3DSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("Model3DSample::LoadImage pImage = %p", pImage->ppPlane[0]);

}

/**
 * 绘制函数
 * 功能：渲染 3D 模型并应用光照效果
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void Model3DSample::Draw(int screenW, int screenH)
{
	if(m_pModel == nullptr || m_pShader == nullptr) return;
    LOGCATE("Model3DSample::Draw()");

    // 设置背景色为灰色并清除颜色和深度缓冲区
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);  // 启用深度测试，确保 3D 对象正确遮挡

	// 更新 MVP 矩阵（根据旋转角度和屏幕宽高比）
	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);

	// 激活着色器程序
    m_pShader->use();

    // 设置 uniform 变量
    m_pShader->setMat4("u_MVPMatrix", m_MVPMatrix);        // MVP 矩阵
    m_pShader->setMat4("u_ModelMatrix", m_ModelMatrix);    // 模型矩阵
    m_pShader->setVec3("lightPos", glm::vec3(0, 0, m_pModel->GetMaxViewDistance()));  // 光源位置
    m_pShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));  // 白色光源
    m_pShader->setVec3("viewPos", glm::vec3(0, 0, m_pModel->GetMaxViewDistance()));   // 相机位置

    // 绘制模型（Model 类内部会遍历所有网格并绘制）
    m_pModel->Draw((*m_pShader));
}

/**
 * 销毁资源
 * 释放模型和着色器占用的内存
 */
void Model3DSample::Destroy()
{
    LOGCATE("Model3DSample::Destroy");
    if (m_pModel != nullptr) {
        m_pModel->Destroy();
        delete m_pModel;
        m_pModel = nullptr;
    }

    if (m_pShader != nullptr) {
        m_pShader->Destroy();
        delete m_pShader;
        m_pShader = nullptr;
    }
}

/**
 * 更新 MVP 矩阵
 * 功能：根据旋转角度、缩放比例和屏幕宽高比计算模型-视图-投影矩阵
 *
 * @param mvpMatrix 输出的 MVP 矩阵
 * @param angleX X 轴旋转角度（度）
 * @param angleY Y 轴旋转角度（度）
 * @param ratio 屏幕宽高比
 */
void Model3DSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("Model3DSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	angleX = angleX % 360;  // 角度归一化到 [0, 360) 范围
	angleY = angleY % 360;

	// 转化为弧度角（OpenGL 使用弧度制）
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


	// ========== 投影矩阵 (Projection Matrix) ==========
	// 使用透视投影，模拟真实的 3D 视觉效果
	//glm::mat4 Projection = glm::ortho(-ratio, ratio, -1.0f, 1.0f, 0.1f, 100.0f);  // 正交投影（可选）
	glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 1.0f, m_pModel->GetMaxViewDistance() * 4);
	//glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);  // 另一种透视投影方式

	// ========== 视图矩阵 (View Matrix) ==========
	// 定义相机位置和朝向
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, m_pModel->GetMaxViewDistance() * 1.8f), // 相机位置（根据模型大小动态调整）
			glm::vec3(0, 0, 0), // 观察目标（原点）
			glm::vec3(0, 1, 0)  // 上方向（Y 轴正方向）
	);

	// ========== 模型矩阵 (Model Matrix) ==========
	// 对模型进行缩放、旋转和平移变换
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));  // 缩放
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));  // 绕 X 轴旋转
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));  // 绕 Y 轴旋转
	Model = glm::translate(Model, -m_pModel->GetAdjustModelPosVec());   // 平移模型到中心
    m_ModelMatrix = Model;

	// ========== 计算 MVP 矩阵 ==========
	// MVP = Projection × View × Model（顺序不能颠倒）
	mvpMatrix = Projection * View * Model;

}

/**
 * 更新变换矩阵参数
 * 从触摸事件或其他输入更新旋转和缩放参数
 *
 * @param rotateX X 轴旋转角度
 * @param rotateY Y 轴旋转角度
 * @param scaleX X 轴缩放比例
 * @param scaleY Y 轴缩放比例
 */
void Model3DSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}
