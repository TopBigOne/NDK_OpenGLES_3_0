/**
 * 文本渲染示例
 * 使用 FreeType 库实现文本渲染，支持 ASCII 字符和 Unicode 字符（中文）
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "TextRenderSample.h"
#include "../util/GLUtils.h"

// 要渲染的中文文本内容
static const wchar_t BYTE_FLOW[] = L"微信公众号字节流动，欢迎关注交流学习。";
// FreeType 字形推进值转换常量（2^16 = 65536）
static const int MAX_SHORT_VALUE = 65536;

/**
 * 构造函数
 * 初始化所有成员变量为默认值
 */
TextRenderSample::TextRenderSample()
{
	// 采样器 uniform 位置
	m_SamplerLoc = GL_NONE;
	// MVP 矩阵 uniform 位置
	m_MVPMatLoc = GL_NONE;

	// 纹理 ID
	m_TextureId = GL_NONE;
	// 顶点数组对象 ID
	m_VaoId = GL_NONE;
	// 顶点缓冲对象 ID
	m_VboId = GL_NONE;

	// 绕 X 轴旋转角度
	m_AngleX = 0;
	// 绕 Y 轴旋转角度
	m_AngleY = 0;

	// X 轴缩放系数
	m_ScaleX = 1.0f;
	// Y 轴缩放系数
	m_ScaleY = 1.0f;
}

/**
 * 析构函数
 * 释放图像资源
 */
TextRenderSample::~TextRenderSample()
{
	// 释放渲染图像内存
	NativeImageUtil::FreeNativeImage(&m_RenderImage);
}

/**
 * 初始化方法
 * 加载字体、创建着色器程序、设置 OpenGL 状态
 */
void TextRenderSample::Init()
{
	// 防止重复初始化
	if(m_ProgramObj)
		return;

	// 加载 ASCII 字符字形（英文字符）
	LoadFacesByASCII();

	// 加载 Unicode 字符字形（中文字符）
	LoadFacesByUnicode(BYTE_FLOW, sizeof(BYTE_FLOW)/sizeof(BYTE_FLOW[0]) - 1);

	// 创建 RGBA 纹理用于字形渲染
	glGenTextures(1, &m_TextureId);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 顶点着色器
	// 输入：4分量顶点属性 (x, y, texX, texY)
	// 输出：变换后的位置和纹理坐标
	char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;// <vec2 pos, vec2 tex>\n"
            "uniform mat4 u_MVPMatrix;\n"
            "out vec2 v_texCoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = u_MVPMatrix * vec4(a_position.xy, 0.0, 1.0);;\n"
            "    v_texCoord = a_position.zw;\n"
            "}";

	// 片段着色器
	// FreeType 生成的字形纹理是单通道（红色通道）
	// 将单通道值作为 alpha 通道，与指定的文本颜色混合
	char fShaderStr[] =
			"#version 300 es\n"
            "precision mediump float;\n"
            "in vec2 v_texCoord;\n"
            "layout(location = 0) out vec4 outColor;\n"
            "uniform sampler2D s_textTexture;\n"
            "uniform vec3 u_textColor;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    vec4 color = vec4(1.0, 1.0, 1.0, texture(s_textTexture, v_texCoord).r);\n"
            "    outColor = vec4(u_textColor, 1.0) * color;\n"
            "}";

	// 编译链接着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
	if (m_ProgramObj)
	{
		// 获取 uniform 变量位置
		m_SamplerLoc = glGetUniformLocation(m_ProgramObj, "s_textTexture");
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
	}
	else
	{
		LOGCATE("TextRenderSample::Init create program fail");
	}

	// 生成顶点数组对象 VAO
	glGenVertexArrays(1, &m_VaoId);
	// 生成顶点缓冲对象 VBO
	glGenBuffers(1, &m_VboId);

	// 配置 VAO/VBO
	// 每个字符用 6 个顶点（2个三角形），每个顶点 4 个浮点数
	glBindVertexArray(m_VaoId);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    glBindVertexArray(GL_NONE);

	// 上传 RGBA 图像数据（如果有背景图）
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

}

/**
 * 加载图像数据（可选，用于背景）
 * @param pImage 图像数据指针
 */
