/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * FBO大长腿特效示例
 * 本示例实现了图像局部拉伸效果（类似美颜相机的大长腿功能）
 * 核心原理：
 * 1. 通过动态调整顶点坐标和纹理坐标实现局部拉伸
 * 2. 使用FBO进行离屏渲染，将拉伸后的图像渲染到纹理
 * 3. 支持垂直拉伸（大长腿）和水平拉伸（宽身体）两种模式
 * 4. 根据拉伸区域位置自动选择4点、6点或8点网格变形
 * */

#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>
#include "FBOLegLengthenSample.h"

// 顶点属性索引定义
#define VERTEX_POS_INDX  0   // 顶点位置属性索引
#define TEXTURE_POS_INDX 1   // 纹理坐标属性索引

// 普通渲染的顶点着色器（用于将FBO纹理渲染到屏幕）
const char vShaderStr[] =
		"#version 300 es                            \n"
		"layout(location = 0) in vec4 a_position;   \n"
		"layout(location = 1) in vec2 a_texCoord;   \n"
		"out vec2 v_texCoord;                       \n"
		"uniform mat4 u_MVPMatrix;                  \n"
		"void main()                                \n"
		"{                                          \n"
		"   gl_Position = u_MVPMatrix * a_position; \n"
		"   v_texCoord = a_texCoord;                \n"
		"}                                          \n";

// 普通渲染的片段着色器（简单的纹理采样）
const char fShaderStr[] =
		"#version 300 es\n"
		"precision mediump float;\n"
		"in vec2 v_texCoord;\n"
		"layout(location = 0) out vec4 outColor;\n"
		"uniform sampler2D s_TextureMap;\n"
		"void main()\n"
		"{\n"
		"    outColor = texture(s_TextureMap, v_texCoord);\n"
		"}";

// FBO离屏渲染的顶点着色器（用于生成拉伸效果）
const char vFboShaderStr[] =
		"#version 300 es                            \n"
		"layout(location = 0) in vec4 a_position;   \n"
		"layout(location = 1) in vec2 a_texCoord;   \n"
		"out vec2 v_texCoord;                       \n"
		"uniform mat4 u_MVPMatrix;\n"
		"void main()                                \n"
		"{                                          \n"
		"   gl_Position = u_MVPMatrix * a_position; \n"
		"   v_texCoord = a_texCoord;                \n"
		"}                                          \n";

// FBO离屏渲染的片段着色器
const char fFboShaderStr[] =
		"#version 300 es\n"
		"precision mediump float;\n"
		"in vec2 v_texCoord;\n"
		"layout(location = 0) out vec4 outColor;\n"
		"uniform sampler2D s_TextureMap;\n"
		"void main()\n"
		"{\n"
		"    vec4 tempColor = texture(s_TextureMap, v_texCoord);\n"
		"    float luminance = tempColor.r * 0.299 + tempColor.g * 0.587 + tempColor.b * 0.114;\n"
		"    //outColor = vec4(vec3(luminance), tempColor.a);\n"
		"    outColor = tempColor;\n"
		"}"; // 输出原图（可选输出灰度图）

// 垂直拉伸8点网格的索引数组（3个矩形，6个三角形）
// 将图像分成3段：上部不变区域、中部拉伸区域、下部不变区域
const GLushort V_EIGHT_POINT_INDICES[] = { 0, 1, 2,  0, 2, 3,  1, 4, 7,  1, 7, 2,  4, 5, 6,  4, 6, 7};

// 垂直拉伸6点网格的索引数组（2个矩形，4个三角形）
// 将图像分成2段：一段拉伸区域、一段不变区域
const GLushort V_SIX_POINT_INDICES[] = { 0, 1, 2,  0, 2, 3,  1, 4, 5,  1, 5, 2};

// 水平拉伸8点网格的索引数组
const GLushort H_EIGHT_POINT_INDICES[] = { 0, 1, 2,  0, 2, 3,  3, 2, 5,  3, 5, 4,  4, 5, 6,  4, 6, 7};

// 水平拉伸6点网格的索引数组
const GLushort H_SIX_POINT_INDICES[] = { 0, 1, 2,  0, 2, 3,  3, 2, 5,  3, 5, 4,};

