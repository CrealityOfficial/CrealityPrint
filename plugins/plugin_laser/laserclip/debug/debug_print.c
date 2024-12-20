#include <stdio.h>
#include "debug_print.h"
#include "parser.h"
#include "image_iterator.h"
#include "gco_word.h"
#include "parser_private.h"

void gcowd_print(void* wd_ptr) {
    if(!wd_ptr) {
        LOGI("-------- GCore Word -- Nullptr");
        return;
    }

    GcoWord* wd = wd_ptr;
    LOGI("GCore Word: %c%.4f", gcowd_char(gcowd_letter(*wd)), GCO_RESTORE(gcowd_value(*wd)));
}


void gimg_print(const GImage* img) {
    if(!img) {
        LOGI("-------- GCore Image -- Nullptr");
        return;
    }

    LOGI("-------- GCore Image -- Info start.");
    LOGI(" -- Data: %p", img->data);
    LOGI(" -- From: %s", img->src == GIMG_FROM_GCORE_FILE ? "GCore" :
                           img->src == GIMG_FROM_IMAGE_FILE ? "Image" : "Malloc");
    LOGI(" -- Type: %d", img->type);
    LOGI(" -- Width: %d", img->width);
    LOGI(" -- Height: %d", img->height);
    LOGI(" -- Offset: %d", img->data_offset);
    LOGI(" -- Cut Offset: (%d, %d)", img->cut_offset_x, img->cut_offset_y);
    LOGI("-------- GCore Image -- Info end.");
}

void gimg_parser_print(const GImageParser* p) {
    if(!p) {
        LOGI("-------- GCore Image Parser -- Nullptr");
        return;
    }
    LOGI("-------- GCore Image Parser -- Info start.");
    LOGI(" -- CURVING_TIME: %lf",   p->CURVING_TIME);
    LOGI(" -- POWER_FATOR: %u",     p->POWER_FATOR);
    LOGI(" -- CURR_G_VALUE: %d",    p->CURR_G_VALUE);
    LOGI(" -- CURR_S_VALUE: %d",    p->CURR_S_VALUE);
    LOGI(" -- CURR_X_VALUE: %d",    p->CURR_X_VALUE);
    LOGI(" -- CURR_Y_VALUE: %d",    p->CURR_Y_VALUE);
    LOGI(" -- CURR_LINE_NUM: %d",   p->CURR_LINE_NUM);

    LOGI(" -- CURR_PIXEL_DATA: [(%d, %d), %d]", p->CURR_PIXEL_DATA.pos.x, p->CURR_PIXEL_DATA.pos.y, p->CURR_PIXEL_DATA.grayval);
    LOGI(" -- LAST_PIXEL_DATA: [(%d, %d), %d]", p->LAST_PIXEL_DATA.pos.x, p->LAST_PIXEL_DATA.pos.y, p->LAST_PIXEL_DATA.grayval);
    LOGI(" -- PREV_PIXEL_DATA: [(%d, %d), %d]", p->PREV_PIXEL_DATA.pos.x, p->PREV_PIXEL_DATA.pos.y, p->PREV_PIXEL_DATA.grayval);

    LOGI(" -- is_ending: %d",       p->is_ending);
    LOGI(" -- gc_init_idx: %d",     p->gc_init_idx);
    LOGI(" -- gc_init_count: %d",   p->gc_init_count);
    LOGI(" -- gc_ending_idx: %d",   p->gc_ending_idx);
    LOGI(" -- gc_ending_count: %d", p->gc_ending_count);
    LOGI(" -- carving_cnt_now: %u", p->carving_cnt_now);

    iterator_print(&(p->iterator));
    gcoblklist_print(p->line_buffer, true);

    LOGI(" -- build_tail: %p",      p->build_blk);
    LOGI(" -- build_head: %p",      p->build_head);
    LOGI(" -- build_ending: %p",    p->build_ending);
    LOGI(" -- build_newLine: %p",   p->build_newLine);
    LOGI(" -- build_inLine: %p",    p->build_inLine);
    LOGI(" -- build_blk: %p",       p->build_blk);
    LOGI("-------- GCore Image Parser -- Info end.");
}

void parser_printf(const GCoreParser *parser) {
    if(!parser) {
        LOGI("-------- GCore Parser -- Nullptr");
        return;
    }

    LOGI("-------- GCore Parser ------------------------------ Info start.");
    LOGI(" -- FM Ver: %u", parser->fmware);
    LOGI(" -- Data Type: %s", parser->type == 0x01 ? "GImage" : parser->type == 0x02 ? "GCode" : "???");
    LOGI(" -- Configuration:");
    config_print(&(parser->cfg));
    if(parser->type == 0x01) {
        gimg_parser_print(parser->rp);
    }
    LOGI("-------- GCore Parser ------------------------------ Info end.");
}

