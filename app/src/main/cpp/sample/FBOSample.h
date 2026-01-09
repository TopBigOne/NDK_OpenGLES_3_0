/**
 * FBOSample - OpenGL ES 3.0 FBO (帧缓冲对象) 离屏渲染示例（头文件）
 *
 * 功能说明:
 * 演示 FBO 的创建和使用，实现两遍渲染（离屏渲染 + 屏幕渲染）
 *
 * FBO 核心概念:
 * 1. Frame Buffer Object（帧缓冲对象）：
 *    - OpenGL 的离屏渲染目标
 *    - 不直接显示到屏幕，而是渲染到纹理或渲染缓冲区
 *
 * 2. 颜色附件（Color Attachment）：
 *    - 将纹理附加到 FBO，作为颜色输出目标
 *    - 支持多个颜色附件（MRT - Multiple Render Targets）
 *
 * 3. 深度/模板附件（Depth/Stencil Attachment）：
 *    - 用于深度测试和模板测试
 *    - 可以使用纹理或渲染缓冲区（Renderbuffer）
 *
 * 两遍渲染流程:
 * 第一遍（离屏渲染）：
 *   - 绑定 FBO
 *   - 渲染场景到 FBO 的纹理附件（本示例生成灰度图）
 *
 * 第二遍（屏幕渲染）：
 *   - 绑定默认帧缓冲（屏幕）
 *   - 将 FBO 的纹理作为输入进行渲染
 *
 * 应用场景:
 * - 后处理效果：模糊、边缘检测、色彩调整
 * - 实时阴影：Shadow Mapping
 * - 环境映射：反射、折射
 * - 粒子效果、水面反射等
 *
 * Created by 公众号：字节流动 on 2020/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_FBOSAMPLE_H
#define NDK_OPENGLES_3_0_FBOSAMPLE_H


#include "GLSampleBase.h"
#include "../util/ImageDef.h"

/**
 * @class FBOSample
 * @brief FBO 离屏渲染示例类
 *
 * @details
 * 演示 FBO 的完整使用流程：
 * - Init()：创建两套着色器程序、纹理、VAO/VBO、FBO
 * - CreateFrameBufferObj()：创建 FBO 并附加纹理
 * - Draw()：执行两遍渲染（离屏 + 屏幕）
 * - Destroy()：释放所有资源
 */
class FBOSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	FBOSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~FBOSample();

	/**
	 * @brief 加载图像数据
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建两套着色器、纹理、VAO/VBO 和 FBO
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 执行两遍渲染：第一遍离屏渲染生成灰度图，第二遍渲染到屏幕
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除纹理、VAO、VBO、FBO 和着色器程序
	 */
	virtual void Destroy();

	/**
	 * @brief 创建帧缓冲对象
	 * @return 创建成功返回 true，失败返回 false
	 * @details 创建 FBO 并附加颜色纹理，检查 FBO 完整性
	 */
	bool CreateFrameBufferObj();

private:
	GLuint m_ImageTextureId;       // 原始图像纹理 ID
	GLuint m_FboTextureId;         // FBO 颜色附件纹理 ID
	GLuint m_FboId;                // FBO 对象 ID
	GLuint m_VaoIds[2];            // VAO 数组：[0]=普通渲染，[1]=离屏渲染
	GLuint m_VboIds[4];            // VBO 数组：顶点、纹理坐标、索引数据
	GLint m_SamplerLoc;            // 普通渲染的采样器 uniform 位置
	NativeImage m_RenderImage;     // 图像数据
	GLuint m_FboProgramObj;        // 离屏渲染着色器程序 ID
	GLuint m_FboVertexShader;      // 离屏渲染顶点着色器 ID
	GLuint m_FboFragmentShader;    // 离屏渲染片段着色器 ID
	GLint m_FboSamplerLoc;         // 离屏渲染的采样器 uniform 位置

};


#endif //NDK_OPENGLES_3_0_FBOSAMPLE_H
