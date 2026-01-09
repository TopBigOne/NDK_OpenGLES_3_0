/**
 * VaoSample - OpenGL ES 3.0 VAO (顶点数组对象) 示例（头文件）
 *
 * 功能说明:
 * 演示 VAO 和 VBO/EBO 的配合使用，以及如何通过交错式顶点数据布局提高性能
 *
 * OpenGL ES 核心概念:
 * 1. VAO (Vertex Array Object) - 顶点数组对象
 *    - 封装所有顶点属性配置状态
 *    - 避免每帧重复设置顶点属性
 *    - 提高渲染效率
 *
 * 2. VBO (Vertex Buffer Object) - 顶点缓冲对象
 *    - 在 GPU 内存中存储顶点数据
 *    - 支持交错式或分离式数据布局
 *
 * 3. EBO (Element Buffer Object) - 索引缓冲对象
 *    - 存储顶点索引，避免重复顶点
 *    - 也称为 IBO (Index Buffer Object)
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_VAOSAMPLE_H
#define NDK_OPENGLES_3_0_VAOSAMPLE_H


#include "GLSampleBase.h"

/**
 * @class VaoSample
 * @brief VAO 示例类
 *
 * @details
 * 演示如何使用 VAO 管理顶点属性配置，并实现棋盘格渲染效果：
 * - Init()：创建 VAO、VBO、EBO，配置交错式顶点数据
 * - Draw()：绑定 VAO 并使用索引绘制
 * - Destroy()：释放所有缓冲对象
 */
class VaoSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	VaoSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~VaoSample();

	/**
	 * @brief 加载图像（本示例不使用，空实现）
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建 VAO、VBO、EBO，配置顶点属性，编译着色器
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 绑定 VAO 并使用索引绘制，渲染带棋盘格效果的矩形
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除着色器程序、VBO 和 VAO
	 */
	virtual void Destroy();

private:
	GLuint  m_VaoId;       // VAO 对象 ID
	GLuint  m_VboIds[2];   // VBO 数组：[0]=顶点数据，[1]=索引数据(EBO)
};


#endif //NDK_OPENGLES_3_0_VAOSAMPLE_H
