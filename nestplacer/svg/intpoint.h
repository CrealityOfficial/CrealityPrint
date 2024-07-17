//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CX_INT_POINT_H
#define CX_INT_POINT_H

/**
The integer point classes are used as soon as possible and represent microns in 2D or 3D space.
Integer points are used to avoid floating point rounding errors, and because Clipper3r uses them.
*/
#define INLINE static inline

//Include Clipper to get the Clipper3r::IntPoint definition, which we reuse as Point definition.
#include "clipper3r/clipper.hpp"
#include <cmath>
#include <functional> // for hash function object
#include <iostream> // auto-serialization / auto-toString()
#include <limits>
#include <stdint.h>

#include "svg/point3.h" //For applying Point3Matrices.

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

namespace svg
{
    /* 64bit Points are used mostly throughout the code, these are the 2D points from Clipper3r */
    typedef Clipper3r::IntPoint Point;

    class IntPoint {
    public:
        int X, Y;
        Point p() { return Point(X, Y); }
    };
#define POINT_MIN std::numeric_limits<Clipper3r::cInt>::min()
#define POINT_MAX std::numeric_limits<Clipper3r::cInt>::max()

    static Point no_point(std::numeric_limits<Clipper3r::cInt>::min(), std::numeric_limits<Clipper3r::cInt>::min());

    /* Extra operators to make it easier to do math with the 64bit Point objects */
    INLINE Point operator-(const Point& p0) { return Point(-p0.X, -p0.Y); }
    INLINE Point operator+(const Point& p0, const Point& p1) { return Point(p0.X + p1.X, p0.Y + p1.Y); }
    INLINE Point operator-(const Point& p0, const Point& p1) { return Point(p0.X - p1.X, p0.Y - p1.Y); }
    INLINE Point operator*(const Point& p0, const coord_t i) { return Point(p0.X * i, p0.Y * i); }
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type> //Use only for numeric types.
    INLINE Point operator*(const Point& p0, const T i) { return Point(p0.X * i, p0.Y * i); }
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type> //Use only for numeric types.
    INLINE Point operator*(const T i, const Point& p0) { return p0 * i; }
    INLINE Point operator/(const Point& p0, const coord_t i) { return Point(p0.X / i, p0.Y / i); }
    INLINE Point operator/(const Point& p0, const Point& p1) { return Point(p0.X / p1.X, p0.Y / p1.Y); }

    INLINE Point& operator += (Point& p0, const Point& p1) { p0.X += p1.X; p0.Y += p1.Y; return p0; }
    INLINE Point& operator -= (Point& p0, const Point& p1) { p0.X -= p1.X; p0.Y -= p1.Y; return p0; }

    /* ***** NOTE *****
       TL;DR: DO NOT implement operators *= and /= because of the default values in Clipper3r::IntPoint's constructor.

       We DO NOT implement operators *= and /= because the class Point is essentially a Clipper3r::IntPoint and it has a
       constructor IntPoint(int x = 0, int y = 0), and this causes problems. If you implement *= as *=(int) and when you
       do "Point a = a * 5", you probably intend to do "a.x *= 5" and "a.y *= 5", but with that constructor, it will create
       an IntPoint(5, y = 0) and you end up with wrong results.
     */

     //INLINE bool operator==(const Point& p0, const Point& p1) { return p0.X==p1.X&&p0.Y==p1.Y; }
     //INLINE bool operator!=(const Point& p0, const Point& p1) { return p0.X!=p1.X||p0.Y!=p1.Y; }

    INLINE coord_t vSize2(const Point& p0)
    {
        return p0.X * p0.X + p0.Y * p0.Y;
    }
    INLINE float vSize2f(const Point& p0)
    {
        return float(p0.X) * float(p0.X) + float(p0.Y) * float(p0.Y);
    }

    INLINE bool shorterThen(const Point& p0, const coord_t len)
    {
        if (p0.X > len || p0.X < -len)
        {
            return false;
        }
        if (p0.Y > len || p0.Y < -len)
        {
            return false;
        }
        return vSize2(p0) <= len * len;
    }

    INLINE coord_t vSize(const Point& p0)
    {
        return sqrt(vSize2(p0));
    }

    INLINE Point normal(const Point& p0, coord_t len)
    {
        coord_t _len = vSize(p0);
        if (_len < 1)
            return Point(len, 0);
        return p0 * len / _len;
    }

    INLINE Point turn90CCW(const Point& p0)
    {
        return Point(-p0.Y, p0.X);
    }

    INLINE Point rotate(const Point& p0, double angle)
    {
        const double cos_component = std::cos(angle);
        const double sin_component = std::sin(angle);
        return Point(cos_component * p0.X - sin_component * p0.Y, sin_component * p0.X + cos_component * p0.Y);
    }

    INLINE coord_t dot(const Point& p0, const Point& p1)
    {
        return p0.X * p1.X + p0.Y * p1.Y;
    }

    INLINE int angle(const Point& p)
    {
        double angle = std::atan2(p.X, p.Y) / 3.1415926 * 180.0;
        if (angle < 0.0) angle += 360.0;
        return angle;
    }

    inline Point3 operator+(const Point3& p3, const Point& p2) {
        return Point3(p3.x + p2.X, p3.y + p2.Y, p3.z);
    }
    inline Point3& operator+=(Point3& p3, const Point& p2) {
        p3.x += p2.X;
        p3.y += p2.Y;
        return p3;
    }

    inline Point operator+(const Point& p2, const Point3& p3) {
        return Point(p3.x + p2.X, p3.y + p2.Y);
    }


    inline Point3 operator-(const Point3& p3, const Point& p2) {
        return Point3(p3.x - p2.X, p3.y - p2.Y, p3.z);
    }
    inline Point3& operator-=(Point3& p3, const Point& p2) {
        p3.x -= p2.X;
        p3.y -= p2.Y;
        return p3;
    }

    inline Point operator-(const Point& p2, const Point3& p3) {
        return Point(p2.X - p3.x, p2.Y - p3.y);
    }

}//namespace cxutil
#endif//CX_UTILS_INT_POINT_H

