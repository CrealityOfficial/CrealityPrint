#ifndef _GCORE_GEN_H_
#define _GCORE_GEN_H_

#include "type.h"
#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
功能说明：将图像数据和雕刻信息整合到一起，生成一个gcore文件
返回值：
    0 -- 生成并保存.gcore文件成功
  其他 -- 遇到错误
**/
EXPORT_API char gcore_generate(
    const char* imgfile, 		// 雕刻图像文件名及路径
    const char* gcorefile,		// GCore文件[Output To]
    struct GCoreConfig* cfg		// 雕刻配置信息
);


/**
功能说明：将图像数据和雕刻信息整合到一起，生成一个gcore文件
返回值：
    0 -- 生成并保存.gcore文件成功
  其他 -- 遇到错误
**/
EXPORT_API char gcore_generate_from_imgdata(
    const char* imgdata,        // 图像文件数据（JPEG, PNG, GIF....等待格式的文件数据）
    const int32_t imgdata_len, // 图像文件数据的长度
    const char* gcorefile,		// GCore文件[Output To]
    struct GCoreConfig* cfg		// 雕刻配置信息
);


/**
功能说明：将图像数据和雕刻信息整合到一起，生成一个gcore文件
返回值：
    0 -- 生成并保存.gcore文件成功
  其他 -- 遇到错误
**/
EXPORT_API char gcore_generate_from_argb8888(
    const char* argb8888,       // 图像argb888数据
    int32_t     width,          // 图像宽度
    int32_t     height,         // 图像高度
    const char* gcorefile,		// GCore文件[Output To]
    struct GCoreConfig* cfg		// 雕刻配置信息
);

EXPORT_API char _gcore_generate_gcorefile(
        GImage* img,
        const char* gcore_file,		// GCore文件[Output To]
        struct GCoreConfig* cfg		// 雕刻配置信息
);

#ifdef __cplusplus
}
#endif

#endif