void iterator_print(const GIterator *i) {
    if(!i) {
        LOGI("-------- GCore Iterator -- nullptr");
        return;
    }

    LOGI("-------- Iterator ------ info start.");
    LOGI(" -- curr: (%d, %d)", i->curr.x, i->curr.y);
    LOGI(" -- ln: %d", i->ln);
    LOGI(" -- ln_cnt: %d", i->ln_cnt);
    LOGI(" -- Zcurve_idx: %d", i->zigzag_idx);
    LOGI(" -- pix_cnt: %d", i->pix_cnt);
    LOGI(" -- Line Equation: %dx + %dy + %d = 0", i->A, i->B, i->C);
    LOGI(" -- Line Move Unit: X(%d) Y(%d) C(%d)", i->x_unit, i->y_unit, i->c_unit);
    gimg_print(&(i->gimg));
    LOGI("-------- Iterator ------ info end.");
}


const char* get_model_str(MachineModel model) {
    switch (model) {
    case CV_01:          return "CV-01";
    case CV_01_Pro:      return "CV-01 Pro";
    case CV_20:          return "CV-20";
    case CV_20_Pro:      return "CV-20 Pro";
    case CV_30:          return "CV-30";
    case CV_30_Pro:      return "CV-30 Pro";
    case Ender3_S:       return "Ender3 S1";
    case CV_Laser_Module_Pro: return "CV-LaserModule Pro";
    case UndefinedModel: return "Undefined Model";
    default: return "???";
    }
}

const char* get_style_str(GCodeStyle style) {
    switch (style) {
    case DefaultStyle: return "Default(GRBL)";
    case GRBLStyle:    return "GRBL";
    case MarlinStyle:  return "Marlin";
    case CV01Style:    return "CV01Style";
    default: return "???";
    }
}

const char* get_start_str(CarveStartPosition start) {
   switch (start) {
   case DefaultPosition: return "Default(BottomLeft)";
   case TopRight: return "TopRight";
   case TopLeft: return "TopLeft";
   case BottomLeft: return "BottomLeft";
   case BottomRight: return "BottomRight";
   default: return "???";
   }
}

const char* get_dire_str(CarveDirection dire) {
   switch (dire) {
   case DefaultDirection: return "Default(StraightLeft)";
   case StraightLeft: return "StraightLeft";
   case StraightRight: return "StraightRight";
   case DiagonalLeft: return "DiagonalLeft";
   case DiagonalRight: return "DiagonalRight";
   default: return "???";
   }
}

void config_print(const GCoreConfig *cfg) {
    if(!cfg) {
        LOGI("-------- GCore Config -- nullptr");
        return;
    }

    LOGI("-------- GCore Config(Size %d) -- info start.", (int)sizeof(*cfg));
    LOGI(" -- (%zu)model: %s", sizeof(cfg->model), get_model_str(cfg->model));
    LOGI("    -- (%zu)gco_style: %s",  sizeof(cfg->gco_style), get_style_str(cfg->gco_style));
    LOGI("    -- (%zu)start: %s", sizeof(cfg->start), get_start_str(cfg->start));
    LOGI("    -- (%zu)dire: %s",  sizeof(cfg->dire), get_dire_str(cfg->dire));
    LOGI("    -- (%zu)carving count: %d", sizeof(cfg->carving_cnt), cfg->carving_cnt);
    LOGI("    -- (%zu)jog speed: %d", sizeof(cfg->jog_speed), cfg->jog_speed);
    LOGI("    -- (%zu)work speed: %d", sizeof(cfg->work_speed), cfg->work_speed);
    LOGI("    -- (%zu)speed rate: %d", sizeof(cfg->speed_rate), cfg->speed_rate);
    LOGI("    -- (%zu)power rate: %d", sizeof(cfg->power_rate), cfg->power_rate);
    LOGI("    -- (%zu)density: %d", sizeof(cfg->density), cfg->density);
    LOGI("    -- (%zu)offset(%d, %d)", sizeof(cfg->offset), cfg->offset.x, cfg->offset.y);
    LOGI("-------- GCore Config -- info end.");
}

void gcoblklist_print(const GcoBlockList* blklist, bool en_addr) {
    if(!blklist) {
        LOGI("-------- GCore Block List -- nullptr");
        return;
    }

    if(!blklist->len) {
        LOGI("-------- GCore Block List -- null content");
        return;
    }

    GcoBlock *ptr = blklist->head;
    uint32_t idx = 0;
    LOGI("-------- GCore Block List -- info start.");
    while(ptr != NULL) {
        gcoblk_print(ptr, en_addr);
        idx ++;
        ptr = ptr->next;
    }
    LOGI("-------- GCore Block List -- info end.");
}

void gcoblk_print(const GcoBlock *blk, bool en_addr) {
    if(!blk) return;
    if(en_addr) LOGI("This: %p, Prev: %p, Next: %p", blk, blk->prev, blk->next);

    LOGI("Block Len: %d Real Len: %d -- ", blk->arr.len, blk->arr._real_len);
    for(int i = 0; i < blk->arr.len; ++i) {
        LOGI(" -- %c%f", gcowd_char(gcowd_letter(blk->arr.data[i])), GCO_RESTORE(gcowd_value(blk->arr.data[i])));
    }
}
