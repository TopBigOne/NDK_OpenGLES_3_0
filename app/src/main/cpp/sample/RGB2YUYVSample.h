/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */
#ifndef NDK_OPENGLES_3_0_RGB2YUYVSAMPLE_H
#define NDK_OPENGLES_3_0_RGB2YUYVSAMPLE_H


#include "GLSampleBase.h"
#include "../util/ImageDef.h"

class RGB2YUYVSample : public GLSampleBase
{
public:
	RGB2YUYVSample();

	virtual ~RGB2YUYVSample();

	virtual void LoadImage(NativeImage *pImage);

	virtual bool Init();
	virtual void Draw(int screenW, int screenH);

	virtual void Destroy();

	bool CreateFrameBufferObj();

private:
	// 转换结果是否已经保存到文件。原来每次 Draw 都 new 一块缓冲、glReadPixels 并写一次文件，
	// 连续渲染时会持续写盘并阻塞 GL 线程；现在只读回并保存一次
	bool m_YuyvDumped;
	GLuint m_ImageTextureId;
	GLuint m_FboTextureId;
	GLuint m_FboId;
	GLuint m_VaoIds[2] = {GL_NONE};
	GLuint m_VboIds[4] = {GL_NONE};
	GLint m_SamplerLoc;
	NativeImage m_RenderImage;
	GLuint m_FboProgramObj;
	GLuint m_FboVertexShader;
	GLuint m_FboFragmentShader;
	GLint m_FboSamplerLoc;
	//GLuint m_YuyvWidth;

};


#endif //NDK_OPENGLES_3_0_RGB2YUYVSAMPLE_H
