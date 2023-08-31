#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "parser_private.h"
#include "gco_block.h"

#include "gco_style_com.h"
#include "gco_style_grbl.h"

#include "gco_word.h"

//extern inline int32_t GET_GCO_Y_FROM(const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
//extern inline int32_t GET_GCO_X_FROM(const PixelData *pixel, const GCoreConfig *cfg);
//extern inline void GET_GCO_POSITION_FROM(GcoPoint *pos, const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
//extern inline GcoWord gcowd_init(enum GcoLetter letter, int32_t val);
//extern inline enum GcoLetter gcowd_letter(GcoWord wd);
//extern inline int gcowd_decmals(GCodeStyle style, enum GcoLetter letter);
//extern inline GcoValue GCO_MAGNI(int32_t __val__);
//extern inline GcoValue GCO_MAGNI_F(float __val__);

extern inline void gcoblk_serialize(const GCoreConfig *cfg, const GcoBlock *blk, char* l);

void GRBLStyle_build_head(struct GImageParser *p, const struct GCoreConfig *cfg, unsigned char idx, char* l) {
    GcoBlock *blk;
    if(idx > 6) blk = gcoblk_create();
    switch (idx) {
    case 0: strcpy(l, ";Header Start"); return;
    case 1: {
        float max_x = (GCO_MAGNI(p->iterator.gimg.width) * 10U) / cfg->density + cfg->offset.x;
        sprintf(l, ";MAXX: %.2f", max_x / WORD_VAL_MAGNI);
        return;
    }
    case 2: {
        float max_y = (GCO_MAGNI(p->iterator.gimg.height) * 10U) / cfg->density + cfg->offset.y;
        sprintf(l, ";MAXY: %.2f", max_y / WORD_VAL_MAGNI);
        return;
    }
    case 3: {
        float min_x = cfg->offset.x;
        sprintf(l, ";MINX: %.2f", min_x / WORD_VAL_MAGNI);
        return;
    }
    case 4: {
        float min_y = cfg->offset.y;
        sprintf(l, ";MINY: %.2f", min_y / WORD_VAL_MAGNI);
        return;
    }
    case 5: strcpy(l, ";Header End"); return;
    case 6: l[0] = '\n'; l[1] = '\0'; return;
    case 7: {
        gcoblk_append(blk, gcowd_init(GcoG, GCO_MAGNI(90)));
    } break;
    case 8: {
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(4)));
        gcoblk_append(blk, gcowd_init(GcoS, GCO_MAGNI(0)));
    } break;
    case 9: {
        gcoblk_append(blk, GcoG0);
        gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(cfg->jog_speed)));
    } break;
    case 10: {
        gcoblk_append(blk, GcoG1);
        gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(cfg->work_speed)));
    } break;
    case 11: {
        gcoblk_append(blk, GcoG0);
        gcoblk_append(blk, gcowd_init(GcoX, p->CURR_X_VALUE));
        gcoblk_append(blk, gcowd_init(GcoY, p->CURR_Y_VALUE));
    } break;
    default: break;
    }
    GRBLStyle_build_block(blk, l);
    gcoblk_delete(&blk);
}

void GRBLStyle_build_tail(const struct GCoreConfig *cfg, unsigned char idx, char* l) {
    GcoBlock *blk = gcoblk_create();
    switch (idx) {
    case 0: {
        gcoblk_append(blk, gcowd_init(GcoS, WORD_VAL_0));
    } break;
    case 1: {
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(5)));
    } break;
    case 2: {
        gcoblk_append(blk, GcoG0);
        gcoblk_append(blk, gcowd_init(GcoX, GCO_MAGNI(50)));
        gcoblk_append(blk, gcowd_init(GcoY, GCO_MAGNI(50)));
    } break;
    case 3: {
        gcoblk_append(blk, gcowd_init(GcoG, GCO_MAGNI(28)));
    } break;
    default: break;
    }
    GRBLStyle_build_block(blk, l);
    gcoblk_delete(&blk);
}

