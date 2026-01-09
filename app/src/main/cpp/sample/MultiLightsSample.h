/**
 * MultiLightsSample - OpenGL ES 3.0 多光源光照示例（头文件）
 *
 * 功能说明:
 * 演示如何在场景中实现多个光源的光照效果，包括定向光、点光源和聚光灯
 *
 * 多光源光照系统:
 * 在真实世界中，场景通常受到多个光源的影响。本示例实现三种常见光源类型：
 *
 * 1. 定向光（Directional Light）：
 *    - 模拟太阳光等平行光
 *    - 光线方向一致，不受位置影响
 *    - 适用于模拟远距离光源
 *    - 特点：只有方向，没有位置
 *
 * 2. 点光源（Point Light）：
 *    - 模拟灯泡等向四周发光的光源
 *    - 具有位置，光线向各方向辐射
 *    - 包含衰减计算（距离越远越暗）
 *    - 衰减公式：1.0 / (constant + linear * d + quadratic * d^2)
 *
 * 3. 聚光灯（Spot Light）：
 *    - 模拟手电筒、舞台灯等定向照射的光源
 *    - 具有位置和方向
 *    - 有光照范围（内锥角和外锥角）
 *    - 结合了点光源的衰减和方向限制
 *
 * 光照计算:
 * - 对每个光源分别计算冯氏光照（环境光、漫反射、镜面反射）
 * - 将所有光源的贡献累加得到最终颜色
 * - Final Color = Σ(Ambient + Diffuse + Specular) for each light
 *
 * 性能优化:
 * - 使用数组或结构体传递多个光源参数
 * - 在片段着色器中循环计算各光源贡献
 * - 可选：使用延迟渲染（Deferred Rendering）优化大量光源场景
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_MULTI_LIGHTS_H
#define NDK_OPENGLES_3_0_MULTI_LIGHTS_H


#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include "GLSampleBase.h"

/**
 * @class MultiLightsSample
 * @brief 多光源光照示例类
 *
 * @details
 * 演示如何实现包含定向光、点光源和聚光灯的多光源光照系统：
 * - Init()：创建纹理、编译包含多光源计算的着色器
 * - Draw()：设置多个光源参数并渲染场景
 * - UpdateMatrix()：计算多个物体的变换矩阵
 * - 每个光源独立计算冯氏光照并累加结果
 */
class MultiLightsSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	MultiLightsSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~MultiLightsSample();

	/**
	 * @brief 加载图像数据（用作纹理贴图）
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建纹理、编译包含多光源计算的着色器、设置 VAO/VBO
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 设置多个光源参数（定向光、点光源、聚光灯）并渲染场景
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
	 * @param scaleX X 轴缩放比例（未使用）
	 * @param scaleY Y 轴缩放比例（未使用）
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

	/**
	 * @brief 更新变换矩阵（通用版本）
	 * @param mvpMatrix 输出的 MVP 矩阵
	 * @param modelMatrix 输出的模型矩阵
	 * @param angleXRotate 绕 X 轴旋转角度
	 * @param angleYRotate 绕 Y 轴旋转角度
	 * @param scale 缩放比例
	 * @param transVec3 平移向量
	 * @param ratio 屏幕宽高比
	 */
	void UpdateMatrix(glm::mat4 &mvpMatrix, glm::mat4 &modelMatrix, int angleXRotate, int angleYRotate, float scale, glm::vec3 transVec3, float ratio);

private:
	GLuint m_TextureId;            // 纹理对象 ID
	GLint m_SamplerLoc;            // 纹理采样器 uniform 位置
	GLint m_MVPMatLoc;             // MVP 矩阵 uniform 位置
	GLint m_ModelMatrixLoc;        // 模型矩阵 uniform 位置
	GLint m_ViewPosLoc;            // 观察者位置 uniform 位置

	GLuint m_VaoId;                // VAO 对象 ID
	GLuint m_VboIds[1];            // VBO 数组
	NativeImage m_RenderImage;     // 图像数据
	glm::mat4 m_MVPMatrix;         // MVP 变换矩阵
	glm::mat4 m_ModelMatrix;       // 模型矩阵

	int m_AngleX;                  // 绕 X 轴旋转角度
	int m_AngleY;                  // 绕 Y 轴旋转角度
};


#endif //NDK_OPENGLES_3_0_MULTI_LIGHTS_H
