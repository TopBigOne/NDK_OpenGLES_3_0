#include "GLUtils.h"
#include "LogUtil.h"
#include <stdlib.h>
#include <cstring>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

GLuint GLUtils::LoadShader(GLenum shaderType, const char *pSource)
{
    GLuint shader = 0;
	FUN_BEGIN_TIME("GLUtils::LoadShader")
        shader = glCreateShader(shaderType);
        if (shader)
        {
            glShaderSource(shader, 1, &pSource, NULL);
            glCompileShader(shader);
            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                GLint infoLen = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
                if (infoLen)
                {
                    char* buf = (char*) malloc((size_t)infoLen);
                    if (buf)
                    {
                        glGetShaderInfoLog(shader, infoLen, NULL, buf);
                        LOGCATE("GLUtils::LoadShader Could not compile shader %d:\n%s\n", shaderType, buf);
                        free(buf);
                    }
                    glDeleteShader(shader);
                    shader = 0;
                }
            }
        }
	FUN_END_TIME("GLUtils::LoadShader")
	return shader;
}

GLuint GLUtils::CreateProgram(const char *pVertexShaderSource, const char *pFragShaderSource, GLuint &vertexShaderHandle, GLuint &fragShaderHandle)
{
    GLuint program = 0;
    FUN_BEGIN_TIME("GLUtils::CreateProgram")
        vertexShaderHandle = LoadShader(GL_VERTEX_SHADER, pVertexShaderSource);
        if (!vertexShaderHandle) return program;
        fragShaderHandle = LoadShader(GL_FRAGMENT_SHADER, pFragShaderSource);
        if (!fragShaderHandle) return program;

        program = glCreateProgram();
        if (program)
        {
            glAttachShader(program, vertexShaderHandle);
            CheckGLError("glAttachShader");
            glAttachShader(program, fragShaderHandle);
            CheckGLError("glAttachShader");
            glLinkProgram(program);
            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

            glDetachShader(program, vertexShaderHandle);
            glDeleteShader(vertexShaderHandle);
            vertexShaderHandle = 0;
            glDetachShader(program, fragShaderHandle);
            glDeleteShader(fragShaderHandle);
            fragShaderHandle = 0;
            if (linkStatus != GL_TRUE)
            {
                GLint bufLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
                if (bufLength)
                {
                    char* buf = (char*) malloc((size_t)bufLength);
                    if (buf)
                    {
                        glGetProgramInfoLog(program, bufLength, NULL, buf);
                        LOGCATE("GLUtils::CreateProgram Could not link program:\n%s\n", buf);
                        free(buf);
                    }
                }
                glDeleteProgram(program);
                program = 0;
            }
        }
    FUN_END_TIME("GLUtils::CreateProgram")
    LOGCATD("GLUtils::CreateProgram program = %d", program);
	return program;
}

GLuint GLUtils::CreateProgramWithFeedback(const char *pVertexShaderSource, const char *pFragShaderSource, GLuint &vertexShaderHandle, GLuint &fragShaderHandle, GLchar const **varying, int varyingCount)
{
    GLuint program = 0;
    FUN_BEGIN_TIME("GLUtils::CreateProgramWithFeedback")
        vertexShaderHandle = LoadShader(GL_VERTEX_SHADER, pVertexShaderSource);
        if (!vertexShaderHandle) return program;

        fragShaderHandle = LoadShader(GL_FRAGMENT_SHADER, pFragShaderSource);
        if (!fragShaderHandle) return program;

        program = glCreateProgram();
        if (program)
        {
            glAttachShader(program, vertexShaderHandle);
            CheckGLError("glAttachShader");
            glAttachShader(program, fragShaderHandle);
            CheckGLError("glAttachShader");

            //transform feedback
            glTransformFeedbackVaryings(program, varyingCount, varying, GL_INTERLEAVED_ATTRIBS);
            GO_CHECK_GL_ERROR();

            glLinkProgram(program);
            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

            glDetachShader(program, vertexShaderHandle);
            glDeleteShader(vertexShaderHandle);
            vertexShaderHandle = 0;
            glDetachShader(program, fragShaderHandle);
            glDeleteShader(fragShaderHandle);
            fragShaderHandle = 0;
            if (linkStatus != GL_TRUE)
            {
                GLint bufLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
                if (bufLength)
                {
                    char* buf = (char*) malloc((size_t)bufLength);
                    if (buf)
                    {
                        glGetProgramInfoLog(program, bufLength, NULL, buf);
                        LOGCATE("GLUtils::CreateProgramWithFeedback Could not link program:\n%s\n", buf);
                        free(buf);
                    }
                }
                glDeleteProgram(program);
                program = 0;
            }
        }
    FUN_END_TIME("GLUtils::CreateProgramWithFeedback")
    LOGCATD("GLUtils::CreateProgramWithFeedback program = %d", program);
    return program;
}

