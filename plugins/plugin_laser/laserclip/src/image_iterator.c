#ifndef IMAGE_ITERATOR_C
#define IMAGE_ITERATOR_C

#ifdef __APPLE__
//#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <stdlib.h>
#include "include/image_iterator.h"
//#include "image.h"
#include "pro_conf.h"

static inline int _get_y(GIterator *i, int x) {
    return ((-(i->A) * x) - i->C) / i->B;
}
static inline int _get_x(GIterator *i, int y) {
    return ((-(i->B) * y) - i->C) / i->A;
}

typedef struct IntersectionPoint {
    ImgPoint p[2];
} IntersectionPoint;

int _get_intersection(GIterator *i, struct IntersectionPoint* ips) {
    if(!i) return -1;
    if(!ips) return -2;

    int idx = 0;
    ImgPoint p;
    ips->p[0].x = ips->p[0].y =
    ips->p[1].x = ips->p[1].y = -1;
    if(i->B != 0) {
        p.x = 0;
        p.y = _get_y(i, p.x);
        if(p.y >= 0 && p.y < i->gimg.height) {
            ips->p[idx++] = p;
        }

        p.x = i->gimg.width - 1;
        p.y = _get_y(i, p.x);
        if(p.y >= 0 && p.y < i->gimg.height) {
            ips->p[idx++] = p;
        }
    }


    if(i->A != 0) {
        if(idx == 2) return 0; // 防止在角点处重复计算二超过2个交点

        p.y = 0;
        p.x = _get_x(i, p.y);
        if(p.x >= 0 && p.x < i->gimg.width) {
            ips->p[idx++] = p;
        }

        if(idx == 2) return 0; // 防止在角点处重复计算二超过2个交点
        p.y = i->gimg.height - 1;
        p.x = _get_x(i, p.y);
        if(p.x >= 0 && p.x < i->gimg.width) {
            ips->p[idx++] = p;
        }
    }

    if(idx == 0)
        return -3;

    return 0;
}
typedef enum DIRECTION {DIRE_FORWARD, DIRE_BACKWARD} DIRECTION;
int _next(GIterator *i, ImgPoint cur, DIRECTION dir) {
    if(!i) return -1;

    cur.x += (dir == DIRE_FORWARD ? i->x_unit : -i->x_unit);
    cur.y += (dir == DIRE_FORWARD ? i->y_unit : -i->y_unit);
    if(i->x_unit) {
        cur.y = _get_y(i, cur.x);
    }
    else if(i->y_unit) {
        cur.x = _get_x(i, cur.y);
    }
    else {
        return -2;
    }

    if(cur.x < 0 || cur.y < 0 || cur.x >= i->gimg.width || cur.y >= i->gimg.height) {
        return -3;
    }

    i->curr = cur;
    return 0;
}

int _move(GIterator *i, MoveAction act) {
    if(!i) return -1;

    if(act == CURR_LINE_NEXT) {
        if(_next(i, i->curr, DIRE_FORWARD)) {
            return -2;
        }
        return 0;
    }
    else if(act == CURR_LINE_PREV) {
        if(_next(i, i->curr, DIRE_BACKWARD)) {
            return -3;
        }
        return 0;
    }
    else if(act == TO_THE_BEGIN) {
        int idx = GCORE_MAX_SUPPORTED_IMG_W + GCORE_MAX_SUPPORTED_IMG_H;
        while(!_move(i, PREV_LINE_FIRST) && idx) idx--;
        i->zigzag_idx = 0;
        i->ln = 0;
        return 0;
    }
    else if(act == TO_THE_END) {
        int idx = GCORE_MAX_SUPPORTED_IMG_W + GCORE_MAX_SUPPORTED_IMG_H;
        while(!_move(i, NEXT_LINE_LAST) && idx) idx--;
        i->zigzag_idx = i->pix_cnt - 1;
        i->ln = i->ln_cnt - 1;
        return 0;
    }

    struct IntersectionPoint ips;
    if(act == NEXT_LINE_FIRST || act == NEXT_LINE_LAST) {
        i->C += i->c_unit;
        if(_get_intersection(i, &ips)) {
            i->C -= i->c_unit;
            return -4;
        }
    }
    else if(act == PREV_LINE_FIRST || act == PREV_LINE_LAST) {
        i->C -= i->c_unit;
        if(_get_intersection(i, &ips)) {
            i->C += i->c_unit;
            return -5;
        }
    }
    else {
        if(_get_intersection(i, &ips)) {
            return -6;
        }
    }

    if(i->x_unit) {
        if(i->x_unit > 0) {
            if(act == CURR_LINE_FIRST || act == NEXT_LINE_FIRST || act == PREV_LINE_FIRST) {
                i->curr = (ips.p[0].x > ips.p[1].x ? ips.p[1] : ips.p[0]);
                // TODO: update pixel idx;
            }
            else {
                i->curr = (ips.p[0].x > ips.p[1].x ? ips.p[0] : ips.p[1]);
                // TODO: update pixel idx;
            }
        }
        else {
            if(act == CURR_LINE_FIRST || act == NEXT_LINE_FIRST || act == PREV_LINE_FIRST) {
                i->curr = (ips.p[0].x > ips.p[1].x ? ips.p[0] : ips.p[1]);
                // TODO: update pixel idx;
            }
            else {
                i->curr = (ips.p[0].x > ips.p[1].x ? ips.p[1] : ips.p[0]);
                // TODO: update pixel idx;
            }
        }
    }
    else {
        if(i->y_unit > 0) {
            if(act == CURR_LINE_FIRST || act == NEXT_LINE_FIRST || act == PREV_LINE_FIRST) {
                i->curr = (ips.p[0].y > ips.p[1].y ? ips.p[1] : ips.p[0]);
                // TODO: update pixel idx;
            }
            else {
                i->curr = (ips.p[0].y > ips.p[1].y ? ips.p[0] : ips.p[1]);
                // TODO: update pixel idx;
            }
        }
        else {
            if(act == CURR_LINE_FIRST || act == NEXT_LINE_FIRST || act == PREV_LINE_FIRST) {
                i->curr = (ips.p[0].y > ips.p[1].y ? ips.p[0] : ips.p[1]);
                // TODO: update pixel idx;
            }
            else {
                i->curr = (ips.p[0].y > ips.p[1].y ? ips.p[1] : ips.p[0]);
                // TODO: update pixel idx;
            }

        }
    }

    if(act == NEXT_LINE_FIRST || act == NEXT_LINE_LAST) ++(i->ln);
    if(act == PREV_LINE_FIRST || act == PREV_LINE_LAST) --(i->ln);

    return 0;
}



