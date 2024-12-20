/** Copyright (C) 2016 Ultimaker - Released under terms of the AGPLv3 License */
#ifndef CX_UTILS_MATH_H
#define CX_UTILS_MATH_H

#include <cmath>

//c++11 no longer defines M_PI, so add our own constant.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ONE_OVER_SQRT_2 0.7071067811865475244008443621048490392848359376884740 //1 / sqrt(2)
#define ONE_OVER_SQRT_3 0.577350269189625764509148780501957455647601751270126876018 //1 / sqrt(3)
#define ONE_OVER_SQRT_6 0.408248290463863016366214012450981898660991246776111688072 //1 / sqrt(6)
#define SQRT_TWO_THIRD 0.816496580927726032732428024901963797321982493552223376144 //sqrt(2 / 3)
namespace topomesh
{

    static constexpr float sqrt2 = 1.41421356237f;

    template<typename T> inline T square(const T& a) { return a * a; }

    inline unsigned int round_divide(unsigned int dividend, unsigned int divisor) //!< Return dividend divided by divisor rounded to the nearest integer
    {
        return (dividend + divisor / 2) / divisor;
    }
    inline unsigned int round_up_divide(unsigned int dividend, unsigned int divisor) //!< Return dividend divided by divisor rounded to the nearest integer
    {
        return (dividend + divisor - 1) / divisor;
    }

}//namespace cxutil
#endif // CX_UTILS_MATH_H