void TextRenderSample::LoadImage(NativeImage *pImage)
{
	LOGCATE("TextRenderSample::LoadImage pImage = %p", pImage->ppPlane[0]);
	if (pImage)
	{
		// 复制图像尺寸和格式信息
		m_RenderImage.width = pImage->width;
		m_RenderImage.height = pImage->height;
		m_RenderImage.format = pImage->format;
		// 深拷贝图像数据
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImage);
	}
}

/**
 * 绘制方法
 * 渲染文本到屏幕
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void TextRenderSample::Draw(int screenW, int screenH)
{
	m_SurfaceWidth = screenW;
	m_SurfaceHeight = screenH;
	LOGCATE("TextRenderSample::Draw()");
	if(m_ProgramObj == GL_NONE) return;

	// 设置清屏颜色为白色
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// 禁用字节对齐限制（FreeType 生成的纹理不一定是 4 字节对齐）
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// 启用混合模式用于文本渲染
	glEnable(GL_BLEND);
	// 设置混合函数（标准 alpha 混合）
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::vec2 viewport(m_SurfaceWidth, m_SurfaceHeight);

	// 更新 MVP 矩阵
	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, viewport.x / viewport.y);
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);

	// 渲染英文文本
	// (x,y) 为归一化设备坐标，原点在屏幕中心，范围 [-1.0, 1.0]
	RenderText("My WeChat ID is Byte-Flow.", -0.9f, 0.2f, 1.0f, glm::vec3(0.8, 0.1f, 0.1f), viewport);
	RenderText("Welcome to add my WeChat.", -0.9f, 0.0f, 2.0f, glm::vec3(0.2, 0.4f, 0.7f), viewport);

	// 渲染中文文本
	RenderText(BYTE_FLOW, sizeof(BYTE_FLOW)/sizeof(BYTE_FLOW[0]) - 1, -0.9f, -0.2f, 1.0f, glm::vec3(0.7, 0.4f, 0.2f), viewport);
}

void TextRenderSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}

/**
 * 渲染 ASCII 文本
 * @param text 要渲染的文本字符串
 * @param x 起始 x 坐标（归一化坐标）
 * @param y 起始 y 坐标（归一化坐标）
 * @param scale 文本缩放系数
 * @param color 文本颜色（RGB）
 * @param viewport 视口尺寸
 */