char iterator_init(GIterator *iterator, GImage* gimg, CarveStartPosition start, CarveDirection dire) {
    if(!gimg || !iterator)  return -1;

    iterator->gimg = *gimg;
    if(gimg->type == GIMG_Grayscale8) {
        iterator->pix_cnt = gimg->width * gimg->height;
    }
    else {
        LOGE("Unspported Image Type(%d)\n", gimg->type);
        return -2;
    }
    iterator_reset(iterator, start, dire);
    return 0;
}


char iterator_reset(GIterator* i, CarveStartPosition start, CarveDirection dire) {
    if(!i) return -1;

    i->zigzag_idx = 0;
    i->ln = 0;
    if(dire == DiagonalLeft || dire == DiagonalRight) {
        i->ln_cnt = i->gimg.width + i->gimg.height - 1;
    }

    switch (start) {
    case TopLeft: {
        i->curr.x = i->curr.y = 0;
        switch (dire) {
        case StraightRight: i->ln_cnt = i->gimg.height;  i->A = 0; i->B = 1; i->C = 0; i->x_unit = 1; i->y_unit = 0; i->c_unit = -1;  break;
        case StraightLeft:  i->ln_cnt = i->gimg.width; i->A = 1; i->B = 0; i->C = 0; i->x_unit = 0; i->y_unit = 1; i->c_unit = -1;  break;
        case DiagonalRight: i->A = 1; i->B = 1; i->C = 0; i->x_unit = 1; i->y_unit = 0; i->c_unit = -1;  break;
        case DiagonalLeft:  i->A = 1; i->B = 1; i->C = 0; i->x_unit = -1; i->y_unit = 0; i->c_unit = -1; break;
        }
    } break;
    case TopRight: {
        i->curr.x = i->gimg.width - 1;
        i->curr.y = 0;
        switch (dire) {
        case StraightRight:
        case StraightLeft:
        case DiagonalRight:
        case DiagonalLeft: break;
        }
    } break;
    case BottomLeft: {
        i->curr.x = 0;
        i->curr.y = i->gimg.height - 1;
        switch (dire) {
        case StraightRight: i->ln_cnt = i->gimg.width; i->A = 1; i->B = 0; i->C = 0; i->x_unit = 0; i->y_unit = -1; i->c_unit = -1; break;
        case StraightLeft:  i->ln_cnt = i->gimg.height;i->A = 0; i->B = 1; i->C = -(i->gimg.height); i->x_unit = 1; i->y_unit = 0; i->c_unit = 1; break;
        case DiagonalRight:
        case DiagonalLeft: break;
        }
    } break;
    case BottomRight: {
        i->curr.x = i->gimg.width - 1;
        i->curr.y = i->gimg.height - 1;
        switch (dire) {
        case StraightRight:
        case StraightLeft:
        case DiagonalRight:
        case DiagonalLeft: break;
        }
    } break;
    }
    return 0;
}

void iterator_delete(GIterator *i) {
    if(i) gimg_delete(&i->gimg);
}

char iterator_move(GIterator *i, MoveAction act) {
    return _move(i, act);
}

char iterator_zigzag_next(GIterator *i) {
    if(!i) return -1;

    MoveAction act;
    int ret;

    if(i->C % 2) {
        act = CURR_LINE_PREV;
    } else {
        act = CURR_LINE_NEXT;
    }

    ret = iterator_move(i, act);

    if(ret) {
        ret = iterator_move(i, act == CURR_LINE_NEXT ? NEXT_LINE_LAST : NEXT_LINE_FIRST);
    }

    if(!ret) {
        ++(i->zigzag_idx);
    }

    return ret;
}


char iterator_curr_gcore_pixel(GIterator *i, PixelData* p) {
    if(!i) return -1;
    if(!p) return -2;

    p->pos = i->curr;
    return gimg_gval_idx(&(i->gimg), &(p->grayval), i->zigzag_idx);
}

char iterator_curr_image_pixel(GIterator *i, PixelData* p) {
    if(!i) return -1;
    if(!p) return -2;

    p->pos = i->curr;
    return gimg_gval_pos(&(i->gimg), &(p->grayval), p->pos);
}


#endif // IMAGE_ITERATOR_C
