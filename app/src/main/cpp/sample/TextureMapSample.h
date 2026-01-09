/**
 * TextureMapSample - OpenGL ES 3.0 纹理映射示例（头文件）
 *
 * 功能说明:
 * 演示 OpenGL ES 中纹理映射的完整流程，包括纹理的创建、配置和采样
 *
 * OpenGL ES 纹理核心概念:
 * 1. 纹理对象（Texture Object）：存储图像数据的 GPU 内存对象
 * 2. 纹理单元（Texture Unit）：用于在着色器中访问多个纹理
 * 3. 纹理坐标（Texture Coordinates）：用于映射纹理到几何图形的 UV 坐标
 * 4. 采样器（Sampler）：在着色器中读取纹理像素的对象
 *
 * 纹理参数说明:
 * - GL_TEXTURE_WRAP_S/T：纹理环绕模式（CLAMP_TO_EDGE, REPEAT等）
 * - GL_TEXTURE_MIN/MAG_FILTER：纹理过滤方式（LINEAR, NEAREST等）
 *
 * Created by 公众号：字节流动 on 2021/10/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */
#ifndef NDK_OPENGLES_3_0_TEXTUREMAPSAMPLE_H
#define NDK_OPENGLES_3_0_TEXTUREMAPSAMPLE_H


#include "GLSampleBase.h"
#include "../util/ImageDef.h"

/**
 * @class TextureMapSample
 * @brief 纹理映射示例类
 *
 * @details
 * 演示如何使用纹理映射技术将图像贴到几何图形上：
 * - Init()：创建纹理对象、编译着色器程序
 * - LoadImage()：加载图像数据
 * - Draw()：上传纹理数据并进行纹理映射渲染
 * - Destroy()：释放纹理和着色器资源
 */
class TextureMapSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	TextureMapSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~TextureMapSample();

	/**
	 * @brief 加载图像数据
	 * @param pImage 图像数据结构指针
	 */
	void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建纹理对象、编译着色器程序、获取 uniform 位置
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 上传纹理数据，设置顶点和纹理坐标，执行纹理映射渲染
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除纹理对象和着色器程序
	 */
	virtual void Destroy();

private:
	GLuint m_TextureId;          // 纹理对象 ID
	GLint m_SamplerLoc;          // 采样器 uniform 变量在着色器中的位置
	NativeImage m_RenderImage;   // 存储图像数据的结构
};


#endif //NDK_OPENGLES_3_0_TEXTUREMAPSAMPLE_H
