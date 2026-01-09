/**
 * PBOSample - OpenGL ES 3.0 PBO (像素缓冲对象) 示例（头文件）
 *
 * 功能说明:
 * 演示如何使用 PBO (Pixel Buffer Object) 优化纹理上传和像素读取性能
 *
 * PBO 核心概念:
 * PBO 是 OpenGL 的缓冲对象，用于异步传输像素数据，显著提升性能
 *
 * 两种使用模式:
 * 1. 上传 PBO (Upload PBO)：
 *    - 用于将像素数据从 CPU 传输到 GPU
 *    - 配合 glTexImage2D 或 glTexSubImage2D 使用
 *    - 流程：CPU写入PBO → GPU异步读取PBO → 更新纹理
 *    - 绑定目标：GL_PIXEL_UNPACK_BUFFER
 *
 * 2. 下载 PBO (Download PBO)：
 *    - 用于从 GPU 读取像素数据到 CPU
 *    - 配合 glReadPixels 使用
 *    - 流程：GPU写入PBO → CPU异步读取PBO → 获取像素数据
 *    - 绑定目标：GL_PIXEL_PACK_BUFFER
 *
 * 双缓冲技术（Ping-Pong Buffering）:
 * - 使用两个 PBO 交替工作
 * - 当 PBO[0] 在上传/下载时，PBO[1] 可以进行映射和数据操作
 * - 避免 CPU-GPU 同步等待，提高并行度
 * - 帧 N：PBO[0]传输，PBO[1]映射；帧 N+1：交换角色
 *
 * 性能优势:
 * - 异步数据传输：CPU 和 GPU 可以并行工作
 * - 减少数据拷贝：通过内存映射（glMapBufferRange）直接访问 GPU 内存
 * - 流式传输：适合视频流、相机预览等连续帧场景
 * - 相比传统方式可提升 2-5 倍性能
 *
 * 应用场景:
 * - 视频播放和处理
 * - 相机预览和录制
 * - 实时图像处理
 * - GPU 到 CPU 的像素回读（如截图、录屏）
 *
 * Created by 公众号：字节流动 on 2021/10/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_PBOSAMPLE_H
#define NDK_OPENGLES_3_0_PBOSAMPLE_H


#include <detail/type_mat.hpp>
#include "GLSampleBase.h"
#include "../util/ImageDef.h"

/**
 * @class PBOSample
 * @brief PBO 像素缓冲对象示例类
 *
 * @details
 * 演示 PBO 的上传和下载功能，使用双缓冲技术优化性能：
 * - Init()：创建 FBO、纹理、VAO/VBO 和两组 PBO（上传/下载）
 * - UploadPixels()：使用 PBO 异步上传像素数据到纹理
 * - DownloadPixels()：使用 PBO 异步从 GPU 读取像素数据
 * - Draw()：渲染并展示 PBO 的性能优势
 * - CreateFrameBufferObj()：创建 FBO 用于离屏渲染
 */
class PBOSample : public GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 */
	PBOSample();

	/**
	 * @brief 虚析构函数
	 */
	virtual ~PBOSample();

	/**
	 * @brief 加载图像数据
	 * @param pImage 图像数据指针
	 */
	virtual void LoadImage(NativeImage *pImage);

	/**
	 * @brief 初始化 OpenGL 资源
	 * @details 创建 FBO、纹理、VAO/VBO、上传 PBO 和下载 PBO
	 */
	virtual void Init();

	/**
	 * @brief 渲染函数
	 * @param screenW 屏幕宽度
	 * @param screenH 屏幕高度
	 * @details 使用 PBO 上传纹理数据，渲染场景，并从 GPU 读取像素
	 */
	virtual void Draw(int screenW, int screenH);

	/**
	 * @brief 销毁 OpenGL 资源
	 * @details 删除 FBO、纹理、VAO、VBO 和 PBO
	 */
	virtual void Destroy();

	/**
	 * @brief 更新 MVP 矩阵
	 * @param mvpMatrix 输出的 MVP 矩阵
	 * @param angleX 绕 X 轴旋转角度
	 * @param angleY 绕 Y 轴旋转角度
	 * @param ratio 屏幕宽高比
	 */
    void UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio);

	/**
	 * @brief 创建帧缓冲对象
	 * @return 创建成功返回 true，失败返回 false
	 * @details 创建 FBO 并附加纹理，用于离屏渲染
	 */
	bool CreateFrameBufferObj();

	/**
	 * @brief 使用 PBO 上传像素数据到纹理
	 * @details 使用双缓冲技术异步上传图像数据，提高上传性能
	 */
	void UploadPixels();

	/**
	 * @brief 使用 PBO 从 GPU 下载像素数据
	 * @details 使用双缓冲技术异步读取渲染结果，避免阻塞
	 */
	void DownloadPixels();

private:
	GLuint m_ImageTextureId;       // 原始图像纹理 ID
	GLuint m_FboTextureId;         // FBO 颜色附件纹理 ID
	GLuint m_FboId;                // FBO 对象 ID
	GLuint m_VaoIds[2] = {GL_NONE};// VAO 数组
	GLuint m_VboIds[4] = {GL_NONE};// VBO 数组
	GLint m_SamplerLoc;            // 普通渲染的采样器 uniform 位置
	NativeImage m_RenderImage;     // 图像数据
	GLuint m_FboProgramObj;        // 离屏渲染着色器程序 ID
	GLuint m_FboVertexShader;      // 离屏渲染顶点着色器 ID
	GLuint m_FboFragmentShader;    // 离屏渲染片段着色器 ID
	GLint m_FboSamplerLoc;         // 离屏渲染的采样器 uniform 位置
	GLint m_MVPMatrixLoc;          // MVP 矩阵 uniform 位置

    int m_AngleX;                  // 绕 X 轴旋转角度
    int m_AngleY;                  // 绕 Y 轴旋转角度
    float m_ScaleX;                // X 轴缩放比例
    float m_ScaleY;                // Y 轴缩放比例
    glm::mat4 m_MVPMatrix;         // MVP 变换矩阵

    GLuint m_UploadPboIds[2] = {GL_NONE};    // 上传 PBO 数组（双缓冲）
    GLuint m_DownloadPboIds[2] = {GL_NONE};  // 下载 PBO 数组（双缓冲）

    NativeImage m_DownloadImages[2];  // 下载的图像数据缓冲区
    int m_FrameIndex;                 // 当前帧索引（用于双缓冲切换）

};


#endif //NDK_OPENGLES_3_0_PBOSAMPLE_H
