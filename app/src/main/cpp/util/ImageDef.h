/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/NDK_OpenGLES_3_0
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef NDK_OPENGLES_3_0_IMAGEDEF_H
#define NDK_OPENGLES_3_0_IMAGEDEF_H

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "stdio.h"
#include "sys/stat.h"
#include "stdint.h"
#include "LogUtil.h"

#define IMAGE_FORMAT_RGBA           0x01
#define IMAGE_FORMAT_NV21           0x02
#define IMAGE_FORMAT_NV12           0x03
#define IMAGE_FORMAT_I420           0x04
#define IMAGE_FORMAT_YUYV           0x05
#define IMAGE_FORMAT_GRAY           0x06
#define IMAGE_FORMAT_I444           0x07
#define IMAGE_FORMAT_P010           0x08
#define IMAGE_FORMAT_GRAY10         0x09


#define IMAGE_FORMAT_RGBA_EXT       "RGB32"
#define IMAGE_FORMAT_NV21_EXT       "NV21"
#define IMAGE_FORMAT_NV12_EXT       "NV12"
#define IMAGE_FORMAT_I420_EXT       "I420"
#define IMAGE_FORMAT_YUYV_EXT       "YUYV"
#define IMAGE_FORMAT_GRAY_EXT       "GRAY"
#define IMAGE_FORMAT_I444_EXT       "I444"
#define IMAGE_FORMAT_P010_EXT       "P010"   //16bit NV21
#define IMAGE_FORMAT_GRAY10_EXT     "GRAY10" //16bit GRAY


typedef struct NativeRectF
{
	float left;
	float top;
	float right;
	float bottom;
	NativeRectF()
	{
		left = top = right = bottom = 0.0f;
	}
} RectF;

struct SizeF {
	float width;
	float height;
	SizeF() {
		width = height = 0;
	}
};

struct NativeImage
{
	int width;
	int height;
	int format;
	uint8_t *ppPlane[3];

	NativeImage()
	{
		width = 0;
		height = 0;
		format = 0;
		ppPlane[0] = nullptr;
		ppPlane[1] = nullptr;
		ppPlane[2] = nullptr;
	}
};

class NativeImageUtil
{
public:
	// 各格式的平面数
	static int GetPlaneCount(int format)
	{
		switch (format)
		{
			case IMAGE_FORMAT_NV12:
			case IMAGE_FORMAT_NV21:
			case IMAGE_FORMAT_P010:
				return 2;
			case IMAGE_FORMAT_I420:
			case IMAGE_FORMAT_I444:
				return 3;
			case IMAGE_FORMAT_RGBA:
			case IMAGE_FORMAT_YUYV:
			case IMAGE_FORMAT_GRAY:
			case IMAGE_FORMAT_GRAY10:
				return 1;
			default:
				return 0;
		}
	}

	// 第 plane 个平面的字节数（按连续存放、无行填充的布局计算）。不支持的格式/平面返回 0。
	static size_t GetPlaneSize(int format, int width, int height, int plane)
	{
		if (width <= 0 || height <= 0) return 0;
		size_t pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
		switch (format)
		{
			case IMAGE_FORMAT_RGBA:
				return plane == 0 ? pixels * 4 : 0;
			case IMAGE_FORMAT_YUYV:
			case IMAGE_FORMAT_GRAY10:
				return plane == 0 ? pixels * 2 : 0;
			case IMAGE_FORMAT_GRAY:
				return plane == 0 ? pixels : 0;
			case IMAGE_FORMAT_NV12:
			case IMAGE_FORMAT_NV21:
				return plane == 0 ? pixels : (plane == 1 ? pixels / 2 : 0);
			case IMAGE_FORMAT_I420:
				return plane == 0 ? pixels : ((plane == 1 || plane == 2) ? pixels / 4 : 0);
			case IMAGE_FORMAT_I444:
				return plane < 3 ? pixels : 0;
			case IMAGE_FORMAT_P010:
				// Y 平面 16bit；UV 交织、宽高各减半、每分量 16bit，合计 w*h 字节
				return plane == 0 ? pixels * 2 : (plane == 1 ? pixels : 0);
			default:
				return 0;
		}
	}

	// 整幅图像（所有平面连续存放）的字节数，不支持的格式返回 0
	static size_t GetImageSize(int format, int width, int height)
	{
		size_t total = 0;
		for (int i = 0; i < GetPlaneCount(format); ++i)
		{
			total += GetPlaneSize(format, width, height, i);
		}
		return total;
	}

