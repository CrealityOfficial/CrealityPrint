#ifndef _GCORE_DEBUG_PRINT_H_
#define _GCORE_DEBUG_PRINT_H_

#include <stdbool.h>
#include "type.h"

struct GCoreParser;
struct GImage;
struct GIterator;
struct GCoreConfig;
struct GcoBlockList;
struct GcoBlock;

const char* get_model_str(MachineModel model);
const char* get_style_str(GCodeStyle style);
const char* get_start_str(CarveStartPosition start);
const char* get_dire_str(CarveDirection dire);
void gcowd_print(void* wd_ptr);
void parser_printf(const struct GCoreParser *parser);
void gimg_print(const struct GImage* img);
void iterator_print(const struct GIterator* i);
void config_print(const struct GCoreConfig *cfg);
void gcoblk_print(const struct GcoBlock *blk, bool en_addr);
void gcoblklist_print(const struct GcoBlockList* list, bool en_addr);

#endif // _GCORE_DEBUG_PRINT_H_
