/**
 * GLSampleBase - OpenGL ES 3.0 示例基类
 *
 * 功能说明:
 * 这是所有 OpenGL ES 示例的抽象基类，定义了统一的接口规范和公共成员变量
 *
 * 设计模式:
 * - 使用模板方法模式定义渲染流程骨架
 * - 子类通过重写虚函数实现具体的渲染逻辑
 * - 提供可选的扩展接口（如触摸、重力感应等）
 *
 * 核心接口:
 * 1. Init() - 初始化 OpenGL 资源（着色器、纹理、缓冲区等）
 * 2. Draw() - 每帧渲染函数
 * 3. Destroy() - 释放 OpenGL 资源
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_GLSAMPLEBASE_H
#define NDK_OPENGLES_3_0_GLSAMPLEBASE_H

#include "stdint.h"
#include <GLES3/gl3.h>
#include <ImageDef.h>
#include <ByteFlowLock.h>

// ==================== 数学常量定义 ====================
#define MATH_PI 3.1415926535897932384626433832802  // 圆周率 π

// ==================== 示例类型定义（用于样例选择和标识）====================
#define SAMPLE_TYPE                             200  // 基础类型偏移量

// 基础渲染类 (200-209)
#define SAMPLE_TYPE_KEY_TRIANGLE                SAMPLE_TYPE + 0   // 三角形绘制
#define SAMPLE_TYPE_KEY_TEXTURE_MAP             SAMPLE_TYPE + 1   // 纹理映射
#define SAMPLE_TYPE_KEY_YUV_TEXTURE_MAP         SAMPLE_TYPE + 2   // YUV 纹理映射
#define SAMPLE_TYPE_KEY_VAO                     SAMPLE_TYPE + 3   // 顶点数组对象
#define SAMPLE_TYPE_KEY_FBO                     SAMPLE_TYPE + 4   // 帧缓冲对象
#define SAMPLE_TYPE_KEY_FBO_LEG_LENGTHEN        SAMPLE_TYPE + 6   // 大长腿特效（FBO 应用）
#define SAMPLE_TYPE_KEY_COORD_SYSTEM            SAMPLE_TYPE + 7   // 坐标系统（MVP 变换）
#define SAMPLE_TYPE_KEY_BASIC_LIGHTING          SAMPLE_TYPE + 8   // 基础光照（冯氏光照）
#define SAMPLE_TYPE_KEY_TRANSFORM_FEEDBACK      SAMPLE_TYPE + 9   // 变换反馈

// 进阶渲染类 (210-219)
#define SAMPLE_TYPE_KEY_MULTI_LIGHTS            SAMPLE_TYPE + 10  // 多光源
#define SAMPLE_TYPE_KEY_DEPTH_TESTING           SAMPLE_TYPE + 11  // 深度测试
#define SAMPLE_TYPE_KEY_INSTANCING              SAMPLE_TYPE + 12  // 实例化渲染
#define SAMPLE_TYPE_KEY_STENCIL_TESTING         SAMPLE_TYPE + 13  // 模板测试
#define SAMPLE_TYPE_KEY_BLENDING                SAMPLE_TYPE + 14  // 混合（透明度）
#define SAMPLE_TYPE_KEY_PARTICLES               SAMPLE_TYPE + 15  // 粒子系统
#define SAMPLE_TYPE_KEY_SKYBOX                  SAMPLE_TYPE + 16  // 天空盒（立方体贴图）
#define SAMPLE_TYPE_KEY_3D_MODEL                SAMPLE_TYPE + 17  // 3D 模型加载
#define SAMPLE_TYPE_KEY_PBO                     SAMPLE_TYPE + 18  // 像素缓冲对象

// 特效类 (220-229)
#define SAMPLE_TYPE_KEY_BEATING_HEART           SAMPLE_TYPE + 19  // 心跳特效
#define SAMPLE_TYPE_KEY_CLOUD                   SAMPLE_TYPE + 20  // 云层特效
#define SAMPLE_TYPE_KEY_TIME_TUNNEL             SAMPLE_TYPE + 21  // 时空隧道特效
#define SAMPLE_TYPE_KEY_BEZIER_CURVE            SAMPLE_TYPE + 22  // 贝塞尔曲线
#define SAMPLE_TYPE_KEY_BIG_EYES                SAMPLE_TYPE + 23  // 大眼特效
#define SAMPLE_TYPE_KEY_FACE_SLENDER            SAMPLE_TYPE + 24  // 瘦脸特效
#define SAMPLE_TYPE_KEY_BIG_HEAD                SAMPLE_TYPE + 25  // 大头特效
#define SAMPLE_TYPE_KEY_RATARY_HEAD             SAMPLE_TYPE + 26  // 旋转头像
#define SAMPLE_TYPE_KEY_VISUALIZE_AUDIO         SAMPLE_TYPE + 27  // 音频可视化
#define SAMPLE_TYPE_KEY_SCRATCH_CARD            SAMPLE_TYPE + 28  // 刮刮卡效果
#define SAMPLE_TYPE_KEY_AVATAR                  SAMPLE_TYPE + 29  // 头像特效

// 高级特性类 (230-249)
#define SAMPLE_TYPE_KEY_SHOCK_WAVE              SAMPLE_TYPE + 30  // 冲击波特效
#define SAMPLE_TYPE_KEY_MRT                     SAMPLE_TYPE + 31  // 多重渲染目标
#define SAMPLE_TYPE_KEY_FBO_BLIT                SAMPLE_TYPE + 32  // FBO 位块传输
#define SAMPLE_TYPE_KEY_TBO                     SAMPLE_TYPE + 33  // 纹理缓冲对象
#define SAMPLE_TYPE_KEY_UBO                     SAMPLE_TYPE + 34  // 统一缓冲对象
#define SAMPLE_TYPE_KEY_RGB2YUYV                SAMPLE_TYPE + 35  // RGB 转 YUYV
#define SAMPLE_TYPE_KEY_MULTI_THREAD_RENDER     SAMPLE_TYPE + 36  // 多线程渲染
#define SAMPLE_TYPE_KEY_TEXT_RENDER     		SAMPLE_TYPE + 37  // 文本渲染
#define SAMPLE_TYPE_KEY_STAY_COLOR       		SAMPLE_TYPE + 38  // 保留特定颜色
#define SAMPLE_TYPE_KEY_TRANSITIONS_1      		SAMPLE_TYPE + 39  // 转场特效 1
#define SAMPLE_TYPE_KEY_TRANSITIONS_2      		SAMPLE_TYPE + 40  // 转场特效 2
#define SAMPLE_TYPE_KEY_TRANSITIONS_3      		SAMPLE_TYPE + 41  // 转场特效 3
#define SAMPLE_TYPE_KEY_TRANSITIONS_4      		SAMPLE_TYPE + 42  // 转场特效 4
#define SAMPLE_TYPE_KEY_RGB2NV21                SAMPLE_TYPE + 43  // RGB 转 NV21
#define SAMPLE_TYPE_KEY_RGB2I420                SAMPLE_TYPE + 44  // RGB 转 I420
#define SAMPLE_TYPE_KEY_RGB2I444                SAMPLE_TYPE + 45  // RGB 转 I444
#define SAMPLE_TYPE_KEY_COPY_TEXTURE            SAMPLE_TYPE + 46  // 纹理拷贝
#define SAMPLE_TYPE_KEY_BLIT_FRAME_BUFFER       SAMPLE_TYPE + 47  // 帧缓冲位块传输
#define SAMPLE_TYPE_KEY_BINARY_PROGRAM          SAMPLE_TYPE + 48  // 二进制着色器程序

// YUV 渲染类 (250-259)
#define SAMPLE_TYPE_KEY_HWBuffer                SAMPLE_TYPE + 49  // 硬件缓冲区
#define SAMPLE_TYPE_KEY_RENDER_16BIT_GRAY       SAMPLE_TYPE + 50  // 渲染 16 位灰度图
#define SAMPLE_TYPE_KEY_RENDER_P010             SAMPLE_TYPE + 51  // 渲染 P010 格式
#define SAMPLE_TYPE_KEY_RENDER_NV21             SAMPLE_TYPE + 52  // 渲染 NV21 格式
#define SAMPLE_TYPE_KEY_RENDER_I420             SAMPLE_TYPE + 53  // 渲染 I420 格式
#define SAMPLE_TYPE_KEY_RENDER_I444             SAMPLE_TYPE + 54  // 渲染 I444 格式
#define SAMPLE_TYPE_KEY_RENDER_YUYV             SAMPLE_TYPE + 55  // 渲染 YUYV 格式

// 着色器高级特性 (256-262)
#define SAMPLE_TYPE_KEY_COMPUTE_SHADER          SAMPLE_TYPE + 56  // 计算着色器
#define SAMPLE_TYPE_KEY_PORTRAIT_MODE           SAMPLE_TYPE + 57  // 人像模式
#define SAMPLE_TYPE_KEY_MSAA                    SAMPLE_TYPE + 58  // 多重采样抗锯齿
#define SAMPLE_TYPE_KEY_FULLSCREEN_TRIANGLE     SAMPLE_TYPE + 59  // 全屏三角形优化
#define SAMPLE_TYPE_KEY_GEOMETRY_SHADER         SAMPLE_TYPE + 60  // 几何着色器
#define SAMPLE_TYPE_KEY_GEOMETRY_SHADER2        SAMPLE_TYPE + 61  // 几何着色器示例 2
#define SAMPLE_TYPE_KEY_GEOMETRY_SHADER3        SAMPLE_TYPE + 62  // 几何着色器示例 3

// 特殊控制类型
#define SAMPLE_TYPE_KEY_SET_TOUCH_LOC           SAMPLE_TYPE + 999   // 设置触摸位置
#define SAMPLE_TYPE_SET_GRAVITY_XY              SAMPLE_TYPE + 1000  // 设置重力感应 XY

// ==================== 资源路径定义 ====================
#define DEFAULT_OGL_ASSETS_DIR "/sdcard/Android/data/com.byteflow.app/files/Download"  // 默认资源目录

/**
 * @class GLSampleBase
 * @brief OpenGL ES 示例基类
 *
 * @details
 * 所有 OpenGL ES 示例都继承自此类，提供统一的接口规范：
 * - 必须实现：Init(), Draw(), Destroy()
 * - 可选实现：LoadImage(), UpdateTransformMatrix(), SetTouchLocation() 等
 */
