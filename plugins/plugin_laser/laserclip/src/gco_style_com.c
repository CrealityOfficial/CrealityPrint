#include "gco_style_com.h"

uint16_t int_sqrt32(uint32_t x)
{
    uint16_t res=0;
    uint16_t add= 0x8000;
    int32_t i;
    for(i=0;i<16;i++)
    {
        uint16_t temp=res | add;
        int32_t g2=temp*temp;
        if (x>=g2)
        {
            res=temp;
        }
        add>>=1;
    }
    return res;
}
