#ifndef GCOGEN_GEN_H
#define GCOGEN_GEN_H

#include "type.h"

;
#pragma pack(push, 4)

typedef int8_t GCoreParserType;
enum GCoreParserType_ {
    ParserForGImage = 0x01,
    ParserForGCode = 0x02,
    ParserForGCodeCNC = 0x03,
};

typedef struct GCoreParser {
    void            *rp;        // real parser
    uint32_t        fmware;     // firmware version number
    GCoreConfig     cfg;        // carver configration
    GCoreParserType type;       // parser type
} GCoreParser;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 函数功能说明：从Gore文件中初始化一个GCore Parser，用于后续生成G-Code
 * @param gparser 待初始化的 G-Code Parser 结构体
 * @param gcore_file 需要解析的gcore文件
 * @return
 *    0 -- 初始化成功
 *  其他 -- 遇到错误
 */
EXPORT_API char gcore_parser_init(
    struct GCoreParser *gparser,
    const char* gcore_file
);


/**
 * @brief 函数功能说明：释放gparser所占用的内存和关闭打开的gcore文件。
 * @param gparser 初始化成功后的 G-Code Parser 结构体
 */
EXPORT_API void gcore_parser_deinit(
    struct GCoreParser *gparser
);

/**
 * @brief 函数功能说明：从Gcore文件解析出完整的Gcode，并保存到指定文件
 * @param gparser 初始化完成的 G-Code Parser 结构体
 * @param gcode_file GCode文件[Output To]
 * @return
 *    0 -- 保存成功
 *  其他 -- 遇到错误
 */
EXPORT_API char gcore_parser_save(
    struct GCoreParser *gparser,
    const char* gcode_file
);

EXPORT_API char gcore_parser_save_cnc_gcode(
    struct GCoreParser* gparser,
    const char* gcode_file
);

/**
返回值：
**/
/**
 * @brief 函数功能说明： 从Gcore文件中获取下一行Gcode代码
 * @param gparser 初始化完成的 G-Code Parser 结构提
 * @param line 生成新一行G-Code存储所需的buffer
 * @return
 *    0 -- 成功生成下一行G-Code
 *  其他 -- 遇到错误
 */
EXPORT_API char gcore_parser_next(
    struct GCoreParser *gparser,
    char *line
);

/**
 * @brief 函数功能说明：返回当前解析进度
 * @param gparser 初始化完成的 G-Code Parser 结构提
 * @return
 *   返回：[0, 100 * 100] 时代表进度。进度用放大100倍的整数表示
 *      例子：0 = 0.00%
 *           1000 = 10.00%
 *           6250 = 62.50%
 *           10000 = 100.00% (已经雕刻完成)
 *   其他：-- 遇到错误
 */
EXPORT_API int gcore_parser_progress(
    const struct GCoreParser *gparser
);


// 当前，仅仅在完整导出一次GCore后有效，下位机请勿调用
EXPORT_API float gcore_parser_time(
        const struct GCoreParser *gparser
);

// 获取打印总次数
EXPORT_API int gcore_parser_count(
    const struct GCoreParser *gparser
);

// 获取当前的打印次数
EXPORT_API int gcore_parser_count_now(
    const struct GCoreParser *gparser
);

#ifdef __cplusplus
}
#endif

#endif // GCOGEN_GEN_H
