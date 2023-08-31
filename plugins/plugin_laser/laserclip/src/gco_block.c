#include "gco_block.h"
#ifdef __APPLE__
//#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

GcoBlock* gcoblk_create() {
    GcoBlock *blk = (GcoBlock*) malloc(sizeof(GcoBlock));
    if(!blk)
        return NULL;

    blk->arr.len = 0;
    blk->arr._real_len = 4;
    blk->arr.data = (GcoWord*)malloc(sizeof(GcoWord) * blk->arr._real_len);
    blk->prev = NULL;
    blk->next = NULL;
    return blk;
}

void gcoblk_delete(GcoBlock **blk) {
    if(!blk || !*blk)
        return;

    if((*blk)->arr.data) {
       free((*blk)->arr.data);
    }
    free(*blk);
    *blk = NULL;
}

char gcoblk_append(GcoBlock *blk, GcoWord word) {
    if(!blk || blk->arr.len == UCHAR_MAX) return -1;

    if(blk->arr.len + 1 > blk->arr._real_len) {
        GcoWord *array = (GcoWord*)malloc(sizeof(GcoWord) * (blk->arr._real_len + 4));
        if(!array)
            return -2;

        memcpy(array, blk->arr.data, sizeof(GcoWord) * blk->arr.len);
        free(blk->arr.data);
        blk->arr.data = array;
        blk->arr._real_len += 4;
    }

    blk->arr.data[blk->arr.len] = word;
    blk->arr.len += 1;
    return 0;
}


GcoBlockList* gcoblklist_create() {
    GcoBlockList *blklist = (GcoBlockList*) malloc(sizeof(GcoBlockList));
    if(!blklist)
        return NULL;

    blklist->len = 0;
    blklist->head = NULL;
    blklist->tail = NULL;
    return blklist;
}

void gcoblklist_delete(GcoBlockList** blklist) {
    if(*blklist) {
        free(*blklist);
        *blklist = NULL;
    }
}

void gcoblklist_deep_delete(GcoBlockList** blklist) {
    if(*blklist) {
        GcoBlock *blk  = (*blklist)->head;
        while(blk) {
            GcoBlock *next = blk->next;
            gcoblk_delete(&blk);
            if(next) {
                blk = next;
            }
        }
        gcoblklist_delete(blklist);
    }
}


char gcoblklist_pop_front(GcoBlockList* blklist, GcoBlock** blk) {
    if(!blklist || !blk)
        return -1;

    if(blklist->len == 0) {
        return -2;
    }

    if(blklist->head == NULL) {
        return -3;
    }

    *blk = blklist->head;
    blklist->head = (*blk)->next;
    (*blk)->next = NULL;
    --blklist->len;
    return 0;
}

void gcoblklist_append(GcoBlockList* blklist, GcoBlock* blk) {
    if(!blklist || !blk || gcoblklist_contain(blklist, blk))
        return;

    if(blklist->len == 0) {
        blk->next = blk->prev = NULL;
        blklist->head = blklist->tail = blk;
    }
    else {
        blklist->tail->next = blk;
        blk->prev = blklist->tail;
        blk->next = NULL;
        blklist->tail = blklist->tail->next;
    }
    blklist->len += 1;
}

GcoBlock* gcoblklist_idx(GcoBlockList* blklist, uint32_t idx) {
    if(!blklist || idx >= blklist->len ) return NULL;

    if(idx > blklist->len / 2) {
        uint32_t i = blklist->len - 1;
        GcoBlock *ptr = blklist->tail;
        while(ptr) {
            if(i == idx) return ptr;
            --i;
            ptr = ptr->prev;
        }

    }
    else {
        uint32_t i = 0;
        GcoBlock *ptr = blklist->head;
        while(ptr) {
           if(i == idx) return ptr;
           ++i;
           ptr = ptr->next;
        }
    }
    return NULL;
}

bool gcoblklist_contain(GcoBlockList *blklist, GcoBlock *blk) {
    if(!blklist || !blk || !blklist->len) return false;

    GcoBlock *ptr = blklist->head;
    do {
        if(ptr == blk)
            return true;
        ptr = ptr->next;
    } while(ptr);
    return false;
}

size_t gcoblklist_memsize(const GcoBlockList* list) {
    if(!list)
        return 0;

    size_t size = 0;
    size += sizeof(GcoBlockList);
    size += sizeof(GcoBlock) * list->len;
    GcoBlock *ptr = list->head;
    while(ptr) {
        size += ptr->arr.len * sizeof(GcoWord);
        ptr = ptr->next;
    }

    return size;
}
