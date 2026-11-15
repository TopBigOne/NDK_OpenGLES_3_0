/**
 *
 * Created by 公众号：字节流动 on 2024/06/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include <LogUtil.h>
#include "FullScreenTriangleSample.h"

FullScreenTriangleSample::FullScreenTriangleSample()
{
	m_TextureId = 0;
	m_SamplerLoc = -1;  // 原来没有初始化

}

FullScreenTriangleSample::~FullScreenTriangleSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

bool FullScreenTriangleSample::Init()
{
	//create RGBA texture
	glGenTextures(1, &m_TextureId);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	char vShaderStr[] =
			"#version 300 es                            \n"
            "out vec2 v_texCoord;                       \n"
            "void main()                                \n"
            "{                                          \n"
            "   vec2 uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
            "   v_texCoord = vec2(uv.x, 1.0 - uv.y);//纹理坐标做一下上下镜像(针对 Android 设备)\n"
            "   gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);\n"
            "} ";

	char fShaderStr[] =
			"#version 300 es                                     \n"
			"precision mediump float;                            \n"
			"in vec2 v_texCoord;                                 \n"
			"layout(location = 0) out vec4 outColor;             \n"
			"uniform sampler2D s_Texture;                     \n"
			"void main()                                         \n"
			"{                                                   \n"
			"  outColor = texture(s_Texture, v_texCoord);     \n"
			"}                                                   \n";

	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
	if (m_ProgramObj)
	{
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_Texture");
	}
	else
	{
		LOGCATE("FullScreenTriangleSample::Init create program fail");
		return false;
	}

    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

	return true;
}

void FullScreenTriangleSample::Draw(int screenW, int screenH)
{
	LOGCATV("FullScreenTriangleSample::Draw()");

	if(m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram (m_ProgramObj);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
    glUniform1i(m_SamplerLoc, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void FullScreenTriangleSample::LoadImage(NativeImage *pImage)
{
	if (pImage)
	{
		LOGCATD("FullScreenTriangleSample::LoadImage pImage = %p", pImage->ppPlane[0]);
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

void FullScreenTriangleSample::Destroy()
{
	GLUtils::DeleteProgram(m_ProgramObj);
	GLUtils::DeleteTexture(m_TextureId);

}
