#ifndef _GCORE_IMG_PROCESS_H_
#define _GCORE_IMG_PROCESS_H_

int imgproc_rect_cut(
    unsigned char** dst_data,
    const unsigned char* src_data, int src_width, int src_heigth,
    int rect_x, int rect_y, int rect_width, int rect_height
);

int imgproc_find_valid_bounding_box(
    unsigned char* img_data, int img_width, int img_height, int threshold,
    int *rect_x, int *rect_y, int *rect_width, int *rect_height
);

int imgproc_apply_alpha(
    unsigned char** dst_data,
    const unsigned char* src_data, int src_width, int src_height);


#endif // _GCORE_IMG_PROCESS_H_
