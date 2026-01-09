/**
 * Model3DSample - OpenGL ES 3.0 3D 模型加载与渲染示例（头文件）
 *
 * 功能说明:
 * 演示如何加载和渲染复杂的 3D 模型文件（如 OBJ、FBX 等格式）
 *
 * 3D 模型渲染技术要点:
 * 1. 模型文件解析：读取顶点数据、法线、纹理坐标
 * 2. 网格（Mesh）管理：组织模型的几何数据
 * 3. 材质（Material）系统：处理模型的纹理和材质属性
 * 4. 着色器封装：Shader 类封装着色器的编译和使用
 * 5. MVP 变换：应用模型、视图、投影矩阵
 *
 * Model 类功能:
 * - 加载各种格式的 3D 模型文件
 * - 管理模型的网格（Mesh）数据
 * - 处理模型的材质和纹理
 * - 提供渲染接口
 *
 * Shader 类功能:
 * - 封装着色器的编译和链接
 * - 提供 uniform 变量设置接口
 * - 简化着色器程序的使用
 *
 * 应用场景:
 * - 3D 游戏角色渲染
 * - 产品展示（AR/VR）
 * - 建筑可视化
 * - 3D 动画播放
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_MODEL3DSample_H
#define NDK_OPENGLES_3_0_MODEL3DSample_H


#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include <shader.h>
#include <model.h>
#include "GLSampleBase.h"

/**
 * @class Model3DSample
 * @brief 3D 模型加载与渲染示例类
 *
 * @details
 * 演示如何使用 Model 和 Shader 类加载并渲染 3D 模型：
 * - Init()：加载 3D 模型文件，创建着色器
 * - Draw()：应用 MVP 变换并渲染模型
 * - UpdateTransformMatrix()：响应用户交互
 * - UpdateMVPMatrix()：计算 MVP 矩阵
 */
class Model3DSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	Model3DSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~Model3DSample();

	/**
	 * @brief 加载图像数据（用于模型纹理）
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 加载 3D 模型文件，创建并编译着色器程序
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 计算 MVP 矩阵并渲染 3D 模型
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除着色器和模型数据
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
	glm::mat4 m_MVPMatrix;     // MVP 变换矩阵
	glm::mat4 m_ModelMatrix;   // 模型矩阵
	Shader *m_pShader;         // 着色器对象指针
	Model *m_pModel;           // 3D 模型对象指针

	int m_AngleX;              // 绕 X 轴旋转角度
	int m_AngleY;              // 绕 Y 轴旋转角度
	float m_ScaleX;            // X 轴缩放比例
	float m_ScaleY;            // Y 轴缩放比例

};


#endif //NDK_OPENGLES_3_0_MODEL3DSample_H
