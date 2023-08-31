#ifndef IMAGE_ITERATOR_H
#define IMAGE_ITERATOR_H

#include "type.h"
#include "image.h"

;
#pragma pack(push, 4)

typedef enum MoveAction {
    PREV_LINE_FIRST,
    PREV_LINE_LAST,

    CURR_LINE_FIRST,
    CURR_LINE_NEXT,
    CURR_LINE_PREV,
    CURR_LINE_LAST,

    NEXT_LINE_FIRST,
    NEXT_LINE_LAST,

    TO_THE_BEGIN,
    TO_THE_END
} MoveAction;

typedef struct GIterator {
    struct GImage gimg; // 图像数据
    ImgPoint curr;      // 当前迭代光标所在的点（x，y）

    uint16_t ln;        // 当前点所在行号
    uint16_t ln_cnt;    // 总行数
    uint32_t zigzag_idx;// Z行打印时的数据Index
    uint32_t pix_cnt;   // 像数总数

    int A, B, C;        // 打印路线直线参数  Ax + By + C = 0
    int x_unit;         // 打印路线迭代控制参数   直线上移动
    int y_unit;         // 打印路线迭代控制参数   直线上移动
    int c_unit;         // 打印路线迭代控制参数   直线平移
} GIterator;

#pragma pack(pop)

char iterator_init(GIterator *iterator, GImage* img, CarveStartPosition start, CarveDirection dire);
char iterator_reset(GIterator* i, CarveStartPosition start, CarveDirection dire);
void iterator_delete(GIterator *i);
char iterator_move(GIterator *i, MoveAction act); // 迭代光标移动至下一行行首
char iterator_zigzag_next(GIterator *i); // 按Z型迭代至下一个位置
char iterator_curr_gcore_pixel(GIterator *i, PixelData* p); // 获取当前光标位置的像素灰度值
char iterator_curr_image_pixel(GIterator *i, PixelData* p); // 获取当前光标位置的像素灰度值
//char get_line_num(const GIterator *i, int *num); // 获取当前行的行号
//char get_line_count(const GIterator *i, int *cnt); // 获取总行数


#endif // IMAGE_ITERATOR_H