class GLSampleBase
{
public:
	/**
	 * @brief 构造函数
	 * @details 初始化成员变量为默认值
	 */
	GLSampleBase()
	{
		m_ProgramObj = 0;        // 着色器程序对象 ID
		m_VertexShader = 0;      // 顶点着色器 ID
		m_FragmentShader = 0;    // 片段着色器 ID

		m_SurfaceWidth = 0;      // 渲染表面宽度
		m_SurfaceHeight = 0;     // 渲染表面高度
	}

	/**
	 * @brief 虚析构函数
	 * @details 确保派生类的析构函数被正确调用
	 */
	virtual ~GLSampleBase()
	{

	}

	/**
	 * @brief 加载图像数据（单张图像）
	 * @param pImage 图像数据结构指针
	 * @details 用于加载纹理贴图、视频帧等图像数据
	 */
	virtual void LoadImage(NativeImage *pImage)
	{};

	/**
	 * @brief 加载多张图像数据（带索引）
	 * @param index 图像索引
	 * @param pImage 图像数据结构指针
	 * @details 用于多纹理、立方体贴图等场景
	 */
	virtual void LoadMultiImageWithIndex(int index, NativeImage *pImage)
	{};

	/**
	 * @brief 加载音频短整型数组数据
	 * @param pShortArr 短整型数组指针
	 * @param arrSize 数组大小
	 * @details 用于音频可视化等示例
	 */
	virtual void LoadShortArrData(short *const pShortArr, int arrSize)
	{}

