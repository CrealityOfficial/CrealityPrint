#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pro_conf.h"
#include "parser.h"
#include "image.h"
#include "header.h"
#include "image_iterator.h"
#if DEBUG_BUILD && DEBUG
#   include "../debug/debug_print.h"
#endif // BUILD && DEBUG_BUILD

#include "parser_private.h"
#include "gco_block.h"

#ifdef ONLY_SUPPORT_GRBL_PARSER
#if defined(ONLY_SUPPORT_MARLIN_PARSER) || defined(ONLY_SUPPORT_CV01_PARSER)
#error The macro definition(ONLY_SUPPORT_GRBL_PARSER && (ONLY_SUPPORT_MARLIN_PARSER || ONLY_SUPPORT_CV01_PARSER) == TRUE) of the conflict
#endif

#define SUPPORT_GRBL_PARSER
#undef SUPPORT_MARLIN_PARSER
#undef SUPPORT_CV01_PARSER

#elif ONLY_SUPPORT_MARLIN_PARSER
#if defined(ONLY_SUPPORT_GRBL_PARSER) || defined(ONLY_SUPPORT_CV01_PARSER)
#error The macro definition(ONLY_SUPPORT_MARLIN_PARSER && (ONLY_SUPPORT_GRBL_PARSER || ONLY_SUPPORT_CV01_PARSER) == TRUE) of the conflict
#endif

#define SUPPORT_MARLIN_PARSER
#undef SUPPORT_GRBL_PARSER
#undef SUPPORT_CV01_PARSER

#elif ONLY_SUPPORT_CV01_PARSER
#if defined(ONLY_SUPPORT_GRBL_PARSER) || defined(ONLY_SUPPORT_MARLIN_PARSER)
#error The macro definition(ONLY_SUPPORT_CV01_PARSER && (ONLY_SUPPORT_GRBL_PARSER || ONLY_SUPPORT_MARLIN_PARSER) == TRUE) of the conflict
#endif

#define SUPPORT_CV01_PARSER
#undef SUPPORT_MARLIN_PARSER
#undef SUPPORT_GRBL_PARSER

#else // ONLY_SUPPORT_GRBL_PARSER

#ifndef SUPPORT_GRBL_PARSER
    #ifndef SUPPORT_MARLIN_PARSER
        #ifndef SUPPORT_CV01_PARSER
            #error One of the three MACROS(SUPPORT_GRBL_PARSER, SUPPORT_MARLIN_PARSER, SUPPORT_CV01_PARSER) must be turned on
        #endif
    #endif
#endif

#endif // ONLY_SUPPORT_GRBL_PARSER


#ifdef SUPPORT_GRBL_PARSER
#include "gco_style_grbl.h"
#endif

#ifdef SUPPORT_MARLIN_PARSER
#include "gco_style_marlin.h"
#endif

#ifdef SUPPORT_CV01_PARSER
#include "gco_style_cv01.h"
#endif

#include "gco_style_com.h"
extern inline int32_t GET_GCO_Y_FROM(const GImage *img, const PixelData *pixel, const GCoreConfig *cfg);
extern inline int32_t GET_GCO_X_FROM(const PixelData *pixel, const GCoreConfig *cfg);
char gimg_parser_next(GImageParser *ip, GCoreConfig *cfg, char *l);
char gimg_parser_init(GImageParser *ip, GCoreConfig *cfg);
void gimg_parser_reset_to_start(GImageParser *ip, const GCoreConfig *cfg);