void TextRenderSample::RenderText(std::string text, GLfloat x, GLfloat y, GLfloat scale,
                                  glm::vec3 color, glm::vec2 viewport) {
	// 使用着色器程序
	glUseProgram(m_ProgramObj);
	// 设置文本颜色
	glUniform3f(glGetUniformLocation(m_ProgramObj, "u_textColor"), color.x, color.y, color.z);
	glBindVertexArray(m_VaoId);
	GO_CHECK_GL_ERROR();

	// 将归一化坐标转换为像素坐标
	x *= viewport.x;
	y *= viewport.y;

	// 遍历文本中的所有字符
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		// 获取字符对应的字形信息
		Character ch = m_Characters[*c];

		// 计算字符的渲染位置
		// bearing.x: 字形左侧到基线的水平距离
		GLfloat xpos = x + ch.bearing.x * scale;
		// bearing.y: 字形顶部到基线的垂直距离
		GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

		// 转换回归一化坐标
		xpos /= viewport.x;
		ypos /= viewport.y;

		// 计算字符的宽度和高度
		GLfloat w = ch.size.x * scale;
		GLfloat h = ch.size.y * scale;

		// 转换为归一化尺寸
		w /= viewport.x;
		h /= viewport.y;

		LOGCATE("TextRenderSample::RenderText [xpos,ypos,w,h]=[%f, %f, %f, %f], ch.advance >> 6 = %d", xpos, ypos, w, h, ch.advance >> 6);

		// 构建当前字符的顶点数据（2个三角形组成矩形）
		// 每个顶点包含：位置坐标(x,y) + 纹理坐标(s,t)
		GLfloat vertices[6][4] = {
				{ xpos,     ypos + h,   0.0, 0.0 },  // 左上
				{ xpos,     ypos,       0.0, 1.0 },  // 左下
				{ xpos + w, ypos,       1.0, 1.0 },  // 右下

				{ xpos,     ypos + h,   0.0, 0.0 },  // 左上
				{ xpos + w, ypos,       1.0, 1.0 },  // 右下
				{ xpos + w, ypos + h,   1.0, 0.0 }   // 右上
		};

		// 绑定字形纹理
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		glUniform1i(m_SamplerLoc, 0);
		GO_CHECK_GL_ERROR();

		// 更新 VBO 数据为当前字符的顶点
		glBindBuffer(GL_ARRAY_BUFFER, m_VboId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        GO_CHECK_GL_ERROR();
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// 绘制字符矩形
		glDrawArrays(GL_TRIANGLES, 0, 6);
		GO_CHECK_GL_ERROR();

		// 移动到下一个字符的位置
		// advance 单位是 1/64 像素，右移 6 位相当于除以 64
		x += (ch.advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


/**
 * 加载 ASCII 字符集的字形
 * 使用 FreeType 库加载前 128 个 ASCII 字符
 */
void TextRenderSample::LoadFacesByASCII() {
	// 初始化 FreeType 库
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		LOGCATE("TextRenderSample::LoadFacesByASCII FREETYPE: Could not init FreeType Library");

	// 加载字体文件
	FT_Face face;
    std::string path(DEFAULT_OGL_ASSETS_DIR);
	if (FT_New_Face(ft, (path + "/fonts/Antonio-Regular.ttf").c_str(), 0, &face))
		LOGCATE("TextRenderSample::LoadFacesByASCII FREETYPE: Failed to load font");

	// 设置字形大小（宽度设为 0 表示根据高度自动计算）
	FT_Set_Pixel_Sizes(face, 0, 96);

	// 禁用字节对齐限制
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 加载 ASCII 字符集的前 128 个字符
	for (unsigned char c = 0; c < 128; c++)
	{
		// 加载字符字形并渲染为位图
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
		LOGCATE("TextRenderSample::LoadFacesByASCII FREETYTPE: Failed to load Glyph");
			continue;
		}

		// 为每个字符生成纹理
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		// 上传单通道位图数据（GL_LUMINANCE）
		glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_LUMINANCE,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_LUMINANCE,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
		);
        LOGCATE("TextRenderSample::LoadFacesByASCII [w,h,buffer]=[%d, %d, %p], ch.advance >> 6 = %ld", face->glyph->bitmap.width,face->glyph->bitmap.rows, face->glyph->bitmap.buffer,face->glyph->advance.x >> 6);

		// 设置纹理参数
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// 存储字符信息供后续渲染使用
		Character character = {
				texture,                                                    // 纹理 ID
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),  // 字形尺寸
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),    // 字形偏移
				static_cast<GLuint>(face->glyph->advance.x)                 // 水平推进值
		};
		m_Characters.insert(std::pair<GLint, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// 清理 FreeType 资源
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

/**
 * 加载 Unicode 字符集的字形（用于中文等非ASCII字符）
 * @param text Unicode 字符数组
 * @param size 字符数量
 */
void TextRenderSample::LoadFacesByUnicode(const wchar_t* text, int size) {
	// 初始化 FreeType 库
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		LOGCATE("TextRenderSample::LoadFacesByUnicode FREETYPE: Could not init FreeType Library");

	// 加载支持中文的字体文件（微软雅黑）
	FT_Face face;
    std::string path(DEFAULT_OGL_ASSETS_DIR);
    if (FT_New_Face(ft, (path + "/fonts/msyh.ttc").c_str(), 0, &face))
		LOGCATE("TextRenderSample::LoadFacesByUnicode FREETYPE: Failed to load font");

	// 设置字形大小
	FT_Set_Pixel_Sizes(face, 96, 96);
	// 选择 Unicode 字符映射
	FT_Select_Charmap(face, ft_encoding_unicode);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 遍历所有需要加载的 Unicode 字符
	for (int i = 0; i < size; ++i) {
		// 根据 Unicode 值获取字形索引并加载
		if (FT_Load_Glyph(face, FT_Get_Char_Index(face, text[i]), FT_LOAD_DEFAULT))
		{
			LOGCATE("TextRenderSample::LoadFacesByUnicode FREETYTPE: Failed to load Glyph");
			continue;
		}

		// 获取字形数据
		FT_Glyph glyph;
		FT_Get_Glyph(face->glyph, &glyph );

		// 将字形转换为位图
		FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1 );
		FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

		// 获取位图数据的引用
		FT_Bitmap& bitmap = bitmap_glyph->bitmap;

		// 为字符生成纹理
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		// 上传位图数据到纹理
		glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_LUMINANCE,
				bitmap.width,
				bitmap.rows,
				0,
				GL_LUMINANCE,
				GL_UNSIGNED_BYTE,
				bitmap.buffer
		);

		LOGCATE("TextRenderSample::LoadFacesByUnicode text[i]=%d [w,h,buffer]=[%d, %d, %p], advance.x=%ld", text[i], bitmap.width, bitmap.rows, bitmap.buffer, glyph->advance.x / MAX_SHORT_VALUE);

		// 设置纹理参数
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// 存储字符信息
		// 注意：Unicode 字符的推进值需要特殊处理
		Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				static_cast<GLuint>((glyph->advance.x / MAX_SHORT_VALUE) << 6)  // 转换推进值单位
		};
		m_Characters.insert(std::pair<GLint, Character>(text[i], character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// 清理 FreeType 资源
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

/**
 * 销毁资源
 * 释放 OpenGL 对象和字符纹理
 */
void TextRenderSample::Destroy()
{
	if (m_ProgramObj)
	{
		// 删除着色器程序
		glDeleteProgram(m_ProgramObj);
		// 删除 VBO
		glDeleteBuffers(1, &m_VboId);
		// 删除 VAO
		glDeleteVertexArrays(1, &m_VaoId);
		// 删除背景纹理
		glDeleteTextures(1, &m_TextureId);

		// 删除所有字符纹理
		std::map<GLint, Character>::const_iterator iter;
		for (iter = m_Characters.begin(); iter != m_Characters.end(); iter++)
		{
			glDeleteTextures(1, &m_Characters[iter->first].textureID);
		}
	}
}

/**
 * @param angleX 绕X轴旋转度数
 * @param angleY 绕Y轴旋转度数
 * @param ratio 宽高比
 * */
void TextRenderSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("TextRenderSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	angleX = angleX % 360;
	angleY = angleY % 360;

	//转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


	// Projection matrix
	glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
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

/**
 * 渲染 Unicode 文本（中文等）
 * @param text Unicode 字符数组
 * @param textLen 字符数量
 * @param x 起始 x 坐标（归一化坐标）
 * @param y 起始 y 坐标（归一化坐标）
 * @param scale 文本缩放系数
 * @param color 文本颜色（RGB）
 * @param viewport 视口尺寸
 */
void TextRenderSample::RenderText(const wchar_t *text, int textLen, GLfloat x, GLfloat y, GLfloat scale,
								  glm::vec3 color, glm::vec2 viewport) {
	// 使用着色器程序
	glUseProgram(m_ProgramObj);
	// 设置文本颜色
	glUniform3f(glGetUniformLocation(m_ProgramObj, "u_textColor"), color.x, color.y, color.z);
	glBindVertexArray(m_VaoId);
	GO_CHECK_GL_ERROR();

	// 将归一化坐标转换为像素坐标
	x *= viewport.x;
	y *= viewport.y;

	// 遍历所有 Unicode 字符
	for (int i = 0; i < textLen; ++i)
	{
		// 获取字符对应的字形信息
		Character ch = m_Characters[text[i]];

		// 计算字符的渲染位置
		GLfloat xpos = x + ch.bearing.x * scale;
		GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

		// 转换回归一化坐标
		xpos /= viewport.x;
		ypos /= viewport.y;

		// 计算字符的宽度和高度
		GLfloat w = ch.size.x * scale;
		GLfloat h = ch.size.y * scale;

		// 转换为归一化尺寸
		w /= viewport.x;
		h /= viewport.y;

		LOGCATE("TextRenderSample::RenderText [xpos,ypos,w,h]=[%f, %f, %f, %f]", xpos, ypos, w, h);

		// 构建当前字符的顶点数据
		GLfloat vertices[6][4] = {
				{ xpos,     ypos + h,   0.0, 0.0 },
				{ xpos,     ypos,       0.0, 1.0 },
				{ xpos + w, ypos,       1.0, 1.0 },

				{ xpos,     ypos + h,   0.0, 0.0 },
				{ xpos + w, ypos,       1.0, 1.0 },
				{ xpos + w, ypos + h,   1.0, 0.0 }
		};

		// 绑定字形纹理
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		glUniform1i(m_SamplerLoc, 0);
		GO_CHECK_GL_ERROR();

		// 更新 VBO 数据
		glBindBuffer(GL_ARRAY_BUFFER, m_VboId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		GO_CHECK_GL_ERROR();
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// 绘制字符
		glDrawArrays(GL_TRIANGLES, 0, 6);
		GO_CHECK_GL_ERROR();

		// 移动到下一个字符位置
		x += (ch.advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
