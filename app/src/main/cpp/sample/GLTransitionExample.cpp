/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include "GLTransitionExample.h"
#include "../util/GLUtils.h"

/**
 * 构造函数：初始化转场特效示例 1（圆柱翻页效果）
 * 该示例实现了 3D 圆柱翻页转场效果，模拟真实书页翻转
 */
GLTransitionExample::GLTransitionExample()
{
	// MVP 矩阵的 uniform 变量位置
	m_MVPMatLoc = GL_NONE;

	// 初始化纹理 ID 数组
    for (int i = 0; i < BF_IMG_NUM; ++i) {
        m_TextureIds[i] = GL_NONE;
    }
	// 顶点数组对象 ID
	m_VaoId = GL_NONE;

	// X 轴和 Y 轴的旋转角度
	m_AngleX = 0;
	m_AngleY = 0;

	// X 轴和 Y 轴的缩放系数
	m_ScaleX = 1.0f;
	m_ScaleY = 1.0f;

	// 帧索引，用于控制转场动画进度
    m_frameIndex = 0;
	// 循环计数器，用于切换不同图片
	m_loopCount = 0;
}

/**
 * 析构函数：释放图片资源
 */
GLTransitionExample::~GLTransitionExample()
{
	// 释放所有渲染图片的内存
    for (int i = 0; i < BF_IMG_NUM; ++i) {
        NativeImageUtil::FreeNativeImage(&m_RenderImages[i]);
    }
}

/**
 * 初始化函数：创建着色器程序和 OpenGL 资源
 */
