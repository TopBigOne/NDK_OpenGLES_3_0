/**
 * BasicLightingSample - OpenGL ES 3.0 冯氏光照模型示例（头文件）
 *
 * 功能说明:
 * 实现经典的 Phong（冯氏）光照模型，展示真实感光照渲染
 *
 * 冯氏光照模型（Phong Lighting Model）:
 * 由三个光照分量组成，模拟现实世界中的光照效果：
 *
 * 1. 环境光（Ambient）：
 *    - 模拟场景中的基础光照
 *    - 与光源方向无关，为物体提供最低亮度
 *    - 公式：ambient = ambientStrength * lightColor
 *
 * 2. 漫反射（Diffuse）：
 *    - 模拟光线照射到粗糙表面的散射效果
 *    - 强度与光源方向和表面法线的夹角相关
 *    - 使用兰伯特余弦定律：diffuse = max(dot(normal, lightDir), 0.0) * lightColor
 *
 * 3. 镜面反射（Specular）：
 *    - 模拟光滑表面的高光效果
 *    - 强度与观察方向和反射方向的夹角相关
 *    - 公式：specular = pow(max(dot(viewDir, reflectDir), 0.0), shininess) * lightColor
 *
 * 关键概念:
 * - 法线向量（Normal Vector）：垂直于表面的向量，用于计算光照
 * - 光源位置（Light Position）：影响光照方向
 * - 观察者位置（View Position）：影响镜面反射效果
 * - 反光度（Shininess）：控制高光的锐利程度
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_BASIC_LIGHTING_H
#define NDK_OPENGLES_3_0_BASIC_LIGHTING_H


#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include "GLSampleBase.h"

/**
 * @class BasicLightingSample
 * @brief 冯氏光照模型示例类
 *
 * @details
 * 演示如何在顶点着色器中计算冯氏光照模型：
 * - Init()：创建纹理、编译光照着色器、设置 VAO/VBO
 * - Draw()：传递光源和观察者位置，渲染带光照效果的 3D 立方体
 * - UpdateTransformMatrix()：响应用户交互
 * - UpdateMVPMatrix()：计算 MVP 和 Model 矩阵
 */
class BasicLightingSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	BasicLightingSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~BasicLightingSample();

	/**
	 * @brief 加载图像数据（用作纹理贴图）
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建纹理、编译包含光照计算的着色器、设置 VAO/VBO
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 设置光照参数并渲染带光照效果的 3D 物体
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除纹理、VAO、VBO 和着色器程序
	 */
	virtual void Destroy();

	/**
	 * @brief 更新变换矩阵参数
	 * @param rotateX 绕 X 轴旋转角度
	 * @param rotateY 绕 Y 轴旋转角度
	 * @param scaleX X 轴缩放比例
	 * @param scaleY Y 轴缩放比例
	 */
	virtual void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY);

	/**
	 * @brief 更新 MVP 矩阵
	 * @param mvpMatrix 输出的 MVP 矩阵
	 * @param angleX 绕 X 轴旋转角度
	 * @param angleY 绕 Y 轴旋转角度
	 * @param ratio 屏幕宽高比
	 */
	void UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio);

private:
	GLuint m_TextureId;            // 纹理对象 ID
	GLint m_SamplerLoc;            // 纹理采样器 uniform 位置
	GLint m_MVPMatLoc;             // MVP 矩阵 uniform 位置
	GLint m_ModelMatrixLoc;        // 模型矩阵 uniform 位置（用于变换法线）
	GLint m_LightPosLoc;           // 光源位置 uniform 位置
	GLint m_LightColorLoc;         // 光源颜色 uniform 位置
	GLint m_ViewPosLoc;            // 观察者位置 uniform 位置

	GLuint m_VaoId;                // VAO 对象 ID
	GLuint m_VboIds[1];            // VBO 数组
	GLuint m_TfoId;                // Transform Feedback 对象 ID（可选）
	GLuint m_TfoBufId;             // Transform Feedback 缓冲区 ID（可选）
	NativeImage m_RenderImage;     // 图像数据
	glm::mat4 m_MVPMatrix;         // MVP 变换矩阵
	glm::mat4 m_ModelMatrix;       // 模型矩阵

	int m_AngleX;                  // 绕 X 轴旋转角度
	int m_AngleY;                  // 绕 Y 轴旋转角度

	float m_ScaleX;                // X 轴缩放比例
	float m_ScaleY;                // Y 轴缩放比例
};


#endif //NDK_OPENGLES_3_0_BASIC_LIGHTING_H
