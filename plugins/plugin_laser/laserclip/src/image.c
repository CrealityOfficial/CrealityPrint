#include "image.h"
#ifdef __APPLE__
//#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>

#ifdef BUILD_LIBRARY_GCORE_GEN
#include "./img_process.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#endif

void gimg_delete(GImage *img) {
    if(!img) return;

    // stb 的数据 底层也是调用 free
    if(img->src == GIMG_FROM_IMAGE_FILE || img->src == GIMG_FROM_MALLOC) {
#ifdef BUILD_LIBRARY_GCORE_GEN
        free(img->data);
#else
        LOGE("Error: use [malloc gcore] from [gcore_parser]");
#endif

    }
    else if(img->src == GIMG_FROM_GCORE_FILE) {
        fclose((FILE*)(img->data));
    }
}

#ifdef BUILD_LIBRARY_GCORE_GEN
char gimg_init_from_argb8888(GImage *gimg, const uint8_t* argb, int32_t width, int32_t height) {
    char ret = 0;

    unsigned char *gray = malloc(width * height);
    if(!gray) {
        LOGE("malloc failed.");
        return -1;
    }

    // convert argb to gray
    unsigned char* gray_start = gray;
    unsigned char* gray_end = gray + width * height;
    while(gray_start < gray_end) {
        float alpha = ((float)(*argb)) / 255.f;
        int grayval =  (int)(0.2989f * argb[1] +
                             0.5870f * argb[2] +
                             0.1140f * argb[3]);
        *gray_start++ = 255 - ((int)((255 - grayval) * alpha + 0.5f));
        argb += 4;
    }

    gimg->type = GIMG_Grayscale8;
    gimg->data = gray;
    gimg->data_offset = 0;
    gimg->width = width;
    gimg->height = height;
    gimg->cut_offset_x = 0;
    gimg->cut_offset_y = 0;
    gimg->src = GIMG_FROM_MALLOC;

    return ret;
}

static unsigned char* _gimg_load_from_memory(const uint8_t* src_data, int src_data_len, int *width, int *height, SOURCE_TYPE* src_type) {
    int original_no_channels;
    unsigned char* data = NULL;

    // 查看图像信息
    if(!stbi_info_from_memory(src_data, src_data_len, width, height, &original_no_channels)) {
        LOGE("Can not get the image infomation from memory, %p\n", src_data);
        return data;
    }

    if(*width > GCORE_MAX_SUPPORTED_IMG_W || *height > GCORE_MAX_SUPPORTED_IMG_H) {
        LOGE("the image size is too big to load. current limit is %d x %d\n", GCORE_MAX_SUPPORTED_IMG_W, GCORE_MAX_SUPPORTED_IMG_H);
        return data;
    }

    // 融合aplha通道。因为stb库直接取gray通道时，不会将apha通过效果融合到gray通道上
    if(original_no_channels == STBI_grey_alpha || original_no_channels == STBI_rgb_alpha) {
        if(!(data = stbi_load_from_memory(src_data, src_data_len, width, height, &original_no_channels, STBI_grey_alpha))) {
            LOGE("Can not loading the image from memory: %p\n", src_data);
            return data;
        }

        unsigned char* gray;
        if(imgproc_apply_alpha(&gray, data, *width, *height)) {
            stbi_image_free(data);
            LOGE("Can not alpha: %s\n", (const char*)src_data);
            return data;
        }
        stbi_image_free(data);
        data = gray;
        *src_type = GIMG_FROM_MALLOC;
    }
    else {
        if(!(data = stbi_load_from_memory(src_data, src_data_len, width, height, &original_no_channels, STBI_grey))) {
            LOGE("Can not loading the image from memory: %p\n", src_data);
            return data;
        }
    }

    return data;
}

static unsigned char* _gimg_load(const void *src_data, int *width, int *height, SOURCE_TYPE *src_type) {
    int original_no_channels;
    unsigned char* data = NULL;

    // 查看图像信息
    if(!stbi_info(src_data, width, height, &original_no_channels)) {
        LOGE("Can not get the image infomation from file: %s\n", (const char*)src_data);
        return data;
    }

    if(*width > GCORE_MAX_SUPPORTED_IMG_W || *height > GCORE_MAX_SUPPORTED_IMG_H) {
        LOGE("the image size is too big to load. current limit is %d x %d\n", GCORE_MAX_SUPPORTED_IMG_W, GCORE_MAX_SUPPORTED_IMG_H);
        return data;
    }

    // 融合aplha通道。因为stb库直接取gray通道时，不会将apha通过效果融合到gray通道上
    if(original_no_channels == STBI_grey_alpha || original_no_channels == STBI_rgb_alpha) {
        if(!(data = stbi_load(src_data, width, height, &original_no_channels, STBI_grey_alpha))) {
            LOGE("Can not loading the image from file: %s\n", (const char*)src_data);
            return data;
        }

        unsigned char* gray;
        if(imgproc_apply_alpha(&gray, data, *width, *height)) {
            stbi_image_free(data);
            LOGE("Can not alpha: %s\n", (const char*)src_data);
            return data;
        }
        stbi_image_free(data);
        data = gray;
        *src_type = GIMG_FROM_MALLOC;
    }
    else {
        if(!(data = stbi_load(src_data, width, height, &original_no_channels, STBI_grey))) {
            LOGE("Can not loading the image from file: %s\n", (const char*)src_data);
            return data;
        }
    }

    return data;
}

#endif