void GLTransitionExample::Init()
{
	// 如果着色器程序已创建，直接返回
	if(m_ProgramObj)
		return;

	// 为所有图片创建并配置纹理对象
    for (int i = 0; i < BF_IMG_NUM; ++i) {
        glGenTextures(1, &m_TextureIds[i]);  // 生成纹理 ID
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);  // 绑定纹理
        // 设置纹理环绕方式为边缘截取
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // 设置纹理过滤方式为线性插值
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);  // 解绑纹理
    }

	// 顶点着色器代码
	// 将顶点位置通过 MVP 矩阵变换到裁剪空间，并传递纹理坐标
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

	/**
	 * 片段着色器代码
	 * 实现 3D 圆柱翻页转场效果，模拟书页在圆柱面上的卷曲和翻转
	 * 主要原理：
	 * 1. 将平面映射到圆柱表面
	 * 2. 通过旋转矩阵模拟翻页动作
	 * 3. 计算背面、正面、阴影等多个层次的渲染
	 */
	char fShaderStr[] =
	        "#version 300 es\n"
			"precision mediump float;\n"
			"in vec2 v_texCoord;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"uniform sampler2D u_texture0;\n"  // 当前图片纹理
			"uniform sampler2D u_texture1;\n"  // 下一张图片纹理
			"uniform float u_offset;\n"        // 转场进度 [0, 1]
			"uniform vec2 u_texSize;\n"        // 纹理尺寸
			"\n"
			"const float MIN_AMOUNT = -0.16;\n"  // 最小转场量
			"const float MAX_AMOUNT = 1.5;\n"    // 最大转场量
			"\n"
			"const float PI = 3.141592653589793;\n"
			"\n"
			"const float scale = 512.0;\n"       // 抗锯齿缩放因子
			"const float sharpness = 3.0;\n"     // 边缘锐度
			"\n"
			"const float cylinderRadius = 1.0 / PI / 2.0;\n"  // 圆柱半径
			"\n"
			"float amount = 0.0;\n"          // 实际转场量
			"float cylinderCenter = 0.0;\n"  // 圆柱中心位置
			"float cylinderAngle = 0.0;\n"   // 圆柱旋转角度
			"\n"
			"// 计算圆柱面上的点击中点\n"
			"// hitAngle: 击中角度, yc: y坐标偏移, point: 原始点, rrotation: 反向旋转矩阵\n"
			"vec3 hitPoint(float hitAngle, float yc, vec3 point, mat3 rrotation)\n"
			"{\n"
			"    float hitPoint = hitAngle / (2.0 * PI);\n"
			"    point.y = hitPoint;\n"
			"    return rrotation * point;\n"  // 应用反向旋转获取纹理坐标
			"}\n"
			"\n"
			"// 抗锯齿混合函数\n"
			"// color1: 第一种颜色, color2: 第二种颜色, distanc: 距离\n"
			"vec4 antiAlias(vec4 color1, vec4 color2, float distanc)\n"
			"{\n"
			"    distanc *= scale;\n"  // 缩放距离以增强效果
			"    if (distanc < 0.0) return color2;\n"
			"    if (distanc > 2.0) return color1;\n"
			"    float dd = pow(1.0 - distanc / 2.0, sharpness);\n"  // 计算混合因子
			"    return ((color2 - color1) * dd) + color1;\n"
			"}\n"
			"\n"
			"// 计算点到边缘的距离（用于抗锯齿）\n"
			"float distanceToEdge(vec3 point)\n"
			"{\n"
			"    float dx = abs(point.x > 0.5 ? 1.0 - point.x : point.x);\n"
			"    float dy = abs(point.y > 0.5 ? 1.0 - point.y : point.y);\n"
			"    if (point.x < 0.0) dx = -point.x;\n"
			"    if (point.x > 1.0) dx = point.x - 1.0;\n"
			"    if (point.y < 0.0) dy = -point.y;\n"
			"    if (point.y > 1.0) dy = point.y - 1.0;\n"
			"    if ((point.x < 0.0 || point.x > 1.0) && (point.y < 0.0 || point.y > 1.0)) return sqrt(dx * dx + dy * dy);\n"
			"    return min(dx, dy);\n"
			"}\n"
			"\n"
			"// 穿透视图：当圆柱背面可见时显示下一张图片\n"
			"vec4 seeThrough(float yc, vec2 p, mat3 rotation, mat3 rrotation)\n"
			"{\n"
			"    float hitAngle = PI - (acos(yc / cylinderRadius) - cylinderAngle);\n"
			"    vec3 point = hitPoint(hitAngle, yc, rotation * vec3(p, 1.0), rrotation);\n"
			"    if (yc <= 0.0 && (point.x < 0.0 || point.y < 0.0 || point.x > 1.0 || point.y > 1.0))\n"
			"    {\n"
			"        return texture(u_texture1, p);\n"  // 显示下一张图片
			"    }\n"
			"\n"
			"    if (yc > 0.0) return texture(u_texture0, p);\n"  // 显示当前图片
			"\n"
			"    vec4 color = texture(u_texture0, point.xy);\n"
			"    vec4 tcolor = vec4(0.0);\n"
			"\n"
			"    return antiAlias(color, tcolor, distanceToEdge(point));\n"
			"}\n"
			"\n"
			"// 带阴影的穿透视图\n"
			"vec4 seeThroughWithShadow(float yc, vec2 p, vec3 point, mat3 rotation, mat3 rrotation)\n"
			"{\n"
			"    float shadow = distanceToEdge(point) * 30.0;\n"  // 计算阴影强度
			"    shadow = (1.0 - shadow) / 3.0;\n"
			"\n"
			"    if (shadow < 0.0) shadow = 0.0; else shadow *= amount;\n"
			"\n"
			"    vec4 shadowColor = seeThrough(yc, p, rotation, rrotation);\n"
			"    shadowColor.r -= shadow;\n"  // 应用阴影到 RGB 通道
			"    shadowColor.g -= shadow;\n"
			"    shadowColor.b -= shadow;\n"
			"\n"
			"    return shadowColor;\n"
			"}\n"
			"\n"
			"// 圆柱背面渲染：显示灰度化的背面\n"
			"vec4 backside(float yc, vec3 point)\n"
			"{\n"
			"    vec4 color = texture(u_texture0, point.xy);\n"
			"    float gray = (color.r + color.b + color.g) / 15.0;\n"  // 计算灰度值
			"    gray += (8.0 / 10.0) * (pow(1.0 - abs(yc / cylinderRadius), 2.0 / 10.0) / 2.0 + (5.0 / 10.0));\n"  // 添加光照效果
			"    color.rgb = vec3(gray);\n"
			"    return color;\n"
			"}\n"
			"\n"
			"// 圆柱背后的表面（显示下一张图片带阴影）\n"
			"vec4 behindSurface(vec2 p, float yc, vec3 point, mat3 rrotation)\n"
			"{\n"
			"    float shado = (1.0 - ((-cylinderRadius - yc) / amount * 7.0)) / 6.0;\n"  // 计算阴影\n"
			"    shado *= 1.0 - abs(point.x - 0.5);\n"
			"\n"
			"    yc = (-cylinderRadius - cylinderRadius - yc);\n"
			"\n"
			"    float hitAngle = (acos(yc / cylinderRadius) + cylinderAngle) - PI;\n"
			"    point = hitPoint(hitAngle, yc, point, rrotation);\n"
			"\n"
			"    if (yc < 0.0 && point.x >= 0.0 && point.y >= 0.0 && point.x <= 1.0 && point.y <= 1.0 && (hitAngle < PI || amount > 0.5))\n"
			"    {\n"
			"        shado = 1.0 - (sqrt(pow(point.x - 0.5, 2.0) + pow(point.y - 0.5, 2.0)) / (71.0 / 100.0));\n"
			"        shado *= pow(-yc / cylinderRadius, 3.0);\n"
			"        shado *= 0.5;\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        shado = 0.0;\n"
			"    }\n"
			"    return vec4(texture(u_texture1, p).rgb - shado, 1.0);\n"  // 应用阴影到下一张图片
			"}\n"
			"\n"
			"// 主转场函数：根据当前像素位置和转场进度计算最终颜色\n"
			"vec4 transition(vec2 p) {\n"
			"\n"
			"    const float angle = 100.0 * PI / 180.0;\n"  // 旋转角度 100 度
			"    float c = cos(-angle);\n"
			"    float s = sin(-angle);\n"
			"\n"
			"    // 构建旋转矩阵\n"
			"    mat3 rotation = mat3( c, s, 0,\n"
			"    -s, c, 0,\n"
			"    -0.801, 0.8900, 1\n"
			"    );\n"
			"    c = cos(angle);\n"
			"    s = sin(angle);\n"
			"\n"
			"    // 构建反向旋转矩阵\n"
			"    mat3 rrotation = mat3(\tc, s, 0,\n"
			"    -s, c, 0,\n"
			"    0.98500, 0.985, 1\n"
			"    );\n"
			"\n"
			"    vec3 point = rotation * vec3(p, 1.0);\n"  // 应用旋转变换
			"\n"
			"    float yc = point.y - cylinderCenter;\n"  // 计算相对圆柱中心的 y 坐标
			"\n"
			"    if (yc < -cylinderRadius)\n"
			"    {\n"
			"        // 在圆柱背后：显示下一张图片\n"
			"        return behindSurface(p,yc, point, rrotation);\n"
			"    }\n"
			"\n"
			"    if (yc > cylinderRadius)\n"
			"    {\n"
			"        // 在圆柱上方：显示当前图片的平面部分\n"
			"        return texture(u_texture0, p);\n"
			"    }\n"
			"\n"
			"    float hitAngle = (acos(yc / cylinderRadius) + cylinderAngle) - PI;\n"
			"\n"
			"    float hitAngleMod = mod(hitAngle, 2.0 * PI);\n"
			"    if ((hitAngleMod > PI && amount < 0.5) || (hitAngleMod > PI/2.0 && amount < 0.0))\n"
			"    {\n"
			"        return seeThrough(yc, p, rotation, rrotation);\n"
			"    }\n"
			"\n"
			"    point = hitPoint(hitAngle, yc, point, rrotation);\n"
			"\n"
			"    if (point.x < 0.0 || point.y < 0.0 || point.x > 1.0 || point.y > 1.0)\n"
			"    {\n"
			"        return seeThroughWithShadow(yc, p, point, rotation, rrotation);\n"
			"    }\n"
			"\n"
			"    vec4 color = backside(yc, point);\n"  // 渲染圆柱背面
			"\n"
			"    vec4 otherColor;\n"
			"    if (yc < 0.0)\n"
			"    {\n"
			"        // 计算并应用阴影\n"
			"        float shado = 1.0 - (sqrt(pow(point.x - 0.5, 2.0) + pow(point.y - 0.5, 2.0)) / 0.71);\n"
			"        shado *= pow(-yc / cylinderRadius, 3.0);\n"
			"        shado *= 0.5;\n"
			"        otherColor = vec4(0.0, 0.0, 0.0, shado);\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        otherColor = texture(u_texture0, p);\n"
			"    }\n"
			"\n"
			"    color = antiAlias(color, otherColor, cylinderRadius - abs(yc));\n"
			"\n"
			"    vec4 cl = seeThroughWithShadow(yc, p, point, rotation, rrotation);\n"
			"    float dist = distanceToEdge(point);\n"
			"\n"
			"    return antiAlias(color, cl, dist);\n"
			"}\n"
			"\n"
			"void main()\n"
			"{\n"
			"    // 根据进度计算转场量\n"
			"    amount = u_offset * (MAX_AMOUNT - MIN_AMOUNT) + MIN_AMOUNT;\n"
			"    cylinderCenter = amount;\n"
			"    cylinderAngle = 2.0 * PI * amount;\n"
			"\n"
			"    outColor = transition(v_texCoord);\n"
			"}";

	// 编译链接着色器程序
	m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);
	if (m_ProgramObj)
	{
		// 获取 MVP 矩阵的 uniform 位置
		m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
	}
	else
	{
		LOGCATE("GLTransitionExample::Init create program fail");
	}

	// 顶点坐标数组（NDC 坐标系，范围 [-1, 1]）
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // 左上角
			-1.0f, -1.0f, 0.0f,  // 左下角
			1.0f,  -1.0f, 0.0f,  // 右下角
			1.0f,   1.0f, 0.0f,  // 右上角
	};

	// 纹理坐标数组（纹理坐标系，范围 [0, 1]）
	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // 左上角对应纹理左上角
			0.0f,  1.0f,        // 左下角对应纹理左下角
			1.0f,  1.0f,        // 右下角对应纹理右下角
			1.0f,  0.0f         // 右上角对应纹理右上角
	};

	// 索引数组（定义两个三角形组成矩形）
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// 生成并加载 VBO 数据
	glGenBuffers(3, m_VboIds);
	// 加载顶点坐标到 VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

	// 加载纹理坐标到 VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	// 加载索引数据到 EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// 生成并配置 VAO
	glGenVertexArrays(1, &m_VaoId);
	glBindVertexArray(m_VaoId);

	// 设置顶点属性指针（位置属性）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	// 设置顶点属性指针（纹理坐标属性）
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);

	glBindVertexArray(GL_NONE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 设置像素存储对齐方式为 1 字节

	// 上传所有图片到纹理
    for (int i = 0; i < BF_IMG_NUM; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);  // 激活纹理单元
        glBindTexture(GL_TEXTURE_2D, m_TextureIds[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImages[i].width, m_RenderImages[i].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_RenderImages[i].ppPlane[0]);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }
}

