#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "parser_private.h"
#include "gco_block.h"

#include "gco_style_cv01.h"
#include "gco_style_com.h"
#include "gco_word.h"

//extern inline int32_t GET_GCO_Y_FROM(const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
//extern inline int32_t GET_GCO_X_FROM(const PixelData *pixel, const GCoreConfig *cfg);
//extern inline void GET_GCO_POSITION_FROM(GcoPoint *pos, const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
//extern inline GcoWord gcowd_init(enum GcoLetter letter, int32_t val);
//extern inline enum GcoLetter gcowd_letter(GcoWord wd);
//extern inline int gcowd_decmals(GCodeStyle style, enum GcoLetter letter);
//extern inline GcoValue GCO_MAGNI(int32_t __val__);
//extern inline GcoValue GCO_MAGNI_F(float __val__);

void CV01Style_build_head(struct GImageParser *p, const struct GCoreConfig *cfg, unsigned char idx, char* l) {
    GcoBlock *blk = gcoblk_create();
    switch (idx) {
    case 0: {
        gcoblk_append(blk, gcowd_init(GcoG, GCO_MAGNI(28)));
        gcoblk_append(blk, gcowd_init(GcoX, GCO_MAGNI(0)));
        gcoblk_append(blk, gcowd_init(GcoY, GCO_MAGNI(0)));
        gcoblk_append(blk, gcowd_init(GcoZ, GCO_MAGNI(0)));
        break;
    }
    case 1: {
        gcoblk_append(blk, GcoG0);
        gcoblk_append(blk, gcowd_init(GcoX, cfg->offset.x));
        gcoblk_append(blk, gcowd_init(GcoY, cfg->offset.y));
        gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(2500)));
        break;
    }
    case 2: {
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(106)));
        gcoblk_append(blk, gcowd_init(GcoS, GCO_MAGNI(5)));
        break;
    }
    case 7:
    case 3: {
        gcoblk_append(blk, GcoG1);
        gcoblk_append(blk, gcowd_init(GcoX, cfg->offset.x));
        gcoblk_append(blk, gcowd_init(GcoY, cfg->offset.y + GCO_MAGNI(p->iterator.gimg.height * 10 / cfg->density)));
        break;
    }
    case 8:
    case 4: {
        gcoblk_append(blk, GcoG1);
        gcoblk_append(blk, gcowd_init(GcoX, cfg->offset.x + GCO_MAGNI(p->iterator.gimg.width) * 10 / cfg->density));
        gcoblk_append(blk, gcowd_init(GcoY, cfg->offset.y + GCO_MAGNI(p->iterator.gimg.height) * 10 / cfg->density));
        break;
    }
    case 9:
    case 5: {
        gcoblk_append(blk, GcoG1);
        gcoblk_append(blk, gcowd_init(GcoX, cfg->offset.x + GCO_MAGNI(p->iterator.gimg.width) * 10 / cfg->density));
        gcoblk_append(blk, gcowd_init(GcoY, cfg->offset.y));
        break;
    }
    case 10:
    case 6: {
        gcoblk_append(blk, GcoG1);
        gcoblk_append(blk, gcowd_init(GcoX, cfg->offset.x));
        gcoblk_append(blk, gcowd_init(GcoY, cfg->offset.y));
        break;
    }
    case 11: {
        gcoblk_append(blk, gcowd_init(GcoM, WORD_VAL_0));
        break;
    }
    default: break;
    }
    CV01Style_build_block(blk, l);
    gcoblk_delete(&blk);
}

void CV01Style_build_tail(const struct GCoreConfig *cfg, unsigned char idx, char* l) {
    GcoBlock *blk = gcoblk_create();
    switch (idx) {
    case 0: {
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(107)));
        break;
    }
    case 1: {
        gcoblk_append(blk, gcowd_init(GcoG, GCO_MAGNI(28)));
        break;
    }
    default: break;
    }
    CV01Style_build_block(blk, l);
    gcoblk_delete(&blk);
}


void CV01Style_build_ending(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {

    GcoBlock *blk = gcoblk_create();
    int power = p->CURR_PIXEL_DATA.grayval * p->POWER_FATOR;
    GcoPoint gp;
    GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);

    gcoblk_append(blk, GcoG1);
    if(p->CURR_X_VALUE != gp.x)         {  gcoblk_append(blk, gcowd_init(GcoX, gp.x));     p->CURR_X_VALUE = gp.x; }
    if(p->CURR_Y_VALUE != gp.y)         {  gcoblk_append(blk, gcowd_init(GcoY, gp.y));     p->CURR_Y_VALUE = gp.y; }
    if(p->CURR_G_VALUE != WORD_VAL_1)   {  gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(cfg->work_speed)));  p->CURR_G_VALUE = WORD_VAL_1; }
    gcoblklist_append(p->line_buffer, blk);

    if(p->CURR_S_VALUE != power) {
        blk = gcoblk_create();
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(106)));
        gcoblk_append(blk, gcowd_init(GcoS, power));
        p->CURR_S_VALUE = power;
        gcoblklist_append(p->line_buffer, blk);
    }

    blk = gcoblk_create();
    gcoblk_append(blk, GcoG1);
    gcoblk_append(blk, gcowd_init(GcoX, gp.x));
    gcoblk_append(blk, gcowd_init(GcoY, gp.y));
    gcoblklist_append(p->line_buffer, blk);
}