char gcore_parser_init(GCoreParser *gparser, const char* gcore_file) {
    char ret;
    if((!gparser && (ret = -1)) || (!gcore_file && (ret = -2))) {
        return ret;
    }

    if(gparser->rp) {
        LOGE("Can not initialize dirty GCoreParser.\n");
        return -3;
    }

    GImage gimg;
    unsigned char *gcore_header;
    if((ret = gimg_init(&gimg, gcore_file, 0, GIMG_FROM_GCORE_FILE)) != 0) {
        return ret;
    }

    if(!(gcore_header = (unsigned char*)malloc(gimg.data_offset))) {
        gimg_delete(&gimg);
        return 2;
    }

    if((fseek(gimg.data, 0, SEEK_SET) && (ret = -3))) {
        free(gcore_header);
        gimg_delete(&gimg);
        return ret;
    }

    if(((gimg.data_offset != (int16_t)fread(gcore_header, sizeof(unsigned char), gimg.data_offset, gimg.data)) &&
        (ret = -4))) {
        free(gcore_header);
        gimg_delete(&gimg);
        return ret;
    }

    if((ret = gcore_header_read(gparser, gcore_header))) {
        free(gcore_header);
        gimg_delete(&gimg);
        return ret;
    }

#if DEBUG_BUILD && DEBUG
    config_print(&(gparser->cfg));
#endif // BUILD && DEBUG_BUILD

    free(gcore_header);
    // TODO: check cfg

    if(gparser->type == ParserForGImage) {
        GImageParser *gimg_parser = (GImageParser*)malloc(sizeof(GImageParser));
        if(!gimg_parser ||
           (ret = iterator_init(&(gimg_parser->iterator), &gimg, gparser->cfg.start, gparser->cfg.dire))) {
            gimg_delete(&gimg);
            if(!gimg_parser) {
                free(gimg_parser);
                LOGE("iterator_init() failed. return value: %d\n", ret);
                return 3;
            }
            return 4;
        }

        if((ret = gimg_parser_init(gimg_parser, &(gparser->cfg)))) {
            return ret;
        }

        gparser->rp = gimg_parser;
    }
    else {
        /*
        TODO
        GCodeParser *gcode_parser = (GCodeParser*)malloc(sizeof(GCodeParser));
        */
        LOGE("Unsupport type on current firmware version.");
        return 5;
    }

    return 0;
}


void gcore_parser_deinit(struct GCoreParser *gparser) {
    if(!gparser || !(gparser->rp)) return;
    GImageParser* gimg_parser = gparser->rp;
    gcoblklist_deep_delete(&(gimg_parser->line_buffer));
    gimg_delete(&(gimg_parser->iterator.gimg));
    free(gparser->rp);
}