/**
 * 加载图片（此示例未使用此函数）
 */
void GLTransitionExample::LoadImage(NativeImage *pImage)
{
	LOGCATE("GLTransitionExample::LoadImage pImage = %p", pImage->ppPlane[0]);
}

/**
 * 渲染函数：每帧调用，绘制转场动画
 * @param screenW 屏幕宽度
 * @param screenH 屏幕高度
 */
void GLTransitionExample::Draw(int screenW, int screenH)
{
	LOGCATE("GLTransitionExample::Draw()");

	// 检查着色器程序和纹理是否已初始化
	if(m_ProgramObj == GL_NONE || m_TextureIds[0] == GL_NONE) return;

	// 递增帧索引，用于控制动画进度
    m_frameIndex ++;

	// 更新 MVP 矩阵
	UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float)screenW / screenH);

	// 使用着色器程序
	glUseProgram (m_ProgramObj);

	// 绑定 VAO
	glBindVertexArray(m_VaoId);

	// 传递 MVP 矩阵到着色器
	glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);

	// 计算转场进度偏移量 [0, 1]
	float offset = (m_frameIndex % BF_LOOP_COUNT) * 1.0f / BF_LOOP_COUNT;

	// 当一个转场周期完成后，切换到下一组图片
	if(m_frameIndex % BF_LOOP_COUNT == 0)
		m_loopCount ++;

	// 绑定当前图片纹理到纹理单元 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureIds[m_loopCount % BF_IMG_NUM]);
	GLUtils::setInt(m_ProgramObj, "u_texture0", 0);

	// 绑定下一张图片纹理到纹理单元 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_TextureIds[(m_loopCount + 1) % BF_IMG_NUM]);
	GLUtils::setInt(m_ProgramObj, "u_texture1", 1);

	// 传递纹理尺寸和转场进度到着色器
	GLUtils::setVec2(m_ProgramObj, "u_texSize", m_RenderImages[0].width, m_RenderImages[0].height);
	GLUtils::setFloat(m_ProgramObj, "u_offset", offset);

	// 绘制矩形（两个三角形）
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

}