void GRBLStyle_build_ending(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {
    GcoBlock *blk = gcoblk_create();
    int power = p->CURR_PIXEL_DATA.grayval * p->POWER_FATOR;
    GcoPoint gp;
    GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);
    if(p->CURR_G_VALUE != WORD_VAL_1)      {  gcoblk_append(blk, GcoG1);  p->CURR_G_VALUE = WORD_VAL_1; }
    if(p->CURR_X_VALUE != gp.x)         {  gcoblk_append(blk, gcowd_init(GcoX, gp.x));     p->CURR_X_VALUE = gp.x; }
    if(p->CURR_Y_VALUE != gp.y)         {  gcoblk_append(blk, gcowd_init(GcoY, gp.y));     p->CURR_Y_VALUE = gp.y; }
    gcoblklist_append(p->line_buffer, blk);

    blk = gcoblk_create();
    gcoblk_append(blk, GcoG1);
    gcoblk_append(blk, gcowd_init(GcoX, gp.x));
    gcoblk_append(blk, gcowd_init(GcoY, gp.y));
    if(p->CURR_S_VALUE != power) {  gcoblk_append(blk, gcowd_init(GcoS, power));    p->CURR_S_VALUE = power; }
    gcoblklist_append(p->line_buffer, blk);

}

void GRBLStyle_build_newLine(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {
    GcoBlock *blk;
    int power = p->LAST_PIXEL_DATA.grayval * p->POWER_FATOR;
    int G_value = p->CURR_PIXEL_DATA.grayval ? WORD_VAL_1 : WORD_VAL_0;
    GcoPoint gp;

    /** 判断上一行行末是存在灰度，则G1移动至行末 **/
    if(p->LAST_PIXEL_DATA.grayval) {
        blk = gcoblk_create();
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->LAST_PIXEL_DATA), cfg);
        gcoblk_append(blk, GcoG1);                     p->CURR_G_VALUE = WORD_VAL_1;
        gcoblk_append(blk, gcowd_init(GcoX, gp.x));    p->CURR_X_VALUE = gp.x;
        gcoblk_append(blk, gcowd_init(GcoY, gp.y));    p->CURR_Y_VALUE = gp.y;
        gcoblk_append(blk, gcowd_init(GcoS, power));   p->CURR_S_VALUE = power;
        gcoblklist_append(p->line_buffer, blk);
    }

    /** 新行开头第一个像素有灰度的情况(G1) **/
    if(G_value == WORD_VAL_1) {
        blk = gcoblk_create();
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);
        gcoblk_append(blk, GcoG0);                     p->CURR_G_VALUE = WORD_VAL_0;
        gcoblk_append(blk, gcowd_init(GcoX, gp.x));    p->CURR_X_VALUE = gp.x;
        gcoblk_append(blk, gcowd_init(GcoY, gp.y));    p->CURR_Y_VALUE = gp.y;
        gcoblklist_append(p->line_buffer, blk);
    }

    p->PREV_PIXEL_DATA = p->CURR_PIXEL_DATA;
}

void GRBLStyle_build_inLine(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {
    GcoBlock *blk;
    int power = p->LAST_PIXEL_DATA.grayval * p->POWER_FATOR;
    int G_value = p->LAST_PIXEL_DATA.grayval ? WORD_VAL_1 : WORD_VAL_0;
    GcoPoint gp;
    if(p->iterator.ln % 2) {
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->LAST_PIXEL_DATA), cfg);
    }
    else {
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);
    }

    blk = gcoblk_create();
    if(p->CURR_G_VALUE != G_value)          { gcoblk_append(blk, gcowd_init(GcoG, G_value));  p->CURR_G_VALUE = G_value; }
    if(p->CURR_X_VALUE != gp.x)             { gcoblk_append(blk, gcowd_init(GcoX, gp.x));     p->CURR_X_VALUE = gp.x;    }
    if(p->CURR_Y_VALUE != gp.y)             { gcoblk_append(blk, gcowd_init(GcoY, gp.y));     p->CURR_Y_VALUE = gp.y;    }
    if(G_value && p->CURR_S_VALUE != power) { gcoblk_append(blk, gcowd_init(GcoS, power));    p->CURR_S_VALUE = power;   }

    p->PREV_PIXEL_DATA = p->CURR_PIXEL_DATA;
    GRBLStyle_build_block(blk, l);
    gcoblk_delete(&blk);
}

static inline int get_decmals(int letter) {
    switch(letter) {
    default:
    case GcoM:
    case GcoF:
    case GcoS:
    case GcoG: return 0;

    case GcoX:
    case GcoY:
    case GcoZ: return 2;
    }
}

void GRBLStyle_build_block(struct GcoBlock* blk, char *l) {
    for(int j = 0, len = 0; j < blk->arr.len && len < 240 ; ++j) {
        GcoWord *wd = &(blk->arr.data[j]);
        len += gcowd_sprintf(l + len, *wd, get_decmals(gcowd_letter(*wd)));
    }
}
