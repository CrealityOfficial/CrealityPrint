#ifndef GCOGEN_FULL_H
#define GCOGEN_FULL_H

#include "gen.h"
#include "parser.h"

/**
 * @brief 函数功能说明：根据cfg里指定的配置信息，将图片转化为GCodde
 * @param cfg 待初始化的 G-Code Parser 结构体
 * @param image_file 图像文件的路径
 * @param gcore_file 需要解析的gcore文件
 * @return
 *    0 -- 初始化成功
 *  其他 -- 遇到错误
 */
EXPORT_API char gcore_image_to_gcode(
    struct GCoreConfig *cfg,
    const char* image_file,
    const char* gcode_file
);

#endif // GCOGEN_FULL_H
