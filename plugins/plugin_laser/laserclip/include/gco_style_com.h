#ifndef _GCORE_STYLE_COMMON_H_
#define _GCORE_STYLE_COMMON_H_

#include "type_private.h"
#include "image_iterator.h"
#include "type.h"
#include "parser_private.h"

static inline int32_t GET_GCO_X_FROM(const PixelData *pixel, const GCoreConfig *cfg) {
    return ((pixel->pos.x * WORD_VAL_MAGNI * 10) / cfg->density + cfg->offset.x);
}

static inline int32_t GET_GCO_Y_FROM(const GImage *img, const PixelData *pixel, const GCoreConfig *cfg) {
    return (((img->height - 1) - pixel->pos.y) * WORD_VAL_MAGNI * 10) / cfg->density + cfg->offset.y;
}

static inline void GET_GCO_POSITION_FROM(GcoPoint *pos, const GImage *img, const PixelData *pixel, const GCoreConfig *cfg) {
    pos->x = GET_GCO_X_FROM(pixel, cfg);
    pos->y = GET_GCO_Y_FROM(img, pixel, cfg);
}


uint16_t int_sqrt32(uint32_t x);
#include <math.h>
static inline double GET_GCO_DISTANCE(GImageParser *p, uint32_t x, uint32_t y, float speed_mm_per_minute) {
    double x2 =  x / (float)(WORD_VAL_MAGNI);
    double y2 =  y / (float)(WORD_VAL_MAGNI);
    double x1 =  p->CURR_X_VALUE / (float)(WORD_VAL_MAGNI);
    double y1 =  p->CURR_Y_VALUE / (float)(WORD_VAL_MAGNI);
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) / (speed_mm_per_minute / 60.f);
}

#endif // _GCORE_STYLE_COMMON_H_
