# OpenGL ES 3.0 系统学习计划

> 基于 NDK_OpenGLES_3_0 项目的完整学习路径
> 制定时间: 2026-01-07
> 学习目标: 从零基础到掌握 OpenGL ES 3.0 核心技术

---

## 📋 学习计划概览

### 学习周期
- **总时长**: 12-16 周 (根据个人基础和投入时间调整)
- **每日投入**: 2-3 小时
- **实践项目**: 61 个示例 + 3 个综合项目

### 技能目标
1. 掌握 OpenGL ES 3.0 渲染管线
2. 熟练编写 GLSL Shader 程序
3. 实现各种图形特效和滤镜
4. 具备独立开发图形应用的能力
5. 理解跨平台渲染架构设计

---

## 🎯 第一阶段: 入门准备 (第 1 周)

### Week 1: 环境搭建与基础概念

#### Day 1-2: 开发环境搭建
**任务清单**:
- [ ] 安装 Android Studio 4.1+
- [ ] 配置 NDK r21 (21.1.6352462)
- [ ] Clone 项目: `git clone https://github.com/githubhaohao/NDK_OpenGLES_3_0.git`
- [ ] 编译并运行项目
- [ ] 测试至少 5 个不同的示例

**参考资料**:
- [项目 README.md](../README.md)
- [技术架构分析.md](./技术架构分析.md)

**检查点**:
✓ APP 能成功编译并运行
✓ 能够切换不同示例并看到渲染效果

---

#### Day 3-4: OpenGL ES 基础理论
**学习内容**:
1. **图形渲染管线**
   - 顶点着色器 (Vertex Shader)
   - 图元装配 (Primitive Assembly)
   - 光栅化 (Rasterization)
   - 片段着色器 (Fragment Shader)
   - 测试与混合 (Tests and Blending)

2. **OpenGL ES 核心对象**
   - VBO (Vertex Buffer Object) - 顶点缓冲对象
   - VAO (Vertex Array Object) - 顶点数组对象
   - EBO (Element Buffer Object) - 索引缓冲对象
   - FBO (Frame Buffer Object) - 帧缓冲对象
   - Texture - 纹理对象
   - Shader Program - 着色器程序

3. **坐标系统**
   - 局部空间 (Local Space)
   - 世界空间 (World Space)
   - 观察空间 (View Space)
   - 裁剪空间 (Clip Space)
   - 屏幕空间 (Screen Space)

