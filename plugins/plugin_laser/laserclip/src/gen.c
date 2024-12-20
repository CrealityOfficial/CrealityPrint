#include "pro_conf.h"
#include "gen.h"
#include "image.h"
#include "image_iterator.h"
#include "header.h"
#include "type_private.h"
#include <stdio.h>
#include <string.h>
#if DEBUG_BUILD && DEBUG
#include "../debug/debug_print.h"
#endif // BUILD && DEBUG_BUILD

char _gcore_check_conf(struct GCoreConfig *cfg);
char _gcore_write_header(FILE *gcore, struct GCoreConfig *cfg, struct GImage *img);
char _gcore_write_imgdata(FILE *gcore, GCoreConfig *cfg, GIterator *iterator);
char _gcore_generate_gcorefile(GImage* img, const char* gcore_file, struct GCoreConfig* cfg );

EXPORT_API char gcore_generate_from_argb8888(
    const char* argb8888,       // 图像rgba数据
    int32_t     width,          // 图像宽度
    int32_t     height,         // 图像高度
    const char* gcorefile,		// GCore文件[Output To]
    struct GCoreConfig* cfg		// 雕刻配置信息
    ) {
#if DEBUG_BUILD && DEBUG
    LOGI("API Got Args: ");
    LOGI("argb8888: %p", argb8888);
    LOGI("width: %d", width);
    LOGI("height: %d", height);
    LOGI("gcore file: (%p)%s", gcorefile, gcorefile ? gcorefile : "nullptr");
    config_print(cfg);
#endif // BUILD && DEBUG_BUILD
    char ret = 0;
    if((!argb8888 && (ret = -1)) ||
       (!gcorefile && (ret = -2)) ||
       ((width * height <= 0) && (ret = -4)) ||
       (!cfg && (ret = -3)) ) {
        LOGE("Invalid Args, get ret %d\n", ret);
        return -1;
    }

    if((ret = _gcore_check_conf(cfg))) {
        LOGE("_gcore_check_conf() return %d\n", ret);
        ret = -2;
        return ret;
    }

    GImage img = {0};
    if((ret = gimg_init_from_argb8888(&img, argb8888, width, height))) {
        LOGE("gimg_init_from_rgba() return %d\n", ret);
        ret = -3;
        return ret;
    }

    ret = _gcore_generate_gcorefile(&img, gcorefile, cfg);
    gimg_delete(&img);
    return ret;
}


char gcore_generate_from_imgdata(
    const char* imgdata,
    const int32_t imgdata_len,
    const char* gcorefile,
    struct GCoreConfig* cfg
        ) {
#if DEBUG_BUILD && DEBUG
    LOGI("API Got Args: ");
    LOGI("imgdata: %p", imgdata);
    LOGI("imgdata_len: %d", imgdata_len);
    LOGI("gcore file: (%p)%s", gcorefile, gcorefile ? gcorefile : "nullptr");
    config_print(cfg);
#endif // BUILD && DEBUG_BUILD

    char ret = 0;
    if((!imgdata && (ret = -1)) ||
       (!gcorefile && (ret = -2)) ||
       ((imgdata_len <= 0) && (ret = -4)) ||
       (!cfg && (ret = -3)) ) {
        LOGE("Invalid Args, get ret %d\n", ret);
        return -1;
    }

    if((ret = _gcore_check_conf(cfg))) {
        LOGE("_gcore_check_conf() return %d\n", ret);
        ret = -2;
        return ret;
    }

    GImage img = {0};
    if((ret = gimg_init(&img, imgdata, imgdata_len, GIMG_FROM_IMAGE_FILE))) {
        LOGE("gimg_init() return %d\n", ret);
        ret = -3;
        return ret;
    }

    ret = _gcore_generate_gcorefile(&img, gcorefile, cfg);
    gimg_delete(&img);
    return ret;
}


char gcore_generate(
    const char* data_ptr, 		// 雕刻图像文件名及路径
    const char* gcore_file,		// GCore文件[Output To]
    struct GCoreConfig* cfg		// 雕刻配置信息
        ) {

#if DEBUG_BUILD && DEBUG
    LOGI("API Got Args: ");
    LOGI("data_ptr: (%p)%s", data_ptr, data_ptr ? data_ptr : "nullptr");
    LOGI("gcore file: (%p)%s", gcore_file, gcore_file ? gcore_file : "nullptr");
    config_print(cfg);
#endif // BUILD && DEBUG_BUILD

    char ret = 0;
    // 输入检查
    if((!data_ptr && (ret = -1)) ||
       (!gcore_file && (ret = -2)) ||
       (!cfg && (ret = -3)) ) {
        LOGE("Invalid Args, get ret %d\n", ret);
        return -1;
    }

    if((ret = _gcore_check_conf(cfg))) {
        LOGE("_gcore_check_conf() return %d\n", ret);
        ret = -2;
        return ret;
    }

    GImage img = {0};
    if((ret = gimg_init(&img, data_ptr, 0, GIMG_FROM_IMAGE_FILE))) {
        LOGE("gimg_init() return %d\n", ret);
        ret = -3;
        return ret;
    }

    ret = _gcore_generate_gcorefile(&img, gcore_file, cfg);
    gimg_delete(&img);
    return ret;
}

