/**
 * TriangleSample - OpenGL ES 3.0 三角形绘制示例（头文件）
 *
 * 功能说明:
 * 这是 OpenGL ES 最基础的示例，演示如何使用顶点着色器和片段着色器绘制一个简单的三角形
 *
 * OpenGL ES 渲染管线核心概念:
 * 1. 顶点着色器（Vertex Shader）：处理顶点位置变换
 * 2. 片段着色器（Fragment Shader）：计算每个像素的颜色
 * 3. 图元装配：将顶点组装成三角形等基本图元
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_TRIANGLESAMPLE_H
#define NDK_OPENGLES_3_0_TRIANGLESAMPLE_H


#include "GLSampleBase.h"

/**
 * @class TriangleSample
 * @brief 三角形绘制示例类
 *
 * @details
 * 继承自 GLSampleBase，实现最基础的 OpenGL ES 渲染：
 * - Init()：创建并编译着色器程序
 * - Draw()：定义顶点数据并绘制三角形
 * - Destroy()：释放着色器程序资源
 */
class TriangleSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	TriangleSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~TriangleSample();

	/**
	 * @brief 加载图像数据（本示例不使用，空实现）
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建并编译顶点着色器和片段着色器
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 定义三角形顶点数据并执行绘制
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除着色器程序
	 */
	virtual void Destroy();
};


#endif //NDK_OPENGLES_3_0_TRIANGLESAMPLE_H
