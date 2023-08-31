#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "parser_private.h"
#include "gco_block.h"

#include "gco_style_com.h"
#include "gco_style_marlin.h"


//extern inline int32_t GET_GCO_Y_FROM(const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
//extern inline int32_t GET_GCO_X_FROM(const PixelData *pixel, const GCoreConfig *cfg);
extern inline void GET_GCO_POSITION_FROM(GcoPoint *pos, const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
extern inline GcoWord gcowd_init(enum GcoLetter letter, int32_t val);
extern inline enum GcoLetter gcowd_letter(GcoWord wd);
extern inline GcoValue GCO_MAGNI(int32_t __val__);
extern inline GcoValue GCO_MAGNI_F(float __val__);
extern inline double GET_GCO_DISTANCE(GImageParser *p, uint32_t x, uint32_t y, float speed);

void MarlinStyle_build_head(struct GImageParser *p, const struct GCoreConfig *cfg, unsigned char idx, char* l) {
    switch(idx) {
    case 0: strcpy(l, ";Header Start"); break;
    case 1: strcpy(l, ";estimated_time(s): NNNNNNNNN.NN"); break;
    case 2: {
        float max_x = (GCO_MAGNI(p->iterator.gimg.width) * 10U) / cfg->density + cfg->offset.x;
        sprintf(l, ";MAXX: %.2f", max_x / WORD_VAL_MAGNI - cfg->extra_travel_distance_x_right / 10.);
        break;
    }
    case 3: {
        float max_y = (GCO_MAGNI(p->iterator.gimg.height) * 10U) / cfg->density + cfg->offset.y;
        sprintf(l, ";MAXY: %.2f", max_y / WORD_VAL_MAGNI);
        break;
    }
    case 4: {
        float min_x = cfg->offset.x + cfg->extra_travel_distance_x_left;
        sprintf(l, ";MINX: %.2f", min_x / WORD_VAL_MAGNI + cfg->extra_travel_distance_x_left / 10.);
        break;
    }
    case 5: {
        float min_y = cfg->offset.y;
        sprintf(l, ";MINY: %.2f", min_y / WORD_VAL_MAGNI);
        break;
    }
    case 6: strcpy(l, ";Header End"); break;
    case 7: l[0] = '\n'; l[1] = '\0'; break;
    case 8: strcpy(l, "G92 X0 Y0 Z0"); break;
    case 9: strcpy(l, "G90"); break;
    case 10: sprintf(l, "G0 F%d",  cfg->jog_speed); break;
    case 11: 
    {
        if(cfg->vector_path.ptNum == 0)
            sprintf(l, "G1 F%d", cfg->work_speed); break;
    }
    case 12: {
        if(cfg->model == Ender3_V2_Laser)
        {
            char l_blk[255];
            strcpy(l_blk, "\nG28\n\n");
            strcpy(l_blk + strlen(l_blk), cfg->laser_stop_command);
            char offsetCmd[255];
            sprintf(offsetCmd, "\nG0 X%0.3fZ3.000", cfg->v2_offsetX / 10000.);
            strcpy(l_blk + strlen(l_blk), offsetCmd);
            sprintf(l, l_blk);    
        }
        else if(cfg->laser_start_command == NULL || strstr(cfg->laser_start_command, "M4") == NULL)
        {
            strcpy(l, "M3 I");
        }
        
    }break;
    case 13: {
        if (cfg->laser_stop_command != NULL)
            sprintf(l, cfg->laser_stop_command);
    }break;
    case 14: {
        float x =  ((float)(p->CURR_X_VALUE)) / WORD_VAL_MAGNI;
        float y = ((float)(p->CURR_Y_VALUE)) / WORD_VAL_MAGNI;
        float h = p->iterator.gimg.height / (cfg->density / 10.);
        float w = p->iterator.gimg.width / (cfg->density / 10.);  
        if (cfg->model == Ender3_V2_Laser)
        {
            double circle_time = 0.;
            char data[255];
            char l_blk[255];
            sprintf(data, "\nG1X%0.3f", x + w);
            circle_time += GET_GCO_DISTANCE(p, x + w, y, cfg->jog_speed);
            strcpy(l_blk, data);
            sprintf(data, "\nG1Y%0.3f", y + h);
            circle_time += GET_GCO_DISTANCE(p, x + w, y + h, cfg->jog_speed);
            strcpy(l_blk + strlen(l_blk), data);
            sprintf(data, "\nG1X%0.3f", x);
            circle_time += GET_GCO_DISTANCE(p, x, y + h, cfg->jog_speed);
            strcpy(l_blk + strlen(l_blk), data);
            sprintf(data, "\nG1Y%0.3f", y);
            circle_time += GET_GCO_DISTANCE(p, x, y, cfg->jog_speed);
            strcpy(l_blk + strlen(l_blk), data);
            char l_blk_all[255];
            p->CURVING_TIME += GET_GCO_DISTANCE(p, x, y, cfg->jog_speed);
            sprintf(data, "G0X%0.3fY%0.3f\nM106 S5", x, y);
            strcpy(l_blk_all, data);
            strcpy(l_blk_all + strlen(l_blk_all), l_blk);
            strcpy(l_blk_all + strlen(l_blk_all), l_blk);
            strcpy(l_blk_all + strlen(l_blk_all), l_blk);;
            sprintf(l, l_blk_all);
            p->CURVING_TIME += 3 * circle_time;
        }
        else
        {
            p->CURVING_TIME += GET_GCO_DISTANCE(p, x, y, cfg->jog_speed);
            LOGI("TIME: %.6f, %f, %f", p->CURVING_TIME, x, y);
            sprintf(l, "G0X%0.3fY%0.3f", x, y);
        }
    } break;
    default: break;
    }
}

void MarlinStyle_build_tail(const struct GCoreConfig *cfg, unsigned char idx, char* l) {
    switch(idx) {
    case 0: {
        if (cfg->laser_stop_command != NULL)
        {
            sprintf(l, cfg->laser_stop_command);
            strcpy(l + strlen(l), "\n");
            strcpy(l + strlen(l), "M5 I");
        }
        else
            strcpy(l + strlen(l), "\nM5 I");
    }break;
    case 1: sprintf(l, "G0 F%d X0 Y0", cfg->jog_speed); break;
    }
}

void MarlinStyle_build_ending(
        struct GImageParser *p,
        const struct GCoreConfig *cfg,
        char *l) {

    GcoBlock *blk = gcoblk_create();
    int power = p->CURR_PIXEL_DATA.grayval * p->POWER_FATOR;
    GcoPoint gp;
    GET_GCO_POSITION_FROM(&gp, &(p->iterator.gimg), &(p->CURR_PIXEL_DATA), cfg);
    p->CURVING_TIME += GET_GCO_DISTANCE(p, gp.x, gp.y, cfg->work_speed);
    gcoblk_append(blk, GcoG1);
    if(p->CURR_X_VALUE != gp.x)         {  gcoblk_append(blk, gcowd_init(GcoX, gp.x));     p->CURR_X_VALUE = gp.x; }
    if(p->CURR_Y_VALUE != gp.y)         {  gcoblk_append(blk, gcowd_init(GcoY, gp.y));     p->CURR_Y_VALUE = gp.y; }
    gcoblklist_append(p->line_buffer, blk);

    blk = gcoblk_create();
    p->CURVING_TIME += GET_GCO_DISTANCE(p, gp.x, gp.y, cfg->work_speed);
    gcoblk_append(blk, GcoG1);
    gcoblk_append(blk, gcowd_init(GcoX, gp.x));
    gcoblk_append(blk, gcowd_init(GcoY, gp.y));
    gcoblk_append(blk, gcowd_init(GcoS, power));
    gcoblklist_append(p->line_buffer, blk);
}

void MarlinStyle_build_newLine(
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
        p->CURVING_TIME += GET_GCO_DISTANCE(p, gp.x, gp.y, cfg->work_speed);
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
        p->CURVING_TIME += GET_GCO_DISTANCE(p, gp.x, gp.y, cfg->jog_speed);
        gcoblk_append(blk, GcoG0);                     p->CURR_G_VALUE = WORD_VAL_0;
        gcoblk_append(blk, gcowd_init(GcoX, gp.x));    p->CURR_X_VALUE = gp.x;
        gcoblk_append(blk, gcowd_init(GcoY, gp.y));    p->CURR_Y_VALUE = gp.y;
        gcoblklist_append(p->line_buffer, blk);
    }

    p->PREV_PIXEL_DATA = p->CURR_PIXEL_DATA;
}