char gimg_parser_init(GImageParser *ip, GCoreConfig *cfg) {
    if(!ip || !cfg) {
        return -1;
    }

    switch (cfg->model) {
    default:
    case UndefinedModel:
        cfg->jog_speed = (cfg->jog_speed > 0 && cfg->jog_speed < 6400) ? cfg->jog_speed : 5000;
        cfg->work_speed = (cfg->work_speed > 0 && cfg->work_speed <= 6400) ? cfg->work_speed :
                          (cfg->speed_rate >= 0 && cfg->speed_rate <= 1000) ? (cfg->speed_rate / 1000.f) * 6400:
                          3000;

        break;
    case CV_01:
        cfg->jog_speed = (cfg->jog_speed > 0 && cfg->jog_speed < 6400) ? cfg->jog_speed : 6400;
        cfg->work_speed = (cfg->work_speed > 0 && cfg->work_speed <= 6000) ? cfg->work_speed :
                          (cfg->speed_rate >= 0 && cfg->speed_rate <= 1000) ? (cfg->speed_rate / 1000.f) * 6000:
                          2000;
        break;
    case CV_01_Pro:
        cfg->jog_speed = (cfg->jog_speed > 0 && cfg->jog_speed < 6400) ? cfg->jog_speed : 4000;
        cfg->work_speed = (cfg->work_speed > 0 && cfg->work_speed <= 6000) ? cfg->work_speed :
                          (cfg->speed_rate >= 0 && cfg->speed_rate <= 1000) ? (cfg->speed_rate / 1000.f) * 6400:
                          2000;
        break;
    case CV_20:
        cfg->jog_speed = (cfg->jog_speed > 0 && cfg->jog_speed < 6000) ? cfg->jog_speed : 4000;
        cfg->work_speed = (cfg->work_speed > 0 && cfg->work_speed <= 6000) ? cfg->work_speed :
                          (cfg->speed_rate >= 0 && cfg->speed_rate <= 1000) ? (cfg->speed_rate / 1000.f) * 6000:
                          1500;
        break;
    case Ender3_S:
        cfg->jog_speed = (cfg->jog_speed > 0 && cfg->jog_speed < 6400) ? cfg->jog_speed : 4000;
        cfg->work_speed = (cfg->work_speed > 0 && cfg->work_speed <= 6000) ? cfg->work_speed :
                          (cfg->speed_rate >= 0 && cfg->speed_rate <= 1000) ? (cfg->speed_rate / 1000.f) * 6000:
                          2500;
        break;
    }

    // 设置最小雕刻速率
    if(cfg->work_speed < 10) {
        cfg->work_speed = 10;
    }

    // 设置最小空走速率
    if(cfg->jog_speed < 100) {
        cfg->jog_speed = 100;
    }

    switch (cfg->gco_style) {
    case MarlinStyle:
#ifdef SUPPORT_MARLIN_PARSER
        ip->gc_init_count = 15;
        ip->gc_ending_count = 2;
        ip->build_head = MarlinStyle_build_head;
        ip->build_tail = MarlinStyle_build_tail;
        ip->build_ending = MarlinStyle_build_ending;
        ip->build_newLine = MarlinStyle_build_newLine;
        ip->build_inLine = MarlinStyle_build_inLine;
        ip->build_blk = MarlinStyle_build_block;
        if(cfg->model == Ender3_V2_Laser)
            ip->POWER_FATOR = GCO_MAGNI_F(cfg->power_rate / (10 * 100.f));
        else
            ip->POWER_FATOR = GCO_MAGNI_F(((cfg->power_rate / (10 * 100.f)) * 1000) / 255.f);
        
        break;
#else
        LOGE("MarlinStyle is not support in current library. Please recompile it with macro [SUPPORT_MARLIN_PARSER]\n");
        return -2;
#endif
    case CV01Style:
#ifdef SUPPORT_CV01_PARSER
        ip->gc_init_count = 12;
        ip->gc_ending_count = 2;
        ip->build_head = CV01Style_build_head;
        ip->build_tail = CV01Style_build_tail;
        ip->build_ending = CV01Style_build_ending;
        ip->build_newLine = CV01Style_build_newLine;
        ip->build_inLine = CV01Style_build_inLine;
        ip->build_blk = CV01Style_build_block;
        ip->POWER_FATOR = GCO_MAGNI_F(((cfg->power_rate / (10 * 100.f)) * 255.f) / 255.f);
        break;
#else
        LOGE("CV01Style is not support in current library. Please recompile it with macro [SUPPORT_CV01_PARSER]\n");
        return -3;
#endif
    case DefaultStyle:
    case GRBLStyle:
#ifdef SUPPORT_GRBL_PARSER
        ip->gc_init_count = 12;
        ip->gc_ending_count = 4;
        ip->build_head = GRBLStyle_build_head;
        ip->build_tail = GRBLStyle_build_tail;
        ip->build_ending = GRBLStyle_build_ending;
        ip->build_newLine = GRBLStyle_build_newLine;
        ip->build_inLine = GRBLStyle_build_inLine;
        ip->build_blk = GRBLStyle_build_block;
        ip->POWER_FATOR = GCO_MAGNI_F(((cfg->power_rate / (10 * 100.f)) * 1000) / 255.f);
        break;
#else
        LOGE("GRBLStyle is not support in current library. Please recompile it with macro [SUPPORT_GRBL_PARSER]\n");
        return -4;
#endif
    default:
        LOGE("Unknown G-Code Style. Value: %d\n", cfg->gco_style);
        return -5;
    }

    // TODO: Machine Type Switch, 适用机型配置
    ip->carving_cnt_now = 0;
    ip->gc_ending_idx = 0;
    ip->gc_init_idx = 0;
    ip->CURVING_TIME = 0U;

    ip->line_buffer = gcoblklist_create();
    if(!(ip->line_buffer)) {
        return -2;
    }
    GcoBlock *blk = gcoblk_create();
    if(!blk) {
        return -3;
    }

    gcoblklist_append(ip->line_buffer, blk);

    gimg_parser_reset_to_start(ip, cfg);
    return 0;
}

