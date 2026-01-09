/**
 *
 * Created by 公众号：字节流动 on 2024/01/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * NV21 格式渲染示例 (单纹理方法)
 * NV21 格式说明:
 * - YUV 4:2:0 采样格式
 * - 平面布局: Y 平面 + 交错的 VU 平面
 * - 单纹理存储: 将 Y 和 VU 数据打包到一个纹理中
 * - 纹理布局: 上部 2/3 存储 Y 数据, 下部 1/3 存储 VU 数据
 * - 总尺寸: width × (height × 1.5)
 * */
#include <GLUtils.h>
#include "RenderNV21Sample.h"
#include "YUVP010Example.h"

/**
 * 加载 NV21 格式图像数据
 * @param pImage 包含 NV21 数据的图像结构
 *               ppPlane[0]: Y 和 VU 数据连续存储
 */
void RenderNV21Sample::LoadImage(NativeImage *pImage)
{
	LOGCATE("RenderNV21Sample::LoadImage pImage = %p", pImage->ppPlane[0]);
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
void RenderNV21Sample::Init()
{
	// 顶点着色器: 传递顶点位置和纹理坐标
	char vShaderStr[] =R"(
			#version 300 es
			layout(location = 0) in vec4 a_position;
			layout(location = 1) in vec2 a_texCoord;
			out vec2 v_texCoord;
			void main()
			{
			   gl_Position = a_position;
			   v_texCoord = a_texCoord;
			})";

	/**
	 * 片段着色器: NV21 转 RGB (单纹理方法)
	 *
	 * 纹理布局说明:
	 * - 纹理高度: height * 1.5
	 * - Y 平面区域: [0, height), 占纹理的前 2/3
	 * - VU 平面区域: [height, height*1.5), 占纹理的后 1/3
	 *
	 * 采样策略:
	 * 1. Y 采样: 将纹理坐标 y 乘以 2/3, 映射到 Y 平面区域
	 * 2. UV 采样: 使用 texelFetch 进行像素精确采样
	 *    - 4:2:0 采样: 每 2×2 的 Y 像素块共享一对 UV 值
	 *    - V 在偶数位置 (pixelUV.x)
	 *    - U 在奇数位置 (pixelUV.x + 1)
	 *
	 * YUV 转 RGB 矩阵 (BT.601 标准):
	 * R = 1.164(Y - 16/255) + 1.596(V - 128/255)
	 * G = 1.164(Y - 16/255) - 0.392(U - 128/255) - 0.813(V - 128/255)
	 * B = 1.164(Y - 16/255) + 2.017(U - 128/255)
	 */
	char fShaderStr[] =R"(
		#version 300 es
		#extension GL_EXT_YUV_target: require
		precision highp float;
		in vec2 v_texCoord;
		uniform sampler2D y_texture;
		uniform vec2 inputSize;
		out vec4 outColor;
		void main() {
				// 采样 Y 分量: 将纹理坐标映射到 Y 平面区域 (前 2/3)
				vec2 uv = v_texCoord;
				uv.y *= 2.0 / 3.0;
				float y = texture(y_texture, uv).r - 0.063;

				// 计算 UV 采样位置 (像素坐标)
				vec2 pixelUV = v_texCoord * inputSize;
				// 4:2:0 采样: X 坐标向下取偶数 (每 2 个像素共享 UV)
				pixelUV.x = floor(pixelUV.x / 2.0) * 2.0;
				// Y 坐标向下取整并除以 2 (高度减半)
				pixelUV.y = floor(pixelUV.y / 2.0);
				// 偏移到 VU 平面区域 (加上 Y 平面高度)
				pixelUV.y += inputSize.y;
				// 采样 V 分量 (偶数位置)
				float v = texelFetch(y_texture, ivec2(int(pixelUV.x), int(pixelUV.y)), 0).r - 0.502;

				// 采样 U 分量 (奇数位置, X + 1)
				pixelUV.x += 1.0;
				float u = texelFetch(y_texture, ivec2(int(pixelUV.x), int(pixelUV.y)), 0).r - 0.502;
				vec3 yuv = vec3(y,u,v);

				// 使用 BT.601 标准矩阵将 YUV 转换为 RGB
				highp vec3 rgb = mat3(1.164, 1.164, 1.164,
							0, 		 -0.392, 	2.017,
							1.596,   -0.813,    0.0) * yuv;
				outColor = vec4(rgb, 1.0);
		}
)";

	// 编译链接着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	// 获取纹理采样器的 uniform 位置
	m_TextureLoc = glGetUniformLocation (m_ProgramObj, "y_texture" );

	// 创建单个纹理对象 (存储 Y + VU 数据)
	GLuint textureIds[1] = {0};
	glGenTextures(1, textureIds);

	m_TextureId = textureIds[0];

	/**
	 * 上传 NV21 数据到纹理
	 * - 格式: GL_LUMINANCE (单通道)
	 * - 尺寸: width × (height × 1.5)
	 * - 数据布局:
	 *   [0, width*height): Y 平面数据
	 *   [width*height, width*height*1.5): VU 交错数据
	 */
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, m_RenderImage.width, m_RenderImage.height * 3 / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

/**
 * 渲染 NV21 图像
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void RenderNV21Sample::Draw(int screenW, int screenH)
{
	LOGCATE("RenderNV21Sample::Draw()");
	// 设置像素对齐方式为 1 字节 (处理非 4 字节对齐的数据)
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	if(m_ProgramObj == GL_NONE || m_TextureId == GL_NONE) return;

	// 顶点坐标 (NDC 归一化设备坐标, 全屏矩形)
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // Position 0 左上
			-1.0f, -1.0f, 0.0f,  // Position 1 左下
			 1.0f, -1.0f, 0.0f,  // Position 2 右下
			 1.0f, 1.0f, 0.0f,  // Position 3 右上
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

	// 绑定纹理到纹理单元 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);

	// 设置采样器对应纹理单元 0
	glUniform1i(m_TextureLoc, 0);

	// 传递图像尺寸到着色器 (用于像素精确采样)
	GLUtils::setVec2(m_ProgramObj, "inputSize", m_RenderImage.width, m_RenderImage.height);

	// 绘制矩形 (使用索引绘制两个三角形)
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

/**
 * 销毁资源
 * 释放着色器程序和纹理对象
 */
void RenderNV21Sample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
		glDeleteTextures(1, &m_TextureId);
		m_ProgramObj = GL_NONE;
	}
}
