#include "header.h"
#include "image.h"
#include "image_iterator.h"
#include "type.h"
#include "gen.h"
#include "parser.h"
#include "pro_conf.h"
#include <string.h>
#if DEBUG_BUILD && DEBUG
#include "../debug/debug_print.h"
#endif // BUILD && DEBUG_BUILD

char gcore_header_write(struct GCoreConfig *gconfig, GImage *gimg, unsigned char* buffer) {
    if(!gconfig || !gimg || !buffer) return -1;
    buffer[0] = (FIRMWARE_VERSION_NUM & 0xff000000U) >> 24;
    buffer[1] = (FIRMWARE_VERSION_NUM & 0x00ff0000U) >> 16;
    buffer[2] = (FIRMWARE_VERSION_NUM & 0x0000ff00U) >> 8;
    buffer[3] = (FIRMWARE_VERSION_NUM & 0x000000ffU);
    buffer[4] = (GCORE_HEADER_LENGTH & 0x0000ff00U) >> 8;
    buffer[5] = (GCORE_HEADER_LENGTH & 0x000000ffU);
    buffer[6] = 0x01; // GCore Data Type

    memcpy(&(buffer[16]), gconfig, sizeof(GCoreConfig));
    buffer[GCORE_HEADER_LENGTH - 5] = (gimg->type);
    buffer[GCORE_HEADER_LENGTH - 4] = (gimg->width  & 0xff00) >> 8;
    buffer[GCORE_HEADER_LENGTH - 3] = (gimg->width  & 0x00ff);
    buffer[GCORE_HEADER_LENGTH - 2] = (gimg->height & 0xff00) >> 8;
    buffer[GCORE_HEADER_LENGTH - 1] = (gimg->height & 0x00ff);
    return 0;
}


char _gcore_header_read_v01(struct GCoreParser *gparser, unsigned char* buffer);
char gcore_header_read(struct GCoreParser *gparser, unsigned char* buffer) {
    if(!gparser || !buffer) return -1;
    gparser->fmware =
            ((uint32_t)buffer[0]) << 24 |
            ((uint32_t)buffer[1]) << 16 |
            ((uint32_t)buffer[2]) << 8 |
            ((uint32_t)buffer[3]);
    gparser->type = buffer[6];
    switch (gparser->fmware) {
    case 2109000100U: return _gcore_header_read_v01(gparser, buffer + 16);
    default: return 1;
    }
    return 2;
}

char _gcore_header_read_v01(struct GCoreParser *gparser, unsigned char* buffer) {
    memcpy(&(gparser->cfg), buffer, sizeof(GCoreConfig));
    return 0;
}
