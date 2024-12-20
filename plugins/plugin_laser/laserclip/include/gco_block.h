#ifndef GCOGEN_GCO_BLOCK_H
#define GCOGEN_GCO_BLOCK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "gco_word.h"
;
#pragma pack(push, 4)

typedef struct GcoWordArray {
    int16_t len;
    int16_t _real_len;
    GcoWord *data;
} GcoWordArray;

typedef struct GcoBlock {
    struct GcoBlock* prev;
    struct GcoBlock* next;
    struct GcoWordArray arr;
} GcoBlock;

GcoBlock* gcoblk_create();
void gcoblk_delete(GcoBlock **blk);
char gcoblk_append(GcoBlock *blk, GcoWord word);

typedef struct GcoBlockList {
    GcoBlock *head;
    GcoBlock *tail;
    uint32_t len;
} GcoBlockList;

GcoBlockList* gcoblklist_create();
void gcoblklist_delete(GcoBlockList** list);
void gcoblklist_deep_delete(GcoBlockList** list);
char gcoblklist_pop_front(GcoBlockList* list, GcoBlock** line);
void gcoblklist_append(GcoBlockList* list, GcoBlock* line);
GcoBlock* gcoblklist_idx(GcoBlockList* list, uint32_t idx);
bool gcoblklist_contain(GcoBlockList* list, GcoBlock *line);
size_t gcoblklist_memsize(const GcoBlockList* list);

#pragma pack(pop)


#endif // GCOGEN_GCO_BLOCK_H
