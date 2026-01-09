/**
 *
 * Created by 公众号：字节流动 on 2023/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * NV21 格式纹理映射示例
 * NV21 格式说明:
 * - YUV 4:2:0 采样格式
 * - 平面布局: Y 平面 + 交错的 VU 平面
 * - 数据排列: YYYYYYYY VU VU VU VU
 * - 总大小: width * height * 1.5 字节
 * - Y 平面: width * height (全分辨率亮度信息)
 * - VU 平面: width * height / 2 (V 和 U 分量交错排列,各占 1/4)
 * */

#include <GLUtils.h>
#include "NV21TextureMapSample.h"

/**
 * 加载 NV21 格式图像数据
 * @param pImage 包含 NV21 数据的图像结构
 *               ppPlane[0]: Y 平面数据
 *               ppPlane[1]: VU 交错平面数据
 */
void NV21TextureMapSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("NV21TextureMapSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

/**
 * 初始化 NV21 渲染器
 * 创建着色器程序和纹理对象
 */
void NV21TextureMapSample::Init()
{
	// 顶点着色器: 传递顶点位置和纹理坐标
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

	/**
	 * 片段着色器: NV21 转 RGB
	 *
	 * NV21 采样说明:
	 * - Y 分量: 从 y_texture 的 r 通道采样 (全分辨率)
	 * - U 分量: 从 uv_texture 的 a 通道采样 (NV21 格式中 V 在前, U 在后)
	 * - V 分量: 从 uv_texture 的 r 通道采样
	 *
	 * YUV 取值范围归一化:
	 * - Y: [16, 235] -> 减去 16/255 ≈ 0.063 归一化到 [0, 1]
	 * - U/V: [16, 240] -> 减去 128/255 ≈ 0.502 归一化到 [-0.5, 0.5]
	 *
	 * YUV 转 RGB 矩阵 (BT.601 标准):
	 * R = 1.164(Y - 16/255) + 1.596(V - 128/255)
	 * G = 1.164(Y - 16/255) - 0.392(U - 128/255) - 0.813(V - 128/255)
	 * B = 1.164(Y - 16/255) + 2.017(U - 128/255)
	 *
	 * 矩阵形式:
	 * | R |   | 1.164   0.000   1.596 |   | Y - 0.063 |
	 * | G | = | 1.164  -0.392  -0.813 | × | U - 0.502 |
	 * | B |   | 1.164   2.017   0.000 |   | V - 0.502 |
	 */
	char fShaderStr[] =
			"#version 300 es                                     \n"
			"precision mediump float;                            \n"
			"in vec2 v_texCoord;                                 \n"
			"layout(location = 0) out vec4 outColor;             \n"
			"uniform sampler2D y_texture;                        \n"
			"uniform sampler2D uv_texture;                        \n"
			"void main()                                         \n"
			"{                                                   \n"
			"	vec3 yuv;										\n"
			// 采样 Y 分量 (亮度),减去偏移量 16/255
			"   yuv.x = texture(y_texture, v_texCoord).r -0.063;  	\n"
			// 采样 U 分量 (蓝色色度),NV21 格式 U 在 alpha 通道
			"   yuv.y = texture(uv_texture, v_texCoord).a-0.502;	\n"
			// 采样 V 分量 (红色色度),NV21 格式 V 在 red 通道
			"   yuv.z = texture(uv_texture, v_texCoord).r-0.502;	\n"
			// 使用 BT.601 标准矩阵将 YUV 转换为 RGB
			"	highp vec3 rgb = mat3(1.164, 1.164, 1.164,		\n"
			"               0, 		 -0.392, 	2.017,				\n"
			"               1.596,   -0.813,    0.0) * yuv; 		\n"
			"	outColor = vec4(rgb, 1.0);						\n"
			"}                                                   \n";

	// 编译链接着色器程序
	m_ProgramObj= GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// 获取纹理采样器的 uniform 位置
	m_ySamplerLoc = glGetUniformLocation (m_ProgramObj, "y_texture" );
	m_uvSamplerLoc = glGetUniformLocation(m_ProgramObj, "uv_texture");

	// 创建两个纹理对象: Y 平面和 UV 平面
	GLuint textureIds[2] = {0};
	glGenTextures(2, textureIds);

	m_yTextureId = textureIds[0];   // Y 平面纹理
	m_uvTextureId = textureIds[1];  // UV 平面纹理
}

/**
 * 渲染 NV21 图像
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void NV21TextureMapSample::Draw(int screenW, int screenH)
{
	LOGCATE("NV21TextureMapSample::Draw()");

	if(m_ProgramObj == GL_NONE || m_yTextureId == GL_NONE || m_uvTextureId == GL_NONE) return;

	/**
	 * 上传 Y 平面数据
	 * - 格式: GL_LUMINANCE (单通道亮度)
	 * - 尺寸: width × height (全分辨率)
	 * - 数据: m_RenderImage.ppPlane[0]
	 */
	glBindTexture(GL_TEXTURE_2D, m_yTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width, m_RenderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	/**
	 * 上传 UV 平面数据
	 * - 格式: GL_LUMINANCE_ALPHA (双通道: V 在 L, U 在 A)
	 * - 尺寸: (width/2) × (height/2) (4:2:0 采样,宽高各减半)
	 * - 数据: m_RenderImage.ppPlane[1] (VU 交错排列)
	 * - NV21 特点: V 和 U 交错存储 VUVUVU...
	 */
	glBindTexture(GL_TEXTURE_2D, m_uvTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, m_RenderImage.width >> 1, m_RenderImage.height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[1]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 顶点坐标 (NDC 归一化设备坐标)
	GLfloat verticesCoords[] = {
			-1.0f,  0.78f, 0.0f,  // Position 0 左上
			-1.0f, -0.78f, 0.0f,  // Position 1 左下
			1.0f,  -0.78f, 0.0f,  // Position 2 右下
			1.0f,   0.78f, 0.0f,  // Position 3 右上
	};

	// 纹理坐标 (左下角为原点)
	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // TexCoord 0 左下
			0.0f,  1.0f,        // TexCoord 1 左上
			1.0f,  1.0f,        // TexCoord 2 右上
			1.0f,  0.0f         // TexCoord 3 右下
	};

	// 索引数组 (两个三角形组成矩形)
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// 使用着色器程序
	glUseProgram (m_ProgramObj);

	// 加载顶点位置数据
	glVertexAttribPointer (0, 3, GL_FLOAT,
						   GL_FALSE, 3 * sizeof (GLfloat), verticesCoords);
	// 加载纹理坐标数据
	glVertexAttribPointer (1, 2, GL_FLOAT,
						   GL_FALSE, 2 * sizeof (GLfloat), textureCoords);

	glEnableVertexAttribArray (0);
	glEnableVertexAttribArray (1);

	// 绑定 Y 平面纹理到纹理单元 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_yTextureId);

	// 设置 Y 平面采样器对应纹理单元 0
	glUniform1i(m_ySamplerLoc, 0);

	// 绑定 UV 平面纹理到纹理单元 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_uvTextureId);

	// 设置 UV 平面采样器对应纹理单元 1
	glUniform1i(m_uvSamplerLoc, 1);

	// 绘制矩形 (使用索引绘制两个三角形)
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

/**
 * 销毁资源
 * 释放着色器程序和纹理对象
 */
void NV21TextureMapSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
		glDeleteTextures(1, &m_yTextureId);
		glDeleteTextures(1, &m_uvTextureId);
		m_ProgramObj = GL_NONE;
	}

}