void MarlinStyle_build_inLine(
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
    p->CURVING_TIME += GET_GCO_DISTANCE(p, gp.x, gp.y, G_value == WORD_VAL_0 ? cfg->jog_speed : cfg->work_speed);

    blk = gcoblk_create();
    gcoblk_append(blk, gcowd_init(GcoG, G_value));
    if(p->CURR_X_VALUE != gp.x) { gcoblk_append(blk, gcowd_init(GcoX, gp.x));   p->CURR_X_VALUE = gp.x;    }
    if(p->CURR_Y_VALUE != gp.y) { gcoblk_append(blk, gcowd_init(GcoY, gp.y));   p->CURR_Y_VALUE = gp.y;    }
    if(G_value)                 { gcoblk_append(blk, gcowd_init(GcoS, power));  }

    p->PREV_PIXEL_DATA = p->CURR_PIXEL_DATA;
    MarlinStyle_build_block(blk, l);
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
    case GcoY: return 3;
    }
}

void MarlinStyle_build_block(struct GcoBlock* blk, char *l) {
    for(int j = 0, len = 0; j < blk->arr.len && len < 240 ; ++j) {
        GcoWord *wd = &(blk->arr.data[j]);
        len += gcowd_sprintf(l + len, *wd, get_decmals(gcowd_letter(*wd)));
    }
}