	// 以 pBase 为起点按连续布局设置各平面指针。
	// JNI 传进来的 YUV 数据是一整块内存，各平面依次存放，这里统一计算偏移，避免各处手写。
	static void AttachContiguousPlanes(NativeImage *pImage, uint8_t *pBase)
	{
		if (pImage == nullptr) return;
		pImage->ppPlane[0] = pBase;
		pImage->ppPlane[1] = nullptr;
		pImage->ppPlane[2] = nullptr;
		if (pBase == nullptr) return;
		uint8_t *p = pBase;
		int planeCount = GetPlaneCount(pImage->format);
		for (int i = 0; i < planeCount; ++i)
		{
			pImage->ppPlane[i] = p;
			p += GetPlaneSize(pImage->format, pImage->width, pImage->height, i);
		}
	}

	static void AllocNativeImage(NativeImage *pImage)
	{
		if (pImage == nullptr || pImage->height <= 0 || pImage->width <= 0) return;

		size_t size = GetImageSize(pImage->format, pImage->width, pImage->height);
		if (size == 0)
		{
			LOGCATE("NativeImageUtil::AllocNativeImage do not support the format. Format = %d", pImage->format);
			return;
		}

		// 原实现中 I444 只设置了 ppPlane[0]，DumpNativeImage 写 ppPlane[1]/[2] 时会访问空指针；
		// 现在所有多平面格式都设置完整的平面指针。
		AttachContiguousPlanes(pImage, static_cast<uint8_t *>(malloc(size)));
		if (pImage->ppPlane[0] == nullptr)
		{
			LOGCATE("NativeImageUtil::AllocNativeImage malloc %zu bytes fail", size);
		}
	}

	static void FreeNativeImage(NativeImage *pImage)
	{
		if (pImage == nullptr || pImage->ppPlane[0] == nullptr) return;

		free(pImage->ppPlane[0]);
		pImage->ppPlane[0] = nullptr;
		pImage->ppPlane[1] = nullptr;
		pImage->ppPlane[2] = nullptr;
	}

	/**
	 * 把 pSrcImg 深拷贝到 pDstImg，拷贝后 pDstImg 的格式和宽高与源图一致。
	 *
	 * 原实现在源/目标格式或尺寸不一致时直接 return，既不拷贝也不报错；而所有调用方都是
	 * “先把目标宽高改成新图的宽高，再调用拷贝”，于是第二次传入不同尺寸的图时，检查总能通过，
	 * 却沿用旧尺寸的缓冲区 memcpy 新尺寸的数据，造成堆越界写。
	 *
	 * 新语义：目标的格式/宽高以它当前持有的缓冲区为准（调用方不要再预先修改目标的宽高格式），
	 * 与源图不一致时释放旧缓冲区并按源图重新分配。目标必须是由 NativeImageUtil 分配
	 * （或为空）的图像。返回是否拷贝成功。
	 */
	static bool CopyNativeImage(const NativeImage *pSrcImg, NativeImage *pDstImg)
	{
		if (pSrcImg == nullptr || pSrcImg->ppPlane[0] == nullptr || pDstImg == nullptr) return false;
		if (pSrcImg == pDstImg) return true;

		if (GetImageSize(pSrcImg->format, pSrcImg->width, pSrcImg->height) == 0)
		{
			LOGCATE("NativeImageUtil::CopyNativeImage do not support the format. Format = %d, [w,h]=[%d,%d]",
					pSrcImg->format, pSrcImg->width, pSrcImg->height);
			return false;
		}

		if (pDstImg->ppPlane[0] != nullptr &&
			(pDstImg->format != pSrcImg->format ||
			 pDstImg->width != pSrcImg->width ||
			 pDstImg->height != pSrcImg->height))
		{
			FreeNativeImage(pDstImg);
		}

		if (pDstImg->ppPlane[0] == nullptr)
		{
			pDstImg->format = pSrcImg->format;
			pDstImg->width = pSrcImg->width;
			pDstImg->height = pSrcImg->height;
			AllocNativeImage(pDstImg);
			if (pDstImg->ppPlane[0] == nullptr) return false;
		}

		// 源图的次级平面可能没有设置（例如只给了一整块连续内存），此时按连续布局推算
		NativeImage src = *pSrcImg;
		if (GetPlaneCount(src.format) > 1 && src.ppPlane[1] == nullptr)
		{
			AttachContiguousPlanes(&src, src.ppPlane[0]);
		}

		for (int i = 0; i < GetPlaneCount(src.format); ++i)
		{
			if (src.ppPlane[i] == nullptr || pDstImg->ppPlane[i] == nullptr) return false;
			memcpy(pDstImg->ppPlane[i], src.ppPlane[i], GetPlaneSize(src.format, src.width, src.height, i));
		}
		return true;
	}