void gimg_parser_reset_to_start(GImageParser *ip, const GCoreConfig *cfg) {
    ip->is_ending = 0x00;
    iterator_move(&(ip->iterator), TO_THE_BEGIN);
    iterator_curr_gcore_pixel(&(ip->iterator), &(ip->CURR_PIXEL_DATA));
    ip->PREV_PIXEL_DATA = ip->CURR_PIXEL_DATA;
    ip->CURR_X_VALUE = GET_GCO_X_FROM(&(ip->CURR_PIXEL_DATA), cfg);
    ip->CURR_Y_VALUE = GET_GCO_Y_FROM(&(ip->iterator.gimg), &(ip->CURR_PIXEL_DATA), cfg);
    ip->CURR_S_VALUE = WORD_VAL_0;
    ip->CURR_G_VALUE = WORD_VAL_0;
    ip->CURR_LINE_NUM = 0;
    iterator_reset(&(ip->iterator), cfg->start, cfg->dire);
}

char gcore_parser_next(GCoreParser *parser, char *line) {
    if(!parser || !line)
        return -1;

    if(parser->type == ParserForGImage) {
        char ret;
        GImageParser *gimg_parser = (GImageParser*)(parser->rp);
        do {
            ret = gimg_parser_next(gimg_parser, &(parser->cfg), line);
        } while(ret == 0 && line[0] == '\0');
        //printf("%d %d\n", gimg_parser->iterator.pix_cnt, gimg_parser->iterator.zigzag_idx);

        return ret;
    }
    else if(parser->type == ParserForGCode) {
        /*
        TODO:
        GCodeParser *gcode_parser = (GCodeParser*)(parser->rp);
        return gcode_parser_next(gcode_parser, line);
        */
        LOGE("Unsupport type on current firmware version.");
        return 1;
    }
    else {
        LOGE("Unkown type on current firmware version.");
    }
    return -2;
}

int16_t getCommand(char* str)
{
    int size = strlen(str);
    if (size < 2) return -1;
    if(str[0] == 'G' && str[1] == '0')
        return 0;
    if (str[0] == 'G' && str[1] == '1')
        return 1;
    return -1;
}
void addCommand(GCoreConfig* cfg, char* l)
{
    if (strlen(l) > 2)
    {
        int16_t command = getCommand(l);
        if (command == 0 && command != cfg->cur_command && cfg->laser_stop_command != NULL)
        {
            char l_blk[255];
            strcpy(l_blk, cfg->laser_stop_command);
            strcpy(l_blk + strlen(l_blk), "\n");
            strcpy(l_blk + strlen(l_blk), l);
            sprintf(l, l_blk);
            cfg->cur_command = 0;
        }
        if (command == 1 && command != cfg->cur_command && cfg->laser_start_command != NULL)
        {
            char l_blk[255];
            strcpy(l_blk, cfg->laser_start_command);
            strcpy(l_blk + strlen(l_blk), "\n");
            strcpy(l_blk + strlen(l_blk), l);
            sprintf(l, l_blk);
            cfg->cur_command = 1;
        }
    }
}

