/**
 * 大眼特效示例 - 基于局部网格变形的放大效果
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * 特效原理：
 * 1. 定义左右眼的中心点和影响半径
 * 2. 使用平滑的距离衰减函数对眼部区域进行放大
 * 3. 通过纹理坐标偏移实现像素重映射
 * 4. 使用gamma函数控制放大强度的径向分布
 * */

#include <gtc/matrix_transform.hpp>
#include "BigEyesSample.h"
#include "../util/GLUtils.h"

// ========== 眼部特征点配置 ==========
// 左眼中心点坐标（图像坐标系）
float LeftEyePoint[] = {283, 361};
// 右眼中心点坐标
float RightEyePoint[] = {395, 360};
// 眼部影响半径（像素）
float EyeRadius = 36;

BigEyesSample::BigEyesSample()
{

	m_SamplerLoc = GL_NONE;
	m_MVPMatLoc = GL_NONE;

	m_TextureId = GL_NONE;
	m_VaoId = GL_NONE;

	m_AngleX = 0;
	m_AngleY = 0;

	m_ScaleX = 1.0f;
	m_ScaleY = 1.0f;

	m_FrameIndex = 0;
}

BigEyesSample::~BigEyesSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);

}

void BigEyesSample::Init()
{
	if(m_ProgramObj)
		return;

	// 顶点着色器：传递纹理坐标
	char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;\n"
            "layout(location = 1) in vec2 a_texCoord;\n"
            "uniform mat4 u_MVPMatrix;\n"
            "out vec2 v_texCoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = u_MVPMatrix * a_position;\n"
            "    v_texCoord = a_texCoord;\n"
            "}";

	// 片段着色器：大眼变形算法
	char fShaderStr[] =
			"#version 300 es\n"
            "precision highp float;\n"
            "layout(location = 0) out vec4 outColor;\n"
            "uniform sampler2D s_TextureMap;\n"
            "in vec2 v_texCoord;\n"
			"uniform highp vec2 u_LeftEyeCenterPos;  // 左眼中心点（归一化坐标）\n"
			"uniform highp vec2 u_RightEyeCenterPos; // 右眼中心点（归一化坐标）\n"
            "uniform highp float u_ScaleRatio;       // 放大系数[0,1]，0无效果，1最大效果\n"
            "uniform highp float u_Radius;           // 影响半径（像素单位）\n"
            "uniform vec2 u_ImgSize;                 // 图片分辨率（用于坐标转换）\n"
            "\n"
            "// ========== 眼部变形函数 ==========\n"
            "// @param centerPos 眼睛中心点（像素坐标）\n"
            "// @param curPos 当前像素位置（归一化坐标）\n"
            "// @param radius 影响半径\n"
            "// @param scaleRatio 放大强度\n"
            "// @return 变形后的纹理坐标\n"
            "vec2 warpEyes(vec2 centerPos, vec2 curPos, float radius, float scaleRatio)\n"
            "{\n"
            "    vec2 result = curPos;\n"
            "    // 将当前位置转换为像素坐标\n"
            "    vec2 imgCurPos = curPos * u_ImgSize;\n"
            "    // 计算当前点到眼睛中心的距离\n"
            "    float d = distance(imgCurPos, centerPos);\n"
            "\n"
            "    // 只对半径内的区域进行变形\n"
            "    if(d < radius)\n"
            "    {\n"
            "        // ========== 放大算法核心 ==========\n"
            "        // 1. 计算归一化距离：d/radius ∈ [0,1]\n"
            "        // 2. smoothstep(0.0, 1.0, d/radius) 产生S型平滑曲线\n"
            "        // 3. (smoothstep - 1.0) ∈ [-1, 0]，平方后 ∈ [0, 1]\n"
            "        // 4. gamma = 1.0 - scaleRatio * pow(..., 2.0)\n"
            "        //    - 在中心处：smoothstep ≈ 0，pow ≈ 1，gamma最小（放大最强）\n"
            "        //    - 在边缘处：smoothstep ≈ 1，pow ≈ 0，gamma ≈ 1（无变形）\n"
            "        //    - 二次函数确保平滑过渡，避免突变\n"
            "        float gamma = 1.0 - scaleRatio * pow(smoothstep(0.0, 1.0, d / radius) - 1.0, 2.0);\n"
            "        \n"
            "        // 计算新的像素位置\n"
            "        // centerPos + gamma * (imgCurPos - centerPos)\n"
            "        // 相当于：从中心点向外做径向缩放\n"
            "        // gamma < 1时，点向中心收缩，导致采样的纹理区域变大（放大效果）\n"
			"        result = centerPos + gamma * (imgCurPos - centerPos);\n"
			"        // 转换回归一化坐标\n"
            "        result = result / u_ImgSize;\n"
            "    }\n"
            "    return result;\n"
            "}\n"
            "\n"
            "void main()\n"
            "{\n"
            "    // 先对左眼进行变形\n"
            "    vec2 newTexCoord = warpEyes(u_LeftEyeCenterPos, v_texCoord, u_Radius, u_ScaleRatio);\n"
            "    // 再对右眼进行变形（基于已变形的坐标）\n"
            "    newTexCoord = warpEyes(u_RightEyeCenterPos, newTexCoord, u_Radius, u_ScaleRatio);\n"
            "    // 使用变形后的坐标采样纹理\n"
            "    outColor = texture(s_TextureMap, newTexCoord);\n"
            "}\n"
            "\n"
            "";

	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
	}
	else
	{
		LOGCATE("BigEyesSample::Init create program fail");
	}

	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // Position 0
			-1.0f, -1.0f, 0.0f,  // Position 1
			1.0f,  -1.0f, 0.0f,  // Position 2
			1.0f,   1.0f, 0.0f,  // Position 3
	};

	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // TexCoord 0
			0.0f,  1.0f,        // TexCoord 1
			1.0f,  1.0f,        // TexCoord 2
			1.0f,  0.0f         // TexCoord 3
	};

	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// Generate VBO Ids and load the VBOs with data
	glGenBuffers(3, m_VboIds);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Generate VAO Id
	glGenVertexArrays(1, &m_VaoId);
	glBindVertexArray(m_VaoId);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

	glBindVertexArray(GL_NONE);

}

void BigEyesSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("BigEyesSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
	    ScopedSyncLock lock(&m_Lock);
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}

}

void BigEyesSample::Draw(int screenW, int screenH)
{
	LOGCATE("BigEyesSample::Draw() [w,h]=[%d,%d]", screenW, screenH);

	if(m_ProgramObj == GL_NONE) return;

	// 首次创建纹理
    if(m_TextureId == GL_NONE)
    {
        ScopedSyncLock lock(&m_Lock);
        if(m_RenderImage.ppPlane[0] != nullptr)
        {
            glGenTextures(1, &m_TextureId);
            glBindTexture(GL_TEXTURE_2D, m_TextureId);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
        }
        return;
    }

	glViewport(0, 0, screenW, screenH);

	// ========== 动画控制 ==========
	m_FrameIndex ++;
	// 计算动画进度：0 -> 1 -> 0 循环
    float offset = (m_FrameIndex % 100) * 1.0f / 100;
    // 每100帧反向一次，实现来回摆动
    offset = (m_FrameIndex / 100) % 2 == 1 ? (1 - offset) : offset;

	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);

	// 使用着色器程序
	glUseProgram (m_ProgramObj);
	glBindVertexArray(m_VaoId);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);

	// 绑定输入纹理
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glUniform1i(m_SamplerLoc, 0);

	// ========== 传递大眼特效参数 ==========
	// 放大系数：offset * 1.6，最大放大1.6倍
	GLUtils::setFloat(m_ProgramObj, "u_ScaleRatio", offset * 1.6f);
	// 影响半径
	GLUtils::setFloat(m_ProgramObj, "u_Radius", EyeRadius);
	// 左眼中心点（像素坐标）
	GLUtils::setVec2(m_ProgramObj, "u_LeftEyeCenterPos", LeftEyePoint[0], LeftEyePoint[1]);
	// 右眼中心点（像素坐标）
    GLUtils::setVec2(m_ProgramObj, "u_RightEyeCenterPos", RightEyePoint[0], RightEyePoint[1]);
    // 图片尺寸
    GLUtils::setVec2(m_ProgramObj, "u_ImgSize", m_RenderImage.width, m_RenderImage.height);

	// 绘制四边形
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
}

void BigEyesSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
		glDeleteBuffers(3, m_VboIds);
		glDeleteVertexArrays(1, &m_VaoId);
		glDeleteTextures(1, &m_TextureId);
	}
}


/**
 * @param angleX 绕X轴旋转度数
 * @param angleY 绕Y轴旋转度数
 * @param ratio 宽高比
 * */
void BigEyesSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("BigEyesSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	angleX = angleX % 360;
	angleY = angleY % 360;

	//转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


	// Projection matrix
	glm::mat4 Projection = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	//glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
	//glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

	// View matrix
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
			glm::vec3(0, 0, 0), // and looks at the origin
			glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

	mvpMatrix = Projection * View * Model;

}

void BigEyesSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}