char gimg_init(GImage *img, const void *src_data, int32_t src_data_len, SOURCE_TYPE src_type) {
    char ret = 0;
    if((!img && (ret = -1)) ||
       (!src_data && (ret = -2))) {
        return ret;
    }

    if(src_type == GIMG_FROM_IMAGE_FILE) {
#ifdef BUILD_LIBRARY_GCORE_GEN
        int width, height;
        int16_t cut_offset_x = 0;
        int16_t cut_offset_y = 0;
        unsigned char* data = NULL;

        // 查看图像信息
        if(src_data_len <= 0) {
            if(!(data = _gimg_load(src_data, &width, &height, &src_type))) {
                return 1;
            }
        }
        else {
            if(!(data = _gimg_load_from_memory(src_data, src_data_len, &width, &height, &src_type))) {
                return 2;
            }
        }

        int rect_x, rect_y, rect_w, rect_h;
        if(!imgproc_find_valid_bounding_box(data, width, height, 254, &rect_x, &rect_y, &rect_w, &rect_h)) {
            if(!rect_w || !rect_h) {
                if(src_type == GIMG_FROM_MALLOC) {
                    free(data);
                } else if(src_type == GIMG_FROM_IMAGE_FILE) {
                    stbi_image_free(data);
                }
                LOGE("It makes no sense to carve this picture.(%s)(%d %d %d %d)\n",
                     (const char*)src_data,
                     rect_x, rect_y, rect_w, rect_h
                     );
                return 111;
            }
            //if(width * height - rect_w * rect_h > 4096) {
                unsigned char* cut_img;
                if(!imgproc_rect_cut(&cut_img, data, width, height, rect_x, rect_y, rect_w, rect_h)) {
                    if(src_type == GIMG_FROM_MALLOC) {
                        free(data);
                    } else if(src_type == GIMG_FROM_IMAGE_FILE) {
                        stbi_image_free(data);
                    }
                    data = cut_img;
                    cut_offset_x = rect_x;
                    cut_offset_y = height - rect_y - rect_h;
                    width = rect_w;
                    height = rect_h;
                    src_type = GIMG_FROM_MALLOC;
                }
            //}
        }

        img->type = GIMG_Grayscale8;
        img->data = data;
        img->data_offset = 0;
        img->width = width;
        img->height = height;
        img->cut_offset_x = cut_offset_x;
        img->cut_offset_y = cut_offset_y;
        img->src = src_type;
        return 0;
#else
        LOGE("Error: use [image gcore] from [gcore_parser]");
        return -3;
#endif
    }

    FILE *gcore;
    if(!(gcore = fopen(src_data, "rb"))) {
        LOGE("Can not load Gcore file: %s\n", (const char*)src_data);
        return 2;
    }

    uint32_t version_num;
    uint16_t header_len;
    char data_type;
    {
        unsigned char ver_and_len[16];
        if(16 != fread(ver_and_len, sizeof(unsigned char), 16, gcore)) {
            fclose(gcore);
            LOGE("Can not read Gcore file header.\n");
            return 3;
        }

        version_num = ((uint32_t)ver_and_len[0]) << 24 | ((uint32_t)ver_and_len[1]) << 16 | ((uint32_t)ver_and_len[2]) << 8 | ((uint32_t)ver_and_len[3]);
        header_len = ((uint32_t)ver_and_len[4]) << 8 | ((uint32_t)ver_and_len[5]);
        data_type = ver_and_len[6];

        if(version_num < 2108000000U || header_len > 1024) {
            fclose(gcore);
            LOGI("Version Num: %d\n", version_num);
            LOGI("Header Len: %d\n", header_len);
            return 4;
        }
    }

    if(data_type == 0x01) {
        unsigned char image_header[5];
        if(!fseek(gcore, header_len - 5, SEEK_SET) &&
           5 != fread(image_header, sizeof(unsigned char), 5, gcore)) {
            fclose(gcore);
            LOGE("Can not read Image header.\n");
            return 5;
        }

        img->data = gcore;
        img->src = GIMG_FROM_GCORE_FILE;
        img->data_offset = header_len;
        img->type = image_header[0];
        img->width = ((int16_t)image_header[1]) << 8 | ((int16_t)image_header[2]);
        img->height = ((int16_t)image_header[3]) << 8 | ((int16_t)image_header[4]);
        LOGI("Width: %d Height: %d\n", img->width, img->height);
        return 0;
    }

    return 127;
}
/*
GImage* gimg_create_frombuffer(unsigned char* buffer) {

}
*/

uint32_t gimg_bytelen(const GImage *img) {
    if(!img)
        return 0;

    switch (img->type) {
        case GIMG_Grayscale8: return img->width * img->height;
        default: return 0;
    }
}

char gimg_gval_idx(const GImage* img, unsigned char *gval, uint32_t idx) {
    if(!img) return -1;
    if(!img->data) return -2;
    if(!gval) return -3;
    switch (img->type) {
    case GIMG_Grayscale8: {
        if(img->src == GIMG_FROM_GCORE_FILE) {
            if(!fseek((FILE*)(img->data), idx + img->data_offset, SEEK_SET)) {
                if(fread(gval, sizeof(unsigned char), 1, (FILE*)(img->data))) {
                    return 0;
                }
                else {
                    LOGE("Error %d\n", ferror((FILE*)(img->data)));
                    printf("idx: %u\n", idx);
                    return -4;
                }
            }
            else {
                return -5;
            }
        }
        else {
            *gval = ((unsigned char*)img->data)[idx];
            return 0;
        }
    } break;
    default: return -6;
    }
    return -7;
}

char gimg_gval_pos(const GImage* img, unsigned char *gval, ImgPoint pos) {
    if(!img) return -1;
    return gimg_gval_idx(img, gval, pos.x + pos.y * img->width);
}

char gimg_gval_xy(const GImage* img, unsigned char *gval, int16_t x, int16_t y) {
    if(!img) return -1;
    return gimg_gval_idx(img, gval, x + y * img->width);
}

