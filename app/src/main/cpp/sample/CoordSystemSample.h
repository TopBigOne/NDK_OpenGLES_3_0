/**
 * CoordSystemSample - OpenGL ES 3.0 坐标系统与 MVP 变换示例（头文件）
 *
 * 功能说明:
 * 演示 3D 图形渲染的核心 - MVP (Model-View-Projection) 矩阵变换系统
 *
 * OpenGL ES 坐标系统:
 * 1. 局部空间（Local Space）：模型自身的坐标系
 * 2. 世界空间（World Space）：所有物体在同一世界中的坐标
 * 3. 观察空间（View Space）：以相机为原点的坐标系
 * 4. 裁剪空间（Clip Space）：应用透视投影后的坐标
 * 5. 屏幕空间（Screen Space）：最终显示在屏幕上的2D坐标
 *
 * MVP 矩阵变换:
 * - Model Matrix（模型矩阵）：局部空间 → 世界空间（平移、旋转、缩放）
 * - View Matrix（观察矩阵）：世界空间 → 观察空间（定义相机位置和方向）
 * - Projection Matrix（投影矩阵）：观察空间 → 裁剪空间（透视或正交投影）
 *
 * GLM 数学库:
 * - 提供矩阵和向量运算函数
 * - 与 GLSL 语法高度一致
 * - 常用函数：translate()、rotate()、scale()、perspective()、lookAt()
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_COORDSYSTEMSAMPLE_H
#define NDK_OPENGLES_3_0_COORDSYSTEMSAMPLE_H


#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include "GLSampleBase.h"

/**
 * @class CoordSystemSample
 * @brief 坐标系统和 MVP 变换示例类
 *
 * @details
 * 演示如何使用 MVP 矩阵实现 3D 变换和渲染：
 * - Init()：创建纹理、着色器、VAO/VBO
 * - Draw()：应用 MVP 变换矩阵渲染 3D 立方体
 * - UpdateTransformMatrix()：响应用户交互，更新旋转和缩放参数
 * - UpdateMVPMatrix()：计算 MVP 矩阵
 */
class CoordSystemSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	CoordSystemSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~CoordSystemSample();

	/**
	 * @brief 加载图像数据
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建纹理、编译着色器、设置 VAO/VBO
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 更新 MVP 矩阵并渲染 3D 立方体
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除纹理、VAO、VBO 和着色器程序
	 */
	virtual void Destroy();

	/**
	 * @brief 更新变换矩阵参数（响应用户交互）
	 * @param rotateX 绕 X 轴旋转角度
	 * @param rotateY 绕 Y 轴旋转角度
	 * @param scaleX X 轴缩放比例
	 * @param scaleY Y 轴缩放比例
	 */
	virtual void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY);

	/**
	 * @brief 更新 MVP 矩阵
	 * @param mvpMatrix 输出的 MVP 矩阵
	 * @param angleX 绕 X 轴旋转角度（度数）
	 * @param angleY 绕 Y 轴旋转角度（度数）
	 * @param ratio 屏幕宽高比
	 * @details 计算 Model、View、Projection 矩阵并组合成 MVP 矩阵
	 */
	void UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio);

private:
	GLuint m_TextureId;          // 纹理对象 ID
	GLint m_SamplerLoc;          // 采样器 uniform 变量位置
	GLint m_MVPMatLoc;           // MVP 矩阵 uniform 变量位置
	GLuint m_VaoId;              // VAO 对象 ID
	GLuint m_VboIds[3];          // VBO 数组：[0]=顶点坐标，[1]=纹理坐标，[2]=索引
	NativeImage m_RenderImage;   // 图像数据
	glm::mat4 m_MVPMatrix;       // MVP 变换矩阵

	int m_AngleX;                // 绕 X 轴旋转角度（度数）
	int m_AngleY;                // 绕 Y 轴旋转角度（度数）
	float m_ScaleX;              // X 轴缩放比例
	float m_ScaleY;              // Y 轴缩放比例

};


#endif //NDK_OPENGLES_3_0_COORDSYSTEMSAMPLE_H