	static void DumpNativeImage(NativeImage *pSrcImg, const char *pPath, const char *pFileName)
	{
		if (pSrcImg == nullptr || pPath == nullptr || pFileName == nullptr) return;

		if(access(pPath, F_OK) == -1)
		{
			// 目录需要可执行位才能在其中创建文件，原来的 0666 创建出的目录无法使用
			mkdir(pPath, 0777);
		}

		char imgPath[512] = {0};
		const char *pExt = nullptr;
		switch (pSrcImg->format)
		{
			case IMAGE_FORMAT_I420:
				pExt = IMAGE_FORMAT_I420_EXT;
				break;
			case IMAGE_FORMAT_NV12:
				pExt = IMAGE_FORMAT_NV12_EXT;
				break;
			case IMAGE_FORMAT_NV21:
				pExt = IMAGE_FORMAT_NV21_EXT;
				break;
			case IMAGE_FORMAT_RGBA:
				pExt = IMAGE_FORMAT_RGBA_EXT;
				break;
			case IMAGE_FORMAT_YUYV:
				pExt = IMAGE_FORMAT_YUYV_EXT;
				break;
			case IMAGE_FORMAT_GRAY:
				pExt = IMAGE_FORMAT_GRAY_EXT;
				break;
			case IMAGE_FORMAT_I444:
				pExt = IMAGE_FORMAT_I444_EXT;
				break;
			case IMAGE_FORMAT_P010:
				pExt = IMAGE_FORMAT_P010_EXT;
				break;
			case IMAGE_FORMAT_GRAY10:
				pExt = IMAGE_FORMAT_GRAY10_EXT;
				break;
			default:
				pExt = "Default";
				break;
		}

		// 路径来自调用方，长度不可控，用 snprintf 防止栈缓冲区溢出
		snprintf(imgPath, sizeof(imgPath), "%s/IMG_%dx%d_%s.%s", pPath, pSrcImg->width, pSrcImg->height, pFileName, pExt);

		FILE *fp = fopen(imgPath, "wb");

		LOGCATD("DumpNativeImage fp=%p, file=%s", fp, imgPath);

		if(fp)
		{
			switch (pSrcImg->format)
			{
				case IMAGE_FORMAT_I420:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					fwrite(pSrcImg->ppPlane[1],
						   static_cast<size_t>((pSrcImg->width >> 1) * (pSrcImg->height >> 1)), 1, fp);
					fwrite(pSrcImg->ppPlane[2],
							static_cast<size_t>((pSrcImg->width >> 1) * (pSrcImg->height >> 1)),1,fp);
					break;
				}
				case IMAGE_FORMAT_NV21:
				case IMAGE_FORMAT_NV12:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					fwrite(pSrcImg->ppPlane[1],
						   static_cast<size_t>(pSrcImg->width * (pSrcImg->height >> 1)), 1, fp);
					break;
				}
				case IMAGE_FORMAT_RGBA:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height * 4), 1, fp);
					break;
				}
				case IMAGE_FORMAT_YUYV:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height * 2), 1, fp);
					break;
				}
				case IMAGE_FORMAT_GRAY:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					break;
				}
				case IMAGE_FORMAT_P010:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height * 3), 1, fp);
					break;
				}
				case IMAGE_FORMAT_I444:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					fwrite(pSrcImg->ppPlane[1],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					fwrite(pSrcImg->ppPlane[2],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					break;
				}
				case IMAGE_FORMAT_GRAY10:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height * 2), 1, fp);
					break;
				}
				default:
				{
					fwrite(pSrcImg->ppPlane[0],
						   static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
					LOGCATD("DumpNativeImage default");
					break;
				}
			}

			fclose(fp);
			fp = NULL;
		}
	}

	/**
	 * 从文件读取原始图像数据到 pSrcImg（调用方需先设置格式/宽高并分配好缓冲区）。
	 * 返回是否完整读取：原实现不检查缓冲区和 fread 的返回值，文件缺失或长度不足时
	 * 调用方会把未初始化的内存当作图像上传。
	 */
	static bool LoadNativeImage(NativeImage *pSrcImg, const char *pPath)
	{
		if (pSrcImg == nullptr || pPath == nullptr || pSrcImg->ppPlane[0] == nullptr) return false;

		size_t dataSize = GetImageSize(pSrcImg->format, pSrcImg->width, pSrcImg->height);
		if (dataSize == 0)
		{
			LOGCATE("LoadNativeImage not support the format %d.", pSrcImg->format);
			return false;
		}

		FILE *fp = fopen(pPath, "rb");
		LOGCATD("LoadNativeImage fp=%p, file=%s", fp, pPath);
		if (fp == nullptr)
		{
			LOGCATE("LoadNativeImage open file fail, file=%s", pPath);
			return false;
		}

		// 假设缓冲区按连续布局分配（AllocNativeImage 保证这一点）
		size_t readSize = fread(pSrcImg->ppPlane[0], 1, dataSize, fp);
		fclose(fp);
		if (readSize != dataSize)
		{
			LOGCATE("LoadNativeImage read fail, file=%s, expect=%zu, read=%zu", pPath, dataSize, readSize);
			return false;
		}
		return true;
	}

	static int ConvertP010toNV21(NativeImage* pP010Img, NativeImage* pNV21Img) {
		if(pP010Img == nullptr
		|| pNV21Img == nullptr
		|| pP010Img->format != IMAGE_FORMAT_P010
		|| pNV21Img->format != IMAGE_FORMAT_NV21) return -1;

		int width = pP010Img->width, height = pP010Img->height;
		for (int i = 0; i < height; ++i) {
			uint16_t *pu16YData = (uint16_t *)(pP010Img->ppPlane[0] + pP010Img->width * 2 * i);
			uint8_t  *pu8YData = pNV21Img->ppPlane[0] + pNV21Img->width * i;
			for (int j = 0; j < width; j++, pu8YData++, pu16YData++) {
				*pu8YData = (u_int8_t)(*pu16YData >> 8);
			}
		}

		width /= 2; height /= 2;
		for (int i = 0; i < height; ++i) {
			uint16_t *pu16UVData = (uint16_t *)(pP010Img->ppPlane[1] + pP010Img->width * 2 * i);
			uint8_t  *pu8UVData = pNV21Img->ppPlane[1] + pNV21Img->width * i;
			for (int j = 0; j < width; ++j, pu8UVData+=2, pu16UVData+=2) {
				*pu8UVData = *pu16UVData >> 8;
				*(pu8UVData + 1) = *(pu16UVData + 1) >> 8;
			}
		}
		return 0;
	}

	static void ConvertP010PlaneTo8Bit(uint16_t *pSrcData, uint8_t  *pDstData, int width, int height) {
		if(pSrcData == nullptr
		   || pDstData == nullptr
		   || width <= 0
		   || height <= 0) return;

		for (int i = 0; i < height; ++i) {
			uint16_t *pu16YData = pSrcData + width * i;
			uint8_t  *pu8YData = pDstData + width * i;
			for (int j = 0; j < width; j++, pu8YData++, pu16YData++) {
				*pu8YData = (u_int8_t)(*pu16YData >> 8);
			}
		}
	}

	static int ConvertNV21toP010(NativeImage* pNV21Img, NativeImage* pP010Img) {
		if(pP010Img == nullptr
		   || pNV21Img == nullptr
		   || pP010Img->format != IMAGE_FORMAT_P010
		   || pNV21Img->format != IMAGE_FORMAT_NV21) return -1;

		int width = pP010Img->width, height = pP010Img->height;
		for (int i = 0; i < height; ++i) {
			uint16_t *pu16YData = (uint16_t *)(pP010Img->ppPlane[0] + pP010Img->width * 2 * i);
			uint8_t  *pu8YData = pNV21Img->ppPlane[0] + pNV21Img->width * i;
			for (int j = 0; j < width; j++, pu8YData++, pu16YData++) {
				*pu16YData = (u_int16_t)*pu8YData << 6;
			}
		}

		width /= 2; height /= 2;
		for (int i = 0; i < height; ++i) {
			uint16_t *pu16UVData = (uint16_t *)(pP010Img->ppPlane[1] + pP010Img->width * 2 * i);
			uint8_t  *pu8UVData = pNV21Img->ppPlane[1] + pNV21Img->width * i;
			for (int j = 0; j < width; ++j, pu8UVData+=2, pu16UVData+=2) {
				*pu16UVData = (u_int16_t)*pu8UVData << 8;
				*(pu16UVData + 1) = (u_int16_t)*(pu8UVData + 1) << 8;
			}
		}

		return 0;
	}
};


#endif //NDK_OPENGLES_3_0_IMAGEDEF_H