// 4点网格索引数组（全图拉伸或不拉伸）
const GLushort FOUR_POINT_INDICES[] = { 0, 1, 2,  0, 2, 3};

// 构造函数：初始化成员变量
FBOLegLengthenSample::FBOLegLengthenSample()
{
	// OpenGL对象ID初始化
	m_ImageTextureId = GL_NONE;      // 输入图像纹理
	m_FboTextureId = GL_NONE;        // FBO输出纹理（拉伸后的图像）
	m_SamplerLoc = GL_NONE;          // 普通渲染的采样器位置
	m_FboId = GL_NONE;               // FBO对象ID
	m_FboProgramObj = GL_NONE;       // FBO离屏渲染的着色器程序
	m_FboVertexShader = GL_NONE;     // FBO顶点着色器
	m_FboFragmentShader = GL_NONE;   // FBO片段着色器
	m_FboSamplerLoc = GL_NONE;       // FBO采样器位置

	// 动画参数
	m_dt = 0.0;        // 拉伸偏移量（动态变化，实现动画效果）
	m_isgo = true;     // 动画方向标志

	// 默认拉伸模式：垂直8点拉伸（大长腿效果）
	m_StretchMode = VERTICAL_STRETCH_8_POINTS;
}

FBOLegLengthenSample::~FBOLegLengthenSample()
{
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

void FBOLegLengthenSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("FBOLegLengthenSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

// 初始化OpenGL资源和拉伸参数
void FBOLegLengthenSample::Init()
{
	// 设置拉伸模式：true为垂直拉伸（大长腿），false为水平拉伸
	m_bIsVerticalMode = true;

	// 定义拉伸区域（像素坐标）
	// 这里设置拉伸图像的下半部分（腿部区域）
	RectF inRectF;
	inRectF.left = 0.0f ;
	inRectF.right = m_RenderImage.width;
	inRectF.top = m_RenderImage.height * 0.5f;    // 从图像中间开始
	inRectF.bottom = m_RenderImage.height;        // 到图像底部

	// 将像素坐标归一化到[0,1]范围（纹理坐标）
	m_StretchRect.left = inRectF.left / m_RenderImage.width;
	m_StretchRect.right = inRectF.right / m_RenderImage.width;
	m_StretchRect.top = inRectF.top / m_RenderImage.height;
	m_StretchRect.bottom = inRectF.bottom / m_RenderImage.height;

	// 根据拉伸区域位置自动选择合适的网格模式
	// 不同的网格模式使用不同数量的顶点，优化性能
	if (m_bIsVerticalMode)  // 垂直拉伸模式
	{
		if (m_StretchRect.top == 0 && m_StretchRect.bottom == 1.0f)
		{
			// 拉伸整个图像，使用4点网格（2个三角形）
			m_StretchMode = VERTICAL_STRETCH_4_POINTS;
		}
		else if (m_StretchRect.top == 0.0f)
		{
			// 从顶部开始拉伸，使用6点网格（4个三角形）
			m_StretchMode = VERTICAL_STRETCH_TOP_6_POINTS;
		}
		else if (m_StretchRect.bottom == 1.0f)
		{
			// 拉伸到底部，使用6点网格（4个三角形）
			m_StretchMode = VERTICAL_STRETCH_BOTTOM_6_POINTS;
		}
		else
		{
			// 拉伸中间区域，使用8点网格（6个三角形）
			// 这是最常用的模式，可以保持上下部分不变，只拉伸中间
			m_StretchMode = VERTICAL_STRETCH_8_POINTS;
		}
	}
	else  // 水平拉伸模式
	{
		if (m_StretchRect.left == 0 && m_StretchRect.right == 1.0f)
		{
			m_StretchMode = HORIZONTAL_STRETCH_4_POINTS;
		}
		else if (m_StretchRect.left == 0.0f)
		{
			m_StretchMode = HORIZONTAL_STRETCH_LEFT_6_POINTS;
		}
		else if (m_StretchRect.right == 1.0f)
		{
			m_StretchMode = HORIZONTAL_STRETCH_RIGHT_6_POINTS;
		}
		else
		{
			m_StretchMode = HORIZONTAL_STRETCH_8_POINTS;
		}
	}

	// 动画控制：m_dt在[-0.2, 0.2]范围内往返变化
	// 这样可以实现拉伸效果的动态演示
	if (m_dt <= -0.2)
	{
		m_isgo = true;   // 开始正向增加
	}

	if (m_dt >= 0.2)
	{
		m_isgo = false;  // 开始反向减少
	}

	if (m_isgo)
	{
		m_dt += 0.01;  // 增加拉伸量
	}
	else
	{
		m_dt -= 0.01;  // 减少拉伸量
	}

	// 将纹理坐标[0,1]转换为NDC坐标[-1,1]
	float y1 = 1 - 2 * m_StretchRect.top;      // 拉伸区域上边界的NDC坐标
	float y2 = 1 - 2 * m_StretchRect.bottom;   // 拉伸区域下边界的NDC坐标
	float x1 = 2 * m_StretchRect.left - 1;     // 拉伸区域左边界的NDC坐标
	float x2 = 2 * m_StretchRect.right - 1;    // 拉伸区域右边界的NDC坐标

	// 计算图像宽高比，用于保持图像比例
	float wbl = m_RenderImage.width*1.0f / m_RenderImage.height;
	float hbl = 1 / wbl;
	if (wbl > hbl)
	{
		wbl = 1.0f;
	}
	else
	{
		hbl = 1.0f;
	}

	/**垂直拉伸的顶点坐标（用于最终显示到屏幕）
	 * 这4个顶点定义了显示矩形，通过m_dt控制上下拉伸
	 * */
	GLfloat vVertices[] = {
			-0.8f * wbl,   0.8f* hbl + m_dt*0.8f, 0.0f,  // 左上角（向上拉伸）
			-0.8f * wbl,  -0.8f* hbl - m_dt*0.8f, 0.0f,  // 左下角（向下拉伸）
			 0.8f * wbl,  -0.8f* hbl - m_dt*0.8f, 0.0f,  // 右下角（向下拉伸）
			 0.8f * wbl,   0.8f* hbl + m_dt*0.8f, 0.0f,  // 右上角（向上拉伸）
	};

	/**水平拉伸的顶点坐标（用于最终显示到屏幕）
	 * 这4个顶点定义了显示矩形，通过m_dt控制左右拉伸
	 * */
	GLfloat vHVertices[] = {
			-0.8f * wbl  - m_dt*0.8f,   0.8f* hbl, 0.0f,  // 左上角（向左拉伸）
			-0.8f * wbl  - m_dt*0.8f,  -0.8f* hbl, 0.0f,  // 左下角（向左拉伸）
			 0.8f * wbl  + m_dt*0.8f,  -0.8f* hbl, 0.0f,  // 右下角（向右拉伸）
			 0.8f * wbl  + m_dt*0.8f,   0.8f* hbl, 0.0f,  // 右上角（向右拉伸）
	};
//	GLfloat vHVertices[] = {
//			-0.8f   - m_dt*0.8f,   0.8f, 0.0f,
//			-0.8f   - m_dt*0.8f,  -0.8f, 0.0f,
//			0.8f  + m_dt*0.8f,  -0.8f, 0.0f,
//			0.8f  + m_dt*0.8f,   0.8f, 0.0f,
//	};

	// 标准纹理坐标（用于最终显示）
	GLfloat vTexCoors[] = {
			0.0f, 0.0f,  // 左下
			0.0f, 1.0f,  // 左上
			1.0f, 1.0f,  // 右上
			1.0f, 0.0f,  // 右下
	};


	/**垂直8点网格的顶点坐标（用于FBO离屏渲染）
	 * 8个顶点将图像分成3个区域：
	 * - 顶部4个顶点（0-3）：不拉伸区域
	 * - 中部：拉伸区域（通过调整y1和y2实现）
	 * - 底部4个顶点（4-7）：不拉伸区域
	 * m_dt控制拉伸量，正值向外拉伸，负值向内压缩
	 * */
	GLfloat vEightPointsFboVertices[] = {
			-1.0f,  1.0f, 0.0f,              // 顶点0：左上角（固定）
			-1.0f,  y1 + m_dt, 0.0f,         // 顶点1：拉伸区域上边界左侧（动态）
			 1.0f,  y1 + m_dt, 0.0f,         // 顶点2：拉伸区域上边界右侧（动态）
			 1.0f,  1.0f, 0.0f,              // 顶点3：右上角（固定）

			-1.0f,  y2 - m_dt, 0.0f,         // 顶点4：拉伸区域下边界左侧（动态）
			-1.0f, -1.0f , 0.0f,             // 顶点5：左下角（固定）
			 1.0f, -1.0f, 0.0f,              // 顶点6：右下角（固定）
			 1.0f,  y2 - m_dt, 0.0f,         // 顶点7：拉伸区域下边界右侧（动态）
	};

	/**垂直8点网格的纹理坐标（用于FBO离屏渲染）
	 * 纹理坐标对应顶点坐标，确保正确的纹理映射
	 * 关键点：y1和y2位置的纹理坐标保持不变，只改变顶点位置，实现拉伸效果
	 * */
	GLfloat vEightPointsFboTexCoors[] = {
			0.0f, 0.0f,                      // 对应顶点0
			0.0f, m_StretchRect.top,         // 对应顶点1（拉伸区域上边界）
			1.0f, m_StretchRect.top,         // 对应顶点2
			1.0f, 0.0f,                      // 对应顶点3

			0.0f, m_StretchRect.bottom,      // 对应顶点4（拉伸区域下边界）
			0.0f, 1.0f,                      // 对应顶点5
			1.0f, 1.0f,                      // 对应顶点6
			1.0f, m_StretchRect.bottom,      // 对应顶点7
	};

	/** 8 points horizontal*/
	GLfloat vHEightPointsFboVertices[] = {
			-1.0f,       1.0f, 0.0f,
			-1.0f,      -1.0f, 0.0f,
			x1 - m_dt,  -1.0f, 0.0f,
			x1 - m_dt,   1.0f, 0.0f,

			x2 + m_dt,   1.0f, 0.0f,
			x2 + m_dt,  -1.0f, 0.0f,
			 1.0f,      -1.0f, 0.0f,
			 1.0f,       1.0f, 0.0f,
	};

	//fbo 纹理坐标
	GLfloat vHEightPointsFboTexCoors[] = {
			0.0f,               0.0f,
			0.0f,               1.0f,
			m_StretchRect.left, 1.0f,
			m_StretchRect.left, 0.0f,

			m_StretchRect.right, 0.0f,
			m_StretchRect.right, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,
	};


	/** 6 points vertical top == 0 **/
	GLfloat vSixPointsTopFboVertices[] = {
			-1.0f,  1.0f, 0.0f,
			-1.0f,  y2 - 2*m_dt, 0.0f,
			1.0f,   y2 - 2*m_dt, 0.0f,
			1.0f,  1.0f, 0.0f,

			-1.0f, -1.0f , 0.0f,
			1.0f, -1.0f, 0.0f,

	};

	//fbo 纹理坐标
	GLfloat vSixPointsTopFboTexCoors[] = {
			0.0f, 0.0f,
			0.0f, m_StretchRect.bottom,
			1.0f, m_StretchRect.bottom,
			1.0f, 0.0f,

			0.0f, 1.0f,
			1.0f, 1.0f,
	};

	/** 6 points horizontal left == 0 **/
	GLfloat vHSixPointsTopFboVertices[] = {
			-1.0f,          1.0f, 0.0f,
			-1.0f,         -1.0f, 0.0f,
			 x2 + 2*m_dt,  -1.0f, 0.0f,
			 x2 + 2*m_dt,   1.0f, 0.0f,

			1.0f,           1.0f, 0.0f,
			1.0f,          -1.0f, 0.0f,

	};

	//fbo 纹理坐标
	GLfloat vHSixPointsTopFboTexCoors[] = {
			0.0f,                0.0f,
			0.0f,                1.0f,
			m_StretchRect.right, 1.0f,
			m_StretchRect.right, 0.0f,

			1.0f, 0.0f,
			1.0f, 1.0f,
	};

	/** 6 points vertical bottom == height*/
	GLfloat vSixPointsBottomFboVertices[] = {
			-1.0f,  1.0f, 0.0f,
			-1.0f,  y1 + 2*m_dt, 0.0f,
			1.0f,   y1 + 2*m_dt, 0.0f,
			1.0f,   1.0f, 0.0f,

			-1.0f, -1.0f , 0.0f,
			1.0f, -1.0f, 0.0f,
	};

	//fbo 纹理坐标
	GLfloat vSixPointsBottomFboTexCoors[] = {
			0.0f, 0.0f,
			0.0f, m_StretchRect.top,
			1.0f, m_StretchRect.top,
			1.0f, 0.0f,

			0.0f, 1.0f,
			1.0f, 1.0f,
	};

	/** 6 points horizontal right == width*/
	GLfloat vHSixPointsBottomFboVertices[] = {
			-1.0f,  1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f,
			 x1 - 2*m_dt,   -1.0f, 0.0f,
			 x1 - 2*m_dt,    1.0f, 0.0f,

			 1.0f, 1.0f , 0.0f,
			 1.0f, -1.0f, 0.0f,
	};

	//fbo 纹理坐标
	GLfloat vHSixPointsBottomFboTexCoors[] = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			m_StretchRect.left, 1.0f,
			m_StretchRect.left, 0.0f,

			1.0f, 0.0f,
			1.0f, 1.0f,
	};

	/**4 points vertical top == 0 && bottom == height, horizontal left == 0 && right == width*/
	GLfloat vFourPointsFboVertices[] = {
			-1.0f,  1.0f, 0.0f,
			-1.0f,  -1.0f, 0.0f,
			 1.0f,  -1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,

	};

	//fbo 纹理坐标
	GLfloat vFourPointsFboTexCoors[] = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,
	};


	if(m_FboProgramObj) {
		if (m_bIsVerticalMode)
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices);
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHVertices), vHVertices);
		}


		switch (m_StretchMode)
		{
			case VERTICAL_STRETCH_8_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vEightPointsFboVertices), vEightPointsFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vEightPointsFboTexCoors), vEightPointsFboTexCoors);
				break;
			case VERTICAL_STRETCH_TOP_6_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vSixPointsTopFboVertices), vSixPointsTopFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vSixPointsTopFboTexCoors), vSixPointsTopFboTexCoors);
				break;
			case VERTICAL_STRETCH_BOTTOM_6_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vSixPointsBottomFboVertices), vSixPointsBottomFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vSixPointsBottomFboTexCoors), vSixPointsBottomFboTexCoors);
				break;
			case VERTICAL_STRETCH_4_POINTS:
			case HORIZONTAL_STRETCH_4_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vFourPointsFboVertices), vFourPointsFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vFourPointsFboTexCoors), vFourPointsFboTexCoors);
				break;
			case HORIZONTAL_STRETCH_8_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHEightPointsFboVertices), vHEightPointsFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHEightPointsFboTexCoors), vHEightPointsFboTexCoors);
				break;
			case HORIZONTAL_STRETCH_LEFT_6_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHSixPointsTopFboVertices), vHSixPointsTopFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHSixPointsTopFboTexCoors), vHSixPointsTopFboTexCoors);
				break;
			case HORIZONTAL_STRETCH_RIGHT_6_POINTS:
				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHSixPointsBottomFboVertices), vHSixPointsBottomFboVertices);

				glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vHSixPointsBottomFboTexCoors), vHSixPointsBottomFboTexCoors);
				break;
			default:
				LOGCATE("FBOLegLengthenSample::Init don't support this mode");
				return;
		}


		if (m_FboTextureId)
		{
			glDeleteTextures(1, &m_FboTextureId);
		}

		glGenTextures(1, &m_FboTextureId);
		glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
		glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FboTextureId, 0);
		if (m_bIsVerticalMode)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width,
						 static_cast<GLsizei>(m_RenderImage.height * (1 + 2*m_dt)), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
						 static_cast<GLsizei>(m_RenderImage.width * (1 + 2 * m_dt)),
						 m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
			LOGCATE("FBOLegLengthenSample::Init glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
		}
		glBindTexture(GL_TEXTURE_2D, GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
		return;
	}

	// Model matrix vertical mirror
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(1.0f, -1.0f, 1.0f));
	//Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
	Model = glm::rotate(Model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

	m_FboMVPMatrix = Model;

	float ratio = 1080 / 1950.f;
	glm::mat4 Projection = glm::ortho(-ratio, ratio, -1.0f, 1.0f, 0.0f, 1.0f);
	//glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
	//glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 0.1f, 100.0f);
	//glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

	// View matrix
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 1), // Camera is at (0,0,1), in World Space
			glm::vec3(0, 0, 0), // and looks at the origin
			glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix
	Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(1.0f, 1.0f, 1.0f));
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

	m_MVPMatrix = Projection * View * Model;


	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);

	m_FboProgramObj = GLUtils::CreateProgram(vFboShaderStr, fFboShaderStr, m_FboVertexShader, m_FboFragmentShader);

	if (m_ProgramObj == GL_NONE || m_FboProgramObj == GL_NONE)
	{
		LOGCATE("FBOLegLengthenSample::Init m_ProgramObj == GL_NONE");
		return;
	}
	m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_TextureMap");
	m_FboSamplerLoc = glGetUniformLocation(m_FboProgramObj, "s_TextureMap");

	m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
	m_FboMVPMatLoc = glGetUniformLocation(m_FboProgramObj, "u_MVPMatrix");

	// Generate VBO Ids and load the VBOs with data
	if (m_bIsVerticalMode)
	{
		glGenBuffers(6, m_VboIds);
		glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_DYNAMIC_DRAW);
	}
	else
	{
		glGenBuffers(6, m_VboIds);
		glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vHVertices), vHVertices, GL_DYNAMIC_DRAW);
	}


	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vTexCoors), vTexCoors, GL_STATIC_DRAW);

	switch (m_StretchMode)
	{
		case VERTICAL_STRETCH_8_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vEightPointsFboTexCoors), vEightPointsFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(V_EIGHT_POINT_INDICES), V_EIGHT_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vEightPointsFboVertices), vEightPointsFboVertices, GL_DYNAMIC_DRAW);

			break;
		case VERTICAL_STRETCH_TOP_6_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vSixPointsTopFboTexCoors), vSixPointsTopFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(V_SIX_POINT_INDICES), V_SIX_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vSixPointsTopFboVertices), vSixPointsTopFboVertices, GL_DYNAMIC_DRAW);

			break;
		case VERTICAL_STRETCH_BOTTOM_6_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vSixPointsBottomFboTexCoors), vSixPointsBottomFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(V_SIX_POINT_INDICES), V_SIX_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vSixPointsBottomFboVertices), vSixPointsBottomFboVertices, GL_DYNAMIC_DRAW);
			break;
		case VERTICAL_STRETCH_4_POINTS:
		case HORIZONTAL_STRETCH_4_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vFourPointsFboTexCoors), vFourPointsFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(FOUR_POINT_INDICES), FOUR_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vFourPointsFboVertices), vFourPointsFboVertices, GL_DYNAMIC_DRAW);
			break;
		case HORIZONTAL_STRETCH_8_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vHEightPointsFboTexCoors), vHEightPointsFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(H_EIGHT_POINT_INDICES), H_EIGHT_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vHEightPointsFboVertices), vHEightPointsFboVertices, GL_DYNAMIC_DRAW);

			break;
		case HORIZONTAL_STRETCH_LEFT_6_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vHSixPointsTopFboTexCoors), vHSixPointsTopFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(H_SIX_POINT_INDICES), H_SIX_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vHSixPointsTopFboVertices), vHSixPointsTopFboVertices, GL_DYNAMIC_DRAW);

			break;
		case HORIZONTAL_STRETCH_RIGHT_6_POINTS:
			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vHSixPointsBottomFboTexCoors), vHSixPointsBottomFboTexCoors, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[3]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(H_SIX_POINT_INDICES), H_SIX_POINT_INDICES, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vHSixPointsBottomFboVertices), vHSixPointsBottomFboVertices, GL_DYNAMIC_DRAW);
			break;

		default:
			LOGCATE("FBOLegLengthenSample::Init don't support this mode");
			return;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[5]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(FOUR_POINT_INDICES), FOUR_POINT_INDICES, GL_STATIC_DRAW);

	GO_CHECK_GL_ERROR();

	// Generate VAO Ids
	glGenVertexArrays(2, m_VaoIds);

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

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[5]);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(GL_NONE);


	// FBO off screen rendering VAO
	glBindVertexArray(m_VaoIds[1]);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[4]);
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
		LOGCATE("FBOLegLengthenSample::Init CreateFrameBufferObj fail");
		return;
	}
}