GLuint GLUtils::CreateProgramWithGeometryShader(const char *pVertexShaderSource, const char *pFragShaderSource, const char *pGeometryShaderSource)
{
    GLuint program = GL_NONE;
    GLuint vertexShaderHandle = GL_NONE, fragShaderHandle = GL_NONE, geometryShaderHandle = GL_NONE;
    FUN_BEGIN_TIME("GLUtils::CreateProgramWithGeometryShader")
        vertexShaderHandle = LoadShader(GL_VERTEX_SHADER, pVertexShaderSource);
        if (!vertexShaderHandle) return program;
        fragShaderHandle = LoadShader(GL_FRAGMENT_SHADER, pFragShaderSource);
        if (!fragShaderHandle) return program;
        geometryShaderHandle = LoadShader(GL_GEOMETRY_SHADER, pGeometryShaderSource);
        if (!geometryShaderHandle) return program;
        program = glCreateProgram();
        if (program)
        {
            glAttachShader(program, vertexShaderHandle);
            CheckGLError("CreateProgramWithGeometryShader::glAttachShader vs");
            glAttachShader(program, geometryShaderHandle);
            CheckGLError("CreateProgramWithGeometryShader::glAttachShader gs");
            glAttachShader(program, fragShaderHandle);
            CheckGLError("CreateProgramWithGeometryShader::glAttachShader fs");
            glLinkProgram(program);
            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

            glDetachShader(program, vertexShaderHandle);
            glDeleteShader(vertexShaderHandle);
            vertexShaderHandle = GL_NONE;
            glDetachShader(program, geometryShaderHandle);
            glDeleteShader(geometryShaderHandle);
            geometryShaderHandle = GL_NONE;
            glDetachShader(program, fragShaderHandle);
            glDeleteShader(fragShaderHandle);
            fragShaderHandle = GL_NONE;
            if (linkStatus != GL_TRUE)
            {
                GLint bufLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
                if (bufLength)
                {
                    char* buf = (char*) malloc((size_t)bufLength);
                    if (buf)
                    {
                        glGetProgramInfoLog(program, bufLength, NULL, buf);
                        LOGCATE("GLUtils::CreateProgramWithGeometryShader Could not link program:\n%s\n", buf);
                        free(buf);
                    }
                }
                glDeleteProgram(program);
                program = 0;
            }
        }
    FUN_END_TIME("GLUtils::CreateProgramWithGeometryShader")
    LOGCATD("GLUtils::CreateProgramWithGeometryShader program = %d", program);
    return program;
}

// 当前线程的“丢弃 GL 对象”嵌套计数，见 ScopedAbandonGLObjects
static thread_local int s_AbandonGLObjectsDepth = 0;

GLUtils::ScopedAbandonGLObjects::ScopedAbandonGLObjects()
{
    ++s_AbandonGLObjectsDepth;
}

GLUtils::ScopedAbandonGLObjects::~ScopedAbandonGLObjects()
{
    --s_AbandonGLObjectsDepth;
}

