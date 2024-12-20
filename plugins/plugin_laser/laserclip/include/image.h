#ifndef GCOGEN_IMAGE_H
#define GCOGEN_IMAGE_H

#include "./type.h"

;
#pragma pack(push, 4)

typedef enum GImageType {
    GIMG_None = 0x00,
    GIMG_MonoMSB = 0x01,
    GIMG_MonoLSB = 0x02,
    GIMG_Grayscale8 = 0x03,
    GIMG_GrayScale16 = 0x04,
    GIMG_Grayscale888 = 0x05,
    GIMG_LastType
} GImageType;

typedef enum SOURCE_TYPE {
    GIMG_FROM_IMAGE_FILE,   // data ==> unsigned char* buff(image buffer)
    GIMG_FROM_GCORE_FILE,   // data ==> struct FILE* file(*.gcore)
    GIMG_FROM_MALLOC        // data ==> malloc()
} SOURCE_TYPE;

typedef struct GImage {
    void *data;
    GImageType type;
    SOURCE_TYPE src;
    uint16_t data_offset;
    int16_t width;
    int16_t height;
    int16_t cut_offset_x;
    int16_t cut_offset_y;
} GImage;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

char gimg_init(GImage *gimg, const void* src, int32_t len, SOURCE_TYPE src_type);
#ifdef BUILD_LIBRARY_GCORE_GEN
char gimg_init_from_argb8888(GImage *gimg, const uint8_t* argb, int32_t width, int32_t height);
#endif
void gimg_delete(GImage *img);
uint32_t gimg_bytelen(const GImage* img);
char gimg_gval_idx(const GImage* img, unsigned char *gval, uint32_t idx);
char gimg_gval_pos(const GImage* img, unsigned char *gval, ImgPoint pos);
char gimg_gval_xy(const GImage* img, unsigned char *gval, int16_t x, int16_t y);

#ifdef DEBUG
void gimg_print(const GImage* img);
#endif

#ifdef __cplusplus
}
#endif

#endif // GCOGEN_IMAGE_H