void CV01Style_build_newLine(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {
    GcoBlock *blk;
    int power = p->LAST_PIXEL_DATA.grayval * p->POWER_FATOR;
    int G_value = p->CURR_PIXEL_DATA.grayval ? WORD_VAL_1 : WORD_VAL_0;
    GcoPoint gp;

    /** 判断上一行行末是存在灰度，则G1移动至行末 **/
    if(p->LAST_PIXEL_DATA.grayval) {
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->LAST_PIXEL_DATA), cfg);

        blk = gcoblk_create();
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(106)));
        gcoblk_append(blk, gcowd_init(GcoS, power));   p->CURR_S_VALUE = power;
        gcoblklist_append(p->line_buffer, blk);

        blk = gcoblk_create();
        gcoblk_append(blk, GcoG1);
        gcoblk_append(blk, gcowd_init(GcoX, gp.x));    p->CURR_X_VALUE = gp.x;
        gcoblk_append(blk, gcowd_init(GcoY, gp.y));    p->CURR_Y_VALUE = gp.y;
        if(p->CURR_G_VALUE != WORD_VAL_1)  {
            gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(cfg->work_speed)));
        }
        p->CURR_G_VALUE = WORD_VAL_1;
        gcoblklist_append(p->line_buffer, blk);
    }

    /** 新行开头第一个像素有灰度的情况(G1) **/
    if(G_value == WORD_VAL_1) {
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);

        blk = gcoblk_create();
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(107)));
        gcoblklist_append(p->line_buffer, blk);

        blk = gcoblk_create();
        gcoblk_append(blk, GcoG0);
        gcoblk_append(blk, gcowd_init(GcoX, gp.x));    p->CURR_X_VALUE = gp.x;
        gcoblk_append(blk, gcowd_init(GcoY, gp.y));    p->CURR_Y_VALUE = gp.y;
        if(p->CURR_G_VALUE != WORD_VAL_0)  {
            gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(cfg->jog_speed)));
        }
        p->CURR_G_VALUE = WORD_VAL_0;
        gcoblklist_append(p->line_buffer, blk);
    }

    p->PREV_PIXEL_DATA = p->CURR_PIXEL_DATA;
}

void CV01Style_build_inLine(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {
    GcoBlock *blk = gcoblk_create();
    int power = p->LAST_PIXEL_DATA.grayval * p->POWER_FATOR;
    int G_value = p->LAST_PIXEL_DATA.grayval ? WORD_VAL_1 : WORD_VAL_0;
    GcoPoint gp;
    if(p->iterator.ln % 2) {
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->LAST_PIXEL_DATA), cfg);
    }
    else {
        GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);
    }

    if(G_value && (p->CURR_S_VALUE != power || p->CURR_G_VALUE != G_value)) {
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(106)));
        gcoblk_append(blk, gcowd_init(GcoS, power));
        gcoblklist_append(p->line_buffer, blk);
        blk = gcoblk_create();
    }
    else if(p->CURR_G_VALUE != WORD_VAL_0) {
        gcoblk_append(blk, gcowd_init(GcoM, GCO_MAGNI(107)));
        gcoblklist_append(p->line_buffer, blk);
        blk = gcoblk_create();
    }

    gcoblk_append(blk, gcowd_init(GcoG, G_value));
    if(p->CURR_X_VALUE != gp.x) { gcoblk_append(blk, gcowd_init(GcoX, gp.x));     p->CURR_X_VALUE = gp.x;    }
    if(p->CURR_Y_VALUE != gp.y) { gcoblk_append(blk, gcowd_init(GcoY, gp.y));     p->CURR_Y_VALUE = gp.y;    }
    if(p->CURR_G_VALUE != G_value)  {
        gcoblk_append(blk, gcowd_init(GcoF, GCO_MAGNI(G_value == WORD_VAL_0 ? cfg->jog_speed : cfg->work_speed)));
    }
    gcoblklist_append(p->line_buffer, blk);

    p->CURR_G_VALUE = G_value;
    p->CURR_S_VALUE = power;
    p->PREV_PIXEL_DATA = p->CURR_PIXEL_DATA;
}

static inline int get_decmals(int letter) {
    switch(letter) {
    default:
    case GcoM:
    case GcoF:
    case GcoG: return 0;

    case GcoS:
    case GcoX:
    case GcoY:
    case GcoZ: return 2;
    }
}

void CV01Style_build_block(struct GcoBlock* blk, char *l) {
    for(int j = 0, len = 0; j < blk->arr.len && len < 240 ; ++j) {
        GcoWord *wd = &(blk->arr.data[j]);
        len += gcowd_sprintf(l + len, *wd, get_decmals(gcowd_letter(*wd)));
    }
}