void FBOLegLengthenSample::Draw(int screenW, int screenH)
{
	LOGCATE("FBOLegLengthenSample::Draw [screenW, screenH] = [%d, %d]", screenW, screenH);
	//纹理就是一个“可以被采样的复杂的数据集合” 纹理作为 GPU 图像数据结构
	//glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	if (m_bIsVerticalMode)
	{
		glViewport(0, 0, static_cast<GLsizei>(m_RenderImage.width),
				   static_cast<GLsizei>(m_RenderImage.height*(1+2*m_dt)));
	}
	else
	{
		glViewport(0, 0, static_cast<GLsizei>(m_RenderImage.width*(1+2*m_dt)),
				   static_cast<GLsizei>(m_RenderImage.height));
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Do FBO off screen rendering
	glUseProgram(m_FboProgramObj);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);

	glBindVertexArray(m_VaoIds[1]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ImageTextureId);
	glUniform1i(m_FboSamplerLoc, 0);
	glUniformMatrix4fv(m_FboMVPMatLoc, 1, GL_FALSE, &m_FboMVPMatrix[0][0]);
	GO_CHECK_GL_ERROR();
	GLsizei indicesNum;
	switch (m_StretchMode)
	{
		case VERTICAL_STRETCH_8_POINTS:
		case HORIZONTAL_STRETCH_8_POINTS:
			indicesNum = sizeof(V_EIGHT_POINT_INDICES) / sizeof(GLushort);
			break;
		case VERTICAL_STRETCH_TOP_6_POINTS:
		case VERTICAL_STRETCH_BOTTOM_6_POINTS:
		case HORIZONTAL_STRETCH_LEFT_6_POINTS:
		case HORIZONTAL_STRETCH_RIGHT_6_POINTS:
			indicesNum = sizeof(V_SIX_POINT_INDICES) / sizeof(GLushort);
			break;
		case VERTICAL_STRETCH_4_POINTS:
		case HORIZONTAL_STRETCH_4_POINTS:
			indicesNum = sizeof(FOUR_POINT_INDICES) / sizeof(GLushort);
			break;
		default:
			break;
	}
	glDrawElements(GL_TRIANGLES, indicesNum, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, screenW, screenH);
	// Do normal rendering
	glUseProgram(m_ProgramObj);
	GO_CHECK_GL_ERROR();
	glBindVertexArray(m_VaoIds[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glUniform1i(m_SamplerLoc, 0);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
	GO_CHECK_GL_ERROR();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
	GO_CHECK_GL_ERROR();
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindVertexArray(GL_NONE);

//	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
//	NativeImage nativeImage = m_RenderImage;
//	nativeImage.format = IMAGE_FORMAT_RGBA;
//	nativeImage.height = static_cast<int>(m_RenderImage.height * (1 + 2 * m_dt));
//	uint8_t *pBuffer = new uint8_t[nativeImage.width * nativeImage.height * 4];
//
//	nativeImage.ppPlane[0] = pBuffer;
//	FUN_BEGIN_TIME("FBO glReadPixels")
//		glReadPixels(0, 0, nativeImage.width, nativeImage.height, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
//	FUN_END_TIME("FBO cost glReadPixels")
//
//	NativeImageUtil::DumpNativeImage(&nativeImage, "/sdcard/DCIM", "NDK");
//	delete []pBuffer;
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void FBOLegLengthenSample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);
	}

	if (m_FboProgramObj)
	{
		glDeleteProgram(m_FboProgramObj);
	}

	if (m_ImageTextureId)
	{
		glDeleteTextures(1, &m_ImageTextureId);
	}

	if (m_FboTextureId)
	{
		glDeleteTextures(1, &m_FboTextureId);
	}

	if (m_VboIds[0])
	{
		glDeleteBuffers(6, m_VboIds);
	}

	if (m_VaoIds[0])
	{
		glDeleteVertexArrays(2, m_VaoIds);
	}

	if (m_FboId)
	{
		glDeleteFramebuffers(1, &m_FboId);
	}

}

bool FBOLegLengthenSample::CreateFrameBufferObj()
{
	glGenTextures(1, &m_FboTextureId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	glGenFramebuffers(1, &m_FboId);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
	glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FboTextureId, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
		LOGCATE("FBOLegLengthenSample::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
		return false;
	}
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	return true;

}