char _gcore_generate_gcorefile(
    GImage* img,
    const char* gcore_file,		// GCore文件[Output To]
    struct GCoreConfig* cfg		// 雕刻配置信息
        ) {
    // TODO:
    // 1. 检查文件名 长度限制512;
    // 2. 如果没用.gcore后缀（提醒外部添加）

#if DEBUG_BUILD && DEBUG
    gimg_print(img);
#endif // DEBUG && DEBUG_BUILD


    char ret = 0;
    FILE *gcore = NULL;
    GIterator iterator;
    if(!(gcore = fopen(gcore_file, "wb"))) {
        LOGE("Can not create GCore file: %s\n", gcore_file);
        return -4;
    }

    if((ret = iterator_init(&iterator, img, cfg->start, cfg->dire))) {
        LOGE("gimg_init return %d\n", ret);
        fclose(gcore);
        return -5;
    }

    if((ret = _gcore_write_header(gcore, cfg, img))) {
        LOGE("_gcore_write_header() return %d\n", ret);
        fclose(gcore);
        return -6;
    }

    if((ret = _gcore_write_imgdata(gcore, cfg, &iterator))) {
        LOGE("_gcore_write_imgdata() return %d\n", ret);
        fclose(gcore);
        return -7;
    }

    fclose(gcore);
    return 0;
}

char _gcore_check_conf(struct GCoreConfig* cfg) {
    // TODO

    return 0;
}

char _gcore_write_header(FILE* gcore, struct GCoreConfig* cfg, struct GImage *gimg) {
    unsigned char header[GCORE_HEADER_LENGTH];
    uint32_t ret;
    uint32_t cut_offset_x = gimg->cut_offset_x * (unsigned long)WORD_VAL_MAGNI * 10UL / cfg->density;
    uint32_t cut_offset_y = gimg->cut_offset_y * (unsigned long)WORD_VAL_MAGNI * 10UL / cfg->density;

    cfg->offset.x += cut_offset_x;
    cfg->offset.y += cut_offset_y;

    gcore_header_write(cfg, gimg, header);

    if((ret = fwrite(header, sizeof(unsigned char), GCORE_HEADER_LENGTH, gcore)) != GCORE_HEADER_LENGTH) {
        LOGE("Write GCore file head error! writed(%u) total(%u)", ret, GCORE_HEADER_LENGTH);
        return -70;
    }
    cfg->offset.x -= cut_offset_x;
    cfg->offset.y -= cut_offset_y;

    return 0;
}

#define BUFFER_SIZE 4096
char _gcore_write_imgdata(FILE *gcore, GCoreConfig *cfg, struct GIterator *iterator) {
    char next_ret = 0;
    PixelData pixel;
    uint32_t write_ret = 0;
    uint32_t buffer_idx = 0;
    uint32_t image_idx = 0;

    if(iterator->gimg.type == GIMG_Grayscale8) {
        unsigned char buffer[BUFFER_SIZE];
        do {
            if(iterator_curr_image_pixel(iterator, &pixel)) {
                LOGE("Get Pxiel Fialed in Position(%d, %d)\n", pixel.pos.x, pixel.pos.y);
            }

            buffer[buffer_idx++] = (255 - pixel.grayval);

            if(buffer_idx == BUFFER_SIZE) {
                if((write_ret = fwrite(buffer, sizeof(unsigned char), buffer_idx, gcore)) != buffer_idx) {
                    LOGE("Write GCore file head error! writed(%u) total(%u)\n", write_ret, buffer_idx);
                    return -80;
                }
                image_idx += buffer_idx;
                buffer_idx = 0;
            }
        }
        while(!(next_ret = iterator_zigzag_next(iterator)));
        LOGI("Next end with %d\n", next_ret);

        if(buffer_idx) {
            if((write_ret = fwrite(buffer, sizeof(unsigned char), buffer_idx, gcore)) != buffer_idx) {
                LOGE("Write GCore file head error! writed(%u) total(%u)\n", write_ret, buffer_idx);
            }
            image_idx += buffer_idx;
        }
    }
    else {
        LOGE("Unspoort image type(%d)\n", iterator->gimg.type);
        return -81;
    }

    LOGI("Wrote %uByte, Image Size: %uByte\n", image_idx, iterator->gimg.width * iterator->gimg.height);

    return 0;
}
#undef BUFFER_SIZE
