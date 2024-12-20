#include "img_process.h"
#include <stdlib.h>
#include <string.h>


int imgproc_rect_cut(
    unsigned char** dst_data,
    const unsigned char* src_data, int src_width, int src_heigth,
    int rect_x, int rect_y, int rect_width, int rect_height
    ) {

    if(dst_data == NULL ||
       src_data == NULL ||
       src_width <= 0 ||
       src_heigth <= 0 ||
       rect_x < 0 ||
       rect_y < 0 ||
       rect_width <= 0 ||
       rect_height <= 0) {
        return -1;
    }

    *dst_data = (unsigned char*)malloc(rect_width * rect_height);
    if(*dst_data == NULL) {
        return -2;
    }
    for(int y = 0; y < rect_height; ++y) {
        int src_y = rect_y + y;
        int src_x = rect_x;
        memcpy(*dst_data + rect_width * y,
               src_data + src_width * src_y + src_x,
               rect_width);
    }

    return 0;
}

int imgproc_apply_alpha(
    unsigned char** dst_data,
    const unsigned char* src_data, int src_width, int src_height
    ) {

    if(dst_data == NULL ||
       src_data == NULL ||
       src_width <= 0 ||
       src_height <= 0) {
        return -1;
    }

    *dst_data = (unsigned char*)malloc(src_width * src_height);
    if(*dst_data == NULL) {
        return -2;
    }
    for(int y = 0; y < src_height; ++y) {
        unsigned char* dst_line_start = *dst_data + y * src_width;
        const unsigned char* src_line_start = src_data + y * (src_width * 2);
        for(int x = 0; x < src_width; ++x) {
            const unsigned char *src_curr = (src_line_start + x * 2);
            float alpha = *(src_curr + 1) / 255.f;
            *(dst_line_start + x) = 255 - ((int)((255 - *src_curr) * alpha));
        }
    }

    return 0;
}

int imgproc_find_valid_bounding_box(
    unsigned char* img_data, int img_width, int img_height, int threshold,
    int *rect_x, int *rect_y, int *rect_width, int *rect_height
) {
    if(img_data == NULL ||
       img_width < 0 ||
       img_height < 0 ||
       threshold < 0 ||
       threshold > 255 ||
       rect_x == NULL ||
       rect_y == NULL ||
       rect_width == NULL ||
       rect_height == NULL
       ) {
        return -1;
    }

    int min_x = img_width;
    int min_y = img_height;
    int max_x = 0;
    int max_y = 0;

    for(int y = 0; y < img_height; ++y) {
        unsigned char* line_start = img_data + y * img_width;
        for(int x = 0; x < img_width; ++x) {
            unsigned char* curr = line_start + x;
            if(*curr <= threshold) {
                if(x < min_x) min_x = x;
                if(x > max_x) max_x = x;
                if(y < min_y) min_y = y;
                if(y > max_y) max_y = y;
            }
            else {
                *curr = 255;
            }
        }
    }

    if(min_x > max_x || min_y > max_y) {
        *rect_x = *rect_y = *rect_width = *rect_height = 0;
    }
    else {
        *rect_x = min_x;
        *rect_y = min_y;
        *rect_width = max_x - min_x + 1;
        *rect_height = max_y - min_y + 1;
    }

    return 0;
}
