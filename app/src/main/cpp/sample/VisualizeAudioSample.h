/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_VISUALIZEAUDIOSAMPLE_H
#define NDK_OPENGLES_3_0_VISUALIZEAUDIOSAMPLE_H

#define MAX_AUDIO_LEVEL 2000
#define RESAMPLE_LEVEL  112

#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include <mutex>
#include "GLSampleBase.h"

using namespace glm;

class VisualizeAudioSample : public GLSampleBase
{
public:
	VisualizeAudioSample();

	virtual ~VisualizeAudioSample();

	virtual void LoadImage(NativeImage *pImage);

    virtual void LoadShortArrData(short *const pShortArr, int arrSize);

    virtual bool Init();
	virtual void Draw(int screenW, int screenH);

	virtual void Destroy();

	virtual void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY);

	void UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio);

	void UpdateMesh();

private:
	// 音频块大小变化时重新分配所有 CPU 缓冲（调用方需持有 m_Mutex）
	void ReallocAudioBuffers(int arrSize);
	// 窗口内数据已经走完时，把待处理的最新音频块移入窗口（调用方需持有 m_Mutex）
	bool ShiftPendingAudioIntoWindow();

private:
	GLint m_SamplerLoc;
	GLint m_MVPMatLoc;
	GLuint m_VaoId;
	GLuint m_VboIds[2];
	glm::mat4 m_MVPMatrix;

	int m_AngleX;
	int m_AngleY;
	float m_ScaleX;
	float m_ScaleY;

	short *m_pCurAudioData;      // 当前绘制位置（指向 m_pAudioBuffer 窗口内）
	short *m_pAudioBuffer;       // 窗口：前一块 + 当前块，容量 m_AudioDataSize * 2
	short *m_pPendingAudio;      // 音频线程写入的最新一块，容量 m_AudioDataSize
	int m_AudioDataSize;         // 单块音频的样本数，即缓冲容量的依据
	bool m_bPendingReady;        // m_pPendingAudio 中有尚未消费的数据
	int m_WindowChunkCount;      // 已经移入窗口的块数，>= 2 时窗口才完整
	std::mutex m_Mutex;

	vec3 *m_pVerticesCoords;
	vec2 *m_pTextureCoords;
	int m_RenderDataSize;        // 网格条数 = m_AudioDataSize / RESAMPLE_LEVEL
	int m_VboRenderDataSize;     // 当前 VBO 按多少条网格分配，与 m_RenderDataSize 不一致时需重建

	bool m_bAudioDataReady;      // 窗口完整且 m_pCurAudioData 还没走完

};


#endif //NDK_OPENGLES_3_0_COORDSYSTEMSAMPLE_H
