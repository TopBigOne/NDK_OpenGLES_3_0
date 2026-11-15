/**
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "BlendingSample.h"
#include "../util/GLUtils.h"

BlendingSample::BlendingSample()
{

	m_SamplerLoc = GL_NONE;
	m_MVPMatLoc = GL_NONE;

	// Init 失败后基类会调用 Destroy，Destroy 会读取所有句柄，因此必须在构造时清零
	for (int i = 0; i < RENDER_IMG_NUM; ++i) m_TextureIds[i] = GL_NONE;
	for (int i = 0; i < 3; ++i)
	{
		m_VaoIds[i] = GL_NONE;
		m_VboIds[i] = GL_NONE;
	}

	m_AngleX = 0;
	m_AngleY = 0;
}

BlendingSample::~BlendingSample()
{
	for (int i = 0; i < RENDER_IMG_NUM; ++i)
	{
		NativeImageUtil::FreeNativeImage(&m_RenderImages[i]);
	}


}

bool BlendingSample::Init()
{
	// 是否已初始化由 GLSampleBase 管理，这里不再自行判断
	//create RGBA textures
	glGenTextures(RENDER_IMG_NUM, m_TextureIds);
	for (int i = 0; i < RENDER_IMG_NUM; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, GL_NONE);
	}

	char vShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"layout(location = 0) in vec4 a_position;\n"
			"layout(location = 1) in vec2 a_texCoord;\n"
			"\n"
			"out vec2 v_texCoord;\n"
			"\n"
			"uniform mat4 u_MVPMatrix;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    gl_Position = u_MVPMatrix * a_position;\n"
			"    v_texCoord = a_texCoord;\n"
			"}";

	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D s_TextureMap;\n"
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"
			"}";

	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
	if (m_ProgramObj)
	{
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
		GO_CHECK_GL_ERROR();
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
		GO_CHECK_GL_ERROR();
	}
	else
	{
		LOGCATE("BlendingSample::Init create program fail");
		return false;
	}

	GLfloat boxVertices[] = {
			 //position            //texture coord
			-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,
			 0.5f, -0.5f, -0.5f,   1.0f, 0.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,   0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,

			-0.5f, -0.5f, 0.5f,    0.0f, 0.0f,
			 0.5f, -0.5f, 0.5f,    1.0f, 0.0f,
			 0.5f,  0.5f, 0.5f,    1.0f, 1.0f,
			 0.5f,  0.5f, 0.5f,    1.0f, 1.0f,
			-0.5f,  0.5f, 0.5f,    0.0f, 1.0f,
			-0.5f, -0.5f, 0.5f,    0.0f, 0.0f,

			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f,

			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f,
			 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f,
			 0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,   1.0f, 1.0f,
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f,

			-0.5f, 0.5f, -0.5f,    0.0f, 1.0f,
			 0.5f, 0.5f, -0.5f,    1.0f, 1.0f,
			 0.5f, 0.5f,  0.5f,    1.0f, 0.0f,
			 0.5f, 0.5f,  0.5f,    1.0f, 0.0f,
			-0.5f, 0.5f,  0.5f,    0.0f, 0.0f,
			-0.5f, 0.5f, -0.5f,    0.0f, 1.0f,
	};

	GLfloat flatVertices[] = {
			 5.0f, -0.5f,  5.0f,  2.0f,  0.0f,
			-5.0f, -0.5f,  5.0f,  0.0f,  0.0f,
			-5.0f, -0.5f, -5.0f,  0.0f,  2.0f,

			 5.0f, -0.5f,  5.0f,  2.0f,  0.0f,
			-5.0f, -0.5f, -5.0f,  0.0f,  2.0f,
			 5.0f, -0.5f, -5.0f,  2.0f,  2.0f
	};

	GLfloat windowVertices[] = {
			0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
			0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
			1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

			0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
			1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
			1.0f,  0.5f,  0.0f,  1.0f,  0.0f
	};

	// Generate VBO Ids and load the VBOs with data
	glGenBuffers(3, m_VboIds);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(flatVertices), flatVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(windowVertices), windowVertices, GL_STATIC_DRAW);

	// Generate VAO Ids
	glGenVertexArrays(3, m_VaoIds);

	glBindVertexArray(m_VaoIds[0]);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *) (3* sizeof(GLfloat)));
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	glBindVertexArray(GL_NONE);

	glBindVertexArray(m_VaoIds[1]);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *) (3* sizeof(GLfloat)));
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	glBindVertexArray(GL_NONE);

	glBindVertexArray(m_VaoIds[2]);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *) (3* sizeof(GLfloat)));
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	glBindVertexArray(GL_NONE);

	//upload RGBA image data
	for (int i = 0; i < RENDER_IMG_NUM; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImages[i].width, m_RenderImages[i].height, 0, GL_RGBA,
					 GL_UNSIGNED_BYTE, m_RenderImages[i].ppPlane[0]);
		glBindTexture(GL_TEXTURE_2D, GL_NONE);
	}

	// 上下文重建后 Init 会再次执行，先清空，避免窗户位置被重复追加
	windowsTrans.clear();
	sorted.clear();
	windowsTrans.push_back(glm::vec3(-1.5f,  0.0f, -0.48f));
	windowsTrans.push_back(glm::vec3( 1.5f,  0.0f,  0.51f));
	windowsTrans.push_back(glm::vec3( 0.0f,  0.0f,  0.7f));
	windowsTrans.push_back(glm::vec3(-0.3f,  0.0f, -2.3f));
	windowsTrans.push_back(glm::vec3( 0.5f,  0.0f, -0.6f));

	for (GLuint i = 0; i < windowsTrans.size(); i++)
	{
		GLfloat distance = std::sqrt(std::pow(windowsTrans[i].x - 0.5f, 2.0f) + std::pow(windowsTrans[i].y - 1.0f, 2.0f) + std::pow(windowsTrans[i].z - 3.0f, 2.0f));
		sorted[distance] = windowsTrans[i];
	}

	return true;
}

void BlendingSample::LoadMultiImageWithIndex(int index, NativeImage *pImage)
{
	GLSampleBase::LoadMultiImageWithIndex(index, pImage);
	// index 来自 Java 层，越界会写坏 m_RenderImages 之后的成员
	if (pImage && index >= 0 && index < RENDER_IMG_NUM)
	{
		LOGCATD("BlendingSample::LoadImage index=%d, pImage = %p", index, pImage->ppPlane[0]);
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImages[index]);
	}

}

void BlendingSample::Draw(int screenW, int screenH)
{
	LOGCATV("BlendingSample::Draw()");

	if (m_ProgramObj == GL_NONE) return;

	float ratio = (float)screenW / screenH;
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(m_ProgramObj);

	glBindVertexArray(m_VaoIds[0]);
	// Bind the RGBA map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureIds[0]);
	glUniform1i(m_SamplerLoc, 0);

	UpdateMatrix(m_MVPMatrix, 0, 0, 1.0,  glm::vec3(-1.0f, 0.0f, -1.0f), ratio);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	UpdateMatrix(m_MVPMatrix, 0, 0, 1.0,  glm::vec3(2.0f, 0.0f, 0.0f), ratio);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	GO_CHECK_GL_ERROR();
	glBindVertexArray(0);

	glBindVertexArray(m_VaoIds[1]);
	// Bind the RGBA map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureIds[1]);
	glUniform1i(m_SamplerLoc, 0);

	UpdateMatrix(m_MVPMatrix, 0, 0, 1.0, glm::vec3(0.0f,  0.0f,  0.0f), ratio);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(0);

	glBindVertexArray(m_VaoIds[2]);
	// Bind the RGBA map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureIds[2]);
	glUniform1i(m_SamplerLoc, 0);

	for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
	{
		UpdateMatrix(m_MVPMatrix, 0, 0 , 1.0, it->second, ratio);
		glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	GO_CHECK_GL_ERROR();
	glBindVertexArray(0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void BlendingSample::Destroy()
{
	// 各对象独立释放并清零，重复调用安全
	GLUtils::DeleteProgram(m_ProgramObj);
	GLUtils::DeleteBuffers(3, m_VboIds);
	GLUtils::DeleteVertexArrays(3, m_VaoIds);
	GLUtils::DeleteTextures(RENDER_IMG_NUM, m_TextureIds);

}

void BlendingSample::UpdateMatrix(glm::mat4 &mvpMatrix, int angleXRotate, int angleYRotate, float scale, glm::vec3 transVec3, float ratio)
{
	LOGCATV("BlendingSample::UpdateMatrix angleX = %d, angleY = %d, ratio = %f", angleXRotate,
			angleYRotate, ratio);
	angleXRotate = angleXRotate % 360;
	angleYRotate = angleYRotate % 360;

	//转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleXRotate);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleYRotate);


	// Projection matrix
	//glm::mat4 Projection = glm::ortho(-ratio, ratio, -1.0f, 1.0f, 0.0f, 100.0f);
	//glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
	glm::mat4 Projection = glm::perspective(45.0f, ratio, 0.1f, 100.f);

	// View matrix
	glm::mat4 View = glm::lookAt(
			glm::vec3(0.5, 1, 3), // Camera is at (0,0,1), in World Space
			glm::vec3(0, 0, 0), // and looks at the origin
			glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(scale, scale, scale));
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
	Model = glm::translate(Model, transVec3);

	mvpMatrix = Projection * View * Model;
}

void BlendingSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
}

