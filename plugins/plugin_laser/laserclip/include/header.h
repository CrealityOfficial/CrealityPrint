#ifndef _GCORE_HEADER_H_
#define _GCORE_HEADER_H_

#include "./type.h"

#define GCORE_HEADER_LENGTH     (unsigned int)(16 + (unsigned int)sizeof(struct GCoreConfig) + 5)

struct GCoreParser;
struct GImage;
char gcore_header_write(struct GCoreConfig *gconfig, struct GImage *gimg, unsigned char* buffer);
char gcore_header_read(struct GCoreParser *gparser, unsigned char* buffer);

#endif //  _GCORE_HEADER_H_
