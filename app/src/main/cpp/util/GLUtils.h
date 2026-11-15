/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef _BYTE_FLOW_GL_UTILS_H_
#define _BYTE_FLOW_GL_UTILS_H_

#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <string>
#include <glm.hpp>

#define SHADER_TO_STRING(s) #s

#define MATH_PI 3.1415926535897932384626433832802

class GLUtils {
public:
    static GLuint LoadShader(GLenum shaderType, const char *pSource);

    static GLuint CreateProgram(const char *pVertexShaderSource, const char *pFragShaderSource,
                                GLuint &vertexShaderHandle,
                                GLuint &fragShaderHandle);

    static GLuint CreateProgram(const char *pVertexShaderSource, const char *pFragShaderSource);

    static GLuint CreateProgramWithFeedback(
            const char *pVertexShaderSource,
            const char *pFragShaderSource,
            GLuint &vertexShaderHandle,
            GLuint &fragShaderHandle,
            const GLchar **varying,
            int varyingCount);

    static GLuint CreateProgramWithGeometryShader(const char *pVertexShaderSource, const char *pFragShaderSource, const char *pGeometryShaderSource);

    // ==================== GL 对象释放 ====================
    // 下面的 DeleteXXX 都会在删除后把句柄清零，重复调用是安全的（防止重复删除）。
    // 在 ScopedAbandonGLObjects 生效期间（上下文已经丢失），它们只清零句柄，不调用 glDelete*：
    // 旧上下文里的名字在新上下文里可能已经属于别的对象，删掉会破坏新上下文中的资源。
    static void DeleteProgram(GLuint &program);
    static void DeleteShader(GLuint &shader);
    static void DeleteTextures(GLsizei n, GLuint *textures);
    static void DeleteBuffers(GLsizei n, GLuint *buffers);
    static void DeleteVertexArrays(GLsizei n, GLuint *arrays);
    static void DeleteFramebuffers(GLsizei n, GLuint *framebuffers);
    static void DeleteRenderbuffers(GLsizei n, GLuint *renderbuffers);
    static void DeleteTransformFeedbacks(GLsizei n, GLuint *ids);
    static void DeleteTexture(GLuint &texture) { DeleteTextures(1, &texture); }
    static void DeleteBuffer(GLuint &buffer) { DeleteBuffers(1, &buffer); }
    static void DeleteVertexArray(GLuint &array) { DeleteVertexArrays(1, &array); }
    static void DeleteFramebuffer(GLuint &framebuffer) { DeleteFramebuffers(1, &framebuffer); }
    static void DeleteRenderbuffer(GLuint &renderbuffer) { DeleteRenderbuffers(1, &renderbuffer); }

    // 当前线程是否处于“丢弃 GL 对象”模式
    static bool IsAbandoningGLObjects();

    // RAII：在作用域内把当前线程切换到“丢弃 GL 对象”模式。
    // 用 thread_local 计数实现，只影响调用线程，不影响其它持有共享上下文的渲染线程。
    class ScopedAbandonGLObjects {
    public:
        ScopedAbandonGLObjects();
        ~ScopedAbandonGLObjects();
        ScopedAbandonGLObjects(const ScopedAbandonGLObjects &) = delete;
        ScopedAbandonGLObjects &operator=(const ScopedAbandonGLObjects &) = delete;
    };

    static void CheckGLError(const char *pGLOperation);

    static GLuint LoadComputeShader(const char* computeShaderSource);

    static void setBool(GLuint programId, const std::string &name, bool value) {
        glUniform1i(glGetUniformLocation(programId, name.c_str()), (int) value);
    }

    static void setInt(GLuint programId, const std::string &name, int value) {
        glUniform1i(glGetUniformLocation(programId, name.c_str()), value);
    }

    static void setFloat(GLuint programId, const std::string &name, float value) {
        glUniform1f(glGetUniformLocation(programId, name.c_str()), value);
    }

    static void setVec2(GLuint programId, const std::string &name, const glm::vec2 &value) {
        glUniform2fv(glGetUniformLocation(programId, name.c_str()), 1, &value[0]);
    }

    static void setVec2(GLuint programId, const std::string &name, float x, float y) {
        glUniform2f(glGetUniformLocation(programId, name.c_str()), x, y);
    }

    static void setVec3(GLuint programId, const std::string &name, const glm::vec3 &value) {
        glUniform3fv(glGetUniformLocation(programId, name.c_str()), 1, &value[0]);
    }

    static void setVec3(GLuint programId, const std::string &name, float x, float y, float z) {
        glUniform3f(glGetUniformLocation(programId, name.c_str()), x, y, z);
    }

    static void setVec4(GLuint programId, const std::string &name, const glm::vec4 &value) {
        glUniform4fv(glGetUniformLocation(programId, name.c_str()), 1, &value[0]);
    }

    static void setVec4(GLuint programId, const std::string &name, float x, float y, float z, float w) {
        glUniform4f(glGetUniformLocation(programId, name.c_str()), x, y, z, w);
    }

    static void setMat2(GLuint programId, const std::string &name, const glm::mat2 &mat) {
        glUniformMatrix2fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    static void setMat3(GLuint programId, const std::string &name, const glm::mat3 &mat) {
        glUniformMatrix3fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    static void setMat4(GLuint programId, const std::string &name, const glm::mat4 &mat) {
        glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    static glm::vec3 texCoordToVertexCoord(glm::vec2 &texCoord) {
        return glm::vec3(2 * texCoord.x - 1, 1 - 2 * texCoord.y, 0);
    }

};

#endif // _BYTE_FLOW_GL_UTILS_H_