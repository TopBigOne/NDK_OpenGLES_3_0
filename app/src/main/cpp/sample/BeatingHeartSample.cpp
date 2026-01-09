/**
 * 心跳特效示例 - 通过Shader数学函数绘制跳动的心形
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * 特效原理：
 * 1. 使用参数方程绘制心形曲线
 * 2. 通过时间函数控制心形缩放，实现跳动动画
 * 3. 使用距离场技术进行着色和边缘平滑处理
 * */

#include <gtc/matrix_transform.hpp>
#include "BeatingHeartSample.h"
#include "../util/GLUtils.h"

BeatingHeartSample::BeatingHeartSample()
{
	// 初始化纹理采样器位置
	m_SamplerLoc = GL_NONE;
	// 初始化MVP矩阵位置
	m_MVPMatLoc = GL_NONE;

	// 初始化纹理ID
	m_TextureId = GL_NONE;
	// 初始化顶点数组对象ID
	m_VaoId = GL_NONE;

	// 初始化X轴旋转角度
	m_AngleX = 0;
	// 初始化Y轴旋转角度
	m_AngleY = 0;

	// 初始化X轴缩放比例
	m_ScaleX = 1.0f;
	// 初始化Y轴缩放比例
	m_ScaleY = 1.0f;
}

BeatingHeartSample::~BeatingHeartSample()
{
	// 释放图像资源
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

void BeatingHeartSample::Init()
{
	// 如果着色器程序已创建，直接返回
	if(m_ProgramObj)
		return;

	// 创建RGBA纹理对象（本示例中未使用纹理，保留用于扩展）
	glGenTextures(1, &m_TextureId);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 顶点着色器：简单的MVP变换
	char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;\n"
            "uniform mat4 u_MVPMatrix;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = u_MVPMatrix * a_position;\n"
            "}";

	// 片段着色器：心跳动画核心算法
	char fShaderStr[] =
			"#version 300 es\n"
			"precision highp float;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform float u_time;\n"           // 时间参数，用于动画
			"uniform vec2 u_screenSize;\n"      // 屏幕尺寸
			"void main()\n"
			"{\n"
			"    vec2 fragCoord = gl_FragCoord.xy;\n"
			"    // 将片段坐标归一化到[-1,1]范围，保持宽高比\n"
			"    vec2 p = (2.0*fragCoord-u_screenSize.xy)/min(u_screenSize.y,u_screenSize.x);\n"
			"\n"
			"    // ========== 背景颜色 ==========\n"
            "    vec2 uv = fragCoord / u_screenSize;\n"
            "    // 粉色渐变背景，从中心向外变暗\n"
            "    // length(p) 计算距离中心的距离，乘以0.38控制渐变强度\n"
            "    vec3 bcol = vec3(1.0,0.8,0.8)*(1.0-0.38*length(p));\n"
            "\n"
			"    // ========== 心跳动画 ==========\n"
			"    float tt = u_time;\n"
			"    // 缓动函数：pow(tt, 0.2) 产生缓慢增长的值\n"
			"    float ss = pow(tt,.2)*0.5 + 0.5;\n"
			"    // 心跳效果：正弦波模拟跳动，exp(-tt*4.0) 控制衰减\n"
			"    // sin(tt*6.2831*3.0 + p.y*0.5) - 频率为3Hz的心跳\n"
			"    // p.y*0.5 添加垂直方向的相位差，产生波浪效果\n"
			"    ss = 1.0 + ss*0.5*sin(tt*6.2831*3.0 + p.y*0.5)*exp(-tt*4.0);\n"
			"    // 根据动画参数变形坐标，实现心形的缩放和拉伸\n"
			"    // vec2(0.5,1.5) 控制基础形状，ss 控制动画变化量\n"
			"    p *= vec2(0.5,1.5) + ss*vec2(0.5,-0.5);\n"
			"\n"
			"    // ========== 心形绘制 ==========\n"
			"    #if 0\n"
			"    // 方法1：简化心形（未启用）\n"
			"    p *= 0.8;\n"
			"    p.y = -0.1 - p.y*1.2 + abs(p.x)*(1.0-abs(p.x));\n"
			"    float r = length(p);\n"
			"    float d = 0.5;\n"
			"    #else\n"
			"    // 方法2：精确心形参数方程（启用）\n"
			"    p.y -= 0.25;  // 向下偏移心形中心\n"
			"    // atan(p.x, p.y) 计算极坐标角度，除以π归一化到[-1,1]\n"
			"    float a = atan(p.x,p.y)/3.141592653;\n"
			"    float r = length(p);  // 到中心的距离\n"
			"    float h = abs(a);     // 角度的绝对值\n"
			"    // 心形的参数方程：使用三次多项式定义边界\n"
			"    // d(h) = (13h - 22h² + 10h³) / (6 - 5h)\n"
			"    // 这个方程精确描述了心形曲线\n"
			"    float d = (13.0*h - 22.0*h*h + 10.0*h*h*h)/(6.0-5.0*h);\n"
			"    #endif\n"
			"\n"
			"    // ========== 心形着色 ==========\n"
			"    // 基于x坐标的亮度梯度（左暗右亮）\n"
			"    float s = 0.75 + 0.75*p.x;\n"
			"    // 根据距离中心的远近调整亮度（中心亮，边缘暗）\n"
			"    s *= 1.0-0.4*r;\n"
			"    // 调整亮度范围到[0.3, 1.0]\n"
			"    s = 0.3 + 0.7*s;\n"
			"    // 边缘高光效果：clamp(r/d, 0.0, 1.0) 计算归一化距离\n"
			"    // pow(..., 0.1) 产生边缘处的强烈高光\n"
			"    s *= 0.5+0.5*pow( 1.0-clamp(r/d, 0.0, 1.0 ), 0.1 );\n"
			"    // 心形颜色：红色到橙色渐变，饱和度随距离变化\n"
			"    vec3 hcol = vec3(1.0,0.5*r,0.3)*s;\n"
			"\n"
			"    // ========== 最终混合 ==========\n"
			"    // smoothstep(-0.06, 0.06, d-r) 实现抗锯齿边缘\n"
			"    // d-r < 0 表示在心形内部，> 0 表示在外部\n"
			"    // 在[-0.06, 0.06]范围内平滑过渡，消除锯齿\n"
			"    vec3 col = mix( bcol, hcol, smoothstep( -0.06, 0.06, d-r) );\n"
			"\n"
			"    outColor = vec4(col,1.0);\n"
			"}";

	// 创建着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		// 获取uniform变量位置
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
		m_SizeLoc = glGetUniformLocation(m_ProgramObj, "u_screenSize");
		m_TimeLoc = glGetUniformLocation(m_ProgramObj, "u_time");
	}
	else
	{
		LOGCATE("BeatingHeartSample::Init create program fail");
	}

	// 定义全屏四边形顶点坐标（NDC坐标系）
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // 左上角
			-1.0f, -1.0f, 0.0f,  // 左下角
			1.0f,  -1.0f, 0.0f,  // 右下角
			1.0f,   1.0f, 0.0f,  // 右上角
	};

	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // TexCoord 0
			0.0f,  1.0f,        // TexCoord 1
			1.0f,  1.0f,        // TexCoord 2
			1.0f,  0.0f         // TexCoord 3
	};

	// 索引数组，定义两个三角形组成四边形
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// 生成并加载VBO数据
	glGenBuffers(3, m_VboIds);
	// VBO 0：顶点坐标
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

	// VBO 2：索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// 生成并配置VAO
	glGenVertexArrays(1, &m_VaoId);
	glBindVertexArray(m_VaoId);

	// 绑定顶点坐标属性
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 绑定索引缓冲
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

	glBindVertexArray(GL_NONE);
}

void BeatingHeartSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("BeatingHeartSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		// 复制图像数据（本示例未使用）
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

void BeatingHeartSample::Draw(int screenW, int screenH)
{
	LOGCATE("BeatingHeartSample::Draw()");

	if(m_ProgramObj == GL_NONE) return;

	// 更新MVP矩阵
	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);

	// 使用着色器程序
	glUseProgram (m_ProgramObj);
	glBindVertexArray(m_VaoId);

	// 传递MVP矩阵
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);

	// ========== 时间动画控制 ==========
	// 获取当前时间，模2000ms后归一化到[0,1]
	// fmod用于循环播放动画
	float time = static_cast<float>(fmod(GetSysCurrentTime(), 2000) / 2000);
    LOGCATE("BeatingHeartSample::Draw() time=%f",time);
	glUniform1f(m_TimeLoc, time);

	// 传递屏幕尺寸，用于归一化坐标
    glUniform2f(m_SizeLoc, screenW, screenH);

	// 绘制全屏四边形
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
}

void BeatingHeartSample::Destroy()
{
	// 清理OpenGL资源
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
		glDeleteBuffers(3, m_VboIds);
		glDeleteVertexArrays(1, &m_VaoId);
		glDeleteTextures(1, &m_TextureId);
	}
}

/**
 * 更新MVP矩阵
 * @param mvpMatrix 输出的MVP矩阵
 * @param angleX 绕X轴旋转角度
 * @param angleY 绕Y轴旋转角度
 * @param ratio 屏幕宽高比
 */
void BeatingHeartSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("BeatingHeartSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	// 角度归一化到[0, 360)
	angleX = angleX % 360;
	angleY = angleY % 360;

	// 角度转弧度：radians = degrees * π / 180
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);

	// ========== 投影矩阵 ==========
	// 正交投影：保持形状不变形，适合2D渲染
	glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

	// ========== 视图矩阵 ==========
	// 相机位置在(0,0,4)，看向原点(0,0,0)，上方向为(0,1,0)
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 4), // 相机位置
			glm::vec3(0, 0, 0), // 观察目标点
			glm::vec3(0, 1, 0)  // 上方向向量
	);

	// ========== 模型矩阵 ==========
	glm::mat4 Model = glm::mat4(1.0f);
	// 1. 缩放变换
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
	// 2. 绕X轴旋转
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. 绕Y轴旋转
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
	// 4. 平移变换（保持在原点）
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

	// MVP = Projection * View * Model（注意顺序）
	mvpMatrix = Projection * View * Model;
}

/**
 * 更新变换矩阵参数（触摸交互响应）
 * @param rotateX X轴旋转角度
 * @param rotateY Y轴旋转角度
 * @param scaleX X轴缩放系数
 * @param scaleY Y轴缩放系数
 */
void BeatingHeartSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}