	/**
	 * @brief 更新变换矩阵（旋转和缩放）
	 * @param rotateX 绕 X 轴旋转角度
	 * @param rotateY 绕 Y 轴旋转角度
	 * @param scaleX X 轴缩放比例
	 * @param scaleY Y 轴缩放比例
	 * @details 用于交互式变换控制
	 */
	virtual void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY)
	{}

	/**
	 * @brief 设置触摸位置
	 * @param x 触摸点 X 坐标（归一化或像素坐标）
	 * @param y 触摸点 Y 坐标（归一化或像素坐标）
	 * @details 用于触摸交互特效（如大眼、瘦脸等）
	 */
	virtual void SetTouchLocation(float x, float y)
	{}

	/**
	 * @brief 设置重力感应 XY 值
	 * @param x 重力加速度 X 分量
	 * @param y 重力加速度 Y 分量
	 * @details 用于重力感应相关效果
	 */
	virtual void SetGravityXY(float x, float y)
	{}

	/**
	 * @brief 初始化 OpenGL 资源（纯虚函数，必须实现）
	 * @details 在此函数中创建着色器程序、纹理、缓冲区等 OpenGL 对象
	 */
	virtual void Init() = 0;

	/**
	 * @brief 渲染函数（纯虚函数，必须实现）
	 * @param screenW 屏幕/视口宽度（像素）
	 * @param screenH 屏幕/视口高度（像素）
	 * @details 每帧调用一次，执行实际的渲染操作
	 */
	virtual void Draw(int screenW, int screenH) = 0;

	/**
	 * @brief 销毁 OpenGL 资源（纯虚函数，必须实现）
	 * @details 删除着色器程序、纹理、缓冲区等，防止内存泄漏
	 */
	virtual void Destroy() = 0;

protected:
	GLuint m_VertexShader;      // 顶点着色器对象 ID
	GLuint m_FragmentShader;    // 片段着色器对象 ID
	GLuint m_ProgramObj;        // 着色器程序对象 ID
	MySyncLock m_Lock;          // 线程同步锁（用于多线程渲染）
	int m_SurfaceWidth;         // 渲染表面宽度
	int m_SurfaceHeight;        // 渲染表面高度
};


#endif //NDK_OPENGLES_3_0_GLSAMPLEBASE_H
