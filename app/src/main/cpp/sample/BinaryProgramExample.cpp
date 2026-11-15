/**
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include <vector>
#include <string>
#include <cstdlib>
#include "BinaryProgramExample.h"

#define VERTEX_POS_INDX  0
#define TEXTURE_POS_INDX 1

BinaryProgramExample::BinaryProgramExample()
{
	// Init 失败后基类会调用 Destroy，Destroy 会读取所有句柄，因此数组每个元素都要清零
	m_VaoIds[0] = m_VaoIds[1] = GL_NONE;
	m_VboIds[0] = m_VboIds[1] = m_VboIds[2] = m_VboIds[3] = GL_NONE;

	m_ImageTextureId = GL_NONE;
	m_FboTextureId = GL_NONE;
	m_SamplerLoc = GL_NONE;
	m_FboId = GL_NONE;
	m_FboProgramObj = GL_NONE;
	m_FboVertexShader = GL_NONE;
	m_FboFragmentShader = GL_NONE;
	m_FboSamplerLoc = GL_NONE;
}

BinaryProgramExample::~BinaryProgramExample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

void BinaryProgramExample::LoadImage(NativeImage *pImage)
{
	if (pImage)
	{
		LOGCATD("BinaryProgramExample::LoadImage pImage = %p", pImage->ppPlane[0]);
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

bool BinaryProgramExample::Init()
{
	// 原实现没有“已初始化”守卫，每帧都会重建全部资源并重新写一次二进制缓存文件；
	// 现在由 GLSampleBase 保证每个上下文只 Init 一次。
	//顶点坐标
	GLfloat vVertices[] = {
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,
	};

	//正常纹理坐标
	GLfloat vTexCoors[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
    };

	//fbo 纹理坐标与正常纹理方向不同，原点位于左下角
	GLfloat vFboTexCoors[] = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 1.0f,
	};

	GLushort indices[] = { 0, 1, 2, 1, 3, 2 };

	char vShaderStr[] =
			"#version 300 es                            \n"
			"layout(location = 0) in vec4 a_position;   \n"
			"layout(location = 1) in vec2 a_texCoord;   \n"
			"out vec2 v_texCoord;                       \n"
			"void main()                                \n"
			"{                                          \n"
			"   gl_Position = a_position;               \n"
			"   v_texCoord = a_texCoord;                \n"
			"}                                          \n";

	// 用于普通渲染的片段着色器脚本，简单纹理映射
	char fShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D s_TextureMap;\n"
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"
			"}";

	// 用于离屏渲染的片段着色器脚本，取每个像素的灰度值
	char fFboShaderStr[] =
			"#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D s_TextureMap;\n"
			"void main()\n"
			"{\n"
			"    outColor = texture(s_TextureMap, v_texCoord);\n"
			"    //float luminance = tempColor.r * 0.299 + tempColor.g * 0.587 + tempColor.b * 0.114;\n"
			"    //outColor = vec4(vec3(luminance), tempColor.a);\n"
			"}"; // 输出灰度图


	//检查是否支持获取二进制 program
    GLint programBinaryFormats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &programBinaryFormats);
    if(programBinaryFormats <= GL_NONE) {
        //不支持获取二进制 program, game over
    }
	LOGCATD("BinaryProgramExample programBinaryFormats=%d", programBinaryFormats);

	// 编译链接用于普通渲染的着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// 编译链接用于离屏渲染的着色器程序
	m_FboProgramObj = GLUtils::CreateProgram(vShaderStr, fFboShaderStr, m_FboVertexShader, m_FboFragmentShader);

	// 只有源码编译成功、且驱动支持二进制格式时才演示“导出 -> 重新加载”。
	// 原实现不检查任何返回值：文件打开失败时仍去加载（可能读到上次运行残留的旧文件并覆盖、泄漏
	// 已编译的 program），二进制加载失败时 program 直接变成 0。现在只有完整写入成功后才尝试加载，
	// 加载成功才替换编译出的 program，否则继续使用编译版本（二进制缓存只是优化，不能影响功能）。
	if(programBinaryFormats > GL_NONE && m_ProgramObj != GL_NONE) {
        GLint length = 0;
        glGetProgramiv(m_ProgramObj, GL_PROGRAM_BINARY_LENGTH, &length);

        if (length > 0) {
            std::vector<unsigned char> buffer(static_cast<size_t>(length));
            GLsizei binaryLength = 0;
            GLenum format = 0;
            glGetProgramBinary(m_ProgramObj, length, &binaryLength, &format, buffer.data());

            ///sdcard/Android/data/com.byteflow.app/files/Download/shader_program.bin_format_34624
            char binaryFilePath[256] = {0};
            snprintf(binaryFilePath, sizeof(binaryFilePath), "%s/shader_program.bin_format_%d", DEFAULT_OGL_ASSETS_DIR, format);

            bool written = false;
            FILE *fp = binaryLength > 0 ? fopen(binaryFilePath, "wb") : nullptr;
            LOGCATD("BinaryProgramExample fp=%p, file=%s, binFormat=%d, binLen=%d", fp, binaryFilePath, format, binaryLength);
            if (fp) {
                size_t count = fwrite(buffer.data(), static_cast<size_t>(binaryLength), 1, fp);
                // fclose 也可能因为缓冲区回写失败而出错，两者都成功才算写入完成
                written = (fclose(fp) == 0) && count == 1;
            }

            if (written) {
                GLuint binaryProgram = GL_NONE;
                LoadBinary2Program(&binaryProgram, binaryFilePath);
                if (binaryProgram != GL_NONE) {
                    // 二进制加载成功，用它替换编译出的 program
                    GLUtils::DeleteProgram(m_ProgramObj);
                    m_ProgramObj = binaryProgram;
                } else {
                    LOGCATE("BinaryProgramExample::Init load binary fail, fallback to compiled program");
                }
            } else {
                LOGCATE("BinaryProgramExample::Init write binary fail, file=%s", binaryFilePath);
            }
        }
	}

	if (m_ProgramObj == GL_NONE || m_FboProgramObj == GL_NONE)
	{
		LOGCATE("BinaryProgramExample::Init m_ProgramObj == GL_NONE");
		return false;
	}
	m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
	m_FboSamplerLoc = glGetUniformLocation(m_FboProgramObj, "s_TextureMap");

	// 生成 VBO ，加载顶点数据和索引数据
	// Generate VBO Ids and load the VBOs with data
	glGenBuffers(4, m_VboIds);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoors), vTexCoors, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vFboTexCoors), vFboTexCoors, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GO_CHECK_GL_ERROR();

	// 生成 2 个 VAO，一个用于普通渲染，另一个用于离屏渲染
	// Generate VAO Ids
	glGenVertexArrays(2, m_VaoIds);
    // 初始化用于普通渲染的 VAO
	// Normal rendering VAO
	glBindVertexArray(m_VaoIds[0]);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);


	// 初始化用于离屏渲染的 VAO
	// FBO off screen rendering VAO
	glBindVertexArray(m_VaoIds[1]);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glVertexAttribPointer(VERTEX_POS_INDX, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
	glEnableVertexAttribArray(TEXTURE_POS_INDX);
	glVertexAttribPointer(TEXTURE_POS_INDX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);

	// 创建并初始化图像纹理
	glGenTextures(1, &m_ImageTextureId);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	GO_CHECK_GL_ERROR();

	if (!CreateFrameBufferObj())
	{
		LOGCATE("BinaryProgramExample::Init CreateFrameBufferObj fail");
		return false;
	}
	return true;
}

void BinaryProgramExample::LoadBinary2Program(GLuint *pProgram, char *binFilePath) {
	// 失败时 *pProgram 保持 GL_NONE，由调用方决定回退策略
	*pProgram = GL_NONE;

	// 二进制格式编码在文件名后缀 "format_<N>" 中；找不到或解析失败时不能交给 std::stoi
	// （npos + 7 会越界、非数字会抛异常），直接放弃加载
	std::string path(binFilePath);
	size_t pos = path.rfind("format_");
	if (pos == std::string::npos) {
		LOGCATE("BinaryProgramExample LoadBinary2Program no format in path=%s", binFilePath);
		return;
	}
	const char *pFormatStr = path.c_str() + pos + 7;
	char *pEnd = nullptr;
	long format = strtol(pFormatStr, &pEnd, 10);
	if (pEnd == pFormatStr || *pEnd != '\0' || format <= 0) {
		LOGCATE("BinaryProgramExample LoadBinary2Program invalid format in path=%s", binFilePath);
		return;
	}

	FILE *fp = fopen(binFilePath, "rb");
	LOGCATD("BinaryProgramExample LoadBinary2Program fp=%p, file=%s", fp, binFilePath);
	if (fp == nullptr) return;

	// ftell 失败返回 -1，文件为空时也没有可加载的内容
	long size = -1;
	if (fseek(fp, 0L, SEEK_END) == 0) {
		size = ftell(fp);
	}
	if (size <= 0) {
		LOGCATE("BinaryProgramExample LoadBinary2Program invalid file size=%ld", size);
		fclose(fp);
		return;
	}
	rewind(fp);

	std::vector<unsigned char> buffer(static_cast<size_t>(size));
	size_t count = fread(buffer.data(), static_cast<size_t>(size), 1, fp);
	fclose(fp);
	if (count != 1) {
		LOGCATE("BinaryProgramExample LoadBinary2Program read fail, size=%ld", size);
		return;
	}

	LOGCATD("BinaryProgramExample format=%ld", format);
	GLuint program = glCreateProgram();
	if (program == GL_NONE) return;
	glProgramBinary(program, static_cast<GLenum>(format), buffer.data(), static_cast<GLsizei>(size));

	//检查是否加载成功（驱动升级等原因都会导致旧的二进制不可用）
	GLint status = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if(GL_FALSE == status) {
		LOGCATE("BinaryProgramExample glProgramBinary fail.");
		GLUtils::DeleteProgram(program);
		return;
	}
	*pProgram = program;
}


void BinaryProgramExample::Draw(int screenW, int screenH)
{
	// 离屏渲染
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glViewport(0, 0, m_RenderImage.width, m_RenderImage.height);

	// Do FBO off screen rendering
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	glUseProgram(m_FboProgramObj);
	glBindVertexArray(m_VaoIds[1]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glUniform1i(m_FboSamplerLoc, 0);
	GO_CHECK_GL_ERROR();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

//	uint8_t *pBuffer = new uint8_t[m_RenderImage.width * m_RenderImage.height * 4];
//
//	NativeImage nativeImage = m_RenderImage;
//	nativeImage.format = IMAGE_FORMAT_RGBA;
//	nativeImage.ppPlane[0] = pBuffer;
//	FUN_BEGIN_TIME("FBO glReadPixels")
//		glReadPixels(0, 0, nativeImage.width, nativeImage.height, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
//	FUN_END_TIME("FBO cost glReadPixels")
//
//	NativeImageUtil::DumpNativeImage(&nativeImage, "/sdcard/DCIM", "NDK");
//	delete []pBuffer;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 普通渲染
	// Do normal rendering
	glViewport(0, 0, screenW, screenH);
	glUseProgram(m_ProgramObj);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(m_VaoIds[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glUniform1i(m_SamplerLoc, 0);
	GO_CHECK_GL_ERROR();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindVertexArray(GL_NONE);

}

void BinaryProgramExample::Destroy()
{
	// 各对象独立释放并清零（原实现删除后不清零，再次 Destroy 会重复删除）
	GLUtils::DeleteProgram(m_ProgramObj);
	GLUtils::DeleteProgram(m_FboProgramObj);
	GLUtils::DeleteTexture(m_ImageTextureId);
	GLUtils::DeleteTexture(m_FboTextureId);
	GLUtils::DeleteBuffers(4, m_VboIds);
	GLUtils::DeleteVertexArrays(2, m_VaoIds);
	GLUtils::DeleteFramebuffer(m_FboId);
}

bool BinaryProgramExample::CreateFrameBufferObj()
{
	// 创建并初始化 FBO 纹理
	glGenTextures(1, &m_FboTextureId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 创建并初始化 FBO
	glGenFramebuffers(1, &m_FboId);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FboTextureId, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
		LOGCATE("BinaryProgramExample::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
		// 失败路径同样恢复默认绑定，避免把未完成的 FBO 留作当前绘制目标
		glBindTexture(GL_TEXTURE_2D, GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
		return false;
	}
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	return true;

}