char gimg_parser_next(GImageParser *p, GCoreConfig *cfg, char *l) {
    l[0] = '\0';

    /** 雕刻开始 前处理 **/
    if(p->gc_init_idx < p->gc_init_count) {
        p->build_head(p, cfg, p->gc_init_idx++, l);
        return 0;
    }

    /** Buffer 存在数据, 会优先使用 **/
    if(p->line_buffer->len) {
        GcoBlock *blk;
        if(!gcoblklist_pop_front(p->line_buffer, &blk)) {
            p->build_blk(blk, l);
            gcoblk_delete(&blk);
            addCommand(cfg, l);
            return 0;
        }
        else {
            LOGE("GcoBlockList pop front failed!\n");
            return -1;
        }
    }

    /** 雕刻完成 后处理 **/
    if(p->is_ending) {
        double density = cfg->density / 10.f;
        if (cfg->vector_path.ptNum > 0 && cfg->vector_path.curr_idx < cfg->vector_path.ptNum)
        {
            PixelData temp = cfg->vector_path.pt[cfg->vector_path.curr_idx];
            float x = (temp.pos.x / (float)cfg->vector_factor) / density;
            float y = (cfg->platHeight - temp.pos.y / (float)cfg->vector_factor) / density;

            if (temp.grayval == 0)
            {
                if (cfg->laser_start_command == NULL)
                {
                    if (cfg->vector_path.curr_idx == 0)
                    {
                        sprintf(l, "G0 F%d X%.3fY%.3f\nM3 S%d\n", cfg->jog_speed, x, y, cfg->power_rate);
                    }
                    else
                    {
                        sprintf(l, "M5\nG0F%dX%.3fY%.3f\nM3 S%d", cfg->jog_speed, x, y, cfg->power_rate);
                    }
                }
                else
                {
                    char coordXY[255];
                    sprintf(coordXY, "\nG0F%dX%.3fY%.3f\n", cfg->jog_speed, x, y);
                    strcpy(l, cfg->laser_stop_command);
                    strcpy(l + strlen(l), coordXY);
                    strcpy(l + strlen(l), cfg->laser_start_command);
                }
            }
            else
            {
                if(cfg->vector_path.curr_idx > 0 && cfg->vector_path.pt[cfg->vector_path.curr_idx - 1].grayval != temp.grayval)
                    sprintf(l, "G1 F%d X%.3fY%.3f S%d", cfg->work_speed, x, y, cfg->power_rate);
                else
                    sprintf(l, "G1 X%.3fY%.3f S%d", x, y, cfg->power_rate);
               if (cfg->vector_path.curr_idx == cfg->vector_path.ptNum - 1 && cfg->laser_start_command != NULL)
               {
                   strcpy(l + strlen(l), "\n");
                   strcpy(l + strlen(l), cfg->laser_stop_command);
               }
            }
            cfg->vector_path.curr_idx++;
            if(cfg->vector_path.curr_idx < cfg->vector_path.ptNum)
                return 0;
        }
        p->carving_cnt_now++;
        if(p->carving_cnt_now < cfg->carving_cnt) {
            cfg->vector_path.curr_idx = 0;
            gimg_parser_reset_to_start(p, cfg);
            return 0;
        }
        else if(p->gc_ending_idx < p->gc_ending_count) {
            p->build_tail(cfg, p->gc_ending_idx++, l);
            return 0;
        }

        /** 雕刻完成 **/
        return -127;
    }

    if(iterator_zigzag_next(&(p->iterator))) {
        p->is_ending = 0x01;
        if(p->CURR_PIXEL_DATA.grayval) {
            p->build_ending(p, cfg, l);
        }
        return 0;
    }

    p->LAST_PIXEL_DATA = p->CURR_PIXEL_DATA;
    iterator_curr_gcore_pixel(&(p->iterator), &(p->CURR_PIXEL_DATA));

    /** 处理换行的情况 **/
    if(p->CURR_LINE_NUM != p->iterator.ln) {
        p->CURR_LINE_NUM = p->iterator.ln;
        p->build_newLine(p, cfg, l);
        addCommand(cfg, l);
        return 0;
    }

    /** 处理每一行(行内）的情况 **/
    if(p->CURR_PIXEL_DATA.grayval != p->PREV_PIXEL_DATA.grayval) {
        p->build_inLine(p, cfg, l);
        addCommand(cfg, l);
        return 0;
    }

    /** 跳过相同的像素点 **/
    return 0;
}

