/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <GLUtils.h>
#include "GLRenderLooper.h"
mutex GLRenderLooper::m_Mutex;
GLRenderLooper* GLRenderLooper::m_Instance = nullptr;

GLRenderLooper::~GLRenderLooper() {
    // 必须在派生类析构时就退出线程：如果留给 ~Looper() 去 quit，此时对象已退化为 Looper，
    // 工作线程还在处理的消息（例如 MSG_SurfaceDestroyed）会被派发到 Looper::handleMessage，
    // EGL 上下文和 GL 资源就得不到释放，并且存在对已析构成员的访问。
    quit();
}

GLRenderLooper::GLRenderLooper() {
    // 原来这些成员都没有初始化
    m_GLEnv = nullptr;
    m_VaoId = GL_NONE;
    m_FboTextureId = GL_NONE;
    m_FboId = GL_NONE;
}

void GLRenderLooper::handleMessage(LooperMessage *msg) {
    Looper::handleMessage(msg);
    switch (msg->what) {
        case MSG_SurfaceCreated: {
            LOGCATV("GLRenderLooper::handleMessage MSG_SurfaceCreated");
            m_GLEnv = (GLEnv *)msg->obj;
            OnSurfaceCreated();
        }
            break;
        case MSG_SurfaceChanged:
            LOGCATV("GLRenderLooper::handleMessage MSG_SurfaceChanged");
            OnSurfaceChanged(msg->arg1, msg->arg2);
            break;
        case MSG_DrawFrame:
            LOGCATV("GLRenderLooper::handleMessage MSG_DrawFrame");
            OnDrawFrame();
            break;
        case MSG_SurfaceDestroyed:
            LOGCATV("GLRenderLooper::handleMessage MSG_SurfaceDestroyed");
            OnSurfaceDestroyed();
            break;
        default:
            break;
    }
}

GLRenderLooper *GLRenderLooper::GetInstance() {
    LOGCATV("GLRenderLooper::GetInstance");
    // 原来的双重检查在锁外读 m_Instance，没有原子性保证，属于数据竞争；直接加锁即可
    unique_lock<mutex> lock(m_Mutex);
    if(m_Instance == nullptr) {
        m_Instance = new GLRenderLooper();
    }
    return m_Instance;
}

void GLRenderLooper::ReleaseInstance() {
    LOGCATD("GLRenderLooper::ReleaseInstance");
    unique_lock<mutex> lock(m_Mutex);
    if(m_Instance != nullptr) {
        delete m_Instance;
        m_Instance = nullptr;
    }
}

void GLRenderLooper::OnSurfaceCreated() {
    LOGCATD("GLRenderLooper::OnSurfaceCreated");
    // 主线程上下文是 ES 3，工作线程也要用 VAO 等 ES 3 接口，原来只传 FLAG_RECORDABLE 会请求 ES 2 上下文
    m_EglCore = new EglCore(m_GLEnv->sharedCtx, FLAG_RECORDABLE | FLAG_TRY_GLES3);
    SizeF imgSizeF = m_GLEnv->imgSize;
    m_OffscreenSurface = new OffscreenSurface(m_EglCore, imgSizeF.width, imgSizeF.height);
    m_OffscreenSurface->makeCurrent();

    glGenVertexArrays(1, &m_VaoId);
    glBindVertexArray(m_VaoId);

    glBindBuffer(GL_ARRAY_BUFFER, m_GLEnv->vboIds[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ARRAY_BUFFER, m_GLEnv->vboIds[1]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_GLEnv->vboIds[2]);
    GO_CHECK_GL_ERROR();
    glBindVertexArray(GL_NONE);

    if (!CreateFrameBufferObj())
    {
        LOGCATE("GLRenderLooper::OnSurfaceCreated CreateFrameBufferObj fail");
        // 不完整的 FBO 不能用来渲染，释放后让 OnDrawFrame 走失败分支
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
        GLUtils::DeleteFramebuffer(m_FboId);
        GLUtils::DeleteTexture(m_FboTextureId);
    }
}

void GLRenderLooper::OnSurfaceChanged(int w, int h) {
    LOGCATD("GLRenderLooper::OnSurfaceChanged [w,h]=[%d, %d]", w, h);
}

void GLRenderLooper::OnDrawFrame() {
    LOGCATV("GLRenderLooper::OnDrawFrame");
    SizeF imgSizeF = m_GLEnv->imgSize;
    if (m_EglCore == nullptr || m_EglCore->getEGLContext() == EGL_NO_CONTEXT || m_OffscreenSurface == nullptr || m_FboId == GL_NONE)
    {
        // 工作线程的 EGL/FBO 初始化失败：仍然回调，避免主线程一直等待；纹理传 0 表示无结果
        m_GLEnv->renderDone(m_GLEnv->callbackCtx, 0);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
    glViewport(0, 0, imgSizeF.width, imgSizeF.height);
    glUseProgram(m_GLEnv->program);
    glBindVertexArray(m_VaoId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_GLEnv->inputTexId);
    GLUtils::setInt(m_GLEnv->program, "s_TextureMap", 0);
    float offset = (sin(m_FrameIndex * MATH_PI / 80) + 1.0f) / 2.0f;
    GLUtils::setFloat(m_GLEnv->program, "u_Offset", offset);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_OffscreenSurface->swapBuffers();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_GLEnv->renderDone(m_GLEnv->callbackCtx, m_FboTextureId);
    m_FrameIndex++;
}

void GLRenderLooper::OnSurfaceDestroyed() {
    LOGCATD("GLRenderLooper::OnSurfaceDestroyed");
    m_GLEnv->renderDone(m_GLEnv->callbackCtx, m_FboTextureId);

    // 这些对象属于工作线程自己的上下文，总是真实删除；删除后清零，防止重复删除
    GLUtils::DeleteVertexArray(m_VaoId);
    GLUtils::DeleteFramebuffer(m_FboId);
    GLUtils::DeleteTexture(m_FboTextureId);

    if (m_OffscreenSurface) {
        m_OffscreenSurface->release();
        delete m_OffscreenSurface;
        m_OffscreenSurface = nullptr;
    }

    if (m_EglCore) {
        m_EglCore->release();
        delete m_EglCore;
        m_EglCore = nullptr;
    }
}

bool GLRenderLooper::CreateFrameBufferObj() {
    // 创建并初始化 FBO 纹理
    glGenTextures(1, &m_FboTextureId);
    glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, GL_NONE);

    // 创建并初始化 FBO
    glGenFramebuffers(1, &m_FboId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
    glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FboTextureId, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_GLEnv->imgSize.width, m_GLEnv->imgSize.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)!= GL_FRAMEBUFFER_COMPLETE) {
        LOGCATE("RGB2YUYVSample::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    return true;
}