**学习方法**:
- 阅读文章: [OpenGL ES 3.0 开发（01）：绘制一个三角形](https://blog.csdn.net/Kennethdroid/article/details/95622391)
- 观看视频教程 (公众号"字节流动"获取)
- 手绘渲染管线流程图

**作业**:
- 用自己的话解释渲染管线的每个阶段
- 画出 MVP 矩阵变换的坐标系流程图

---

#### Day 5-7: 代码架构学习
**学习内容**:
1. **项目架构分析**
   - Java 层结构 ([MainActivity.java](../app/src/main/java/com/byteflow/app/MainActivity.java))
   - JNI 桥接层 ([JniImpl.cpp](../app/src/main/cpp/JniImpl.cpp))
   - 渲染管理层 ([MyGLRenderContext](../app/src/main/cpp/render/MyGLRenderContext.cpp))
   - 示例基类 ([GLSampleBase](../app/src/main/cpp/sample/GLSampleBase.h))

2. **设计模式理解**
   - 单例模式 (MyGLRenderContext)
   - 策略模式 (GLSampleBase 继承体系)
   - 工厂模式 (Sample 创建)

**实践任务**:
```bash
# 阅读以下核心文件
app/src/main/cpp/render/MyGLRenderContext.h
app/src/main/cpp/render/MyGLRenderContext.cpp
app/src/main/cpp/sample/GLSampleBase.h
app/src/main/cpp/util/GLUtils.h
```

**作业**:
- 绘制项目架构图
- 追踪一次完整的渲染流程 (从 Java 层点击到 Native 层渲染)

---

## 🚀 第二阶段: 基础渲染 (第 2-4 周)

### Week 2: 基础图形绘制

#### 示例 01: 绘制三角形 (2 天)
**文件**: [TriangleSample.cpp](../app/src/main/cpp/sample/TriangleSample.cpp)

**学习目标**:
- 理解 OpenGL ES 的基本渲染流程
- 掌握顶点着色器和片段着色器的编写
- 学会使用 VBO 和 VAO

**核心代码分析**:
```cpp
// 1. 顶点数据定义
GLfloat vertices[] = {
    0.0f,  0.5f, 0.0f,   // 顶点 0
   -0.5f, -0.5f, 0.0f,   // 顶点 1
    0.5f, -0.5f, 0.0f    // 顶点 2
};

// 2. 着色器编译
m_ProgramObj = GLUtils::CreateProgram(vShaderStr, fShaderStr);

// 3. VBO 创建
glGenBuffers(1, &m_VboId);
glBindBuffer(GL_ARRAY_BUFFER, m_VboId);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

// 4. VAO 配置
glGenVertexArrays(1, &m_VaoId);
glBindVertexArray(m_VaoId);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

// 5. 渲染
glUseProgram(m_ProgramObj);
glBindVertexArray(m_VaoId);
glDrawArrays(GL_TRIANGLES, 0, 3);
```

**实践任务**:
- [ ] 运行并理解 TriangleSample
- [ ] 修改顶点坐标，绘制一个正方形
- [ ] 修改片段着色器，改变三角形颜色
- [ ] 尝试绘制多个不同颜色的三角形

**关键知识点**:
1. `glGenBuffers` vs `glGenVertexArrays` 的区别
2. `glBufferData` 的 `GL_STATIC_DRAW` 参数含义
3. `glVertexAttribPointer` 各参数的作用
4. 为什么需要 VAO？

**博客文章**: [OpenGL ES 3.0 开发（01）：绘制一个三角形](https://blog.csdn.net/Kennethdroid/article/details/95622391)

---

#### 示例 02: 纹理映射 (2 天)
**文件**: [TextureMapSample.cpp](../app/src/main/cpp/sample/TextureMapSample.cpp)

**学习目标**:
- 掌握纹理的创建和绑定
- 理解纹理坐标系统
- 学会使用采样器 (Sampler)

**核心代码分析**:
```cpp
// 1. 创建纹理
glGenTextures(1, &m_TextureId);
glBindTexture(GL_TEXTURE_2D, m_TextureId);

// 2. 设置纹理参数
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// 3. 上传纹理数据
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, pixels);

// 4. 片段着色器中采样
// uniform sampler2D s_TextureMap;
// in vec2 v_texCoord;
// outColor = texture(s_TextureMap, v_texCoord);
```

**实践任务**:
- [ ] 加载并显示一张图片
- [ ] 修改纹理坐标，实现图片翻转 (上下、左右)
- [ ] 修改 `GL_TEXTURE_WRAP` 参数，观察边缘效果
- [ ] 尝试修改 `GL_TEXTURE_MIN/MAG_FILTER`，对比效果

**关键知识点**:
1. 纹理坐标 (0,0) 在哪里？OpenGL vs Android Bitmap 的差异
2. `GL_CLAMP_TO_EDGE` vs `GL_REPEAT` 的区别
3. `GL_LINEAR` vs `GL_NEAREST` 的区别
4. Mipmap 是什么？何时使用？

**博客文章**: [OpenGL ES 3.0 开发（02）：纹理映射](https://blog.csdn.net/Kennethdroid/article/details/96887637)

---

#### 示例 04: VAO、VBO 和 EBO (2 天)
**文件**: [VaoSample.cpp](../app/src/main/cpp/sample/VaoSample.cpp)

**学习目标**:
- 深入理解 VAO 的作用
- 掌握 EBO (索引缓冲) 的使用
- 学习如何减少重复顶点数据

**核心代码分析**:
```cpp
// 顶点数据 (4 个顶点绘制正方形)
GLfloat vertices[] = {
    -0.5f,  0.5f, 0.0f,  // 左上
     0.5f,  0.5f, 0.0f,  // 右上
    -0.5f, -0.5f, 0.0f,  // 左下
     0.5f, -0.5f, 0.0f   // 右下
};

// 索引数据 (2 个三角形 = 6 个索引)
GLushort indices[] = { 0, 1, 2, 1, 3, 2 };

// 创建 EBO
glGenBuffers(1, &m_EboId);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EboId);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

// 使用索引绘制
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
```

**实践任务**:
- [ ] 对比 `glDrawArrays` 和 `glDrawElements` 的差异
- [ ] 绘制一个立方体 (8 个顶点 + 36 个索引)
- [ ] 计算使用 EBO 节省的内存空间

**关键知识点**:
1. 为什么使用索引可以节省内存？
2. VAO 保存了哪些状态？
3. `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)` 和 VAO 的关系

**博客文章**: [OpenGL ES 3.0 开发（04）：VBO、EBO 和 VAO](https://blog.csdn.net/Kennethdroid/article/details/98088890)

---

### Week 3: 坐标系统与 3D 变换

#### 示例 08: 坐标系统 (3 天)
**文件**: [CoordSystemSample.cpp](../app/src/main/cpp/sample/CoordSystemSample.cpp)

**学习目标**:
- 理解 MVP 矩阵变换
- 掌握 GLM 数学库的使用
- 学会 3D 物体的摆放和相机控制

**核心代码分析**:
```cpp
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

// Model 矩阵 (模型变换)
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

// View 矩阵 (观察变换)
glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),  // 相机位置
    glm::vec3(0.0f, 0.0f, 0.0f),  // 观察目标
    glm::vec3(0.0f, 1.0f, 0.0f)   // 上方向
);

// Projection 矩阵 (投影变换)
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),                // FOV (视野角度)
    (float)screenW / (float)screenH,    // 宽高比
    0.1f,                                // 近裁剪面
    100.0f                               // 远裁剪面
);

// MVP 矩阵
glm::mat4 mvp = projection * view * model;

// 传递给着色器
GLUtils::setMat4(m_ProgramObj, "u_MVPMatrix", mvp);
```

**实践任务**:
- [ ] 绘制多个立方体在不同位置
- [ ] 实现立方体的旋转动画
- [ ] 修改相机位置和视角
- [ ] 对比透视投影和正交投影的区别

**关键知识点**:
1. MVP 变换的顺序为什么是 P * V * M？
2. `glm::lookAt` 三个参数的含义
3. FOV (视野角度) 如何影响画面？
4. 近裁剪面和远裁剪面的作用

**作业**:
- 实现一个太阳系模型 (太阳、地球、月球的公转和自转)

**博客文章**: [OpenGL ES 3.0 开发（08）：坐标系统](https://blog.csdn.net/Kennethdroid/article/details/100898155)

---

#### 深度测试与模板测试 (2 天)

**示例 10**: [DepthTestingSample.cpp](../app/src/main/cpp/sample/DepthTestingSample.cpp)
**示例 11**: [StencilTestingSample.cpp](../app/src/main/cpp/sample/StencilTestingSample.cpp)

**学习目标**:
- 理解深度缓冲和 Z-Buffer
- 掌握深度测试的配置
- 学会使用模板测试实现特效

**核心代码**:
```cpp
// 启用深度测试
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);  // 深度值小的通过测试

// 清除深度缓冲
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// 启用模板测试
glEnable(GL_STENCIL_TEST);
glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
```

**实践任务**:
- [ ] 绘制相互遮挡的立方体，观察深度测试效果
- [ ] 使用模板测试实现物体描边效果
- [ ] 禁用深度测试，观察渲染顺序的影响

**博客文章**:
- [OpenGL ES 3.0 开发（10）：深度测试](https://blog.csdn.net/Kennethdroid/article/details/101709694)
- [OpenGL ES 3.0 开发（11）：模板测试](https://blog.csdn.net/Kennethdroid/article/details/102533260)

---

### Week 4: 离屏渲染与 EGL

#### 示例 05: FBO 离屏渲染 (3 天)
**文件**: [FBOSample.cpp](../app/src/main/cpp/sample/FBOSample.cpp)

**学习目标**:
- 掌握 FBO 的创建和使用
- 理解离屏渲染的应用场景
- 学会实现后处理特效

**核心代码**:
```cpp
// 1. 创建 FBO
glGenFramebuffers(1, &m_FboId);
glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);

// 2. 创建颜色附件 (纹理)
glGenTextures(1, &m_FboTextureId);
glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D, m_FboTextureId, 0);

// 3. 检查 FBO 完整性
if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOGCATE("FBO is not complete!");
}

// 4. 渲染到 FBO
glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);
// ... 执行渲染命令

// 5. 渲染到屏幕 (使用 FBO 纹理)
glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
glBindTexture(GL_TEXTURE_2D, m_FboTextureId);
// ... 渲染全屏四边形
```

**实践任务**:
- [ ] 实现灰度滤镜 (在片段着色器中处理)
- [ ] 实现高斯模糊效果 (两遍渲染)
- [ ] 实现边缘检测滤镜 (Sobel 算子)

**关键知识点**:
1. FBO、RBO 和纹理附件的区别
2. 为什么需要离屏渲染？
3. 如何实现多遍渲染？

**博客文章**: [OpenGL ES 3.0 开发（05）：FBO 离屏渲染](https://blog.csdn.net/Kennethdroid/article/details/98883854)

---

#### 示例 06: EGL 后台渲染 (2 天)
**文件**: [EGLRender.cpp](../app/src/main/cpp/egl/EGLRender.cpp)

**学习目标**:
- 理解 EGL 的作用
- 掌握 EGL 上下文创建
- 学会在后台线程进行渲染

**核心代码**:
```cpp
// 1. 获取 Display
m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
eglInitialize(m_eglDisplay, nullptr, nullptr);

// 2. 选择配置
EGLint attribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_NONE
};
eglChooseConfig(m_eglDisplay, attribs, &m_eglConfig, 1, &numConfigs);

// 3. 创建 PBuffer Surface
EGLint pbufferAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
m_eglSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttribs);

// 4. 创建 Context
EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                EGL_NO_CONTEXT, contextAttribs);

// 5. 激活上下文
eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
```

**实践任务**:
- [ ] 在后台线程中渲染图片
- [ ] 使用 PBO 异步读取渲染结果
- [ ] 实现视频帧的离线滤镜处理

**博客文章**: [OpenGL ES 3.0 开发（06）：EGL](https://blog.csdn.net/Kennethdroid/article/details/99655635)

---

#### 周总结与项目 1
**综合项目**: 实现一个简单的图片滤镜 APP

**功能要求**:
1. 加载本地图片
2. 实现至少 5 种滤镜:
   - 灰度
   - 反色
   - 高斯模糊
   - 边缘检测
   - 素描效果
3. 实时预览和保存处理后的图片

**技术要点**:
- 使用 FBO 进行离屏渲染
- 实现多种片段着色器滤镜
- 使用 PBO 异步读取像素数据

---

## 🌟 第三阶段: 进阶渲染 (第 5-8 周)

### Week 5: 光照与材质

#### 示例 09: 基础光照 (3 天)
**文件**: [BasicLightingSample.cpp](../app/src/main/cpp/sample/BasicLightingSample.cpp)

**学习目标**:
- 理解冯氏光照模型 (Phong Lighting Model)
- 掌握法线的计算和使用
- 学会实现环境光、漫反射和镜面反射

**核心代码**:
```glsl
// 片段着色器中的光照计算
vec3 norm = normalize(v_normal);
vec3 lightDir = normalize(u_lightPos - v_fragPos);

// 1. 环境光 (Ambient)
float ambientStrength = 0.1;
vec3 ambient = ambientStrength * u_lightColor;

// 2. 漫反射 (Diffuse)
float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * u_lightColor;

// 3. 镜面反射 (Specular)
float specularStrength = 0.5;
vec3 viewDir = normalize(u_viewPos - v_fragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
vec3 specular = specularStrength * spec * u_lightColor;

// 最终颜色
vec3 result = (ambient + diffuse + specular) * objectColor;
fragColor = vec4(result, 1.0);
```

**实践任务**:
- [ ] 绘制一个带光照的立方体
- [ ] 调整光源位置，观察光照变化
- [ ] 修改材质参数 (环境光、漫反射、镜面反射强度)
- [ ] 实现 Gouraud 着色 (顶点着色器中计算光照)

**关键知识点**:
1. 法线为什么需要归一化？
2. Phong 着色 vs Gouraud 着色的区别
3. 镜面反射中的 "32" 是什么？(镜面高光指数)
4. 如何计算模型的法线？

**博客文章**: [OpenGL ES 3.0 开发（09）：光照基础](https://blog.csdn.net/Kennethdroid/article/details/101220947)

---

#### 多光源与复杂光照 (2 天)
**文件**: [MultiLightsSample.cpp](../app/src/main/cpp/sample/MultiLightsSample.cpp)

**学习目标**:
- 实现多个光源的叠加
- 掌握定向光、点光源、聚光灯的实现
- 理解光照衰减

**核心代码**:
```glsl
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// 定向光计算
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

// 点光源计算 (带衰减)
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    // ... 基础光照计算

    // 衰减计算
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                               light.quadratic * (distance * distance));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}
```

**实践任务**:
- [ ] 场景中添加 3 个不同颜色的点光源
- [ ] 实现一个手电筒效果 (聚光灯)
- [ ] 调整光照衰减参数

---

### Week 6: 混合与透明

#### 示例 12: 混合 (Blending) (2 天)
**文件**: [BlendingSample.cpp](../app/src/main/cpp/sample/BlendingSample.cpp)

**学习目标**:
- 掌握 Alpha 混合的使用
- 理解混合方程和混合因子
- 学会实现半透明效果

**核心代码**:
```cpp
// 启用混合
glEnable(GL_BLEND);

// 设置混合函数
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// 或使用更复杂的混合方程
glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                    GL_ONE, GL_ZERO);

// 设置混合方程
glBlendEquation(GL_FUNC_ADD);
```

**常见混合模式**:
```cpp
// 正常混合 (Normal)
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// 相加混合 (Additive) - 发光效果
glBlendFunc(GL_SRC_ALPHA, GL_ONE);

// 相乘混合 (Multiply) - 变暗效果
glBlendFunc(GL_DST_COLOR, GL_ZERO);

// 滤色混合 (Screen) - 变亮效果
glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
```

**实践任务**:
- [ ] 绘制半透明的玻璃效果
- [ ] 实现粒子发光效果 (Additive Blending)
- [ ] 注意透明物体的绘制顺序问题

**关键知识点**:
1. 为什么透明物体要从后往前绘制？
2. `GL_SRC_ALPHA` 和 `GL_ONE_MINUS_SRC_ALPHA` 的含义
3. 如何实现预乘 Alpha？

**博客文章**: [OpenGL ES 3.0 开发（12）：混合](https://blog.csdn.net/Kennethdroid/article/details/102630858)

---

#### 示例 15: 天空盒 (SkyBox) (3 天)
**文件**: [SkyBoxSample.cpp](../app/src/main/cpp/sample/SkyBoxSample.cpp)

**学习目标**:
- 掌握立方体贴图 (Cube Map) 的使用
- 实现天空盒效果
- 学会环境映射和反射

**核心代码**:
```cpp
// 1. 创建立方体纹理
glGenTextures(1, &m_TextureId);
glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureId);

// 2. 加载 6 个面的纹理
const char* faces[6] = {"right", "left", "top", "bottom", "front", "back"};
for (int i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                 width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data[i]);
}

// 3. 设置纹理参数
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
```

**片段着色器**:
```glsl
#version 300 es
precision mediump float;

in vec3 v_texCoord;
out vec4 fragColor;

uniform samplerCube s_SkyBox;

void main() {
    fragColor = texture(s_SkyBox, v_texCoord);
}
```

**实践任务**:
- [ ] 创建一个天空盒
- [ ] 实现环境反射 (镜面球体)
- [ ] 实现环境折射 (玻璃球体)

**博客文章**: [OpenGL ES 3.0 开发（15）：立方体贴图（天空盒）](https://blog.csdn.net/Kennethdroid/article/details/102991524)

---

### Week 7: 高级特性

#### 示例 13: 实例化渲染 (Instancing) (2 天)
**文件**: [InstancingSample.cpp](../app/src/main/cpp/sample/InstancingSample.cpp)

**学习目标**:
- 掌握实例化渲染技术
- 理解如何高效渲染大量相同物体
- 学会使用实例化数组

**核心代码**:
```cpp
// 1. 准备实例化数据 (100 个物体的位置)
glm::vec2 translations[100];
int index = 0;
for (int y = -10; y < 10; y += 2) {
    for (int x = -10; x < 10; x += 2) {
        translations[index++] = glm::vec2(x + 0.5f, y + 0.5f);
    }
}

// 2. 创建实例化 VBO
glGenBuffers(1, &instanceVBO);
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100,
             &translations[0], GL_STATIC_DRAW);

// 3. 配置实例化属性
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
glVertexAttribDivisor(2, 1);  // 每个实例更新一次

// 4. 实例化绘制
glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);
```

**顶点着色器**:
```glsl
#version 300 es
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;  // 实例化属性

out vec3 fColor;

void main() {
    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
```

**实践任务**:
- [ ] 使用实例化渲染 10000 个立方体
- [ ] 对比实例化渲染和普通渲染的性能差异
- [ ] 实现小行星带效果

**博客文章**: [OpenGL ES 3.0 开发（13）：实例化（Instancing）](https://blog.csdn.net/Kennethdroid/article/details/102770813)

---

#### 示例 14: 粒子系统 (3 天)
**文件**: [ParticlesSample.cpp](../app/src/main/cpp/sample/ParticlesSample.cpp)

**学习目标**:
- 掌握粒子系统的实现
- 学会使用纹理图集 (Texture Atlas)
- 实现粒子的生命周期管理

**核心概念**:
```cpp
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float life;
    float cameraDist;  // 用于排序
};

// 粒子更新
for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle& p = particles[i];

    if (p.life > 0.0f) {
        // 更新生命值
        p.life -= deltaTime;

        if (p.life > 0.0f) {
            // 更新位置和速度
            p.velocity += glm::vec3(0.0f, -9.81f, 0.0f) * deltaTime * 0.5f;
            p.position += p.velocity * deltaTime;

            // 计算相机距离 (用于排序)
            p.cameraDist = glm::length(p.position - cameraPos);
        }
    }
}

// 从后往前排序 (透明物体)
std::sort(particles.begin(), particles.end());
```

**实践任务**:
- [ ] 实现喷泉粒子效果
- [ ] 实现烟雾效果
- [ ] 实现火焰粒子效果

**博客文章**: [OpenGL ES 3.0 开发（14）：粒子（Particles）](https://blog.csdn.net/Kennethdroid/article/details/102881654)

---

### Week 8: 3D 模型加载

#### 示例 20/21: 3D 模型加载与渲染 (4 天)
**文件**: [Model3DSample.cpp](../app/src/main/cpp/sample/Model3DSample.cpp)

**学习目标**:
- 掌握 Assimp 库的使用
- 学会加载常见 3D 模型格式 (OBJ, FBX, GLTF)
- 理解网格 (Mesh) 和材质 (Material) 的概念

**核心代码**:
```cpp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// 1. 加载模型
Assimp::Importer importer;
const aiScene* scene = importer.ReadFile(path,
    aiProcess_Triangulate |           // 转换为三角形
    aiProcess_FlipUVs |               // 翻转纹理坐标
    aiProcess_GenNormals |            // 生成法线
    aiProcess_CalcTangentSpace);      // 计算切线空间

// 2. 递归处理节点
void ProcessNode(aiNode* node, const aiScene* scene) {
    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene));
    }

    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

// 3. 处理网格数据
Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // 提取顶点数据
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x,
                                   mesh->mVertices[i].y,
                                   mesh->mVertices[i].z);
        vertex.Normal = glm::vec3(mesh->mNormals[i].x,
                                 mesh->mNormals[i].y,
                                 mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                        mesh->mTextureCoords[0][i].y);
        }
        vertices.push_back(vertex);
    }

    // 提取索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 加载材质
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // 加载漫反射贴图
        std::vector<Texture> diffuseMaps = LoadMaterialTextures(material,
            aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    return Mesh(vertices, indices, textures);
}
```

**实践任务**:
- [ ] 加载一个 OBJ 模型并渲染
- [ ] 为模型添加光照效果
- [ ] 实现模型的旋转和缩放控制
- [ ] 加载带纹理和材质的复杂模型

**推荐模型资源**:
- [Poly Haven](https://polyhaven.com/) - 免费 3D 资源
- [Sketchfab](https://sketchfab.com/) - 免费和付费模型
- [Free3D](https://free3d.com/) - 免费模型库

**博客文章**:
- [OpenGL ES 3.0 开发（20）：3D 模型](https://blog.csdn.net/Kennethdroid/article/details/103771970)
- [OpenGL ES 3.0 开发（21）：3D 模型加载和渲染](https://blog.csdn.net/Kennethdroid/article/details/103825593)

---

#### 周总结与项目 2
**综合项目**: 3D 模型查看器

**功能要求**:
1. 支持加载 OBJ/FBX 格式的 3D 模型
2. 实现以下功能:
   - 手势旋转模型 (双指旋转)
   - 缩放模型 (双指捏合)
   - 切换光照模式 (环境光、漫反射、镜面反射)
   - 切换渲染模式 (线框、实体、纹理)
3. 性能优化:
   - 使用 VAO 缓存顶点数据
   - 实现视锥体剔除 (Frustum Culling)

---

## 🔥 第四阶段: 高级技术 (第 9-12 周)

### Week 9: YUV 渲染与格式转换

#### 示例 03: YUV 渲染 (2 天)
**文件**: [NV21TextureMapSample.cpp](../app/src/main/cpp/sample/NV21TextureMapSample.cpp)

**学习目标**:
- 理解 YUV 色彩空间
- 掌握 NV21、I420、YV12 等格式的渲染
- 学会 YUV 到 RGB 的转换

**YUV 格式说明**:
- **NV21**: YYYYYYYY VU VU (Android Camera 默认格式)
- **NV12**: YYYYYYYY UV UV (iOS Camera 默认格式)
- **I420**: YYYYYYYY UU VV (YUV420P)
- **YV12**: YYYYYYYY VV UU (YUV420P)

**核心代码 (片段着色器)**:
```glsl
#version 300 es
precision mediump float;

in vec2 v_texCoord;
out vec4 outColor;

uniform sampler2D y_texture;   // Y 平面
uniform sampler2D uv_texture;  // UV 平面 (NV21)

void main() {
    // 采样 YUV
    vec3 yuv;
    yuv.x = texture(y_texture, v_texCoord).r;         // Y
    yuv.y = texture(uv_texture, v_texCoord).a - 0.5;  // U (V 在 alpha 通道)
    yuv.z = texture(uv_texture, v_texCoord).r - 0.5;  // V (在 red 通道)

    // YUV to RGB 转换矩阵 (BT.601)
    mat3 yuv2rgb = mat3(
        1.0,       1.0,      1.0,
        0.0,      -0.344,    1.770,
        1.403,    -0.714,    0.0
    );

    vec3 rgb = yuv2rgb * yuv;
    outColor = vec4(rgb, 1.0);
}
```

**实践任务**:
- [ ] 渲染 NV21 格式的相机数据
- [ ] 实现 I420、YV12 格式的渲染
- [ ] 对比不同 YUV 格式的性能

**博客文章**:
- [OpenGL ES 3.0 开发（03）：YUV 渲染](https://blog.csdn.net/Kennethdroid/article/details/97153407)
- [OpenGL ES 渲染 NV21、NV12、I420、YV12、YUYV、UYVY、I444](https://blog.csdn.net/Kennethdroid/article/details/136213988)

---

#### RGB 到 YUV 格式转换 (3 天)
**文件**:
- [RGB2NV21Sample.cpp](../app/src/main/cpp/sample/RGB2NV21Sample.cpp)
- [RGB2I420Sample.cpp](../app/src/main/cpp/sample/RGB2I420Sample.cpp)

**学习目标**:
- 在 GPU 上实现 RGB 到 YUV 的转换
- 理解色彩空间转换矩阵
- 掌握多重渲染目标 (MRT) 的使用

**核心代码 (片段着色器)**:
```glsl
#version 300 es
precision mediump float;

in vec2 v_texCoord;

layout(location = 0) out vec4 outColor_Y;   // Y 平面输出
layout(location = 1) out vec4 outColor_UV;  // UV 平面输出

uniform sampler2D s_TextureMap;

void main() {
    vec4 rgba = texture(s_TextureMap, v_texCoord);

    // RGB to YUV 转换矩阵 (BT.601)
    mat3 rgb2yuv = mat3(
        0.299,     0.587,     0.114,
       -0.169,    -0.331,     0.5,
        0.5,      -0.419,    -0.081
    );

    vec3 yuv = rgb2yuv * rgba.rgb;

    // 输出到不同的颜色附件
    outColor_Y = vec4(yuv.x, yuv.x, yuv.x, 1.0);
    outColor_UV = vec4(yuv.z + 0.5, 0.0, 0.0, yuv.y + 0.5);  // NV21 格式
}
```

**实践任务**:
- [ ] 实现 RGBA → NV21 转换
- [ ] 实现 RGBA → I420 转换
- [ ] 性能测试: GPU 转换 vs CPU 转换

**博客文章**:
- [使用 OpenGL 实现 RGB 到 YUV 的图像格式转换](https://blog.csdn.net/Kennethdroid/article/details/117675581)
- [OpenGL ES 利用 Shader 实现 RGBA 到 NV21 图像格式转换](https://blog.csdn.net/Kennethdroid/article/details/132055489)

---

### Week 10: 高级渲染技术

#### 示例 22: PBO 异步读取 (2 天)
**文件**: [PBOSample.cpp](../app/src/main/cpp/sample/PBOSample.cpp)

**学习目标**:
- 掌握 PBO (Pixel Buffer Object) 的使用
- 理解异步像素传输
- 学会使用双缓冲技术提高性能

**核心代码**:
```cpp
// 创建 PBO
GLuint pboIds[2];
glGenBuffers(2, pboIds);
glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[0]);
glBufferData(GL_PIXEL_PACK_BUFFER, dataSize, 0, GL_STREAM_READ);
glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[1]);
glBufferData(GL_PIXEL_PACK_BUFFER, dataSize, 0, GL_STREAM_READ);
glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

// 乒乓缓冲读取
int index = frameIndex % 2;
int nextIndex = (frameIndex + 1) % 2;

// 从 PBO[nextIndex] 读取上一帧数据
glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[nextIndex]);
GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, dataSize,
                                          GL_MAP_READ_BIT);
if (ptr) {
    // 处理像素数据 (不会阻塞)
    memcpy(outputBuffer, ptr, dataSize);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
}

// 启动异步读取到 PBO[index]
glBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[index]);
glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
```

**实践任务**:
- [ ] 对比 PBO 和直接 glReadPixels 的性能
- [ ] 实现视频帧的高效录制

**博客文章**:
- [OpenGL ES 3.0 开发（22）：PBO](https://blog.csdn.net/Kennethdroid/article/details/103931627)
- [OpenGL ES 渲染图像读取哪家强](https://blog.csdn.net/Kennethdroid/article/details/109339906)

---

#### 示例 23: 多重渲染目标 (MRT) (2 天)
**文件**: [MRTSample.cpp](../app/src/main/cpp/sample/MRTSample.cpp)

**学习目标**:
- 掌握 MRT (Multiple Render Targets) 的使用
- 学会在一次渲染中输出到多个纹理
- 理解延迟渲染 (Deferred Rendering) 的基础

**核心代码**:
```cpp
// 1. 创建 FBO 和多个颜色附件
glGenFramebuffers(1, &m_FboId);
glBindFramebuffer(GL_FRAMEBUFFER, m_FboId);

// 附加多个纹理
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       GL_TEXTURE_2D, textures[0], 0);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                       GL_TEXTURE_2D, textures[1], 0);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
                       GL_TEXTURE_2D, textures[2], 0);

// 2. 指定绘制缓冲区
GLuint attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                        GL_COLOR_ATTACHMENT2 };
glDrawBuffers(3, attachments);
```

**片段着色器**:
```glsl
#version 300 es
precision mediump float;

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_fragPos;

layout(location = 0) out vec4 fragColor;     // 颜色
layout(location = 1) out vec4 fragNormal;    // 法线
layout(location = 2) out vec4 fragPosition;  // 位置

void main() {
    fragColor = texture(s_Texture, v_texCoord);
    fragNormal = vec4(normalize(v_normal), 1.0);
    fragPosition = vec4(v_fragPos, 1.0);
}
```

**实践任务**:
- [ ] 实现 G-Buffer (几何缓冲)
- [ ] 使用 MRT 实现简单的延迟渲染

**博客文章**: [OpenGL ES 3.0 开发（23）：多重渲染目标（MRT）](https://blog.csdn.net/Kennethdroid/article/details/108873665)

---

#### Transform Feedback (1 天)
**文件**: [TransformFeedbackSample.cpp](../app/src/main/cpp/sample/TransformFeedbackSample.cpp)

**学习目标**:
- 理解 Transform Feedback 的作用
- 学会在 GPU 上进行粒子模拟
- 掌握 GPU 计算技术

**实践任务**:
- [ ] 使用 Transform Feedback 实现 GPU 粒子系统
- [ ] 实现布料模拟

**博客文章**: [OpenGL ES 3.0 开发（07）：Transform Feedback](https://blog.csdn.net/Kennethdroid/article/details/100083599)

---

#### OpenGL ES 3.1/3.2 特性 (2 天)

**示例 25**: Compute Shader
**文件**: [ComputeShaderSample.cpp](../app/src/main/cpp/sample/ComputeShaderSample.cpp)

**学习目标**:
- 掌握计算着色器的使用
- 理解工作组 (Work Group) 的概念
- 学会进行通用 GPU 计算

**核心代码**:
```glsl
#version 310 es
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0, rgba8) uniform readonly image2D inputImage;
layout (binding = 1, rgba8) uniform writeonly image2D outputImage;

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = imageLoad(inputImage, coords);

    // 处理像素 (例如: 灰度化)
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));

    imageStore(outputImage, coords, vec4(gray, gray, gray, color.a));
}
```

**实践任务**:
- [ ] 使用 Compute Shader 实现图像模糊
- [ ] 实现并行前缀和 (Prefix Sum)

---

### Week 11: 相机与实时特效

#### 相机预览与滤镜 (3 天)
**示例 16-19**:
- [相机预览](https://blog.csdn.net/Kennethdroid/article/details/103189489)
- [基础滤镜](https://blog.csdn.net/Kennethdroid/article/details/103335598)
- [LUT 滤镜](https://blog.csdn.net/Kennethdroid/article/details/103355129)
- [抖音滤镜](https://blog.csdn.net/Kennethdroid/article/details/103449935)

**学习目标**:
- 掌握 OES 纹理的使用 (相机数据)
- 实现实时滤镜处理
- 学会使用 LUT (Look-Up Table) 滤镜

**核心代码**:
```glsl
// OES 纹理扩展
#extension GL_OES_EGL_image_external_essl3 : require

uniform samplerExternalOES s_TextureMap;

// LUT 滤镜
uniform sampler2D s_LutTexture;

void main() {
    vec4 color = texture(s_TextureMap, v_texCoord);

    // 应用 LUT
    float blueColor = color.b * 63.0;
    vec2 quad1;
    quad1.y = floor(floor(blueColor) / 8.0);
    quad1.x = floor(blueColor) - (quad1.y * 8.0);

    vec2 quad2;
    quad2.y = floor(ceil(blueColor) / 8.0);
    quad2.x = ceil(blueColor) - (quad2.y * 8.0);

    vec2 texPos1 = (quad1 * 0.125) + 0.5/512.0 +
                   ((0.125 - 1.0/512.0) * color.rg);
    vec2 texPos2 = (quad2 * 0.125) + 0.5/512.0 +
                   ((0.125 - 1.0/512.0) * color.rg);

    vec4 newColor1 = texture(s_LutTexture, texPos1);
    vec4 newColor2 = texture(s_LutTexture, texPos2);

    vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
    outColor = vec4(newColor.rgb, color.a);
}
```

**实践任务**:
- [ ] 接入相机预览
- [ ] 实现 10 种实时滤镜 (黑白、怀旧、冷色调、暖色调等)
- [ ] 使用 LUT 实现电影级调色

---

#### 美颜特效 (4 天)
**示例**:
- [大眼瘦脸](https://blog.csdn.net/Kennethdroid/article/details/104907763) - BigEyesSample, FaceSlenderSample
- [瘦身大长腿](https://blog.csdn.net/Kennethdroid/article/details/104546234) - FBOLegLengthenSample
- [大头效果](https://blog.csdn.net/Kennethdroid/article/details/105208054) - BigHeadSample

**学习目标**:
- 掌握网格变形 (Mesh Warp) 技术
- 学会使用人脸关键点进行定位
- 实现实时美颜效果

**核心算法 (大眼效果)**:
```cpp
// 局部凸变形算法
float CalcWarpRadius(float x, float y, float centerX, float centerY, float radius) {
    float dx = x - centerX;
    float dy = y - centerY;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance < radius) {
        float intensity = 0.5;  // 强度
        float ratio = distance / radius;
        float alpha = 1.0 - ratio * intensity;

        // 新位置
        float newX = centerX + dx * alpha;
        float newY = centerY + dy * alpha;

        return glm::vec2(newX, newY);
    }

    return glm::vec2(x, y);
}
```

**实践任务**:
- [ ] 集成人脸检测库 (OpenCV/MediaPipe)
- [ ] 实现大眼效果
- [ ] 实现瘦脸效果
- [ ] 实现美白磨皮

---

### Week 12: 特效与优化

#### 心跳、云层、时光隧道等特效 (3 天)
**示例**:
- [心动特效](https://blog.csdn.net/Kennethdroid/article/details/104536532) - BeatingHeartSample
- CloudSample
- TimeTunnelSample
- ShockWaveSample (冲击波)

**学习目标**:
- 掌握高级 Shader 编程技巧
- 学会使用数学函数创造视觉特效
- 理解程序化生成 (Procedural Generation)

**实践任务**:
- [ ] 分析心跳特效的 Shader 代码
- [ ] 实现水波纹扩散效果
- [ ] 创造自己的 Shader 特效

**博客文章**:
- [OpenGL ES 实现心动特效](https://blog.csdn.net/Kennethdroid/article/details/104536532)
- [OpenGL ES 实现动态（水波纹）涟漪效果](https://blog.csdn.net/Kennethdroid/article/details/106556584)

---

#### 图像转场效果 (2 天)
**文件**: [GLTransitionExample](https://blog.csdn.net/Kennethdroid/article/details/132656888)

**学习目标**:
- 掌握图像过渡动画
- 学会使用 GLTransitions 库
- 实现各种转场效果

**实践任务**:
- [ ] 实现 10 种转场效果 (淡入淡出、滑动、缩放、旋转等)
- [ ] 制作图片幻灯片应用

---

#### 性能优化与最佳实践 (2 天)

**优化清单**:
- [ ] **渲染优化**
  - 使用 VAO 减少状态切换
  - 批量绘制相同状态的物体
  - 使用实例化渲染大量相同物体
  - 实现视锥体剔除 (Frustum Culling)
  - 使用遮挡剔除 (Occlusion Culling)

- [ ] **内存优化**
  - 使用纹理压缩 (ETC2, ASTC)
  - 生成 Mipmap
  - 及时释放不用的资源
  - 使用对象池避免频繁创建销毁

- [ ] **Shader 优化**
  - 合理使用精度限定符 (lowp, mediump, highp)
  - 避免在片段着色器中使用分支
  - 将计算移到顶点着色器或 CPU
  - 使用预计算和查找表

- [ ] **工具使用**
  - Android Studio Profiler
  - RenderDoc 帧分析
  - ARM Mobile Studio / Snapdragon Profiler

**参考文章**:
- [面试中经常被问到的 OpenGL ES 对象](https://blog.csdn.net/Kennethdroid/article/details/112379836)
- [OpenGL ES 调试工具](https://blog.csdn.net/Kennethdroid/article/details/106695602)

---

## 🎓 第五阶段: 综合项目 (第 13-16 周)

### 项目 3: 短视频特效相机 (4 周)

**项目需求**:
开发一个类似抖音的短视频拍摄应用

**核心功能**:
1. **相机模块**
   - 实时预览
   - 前后摄像头切换
   - 手动对焦
   - 曝光控制

2. **实时滤镜** (至少 10 种)
   - 基础滤镜: 黑白、怀旧、冷色、暖色
   - LUT 滤镜: 日系、胶片、VSCO 风格
   - 特效滤镜: 故障、像素化、抖动

3. **美颜功能**
   - 磨皮美白
   - 大眼瘦脸
   - 瘦身长腿

4. **动态贴纸**
   - 人脸关键点检测
   - 2D/3D 贴纸渲染
   - 动态表情跟随

5. **特效功能**
   - 时光倒流
   - 灵魂出窍
   - 分屏效果
   - 传送带特效

6. **视频录制**
   - 分段录制
   - 回删功能
   - 配乐和原声混合
   - H.264 编码输出

**技术架构**:
```
┌─────────────────────────────────────────┐
│          UI 层 (Activity/Fragment)       │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│         相机管理层 (Camera2 API)         │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│      OpenGL 渲染层 (GLSurfaceView)      │
│  - 滤镜管理器                            │
│  - 美颜处理器                            │
│  - 贴纸渲染器                            │
│  - 特效处理器                            │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│          编码层 (MediaCodec)            │
│  - 视频编码器                            │
│  - 音频编码器                            │
│  - MP4 封装器                            │
└─────────────────────────────────────────┘
```

**开发计划**:

**Week 13**: 基础框架搭建
- [ ] 相机预览接入
- [ ] OpenGL 渲染管线搭建
- [ ] 基础滤镜实现 (5 种)

**Week 14**: 美颜与特效
- [ ] 人脸检测集成 (OpenCV/MediaPipe)
- [ ] 大眼瘦脸功能
- [ ] 磨皮美白算法
- [ ] 动态贴纸渲染

**Week 15**: 高级特效
- [ ] 时光倒流效果
- [ ] 灵魂出窍效果
- [ ] 分屏效果
- [ ] 传送带特效

**Week 16**: 视频编码与优化
- [ ] MediaCodec 视频编码
- [ ] 音视频同步
- [ ] 性能优化 (保持 30fps+)
- [ ] UI 完善和测试

**评估标准**:
- ✅ 实时渲染性能 ≥ 30 FPS
- ✅ 延迟 < 100ms
- ✅ 支持 1080p 录制
- ✅ 代码结构清晰，可维护性高

---

## 📚 学习资源推荐

### 书籍
1. **《OpenGL ES 3.0 编程指南》** (OpenGL ES 3.0 Programming Guide)
   - 作者: Dan Ginsburg, Budirijanto Purnomo
   - 难度: ⭐⭐⭐
   - 推荐理由: OpenGL ES 官方指南，系统全面

2. **《OpenGL 超级宝典》** (OpenGL SuperBible)
   - 作者: Graham Sellers
   - 难度: ⭐⭐⭐⭐
   - 推荐理由: 深入讲解 OpenGL 原理

3. **《Real-Time Rendering》**
   - 作者: Tomas Akenine-Möller
   - 难度: ⭐⭐⭐⭐⭐
   - 推荐理由: 实时渲染圣经

### 在线资源
1. **LearnOpenGL CN** (https://learnopengl-cn.github.io/)
   - 中文 OpenGL 教程，讲解详细
   - 涵盖基础到高级内容

2. **Khronos OpenGL ES Registry** (https://www.khronos.org/opengles/)
   - 官方 API 文档和规范

3. **Shadertoy** (https://www.shadertoy.com/)
   - Shader 特效展示平台
   - 学习高级 Shader 编程

4. **The Book of Shaders** (https://thebookofshaders.com/)
   - Shader 编程入门教程

### 视频课程
1. **公众号"字节流动"视频教程**
   - 本项目配套视频
   - 扫码添加微信: Byte-Flow 获取

2. **YouTube - The Cherno OpenGL 系列**
   - 英文教程，讲解生动

### GitHub 项目
1. **本项目**: https://github.com/githubhaohao/NDK_OpenGLES_3_0
2. **OpenGLCamera2**: https://github.com/githubhaohao/OpenGLCamera2
3. **GPUImage**: https://github.com/BradLarson/GPUImage (iOS 滤镜库)
4. **Android-GPUImage**: https://github.com/cats-oss/android-gpuimage

---

## ✅ 学习检查点

### 基础阶段检查 (Week 1-4)
- [ ] 能独立绘制基本图形 (三角形、矩形、圆形)
- [ ] 理解顶点着色器和片段着色器的作用
- [ ] 掌握纹理映射和纹理坐标
- [ ] 能使用 GLM 进行 MVP 矩阵变换
- [ ] 理解 FBO 和离屏渲染的原理
- [ ] 能实现至少 3 种图片滤镜

### 进阶阶段检查 (Week 5-8)
- [ ] 能实现冯氏光照模型
- [ ] 掌握多光源渲染
- [ ] 理解 Alpha 混合和透明渲染
- [ ] 能使用 Assimp 加载 3D 模型
- [ ] 能实现粒子系统
- [ ] 掌握实例化渲染技术

### 高级阶段检查 (Week 9-12)
- [ ] 理解 YUV 色彩空间和格式转换
- [ ] 掌握 PBO 异步读取
- [ ] 能使用 MRT 进行延迟渲染
- [ ] 能编写复杂的 Shader 特效
- [ ] 掌握相机实时滤镜处理
- [ ] 能实现美颜功能

### 综合能力检查 (Week 13-16)
- [ ] 能独立设计渲染架构
- [ ] 具备性能分析和优化能力
- [ ] 能解决实际项目中的渲染问题
- [ ] 代码质量达到生产级别
- [ ] 具备跨平台移植能力

---

## 🎯 学习建议

### 1. 循序渐进
不要跳过基础示例，每个示例都有其独特的知识点。建议按照本计划的顺序学习。

### 2. 动手实践
**看 10 遍不如写 1 遍**。每个示例都要:
1. 先运行理解效果
2. 阅读源码分析原理
3. 修改代码验证理解
4. 尝试独立实现

### 3. 记录笔记
建议创建自己的学习笔记:
- 核心 API 的使用方法
- 常见问题和解决方案
- Shader 代码片段库
- 性能优化技巧

### 4. 调试技巧
- 使用 `GLUtils::CheckGLError` 检查每个 OpenGL 调用
- 使用 RenderDoc 进行帧分析
- 学会查看 Logcat 日志
- 使用条件编译开关调试代码

### 5. 持续学习
- 关注 Khronos 官方博客
- 阅读图形学论文
- 参与开源项目
- 加入技术社区交流

### 6. 性能意识
从一开始就培养性能优化意识:
- 减少状态切换
- 合理使用缓存
- 避免不必要的计算
- 使用性能分析工具

---

## 🤝 学习社区

### 加入学习群
扫码添加作者微信: **Byte-Flow**
- 领取视频教程
- 加入技术交流群
- 获取学习资料
- 项目答疑

### 知识星球
付费社群，提供:
- 大厂内推
- 面试指导
- 代码 Review
- 学习路线规划

---

## 📝 总结

这份学习计划涵盖了 OpenGL ES 3.0 从入门到精通的完整路径。通过 12-16 周的系统学习:

1. **前 4 周**: 打好基础，理解渲染管线和核心概念
2. **中 4 周**: 掌握进阶技术，学会光照、模型加载、粒子系统
3. **后 4 周**: 学习高级特性，YUV、PBO、MRT、美颜特效
4. **最后 4 周**: 综合项目实战，开发完整的短视频应用

**学习心态**:
- 遇到困难是正常的，图形学本身就很难
- 不要追求速度,理解比完成更重要
- 多思考为什么,而不是只知道怎么做
- 坚持每天学习,积少成多

**最终目标**:
- 掌握 OpenGL ES 3.0 核心技术
- 具备独立开发图形应用的能力
- 为进一步学习 Vulkan/Metal 打下基础
- 在图形渲染领域建立自己的技术竞争力

---

**祝你学习顺利! 加油! 💪**

*如有问题,欢迎在 GitHub 提 Issue 或联系作者交流*

---

**文档版本**: v1.0
**创建时间**: 2026-01-07
**更新时间**: 2026-01-07
**作者**: [你的名字]
