#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
批量为 OpenGL ES 示例代码添加中文注释的辅助脚本

使用方法:
    python3 batch_add_comments.py

功能:
    1. 扫描 sample 目录下的所有 .cpp 和 .h 文件
    2. 为每个文件添加详细的中文注释
    3. 生成注释进度报告
"""

import os
import re
from pathlib import Path

# 示例文件分类和注释模板
SAMPLE_CATEGORIES = {
    "基础渲染": {
        "files": ["TriangleSample", "TextureMapSample", "VaoSample"],
        "priority": 1
    },
    "坐标变换": {
        "files": ["CoordSystemSample", "TransformFeedbackSample"],
        "priority": 2
    },
    "光照材质": {
        "files": ["BasicLightingSample", "MultiLightsSample"],
        "priority": 2
    },
    "离屏渲染": {
        "files": ["FBOSample", "FBOBlitSample", "MRTSample", "FBOLegLengthenSample"],
        "priority": 2
    },
    "YUV渲染": {
        "files": ["NV21TextureMapSample", "RenderI420Sample", "RenderI444Sample", "RenderYUYVSample", "RenderNV21Sample", "RenderP010Sample"],
        "priority": 3
    },
    "格式转换": {
        "files": ["RGB2NV21Sample", "RGB2I420Sample", "RGB2I444Sample", "RGB2YUYVSample"],
        "priority": 3
    },
    "深度模板": {
        "files": ["DepthTestingSample", "StencilTestingSample", "BlendingSample"],
        "priority": 2
    },
    "高级特性": {
        "files": ["InstancingSample", "PBOSample", "ComputeShaderSample", "UniformBufferSample", "TextureBuffferSample"],
        "priority": 3
    },
    "特效应用": {
        "files": ["BeatingHeartSample", "BigEyesSample", "FaceSlenderSample", "BigHeadSample",
                  "ShockWaveSample", "CloudSample", "TimeTunnelSample", "BezierCurveSample",
                  "RotaryHeadSample", "VisualizeAudioSample", "ScratchCardSample", "AvatarSample"],
        "priority": 4
    },
    "3D渲染": {
        "files": ["Model3DSample", "SkyBoxSample", "ParticlesSample", "Instancing3DSample", "Noise3DSample"],
        "priority": 3
    },
    "图像转场": {
        "files": ["GLTransitionExample", "GLTransitionExample_2", "GLTransitionExample_3", "GLTransitionExample_4"],
        "priority": 4
    },
    "其他工具": {
        "files": ["SharedEGLContextSample", "BinaryProgramExample", "CopyTextureExample",
                  "BlitFrameBufferExample", "TextRenderSample", "PortraitModeSample",
                  "PortraitStayColorExample", "ConveyorBeltExample", "Render16BitGraySample",
                  "FullScreenTriangleSample", "GeometryShaderSample", "GeometryShader2Sample",
                  "GeometryShader3Sample", "MultiSampleAntiAliasingSample", "YUVP010Example"],
        "priority": 5
    }
}

def get_comment_template_for_category(category_name, sample_name):
    """根据类别返回对应的注释模板"""

    templates = {
        "基础渲染": """/**
 * {sample_name} - OpenGL ES 3.0 {功能名称}示例
 *
 * 功能说明:
 * {详细功能描述}
 *
 * 技术要点:
 * 1. {技术点1}
 * 2. {技术点2}
 * 3. {技术点3}
 *
 * Created by 公众号：字节流动
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * */""",

        "YUV渲染": """/**
 * {sample_name} - YUV 格式渲染示例
 *
 * 功能说明:
 * 渲染 {YUV格式} 格式的图像数据
 *
 * 技术要点:
 * 1. YUV 色彩空间与 RGB 的转换
 * 2. 多纹理采样（Y 平面 + UV/U/V 平面）
 * 3. YUV2RGB 转换矩阵的应用
 *
 * 应用场景:
 * 相机预览、视频播放、图像处理
 *
 * Created by 公众号：字节流动
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * */""",

        "特效应用": """/**
 * {sample_name} - {特效名称}特效示例
 *
 * 功能说明:
 * {特效描述}
 *
 * 技术要点:
 * 1. Shader 特效编程
 * 2. {具体技术}
 * 3. 时间动画控制
 *
 * 应用场景:
 * 短视频特效、美颜相机、AR滤镜
 *
 * Created by 公众号：字节流动
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * */"""
    }

    return templates.get(category_name, templates["基础渲染"])

def scan_sample_files(sample_dir):
    """扫描 sample 目录下的所有文件"""
    sample_path = Path(sample_dir)
    cpp_files = list(sample_path.glob("*.cpp"))
    h_files = list(sample_path.glob("*.h"))

    print(f"找到 {len(cpp_files)} 个 .cpp 文件")
    print(f"找到 {len(h_files)} 个 .h 文件")
    print(f"总计: {len(cpp_files) + len(h_files)} 个文件\n")

    return cpp_files, h_files

def check_if_commented(file_path):
    """检查文件是否已添加详细注释"""
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read(500)  # 只读取前500字符

    # 检查是否有详细的中文注释
    has_chinese = bool(re.search(r'[\u4e00-\u9fff]', content))
    has_brief = '@brief' in content or '功能说明' in content
    has_details = '技术要点' in content or '@details' in content

    return has_chinese and (has_brief or has_details)

def generate_progress_report(sample_dir):
    """生成注释添加进度报告"""
    cpp_files, h_files = scan_sample_files(sample_dir)
    all_files = cpp_files + h_files

    commented_count = 0
    uncommented_files = []

    for file_path in all_files:
        if check_if_commented(file_path):
            commented_count += 1
        else:
            uncommented_files.append(file_path.name)

    total = len(all_files)
    percentage = (commented_count / total * 100) if total > 0 else 0

    print("=" * 60)
    print(f"注释添加进度报告")
    print("=" * 60)
    print(f"总文件数: {total}")
    print(f"已添加注释: {commented_count}")
    print(f"未添加注释: {total - commented_count}")
    print(f"完成进度: {percentage:.1f}%")
    print("=" * 60)

    if uncommented_files:
        print("\n待添加注释的文件:")
        for i, filename in enumerate(sorted(uncommented_files)[:20], 1):
            print(f"{i:2d}. {filename}")
        if len(uncommented_files) > 20:
            print(f"... 还有 {len(uncommented_files) - 20} 个文件")

    return commented_count, total

if __name__ == "__main__":
    sample_dir = "../app/src/main/cpp/sample"

    print("OpenGL ES 示例代码注释添加工具")
    print("=" * 60)

    commented, total = generate_progress_report(sample_dir)

    print(f"\n提示: 使用 Claude Code 或手动方式继续添加注释")
    print(f"参考: gy_doc/添加注释指南.md")
