#ifndef _GCORE_PARSER_PRIVATE_H_
#define _GCORE_PARSER_PRIVATE_H_

#include "image_iterator.h"
#include "gco_block.h"

;
#pragma pack(push, 4)
struct GImageParser;
typedef void func_build_head(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        unsigned char idx,
        char* l);

typedef void func_build_tail(
        const struct GCoreConfig *cfg,
        unsigned char idx,
        char* l);

typedef void func_build(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l);

typedef void func_build_blk(
        struct GcoBlock* blk,
        char *l);

typedef struct GImageParser {
    double          CURVING_TIME;
    uint32_t        POWER_FATOR;
    int32_t         CURR_G_VALUE;
    int32_t         CURR_S_VALUE;
    int32_t         CURR_X_VALUE;
    int32_t         CURR_Y_VALUE;
    uint16_t        CURR_LINE_NUM;

    PixelData CURR_PIXEL_DATA;
    PixelData LAST_PIXEL_DATA;
    PixelData PREV_PIXEL_DATA;

    char is_ending;
    unsigned char gc_init_idx;
    unsigned char gc_init_count;
    unsigned char gc_ending_idx;
    unsigned char gc_ending_count;
    uint16_t carving_cnt_now;

    GIterator iterator;
    GcoBlockList    *line_buffer;

    func_build_tail *build_tail;
    func_build_head *build_head;
    func_build      *build_ending;
    func_build      *build_newLine;
    func_build      *build_inLine;
    func_build_blk  *build_blk;
} GImageParser;

#pragma pack(pop)

#endif // _GCORE_PARSER_PRIVATE_H_
