#include "gco_word.h"
#include <stdio.h>

GcoWord GcoG1 = (((uint8_t)(GcoG) | (WORD_VAL_1 << 5)) & 0x7fffffff);
GcoWord GcoG0 = (((uint8_t)(GcoG) | (WORD_VAL_0 << 5)) & 0x7fffffff);

extern inline GcoValue gcowd_value(GcoWord wd);
extern inline char gcowd_char(GcoWord wd);

const char* numstr = "0123456789";

int gcowd_sprintf(char *buff, GcoWord wd, int decmals) {
    GcoValue val = gcowd_value(wd);
    int idx = 0;
    int remain;
    int has_num = 0;
    buff[idx++] = gcowd_char(wd);

    if((val & 0x7fffffff) == 0x0) {
        buff[idx++] = '0';
        buff[idx] = '\0';
        return idx;
    }

    // 打印符号
    if(val < 0) {
        buff[idx++] = '-';
        val &= 0x7fffffff;
    }

    if(val >= WORD_VAL_MAGNI) {
        if(val >= ((WORD_VAL_MAGNI) * 1000)) {
            remain = val / ((WORD_VAL_MAGNI) * 1000);
            buff[idx++] = numstr[remain];
            val -= remain * ((WORD_VAL_MAGNI) * 1000);
            has_num = 1;
        }

        if(val >= ((WORD_VAL_MAGNI) * 100)) {
            remain = val / ((WORD_VAL_MAGNI) * 100);
            buff[idx++] = numstr[remain];
            val -= remain * ((WORD_VAL_MAGNI) * 100);
            has_num = 1;
        }
        else if(has_num) {
            buff[idx++] = '0';
        }

        if(val >= ((WORD_VAL_MAGNI) * 10)) {
            remain = val / ((WORD_VAL_MAGNI) * 10);
            buff[idx++] = numstr[remain];
            val -= remain * ((WORD_VAL_MAGNI) * 10);
        }
        else if(has_num) {
            buff[idx++] = '0';
        }

        remain = val / (WORD_VAL_MAGNI);
        buff[idx++] = numstr[remain];
        val -= remain * (WORD_VAL_MAGNI);
    }
    else {
        buff[idx++] = '0';
    }

    do {
        if(decmals <= 0 || decmals > 4 || !val) {
            break;
        }

        buff[idx++] = '.';
        if(val >= ((WORD_VAL_MAGNI) / 10)) {
            remain = val / ((WORD_VAL_MAGNI) / 10);
            buff[idx++] = numstr[remain];
            val -= remain * ((WORD_VAL_MAGNI) / 10);
        }
        else {
            buff[idx++] = '0';
        }
        --decmals;

        if(decmals--) {
            if(val >= ((WORD_VAL_MAGNI) / 100)) {
                remain = val / ((WORD_VAL_MAGNI) / 100);
                buff[idx++] = numstr[remain];
                val -= remain * ((WORD_VAL_MAGNI) / 100);
            }
            else {
                buff[idx++] = '0';
            }
        }
        else {
            break;
        }

        if(decmals--) {
            if(val >= ((WORD_VAL_MAGNI) / 1000)) {
                remain = val / ((WORD_VAL_MAGNI) / 1000);
                buff[idx++] = numstr[remain];
                val -= remain * ((WORD_VAL_MAGNI) / 1000);
            }
            else {
                buff[idx++] = '0';
            }
        }
        else {
            break;
        }

        if(decmals--) {
            buff[idx++] = numstr[val];
        }
    } while(0);

    buff[idx] = '\0';
    return idx;
}