/**
 * 销毁函数：释放 OpenGL 资源
 */
void GLTransitionExample::Destroy()
{
	if (m_ProgramObj)
	{
		glDeleteProgram(m_ProgramObj);           // 删除着色器程序
		glDeleteBuffers(3, m_VboIds);             // 删除 VBO
		glDeleteVertexArrays(1, &m_VaoId);        // 删除 VAO
		glDeleteTextures(BF_IMG_NUM, m_TextureIds);  // 删除纹理
	}
}

/**
 * 更新 MVP 矩阵
 * @param mvpMatrix 输出的 MVP 矩阵
 * @param angleX X 轴旋转角度
 * @param angleY Y 轴旋转角度
 * @param ratio 屏幕宽高比
 */
void GLTransitionExample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio)
{
	LOGCATE("GLTransitionExample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX, angleY, ratio);
	// 将角度限制在 0-360 度范围内
	angleX = angleX % 360;
	angleY = angleY % 360;

	// 转化为弧度角
	float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
	float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);

	// 投影矩阵（正交投影）
	glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	//glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);  // 透视投影（已注释）
	//glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);              // 透视投影（已注释）

	// 视图矩阵：相机位于 (0,0,4)，朝向原点，向上方向为 Y 轴正方向
	glm::mat4 View = glm::lookAt(
			glm::vec3(0, 0, 4), // 相机位置
			glm::vec3(0, 0, 0), // 观察目标点
			glm::vec3(0, 1, 0)  // 相机向上方向
	);

	// 模型矩阵：应用缩放、旋转和平移变换
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));           // 缩放
	Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));       // 绕 X 轴旋转
	Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));       // 绕 Y 轴旋转
	Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));              // 平移

	// 计算最终的 MVP 矩阵
	mvpMatrix = Projection * View * Model;

}