void cnc_gcode_write_header(FILE* gcode)
{
    char line[255];
    sprintf(line, ";BingoStart\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G90\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G0 F3000\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    fwrite("\n", 1, 1, gcode);
}

void cnc_gcode_write_end(FILE* gcode)
{
    char line[255];
    sprintf(line, "G0 X0.0 Y0.0 Z5.0\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "M107\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "M107\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G0 Z6 F4000.0\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, ";end\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
}

void cnc_gcode_write_contour_start(FILE* gcode, struct GCoreConfig* cfg, ImgPoint first_pt)
{
    float x = (first_pt.x / (float)cfg->vector_factor) / cfg->density * 10.f;
    float y = (cfg->platHeight - first_pt.y / (float)cfg->vector_factor) / (float)cfg->density * 10.f;
    char line[255];
    sprintf(line, "M107\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G0 X%.1f Y%.1f Z%.1f F%.1f\n", x, y, cfg->cnc_jog_height / 10.f, (float)cfg->jog_speed);
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "M106 S255\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G4 P0.5\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G1 F%.1f\n", (float)cfg->work_speed);
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G1 Z%.1f\n", cfg->cnc_work_deep / 10.f);
    fwrite(line, sizeof(char), strlen(line), gcode);
}

void cnc_gcode_write_contour_end(FILE* gcode, struct GCoreConfig* cfg)
{
    char line[255];
    sprintf(line, "M107\n");
    fwrite(line, sizeof(char), strlen(line), gcode);
    sprintf(line, "G0 Z%.1f F%.1f\n", cfg->cnc_jog_height / 10.f, (float)cfg->jog_speed);
    fwrite(line, sizeof(char), strlen(line), gcode);
}

char gcore_parser_save_cnc_gcode(
    struct GCoreParser* gparser,	// 初始化完成的 G-Code Parser 结构体
    const char* gcode_file		// GCode文件[Output To]
) {

    if (!gparser || !gcode_file) {
        return -1;
    }
    GCoreConfig* cfg = &gparser->cfg;
    FILE* fp;
    fp = fopen(gcode_file, "wb+");
    cnc_gcode_write_header(fp);
    
    char line[255];
    float density = cfg->density / 10.f;
    for (int i = 0; i < cfg->vector_path.ptNum; i++)
    {
        PixelData pt = cfg->vector_path.pt[i];
        if (pt.grayval == 0)
        {
            if (i != 0) cnc_gcode_write_contour_end(fp, cfg);
            cnc_gcode_write_contour_start(fp, cfg, pt.pos);
        }
        else
        {
            float x = (pt.pos.x / (float)cfg->vector_factor) / density;
            float y = (cfg->platHeight - pt.pos.y / (float)cfg->vector_factor) / density;
            sprintf(line, "G1 X%.1f Y%.1f Z%.1f \n", x, y, cfg->cnc_work_deep / 10.f);
            fwrite(line, sizeof(char), strlen(line), fp);
        }
        if(i == cfg->vector_path.ptNum - 1)
            cnc_gcode_write_contour_end(fp, cfg);
    }

    cnc_gcode_write_end(fp);
    fwrite("\n", 1, 1, fp);
    fclose(fp);
    return 0;
}

char gcore_parser_save(
    struct GCoreParser *gparser,	// 初始化完成的 G-Code Parser 结构体
    const char* gcode_file		// GCode文件[Output To]
        ) {

    if(!gparser || !gcode_file) {
        return -1;
    }

    char buffer[4096 + 256];
    char* buffer_ptr = buffer;
    char* buffer_end = buffer + 4096;
    FILE *fp;
    fp = fopen(gcode_file, "wb+");
    char ret;
    char line[255];
    while(!(ret = gcore_parser_next(gparser, line))) {
        char* start = line;
        while(*start)
            *buffer_ptr++ = *start++;

        *buffer_ptr++ = '\n';

        if(buffer_ptr > buffer_end) {
            fwrite(buffer, sizeof(char), buffer_ptr - buffer, fp);
            buffer_ptr = buffer;
        }
    }
    if(buffer_ptr != buffer) {
        fwrite(buffer, sizeof(char), buffer_ptr - buffer - 1, fp);
    }
    fwrite("\n", 1, 1, fp);

    if(gparser->cfg.gco_style == MarlinStyle) {
        char estimated_time[] = "NNNNNNNNN.NN";
        for(int i = 0; i < sizeof(estimated_time); ++i)
            estimated_time[i] = '\0';

        float sec = gcore_parser_time(gparser);
        sprintf(estimated_time, "%.3f", sec);

        fseek(fp, 34, SEEK_SET);
        fwrite(estimated_time, 1, sizeof(estimated_time) - 1, fp);
        LOGI("Save Done. Ret: %d\n", ret);
    }
    fclose(fp);
    return 0;
}

int gcore_parser_progress(const struct GCoreParser *gparser) {
    if(!gparser) return -1;

    if(gparser->type == ParserForGImage) {
       GImageParser* parser = (GImageParser*)(gparser->rp);
       int head = 0, tail = 0;
       if(parser->carving_cnt_now == 0) {
           head = 1;
       }

      if(parser->carving_cnt_now == (gparser->cfg.carving_cnt - 1)) {
          tail = 1;
      }

      if(head == 1 && tail == 1) { // 雕刻一次
          return ((parser->gc_init_idx + parser->gc_ending_idx + parser->iterator.zigzag_idx + 1U) * 100U /
                  (parser->gc_init_count + parser->gc_ending_count + parser->iterator.pix_cnt)) * 100U;
      }
      else if(head == 1 && tail == 0) { // 雕刻多次，当前处于第一次
          return ((parser->gc_init_idx + parser->iterator.zigzag_idx + 1U) * 100U /
                  (parser->gc_init_count + parser->iterator.pix_cnt)) * 100U;
      }
      else if(head == 0 && tail == 1) { // 雕刻多次,当前处于最后一次
          return ((parser->gc_ending_idx + parser->iterator.zigzag_idx + 1U) * 100U /
                  (parser->gc_ending_count + parser->iterator.pix_cnt)) * 100U;
      }
      else { // 雕刻多次， 非首非尾
          return ((parser->iterator.zigzag_idx + 1U) * 100U /
                  (parser->iterator.pix_cnt)) * 100U;
      }
    }
    else {
        return -2;
    }
}

float gcore_parser_time(const struct GCoreParser *gparser) {
    if(!gparser) return -1;

    if(gparser->type == ParserForGImage) {
       GImageParser* parser = (GImageParser*)(gparser->rp);
       return parser->CURVING_TIME;
    }
    else {
        return -2;
    }
}

int gcore_parser_count(const struct GCoreParser *gparser) {
    if(!gparser) return -1;

    if(gparser->type == ParserForGImage) {
        return gparser->cfg.carving_cnt;
    }
    else {
        return -2;
    }
}

int gcore_parser_count_now(const struct GCoreParser *gparser) {
    if(!gparser) return -1;

    if(gparser->type == ParserForGImage) {
       GImageParser* parser = (GImageParser*)(gparser->rp);
       return parser->carving_cnt_now + 1;
    }
    else {
        return -2;
    }
}
