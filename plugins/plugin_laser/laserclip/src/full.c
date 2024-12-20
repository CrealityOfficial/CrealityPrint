#include "pro_conf.h"
#include "include/gcore.h"
#include "gco_word.h"
#include <string.h>
#include <stdio.h>

typedef char TempGcoreName[32];
void get_temp_name(TempGcoreName *name);

EXPORT_API char gcore_image_to_gcode(
    struct GCoreConfig *cfg,
    const char* image_file,
    const char* gcode_file
        ) {

    char ret = 0;
    // 输入检查
    if((!image_file && (ret = -1)) ||
       (!gcode_file && (ret = -2)) ||
       (!cfg && (ret = -3)) ) {
        LOGE("Invalid Args, get ret %d\n", ret);
        return -1;
    }

    TempGcoreName gcore_name;
    get_temp_name(&gcore_name);
    if((ret = gcore_generate(image_file, gcore_name, cfg))) {
        return ret;
    }

    GCoreParser parser;
    memset(&parser, 0, sizeof(parser));
    if((ret = gcore_parser_init(&parser, gcore_name))) {
        return ret;
    }

    if((ret = gcore_parser_save(&parser, gcode_file))) {
        return ret;
    }

    gcore_parser_deinit(&parser);
    remove(gcore_name);

    return 0;
}

void get_temp_name(TempGcoreName *name) {
    // 生产随机Gcore名字
    void* rand_addr[4];
    rand_addr[0] = name;
    rand_addr[1] = &name;
    rand_addr[2] = rand_addr;
    rand_addr[3] = &rand_addr;
    for(int i = 0; i < sizeof(rand_addr); ++i) {
        (*name)[i] = *(((uint8_t*)(rand_addr)) + i);
    }


    for(int i = 0; i < sizeof(TempGcoreName); ++i) {
        (*name)[i] = gcowd_char(((uint8_t)(*name)[i]) % 26 + 1);
    }

    (*name)[sizeof(TempGcoreName) - 1] = '\0';
}
