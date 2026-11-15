/**
 *
 * Created by 公众号：字节流动 on 2022/12/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_RENDER_P010_SAMPLE_H
#define NDK_OPENGLES_3_0_RENDER_P010_SAMPLE_H


#include "GLSampleBase.h"
#include "../util/ImageDef.h"

class RenderP010Sample: public GLSampleBase
{
public:
	RenderP010Sample()
	{
		m_yTextureId = GL_NONE;
		m_uvTextureId = GL_NONE;

		m_ySamplerLoc = GL_NONE;
		m_uvSamplerLoc = GL_NONE;
	}

	virtual ~RenderP010Sample()
	{
		NativeImageUtil::FreeNativeImage(&m_RenderImage);
	}

	virtual void LoadImage(NativeImage *pImage);

	virtual bool Init();

	virtual void Draw(int screenW, int screenH);

	virtual void Destroy();

private:
	// 从文件读取 P010 图像，读取失败（文件不存在/不完整）返回 false
	bool LoadP010FromFile();

	GLuint m_yTextureId;
	GLuint m_uvTextureId;

	GLint m_ySamplerLoc;
	GLint m_uvSamplerLoc;

	NativeImage m_RenderImage;

};


#endif //NDK_OPENGLES_3_0_RENDER_P010_SAMPLE_H