bool GLUtils::IsAbandoningGLObjects()
{
    return s_AbandonGLObjectsDepth > 0;
}

// 数组里是否存在非 0 句柄；全为 0 时不必调用 GL
static bool HasAnyName(GLsizei n, const GLuint *names)
{
    if (names == nullptr) return false;
    for (GLsizei i = 0; i < n; ++i)
    {
        if (names[i] != 0) return true;
    }
    return false;
}

static void ZeroNames(GLsizei n, GLuint *names)
{
    if (names == nullptr) return;
    for (GLsizei i = 0; i < n; ++i)
    {
        names[i] = 0;
    }
}

void GLUtils::DeleteProgram(GLuint &program)
{
    if (program)
    {
        if (!IsAbandoningGLObjects())
        {
            LOGCATD("GLUtils::DeleteProgram program=%u", program);
            glUseProgram(0);
            glDeleteProgram(program);
        }
        program = 0;
    }
}

void GLUtils::DeleteShader(GLuint &shader)
{
    if (shader)
    {
        if (!IsAbandoningGLObjects()) glDeleteShader(shader);
        shader = 0;
    }
}

void GLUtils::DeleteTextures(GLsizei n, GLuint *textures)
{
    if (HasAnyName(n, textures) && !IsAbandoningGLObjects()) glDeleteTextures(n, textures);
    ZeroNames(n, textures);
}

void GLUtils::DeleteBuffers(GLsizei n, GLuint *buffers)
{
    if (HasAnyName(n, buffers) && !IsAbandoningGLObjects()) glDeleteBuffers(n, buffers);
    ZeroNames(n, buffers);
}

void GLUtils::DeleteVertexArrays(GLsizei n, GLuint *arrays)
{
    if (HasAnyName(n, arrays) && !IsAbandoningGLObjects()) glDeleteVertexArrays(n, arrays);
    ZeroNames(n, arrays);
}

void GLUtils::DeleteFramebuffers(GLsizei n, GLuint *framebuffers)
{
    if (HasAnyName(n, framebuffers) && !IsAbandoningGLObjects()) glDeleteFramebuffers(n, framebuffers);
    ZeroNames(n, framebuffers);
}

void GLUtils::DeleteRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
    if (HasAnyName(n, renderbuffers) && !IsAbandoningGLObjects()) glDeleteRenderbuffers(n, renderbuffers);
    ZeroNames(n, renderbuffers);
}

void GLUtils::DeleteTransformFeedbacks(GLsizei n, GLuint *ids)
{
    if (HasAnyName(n, ids) && !IsAbandoningGLObjects()) glDeleteTransformFeedbacks(n, ids);
    ZeroNames(n, ids);
}

void GLUtils::CheckGLError(const char *pGLOperation)
{
    for (GLint error = glGetError(); error; error = glGetError())
    {
        LOGCATE("GLUtils::CheckGLError GL Operation %s() glError (0x%x)\n", pGLOperation, error);
    }

}

GLuint GLUtils::CreateProgram(const char *pVertexShaderSource, const char *pFragShaderSource) {
    GLuint vertexShaderHandle, fragShaderHandle;
    return CreateProgram(pVertexShaderSource, pFragShaderSource, vertexShaderHandle, fragShaderHandle);
}

GLuint GLUtils::LoadComputeShader(const char* computeShaderSource) {
    LOGCATD("GLUtils::LoadComputeShader Compute shader: %s", computeShaderSource);
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderSource, NULL);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        LOGCATE("GLUtils::LoadComputeShader Compute shader compilation failed: %s", infoLog);
        return 0;
    }

    GLuint computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);

    glGetProgramiv(computeProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(computeProgram, 512, NULL, infoLog);
        LOGCATE("GLUtils::LoadComputeShader Compute shader linking failed: %s", infoLog);
        return 0;
    }

    glDeleteShader(computeShader);
    return computeProgram;
}
