#ifndef GCOGEN_GCO_WORD_H
#define GCOGEN_GCO_WORD_H

#include <stdint.h>
#include "type.h"
#include "type_private.h"
#include "gco_style_cv01.h"
#include "gco_style_marlin.h"
#include "gco_style_grbl.h"

;
#pragma pack(push, 4)
enum GcoLetter {
    GcoA = 1,  GcoB = 2,  GcoC = 3,  GcoD = 4,  GcoE = 5,  GcoF = 6,  GcoG = 7,
    GcoH = 8,  GcoI = 9,  GcoJ = 10, GcoK = 11, GcoL = 12, GcoM = 13, GcoN = 14,
    GcoO = 15, GcoP = 16, GcoQ = 17,
    GcoR = 18, GcoS = 19, GcoT = 20,
    GcoU = 21, GcoV = 22, GcoW = 23, GcoX = 24, GcoY = 25, GcoZ = 26
};
#pragma pack(pop)

typedef int32_t GcoValue;

/**
 * @brief GcoWord
 * 为了加速下位机的计算，Parser中尽可能去掉了浮点值，浮点计算
 * G-Code Word 是由 一个 Letter 和 一个 Value 构成。
 * 当前，将 Letter 和 Value 保存在一个 int32_t 整形数据内。
 * 其bit分布如下：
 *      GcoLetter Bit  > [1:5]  (5)   range: 0 - 2^5(32)
 *      GcoValue Bit   > [6:32] (27)  range: 0 - 2^27(134,217,728)
 *
 * 关于GcoValue:
 *      单位是 0.0001。即 gco_value = 1 代表 0.0001
 *      由于GcoValue是有符号的，所以实际range是：0 - 2^26(67,108,864)
 *      所以，GcoValue可以表示的范围域是： -6710.8864mm ~ 6710.8863mm
 *
 * 关于GcoLetter：
 *      虽然其范围是 0 ~ 32, 但是其 GcoA, GcoB ... GcoX, GcoY, GcoZ 按 1 - 26 定义
 **/
typedef int32_t GcoWord;

extern GcoWord GcoG1;
extern GcoWord GcoG0;

/**
 * @brief gcowd_init
 * 如果传入的val超过值域，多余（27以后的5bit）的高位，将会被丢弃
 * 并且不会检查letter的值是否有效，无效的情况是存在的。
 * @param letter
 * 传入GcoA, GcoB, ... GcoX, GcoY, GcoZ中的一个,其他值后果是未定义的
 * @param val
 * 有效范围是： -67,108,864 ~ 67,108,863
 * @return
 * 组合Letter和Value组合成的GcoWord，如果谨慎检查后发现错误，请检查机器是否是大端储存，当前仅仅在小端存储机器上测试通过
 */
static inline GcoWord gcowd_init(enum GcoLetter letter, int32_t val) {
    return (val & 0x80000000) ?
            (((uint8_t)(letter) | ((-val) << 5)) | 0x80000000) :
            (((uint8_t)(letter) | (val << 5)) & 0x7fffffff) ;
}

static inline enum GcoLetter gcowd_letter(GcoWord wd) {
    return wd & 0x0000001f;
}

static inline char gcowd_char(GcoWord wd) {
    return (char)((wd & 0x0000001f) + 64);
}

static inline GcoValue GCO_MAGNI(int32_t __val__) {
    return (GcoValue)(__val__ * WORD_VAL_MAGNI);
}

/**
 * @brief GCO_MAGNI_F
 * 需要留意的是：如果将一个浮点数值转化为GcoValue则极有可能产生精度丢失的问题
 */
static inline GcoValue GCO_MAGNI_F(float val) {
    return (GcoValue)(val * WORD_VAL_MAGNI);
}

static inline float GCO_RESTORE(GcoValue val) {
    return ((val & 0x7fffffff) / ((float)(WORD_VAL_MAGNI)));
}

static inline GcoValue gcowd_value(GcoWord wd) {
    return (GcoValue)((wd & 0x80000000) ? (((wd & 0x7fffffff) >> 5) | 0x80000000) : (wd >> 5));
}

int gcowd_sprintf(char *buff, GcoWord wd, int decmals);


#endif // GCOGEN_GCO_BLOCK_H
