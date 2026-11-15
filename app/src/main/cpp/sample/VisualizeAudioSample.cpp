/**
 *
 * Created by 公众号：字节流动 on 2022/4/18.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <gtc/matrix_transform.hpp>
#include <thread>
#include "VisualizeAudioSample.h"
#include "../util/GLUtils.h"

VisualizeAudioSample::VisualizeAudioSample() {

    m_SamplerLoc = GL_NONE;
    m_MVPMatLoc = GL_NONE;

    m_VaoId = GL_NONE;

    m_AngleX = 0;
    m_AngleY = 0;

    m_ScaleX = 1.0f;
    m_ScaleY = 1.0f;

    m_pCurAudioData = nullptr;
    m_AudioDataSize = 0;

    m_pTextureCoords = nullptr;
    m_pVerticesCoords = nullptr;

    memset(m_VboIds, 0, sizeof(GLuint) * 2);

    m_pAudioBuffer = nullptr;
    m_pPendingAudio = nullptr;
    m_bPendingReady = false;
    m_WindowChunkCount = 0;
    m_RenderDataSize = 0;
    m_VboRenderDataSize = 0;
    m_bAudioDataReady = false;
}

VisualizeAudioSample::~VisualizeAudioSample() {
    if (m_pAudioBuffer != nullptr) {
        delete [] m_pAudioBuffer;
        m_pAudioBuffer = nullptr;
    }

    if (m_pPendingAudio != nullptr) {
        delete [] m_pPendingAudio;
        m_pPendingAudio = nullptr;
    }

    if (m_pTextureCoords != nullptr) {
        delete [] m_pTextureCoords;
        m_pTextureCoords = nullptr;
    }

    if (m_pVerticesCoords != nullptr) {
        delete [] m_pVerticesCoords;
        m_pVerticesCoords = nullptr;
    }


}

bool VisualizeAudioSample::Init() {
    char vShaderStr[] =
            "#version 300 es\n"
            "layout(location = 0) in vec4 a_position;\n"
            "layout(location = 1) in vec2 a_texCoord;\n"
            "uniform mat4 u_MVPMatrix;\n"
            "out vec2 v_texCoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = u_MVPMatrix * a_position;\n"
            "    v_texCoord = a_texCoord;\n"
            "    gl_PointSize = 4.0f;\n"
            "}";

    char fShaderStr[] =
            "#version 300 es                                     \n"
            "precision mediump float;                            \n"
            "in vec2 v_texCoord;                                 \n"
            "layout(location = 0) out vec4 outColor;             \n"
            "uniform float drawType;                             \n"
            "void main()                                         \n"
            "{                                                   \n"
            "  if(drawType == 1.0)                               \n"
            "  {                                                 \n"
            "      outColor = vec4(1.5 - v_texCoord.y, 0.3, 0.3, 1.0); \n"
            "  }                                                 \n"
            "  else                                              \n"
            "  {                                                 \n"
            "      outColor = vec4(1.0, 1.0, 1.0, 1.0);          \n"
            "  }                                                 \n"
            "}                                                   \n";

    m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr, m_VertexShader, m_FragmentShader);
    if (m_ProgramObj) {
        m_MVPMatLoc = glGetUniformLocation(m_ProgramObj, "u_MVPMatrix");
    } else {
        LOGCATE("VisualizeAudioSample::Init create program fail");
        return false;
    }

    //upload RGBA image data
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, m_TextureId);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderImage.width, m_RenderImage.height, 0, GL_RGBA,
//                 GL_UNSIGNED_BYTE, m_RenderImage.ppPlane[0]);
//    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    return true;
}

void VisualizeAudioSample::LoadImage(NativeImage *pImage) {
    if (pImage == nullptr) return;  // 先判空再访问，原来日志里先解引用了 pImage
    LOGCATD("VisualizeAudioSample::LoadImage pImage = %p", pImage->ppPlane[0]);

}


void VisualizeAudioSample::Draw(int screenW, int screenH) {
    LOGCATV("VisualizeAudioSample::Draw()");
    glClearColor(1.0f, 1.0f, 1.0f, 1.0);
    if (m_ProgramObj == GL_NONE) return;
    // 原来这里在 GL 线程上 sleep 25ms，期间还持有 MyGLRenderContext 的锁，会把 UI 线程和
    // 音频线程的 SetXXX 一起卡住；节奏现在由“每帧推进一步、走完再换下一块音频”控制，不再需要 sleep。
    // 整个 Draw 持有 m_Mutex：网格数组可能被音频线程在尺寸变化时重新分配，上传 VBO 时必须互斥。
    std::unique_lock<std::mutex> lock(m_Mutex);
    UpdateMesh();
    // 窗口第一次攒满之前还没有算过网格，不画；之后即使暂时没有新音频，也继续画上一帧的网格，避免闪白
    if (m_WindowChunkCount < 2 || m_RenderDataSize <= 0 || m_pVerticesCoords == nullptr) return;

    UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, (float) screenW / screenH);

    // 音频块大小变化后网格条数变了，旧 VBO 容量不够，需要按新大小重建（VAO 引用旧 VBO，一并重建）
    if (m_VboIds[0] != GL_NONE && m_VboRenderDataSize != m_RenderDataSize)
    {
        GLUtils::DeleteBuffers(2, m_VboIds);
        GLUtils::DeleteVertexArray(m_VaoId);
    }

    // Generate VBO Ids and load the VBOs with data
    if(m_VboIds[0] == 0)
    {
        glGenBuffers(2, m_VboIds);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * m_RenderDataSize * 6 * 3, m_pVerticesCoords, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * m_RenderDataSize * 6 * 2, m_pTextureCoords, GL_DYNAMIC_DRAW);
        m_VboRenderDataSize = m_RenderDataSize;
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * m_RenderDataSize * 6 * 3, m_pVerticesCoords);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * m_RenderDataSize * 6 * 2, m_pTextureCoords);
    }

    if(m_VaoId == GL_NONE)
    {
        glGenVertexArrays(1, &m_VaoId);
        glBindVertexArray(m_VaoId);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

        glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

        glBindVertexArray(GL_NONE);
    }


    // Use the program object
    glUseProgram(m_ProgramObj);
    glBindVertexArray(m_VaoId);
    glUniformMatrix4fv(m_MVPMatLoc, 1, GL_FALSE, &m_MVPMatrix[0][0]);
    GLUtils::setFloat(m_ProgramObj, "drawType", 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, m_RenderDataSize * 6);
    GLUtils::setFloat(m_ProgramObj, "drawType", 0.0f);
    glDrawArrays(GL_LINES, 0, m_RenderDataSize * 6);


}

void VisualizeAudioSample::Destroy() {
    // VBO/VAO 在 Draw 中懒创建，helper 清零后上下文重建时 Draw 会重新创建
    GLUtils::DeleteProgram(m_ProgramObj);
    GLUtils::DeleteBuffers(2, m_VboIds);
    GLUtils::DeleteVertexArray(m_VaoId);
    m_VboRenderDataSize = 0;

}

void VisualizeAudioSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio) {
    LOGCATV("VisualizeAudioSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX,
            angleY, ratio);
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    //glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f, ratio, 0.1f, 100.f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));

    float radiansZ = static_cast<float>(MATH_PI / 2.0f);

    //Model = glm::rotate(Model, radiansZ, glm::vec3(0.0f, 0.0f, 1.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    mvpMatrix = Projection * View * Model;

}

void VisualizeAudioSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX,
                                                 float scaleY) {
    GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
    m_AngleX = static_cast<int>(rotateX);
    m_AngleY = static_cast<int>(rotateY);
    m_ScaleX = scaleX;
    m_ScaleY = scaleY;
}

void VisualizeAudioSample::LoadShortArrData(short *const pShortArr, int arrSize) {
    LOGCATV("VisualizeAudioSample::LoadShortArrData pShortArr=%p, arrSize=%d", pShortArr, arrSize);
    if (pShortArr == nullptr || arrSize <= 0)
        return;

    // 原实现在这里 m_Cond.wait() 等 GL 线程把当前窗口画完。但本函数是在
    // MyGLRenderContext 的锁内被音频线程调用的，wait 只释放示例自己的 m_Mutex，
    // GL 线程拿不到上下文锁就永远走不到 notify —— 音频一开始就死锁。
    // 现在不再阻塞：只把最新一块数据拷贝到 m_pPendingAudio，GL 线程走完当前窗口后再取用，
    // 期间来不及消费的旧块直接被新块覆盖（可视化只需要最新数据）。
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (arrSize != m_AudioDataSize || m_pPendingAudio == nullptr)
    {
        // 原实现按第一块的大小分配缓冲，之后块变大会越界写；大小变化时按新容量重新分配
        ReallocAudioBuffers(arrSize);
    }
    memcpy(m_pPendingAudio, pShortArr, sizeof(short) * arrSize);
    m_bPendingReady = true;
}

void VisualizeAudioSample::ReallocAudioBuffers(int arrSize) {
    delete [] m_pAudioBuffer;
    delete [] m_pPendingAudio;
    delete [] m_pVerticesCoords;
    delete [] m_pTextureCoords;

    m_AudioDataSize = arrSize;
    m_pAudioBuffer = new short[arrSize * 2];
    m_pPendingAudio = new short[arrSize];
    m_RenderDataSize = arrSize / RESAMPLE_LEVEL;
    m_pVerticesCoords = m_RenderDataSize > 0 ? new vec3[m_RenderDataSize * 6] : nullptr; //(x,y,z) * 6 points
    m_pTextureCoords = m_RenderDataSize > 0 ? new vec2[m_RenderDataSize * 6] : nullptr;  //(x,y) * 6 points

    // 窗口需要重新攒满两块
    m_pCurAudioData = m_pAudioBuffer;
    m_WindowChunkCount = 0;
    m_bPendingReady = false;
    m_bAudioDataReady = false;
}

bool VisualizeAudioSample::ShiftPendingAudioIntoWindow() {
    if (!m_bPendingReady || m_pAudioBuffer == nullptr) return false;

    // 窗口 = 前一块 + 当前块：先把后半段挪到前半段，再把最新块放到后半段（与原实现相同）
    memmove(m_pAudioBuffer, m_pAudioBuffer + m_AudioDataSize, sizeof(short) * m_AudioDataSize);
    memcpy(m_pAudioBuffer + m_AudioDataSize, m_pPendingAudio, sizeof(short) * m_AudioDataSize);
    m_bPendingReady = false;
    if (m_WindowChunkCount < 2) m_WindowChunkCount++;
    if (m_WindowChunkCount < 2) return false;

    m_pCurAudioData = m_pAudioBuffer;
    m_bAudioDataReady = true;
    return true;
}

void VisualizeAudioSample::UpdateMesh() {
    // 当前窗口已经走完（或还没开始）时，换入音频线程送来的最新一块
    if (!m_bAudioDataReady && !ShiftPendingAudioIntoWindow())
        return;
    if (m_RenderDataSize <= 0)
        return;

    //calculate mesh
    // step 至少为 1：块很小时 m_AudioDataSize / 64 为 0，原实现会原地不动
    int step = m_AudioDataSize / 64 > 0 ? m_AudioDataSize / 64 : 1;
    if(m_pAudioBuffer + m_AudioDataSize - m_pCurAudioData >= step)
    {
        LOGCATV("VisualizeAudioSample::Draw() m_pAudioBuffer + m_AudioDataSize - m_pCurAudioData >= step");
        float dy = 0.5f / MAX_AUDIO_LEVEL;
        float dx = 1.0f / m_RenderDataSize;
        for (int i = 0; i < m_RenderDataSize; ++i) {
            int index = i * RESAMPLE_LEVEL;
            float y = m_pCurAudioData[index] * dy * -1;
            y = y < 0 ? y : -y;
            vec2 p1(i * dx, 0 + 1.0f);
            vec2 p2(i * dx, y + 1.0f);
            vec2 p3((i + 1) * dx, y + 1.0f);
            vec2 p4((i + 1) * dx, 0 + 1.0f);

            m_pTextureCoords[i * 6 + 0] = p1;
            m_pTextureCoords[i * 6 + 1] = p2;
            m_pTextureCoords[i * 6 + 2] = p4;
            m_pTextureCoords[i * 6 + 3] = p4;
            m_pTextureCoords[i * 6 + 4] = p2;
            m_pTextureCoords[i * 6 + 5] = p3;

            m_pVerticesCoords[i * 6 + 0] = GLUtils::texCoordToVertexCoord(p1);
            m_pVerticesCoords[i * 6 + 1] = GLUtils::texCoordToVertexCoord(p2);
            m_pVerticesCoords[i * 6 + 2] = GLUtils::texCoordToVertexCoord(p4);
            m_pVerticesCoords[i * 6 + 3] = GLUtils::texCoordToVertexCoord(p4);
            m_pVerticesCoords[i * 6 + 4] = GLUtils::texCoordToVertexCoord(p2);
            m_pVerticesCoords[i * 6 + 5] = GLUtils::texCoordToVertexCoord(p3);
        }
        m_pCurAudioData += step;
    }
    else
    {
        LOGCATV("VisualizeAudioSample::Draw() m_pAudioBuffer + m_AudioDataSize - m_pCurAudioData < step");
        // 走完了：下一帧换入新的音频块；没有新块时保留上一帧网格继续显示
        m_bAudioDataReady = ShiftPendingAudioIntoWindow();
        return;
    }
}