/**
 * 更新变换矩阵参数
 * @param rotateX X 轴旋转角度
 * @param rotateY Y 轴旋转角度
 * @param scaleX X 轴缩放系数
 * @param scaleY Y 轴缩放系数
 */
void GLTransitionExample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
{
	GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
	m_AngleX = static_cast<int>(rotateX);
	m_AngleY = static_cast<int>(rotateY);
	m_ScaleX = scaleX;
	m_ScaleY = scaleY;
}

/**
 * 按索引加载多张图片
 * @param index 图片索引
 * @param pImage 图片数据指针
 */
void GLTransitionExample::LoadMultiImageWithIndex(int index, NativeImage *pImage) {
	LOGCATE("GLTransitionExample::LoadMultiImageWithIndex pImage = %p,[w=%d,h=%d,f=%d]", pImage->ppPlane[0], pImage->width, pImage->height, pImage->format);
	if (pImage && index >=0 && index < BF_IMG_NUM)
	{
		// 复制图片信息
		m_RenderImages[index].width = pImage->width;
		m_RenderImages[index].height = pImage->height;
		m_RenderImages[index].format = pImage->format;
		NativeImageUtil::CopyNativeImage(pImage, &m_RenderImages[index]);
		//NativeImageUtil::DumpNativeImage(&m_GrayImage, "/sdcard/DCIM", "GLTransitionExample");
    }
}
