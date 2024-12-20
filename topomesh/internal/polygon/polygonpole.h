#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include "polygon.h"
#include "ccglobal/tracer.h"

/*
https://blog.mapbox.com/a-new-algorithm-for-finding-a-visual-center-of-a-polygon-7c77e6492fbc
https://github.com/mapbox/polylabel/blob/master/include/mapbox/polylabel.hpp
点在多边形内部判断算法 https://www.cnblogs.com/charlee44/p/10704156.html
*/
namespace polygonPole {
    #ifndef EPS
    #define EPS 1E-8
    #endif // !EPS

    #ifndef SQRT2
    #define SQRT2 1.414213562373095
    #endif // !SQRT2
    enum class poleAlgo :int {
        QUARDTER_COVER = 0,///< 四分法
        REGIONAL_SAMPLE = 1,///< 条件采样
        POISSON_SAMPLE = 2,///< 泊松采样
        INSCRIBED_CIRCLE = 3,///< 最大内接圆
        AXIS_INTERSECTION = 4,///< 中轴线算法
    };
    
    template<typename T>
    constexpr inline T Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }
    template<typename T>
    constexpr inline T Max(const T& a, const T& b)
    {
        return a > b ? a : b;
    }
    template <typename T>
    class Point2D {
    public:
        T x, y;
        constexpr Point2D()
            : x(0), y(0)
        {
        }
        constexpr Point2D(T x_, T y_)
            : x(x_), y(y_)
        {
        }
        inline T Magnitude() const
        {
            return std::sqrt(x * x + y * y);
        }
        inline T Magnitude2() const
        {
            return x * x + y * y;
        }
        inline Point2D Unit() const
        {
            if (Magnitude() < EPS) return Point2D(0, 0);
            return Point2D(x, y) / Magnitude();
        }

        inline Point2D Normalized()
        {
            if (Magnitude() < EPS) return Point2D(0, 0);
            x /= Magnitude(), y /= Magnitude();
            return Point2D(x, y);
        }
    };
    template<typename T>
    inline Point2D<T> operator+(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return Point2D<T>(lhs.x + rhs.x, lhs.y + rhs.y);
    }
    template<typename T>
    inline Point2D<T> operator-(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return Point2D<T>(lhs.x - rhs.x, lhs.y - rhs.y);
    }
    template<typename T>
    inline Point2D<T> operator*(const Point2D<T>& lhs, const T& rhs)
    {
        return Point2D<T>(lhs.x * rhs, lhs.y * rhs);
    }
    template<typename T>
    inline Point2D<T> operator/(const Point2D<T>& lhs, const T& rhs)
    {
        return Point2D<T>(lhs.x / rhs, lhs.y / rhs);
    }
    template <typename T>
    inline bool operator==(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return (lhs - rhs).Magnitude() < EPS;
    }
    template <typename T>
    inline Point2D<T>& operator+=(Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
        return lhs;
    }
    template <typename T>
    inline Point2D<T>& operator+=(Point2D<T>& lhs, T const& rhs)
    {
        lhs.x += rhs;
        lhs.y += rhs;
        return lhs;
    }
    template <typename T>
    inline Point2D<T>& operator-=(Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        lhs.x -= rhs.x;
        lhs.y -= rhs.y;
        return lhs;
    }
    template <typename T>
    inline Point2D<T>& operator-=(Point2D<T>& lhs, const T& rhs)
    {
        lhs.x -= rhs;
        lhs.y -= rhs;
        return lhs;
    }
    template <typename T>
    inline Point2D<T> operator-(const Point2D<T>& lhs)
    {
        return Point2D<T>(-lhs.x, -lhs.y);
    }
    // AC=KCB,C在直线AB上
    template <typename T>
    inline Point2D<T> kEqualPoint(const Point2D<T>& lhs, const Point2D<T>& rhs, const T k)
    {
        return Point2D<T>(lhs.x + k * rhs.x, lhs.y + k * rhs.y) / (1 + k);
    }
    template <typename T>
    inline T Dot(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return lhs.x* rhs.x + lhs.y * rhs.y;
    }
    template<typename T>
    constexpr inline T Cross(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return lhs.x* rhs.y - lhs.y * rhs.x;
    }
    template<typename T>
    inline T GetDistance(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return (lhs - rhs).Magnitude();
    }
    template<typename T>
    inline T GetDistance2(const Point2D<T>& lhs, const Point2D<T>& rhs)
    {
        return (lhs - rhs).Magnitude2();
    }
    template<typename T>
    inline T GetDistance(const Point2D<T>& p, const Point2D<T>& a, const Point2D<T>& b)
    {
        Point2D<T> ab = b - a, ap = p - a;
        return std::fabs(Cross(ab, ap)) / ab.Magnitude();
    }
    template<typename T>
    inline T GetDistance2(const Point2D<T>& p, const Point2D<T>& a, const Point2D<T>& b)
    {
        Point2D<T> ab = b - a, ap = p - a;
        return std::pow(Cross(ab, ap), 2) / ab.Magnitude2();
    }
    //计算点距离线段最近的点
    template<typename T>
    inline Point2D<T> GetSegmentProject(const Point2D<T>& p, const Point2D<T>& a, const Point2D<T>& b)
    {
        Point2D<T> ab = b - a, ap = p - a;
        T k = Dot(ap, ab) / ab.Magnitude2();
        if (k < 0) return a;
        if (k > 1) return b;
        else return a * (1 - k) + b * k;
    }
    // 判断向量是否共线
    template<typename T>
    inline bool Collinear(const Point2D<T>& vec1, const Point2D<T>& vec2)
    {
        Point2D<T> dir1 = vec1.Unit();
        Point2D<T> dir2 = vec2.Unit();
        T area = std::fabs(Cross(dir1, dir2));
        return area < EPS;
    }
    // 判断三点是否共线
    template<typename T>
    inline bool Collinear(const Point2D<T>& p, const Point2D<T>& a, const Point2D<T>& b)
    {
        Point2D<T> dir1 = (p - a).Unit();
        Point2D<T> dir2 = (b - a).Unit();
        T area = std::fabs(Cross(dir1, dir2));
        return area < EPS;
    }
    template <typename T>
    class Segment {
    public:
        Point2D<T> a;
        Point2D<T> b;
        constexpr Segment()
        {
        }
        constexpr Segment(const Point2D<T>& a_, const Point2D<T>&b_)
            : a(a_), b(b_)
        {
        }
    };
    template<typename T>
    inline Point2D<T> GetSegmentProject(const Point2D<T>& p, const Segment<T>& seg)
    {
        Point2D<T> a = seg.a, b = seg.b;
        Point2D<T> ab = b - a, ap = p - a;
        T k = Dot(ap, ab) / ab.Magnitude2();
        if (k < 0) return a;
        if (k > 1) return b;
        else return a * (1 - k) + b * k;
    }
    template <typename T>
    struct BoundBox {
        constexpr BoundBox(){}
        constexpr BoundBox(const Point2D<T>& min_, const Point2D<T>& max_)
            : min(min_), max(max_)
        {
        }
        Point2D<T> min;
        Point2D<T> max;
    };

    template <typename T>
    class Ring {
    public:
        std::vector<Point2D<T>> points;
        constexpr Ring() {}
        constexpr Ring(const std::vector<Point2D<T>>& pts) :points(pts)
        {
        }
        void AddPoint(const Point2D<T>& pt)
        {
            points.push_back(pt);
        }
        void AddPoints(const std::vector<Point2D<T>>& pts)
        {
            points.insert(points.end(), pts.begin(), pts.end());
        }
        void Translate(const Point2D<T>& trans) {
            for (auto& pt : points) {
                pt -= trans;
            }
        }
        Point2D<T> GetCenter() const
        {
            Point2D<T> pt;
            int size = points.size();
            if (size == 0) return pt;
            for (auto& p : points) {
                pt += p;
            }
            return pt / (double)size;
        }
        BoundBox<T> GetBoundBox() const
        {
            using limits = std::numeric_limits<T>;
            T min_t = limits::has_infinity ? -limits::infinity() : limits::min();
            T max_t = limits::has_infinity ? limits::infinity() : limits::max();
            Point2D<T> min(max_t, max_t);
            Point2D<T> max(min_t, min_t);
            for (auto& point : points) {
                if (min.x > point.x) min.x = point.x;
                if (min.y > point.y) min.y = point.y;
                if (max.x < point.x) max.x = point.x;
                if (max.y < point.y) max.y = point.y;
            }
            return BoundBox<T>(min, max);
        }
        void Simplify()
        {
            const size_t len = points.size();
            std::vector<bool> marks(len, true);
            for (size_t i = 0, j = len - 1, k = 1; i < len; j = i++) {
                Point2D<T> prev = points[j];
                Point2D<T> cur = points[i];
                Point2D<T> next = points[k];
                if (Collinear(prev, cur, next)) {
                    marks[i] = false;
                }
                k = (k + 1) % len;
            }
            std::vector<Point2D<T>> swapPts;
            for (size_t i = 0; i < len; ++i) {
                if (marks[i]) {
                    swapPts.push_back(points[i]);
                }
            }
            points.swap(swapPts);
        }
        bool IsRectangle() const
        {
            size_t len = points.size();
            if (len == 4) {
                Point2D<T> ab = points[1] - points[0];
                Point2D<T> dc = points[2] - points[3];
                if (Collinear(ab, dc)) {
                    Point2D<T> ad = points[3] - points[0];
                    Point2D<T> bc = points[2] - points[1];
                    if (Collinear(ad, bc)) {
                        if (std::fabs(Dot(ab, ad)) < EPS) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    };
    //复杂多边形，可能带有孔洞
    template <typename T>
    class Polygon {
    private:
        Point2D<T> min, max;
    public:
        bool bGeneratedBound = false;
        std::vector<Ring<T>> rings;
        constexpr Polygon() {}
        constexpr Polygon(const std::vector<Ring<T>>& rs) :rings(rs)
        {
        }
        void Translate(const Point2D<T>& trans)
        {
            for (auto& ring : rings) {
                auto& points = ring.points;
                for (auto& pt : points) {
                    pt -= trans;
                }
            }
            bGeneratedBound = false;
        }
        Point2D<T> Min() const
        {
            if (bGeneratedBound) return min;
            using limits = std::numeric_limits<T>;
            T max_t = limits::has_infinity ? limits::infinity() : limits::max();
            Point2D<T> minPt(max_t, max_t);
            for (auto& ring : rings) {
                for (auto& point : ring.points) {
                    if (minPt.x > point.x) minPt.x = point.x;
                    if (minPt.y > point.y) minPt.y = point.y;
                }
            }
            return minPt;
        }
        Point2D<T> Max() const
        {
            if (bGeneratedBound) return max;
            using limits = std::numeric_limits<T>;
            T min_t = limits::has_infinity ? -limits::infinity() : limits::min();
            Point2D<T> maxPt(min_t, min_t);
            for (auto& ring : rings) {
                for (auto& point : ring.points) {
                    if (maxPt.x < point.x) maxPt.x = point.x;
                    if (maxPt.y < point.y) maxPt.y = point.y;
                }
            }
            return maxPt;
        }
        BoundBox<T> GetBoundBox() {
            if (bGeneratedBound) return BoundBox<T>(min, max);
            using limits = std::numeric_limits<T>;
            T min_t = limits::has_infinity ? -limits::infinity() : limits::min();
            T max_t = limits::has_infinity ? limits::infinity() : limits::max();
            min.x = max_t, min.y = max_t;
            max.x = min_t, max.y = min_t;
            for (auto& ring : rings) {
                for (auto& point : ring.points) {
                    if (min.x > point.x) min.x = point.x;
                    if (min.y > point.y) min.y = point.y;
                    if (max.x < point.x) max.x = point.x;
                    if (max.y < point.y) max.y = point.y;
                }
            }
            bGeneratedBound = true;
            return BoundBox<T>(min, max);
        }
        Point2D<T> GetBoundCenter()
        {
            if (bGeneratedBound) return (min + max) / 2.0;
            using limits = std::numeric_limits<T>;
            T min_t = limits::has_infinity ? -limits::infinity() : limits::min();
            T max_t = limits::has_infinity ? limits::infinity() : limits::max();
            min.x = max_t, min.y = max_t;
            max.x = min_t, max.y = min_t;
            for (auto& ring : rings) {
                for (auto& point : ring.points) {
                    if (min.x > point.x) min.x = point.x;
                    if (min.y > point.y) min.y = point.y;
                    if (max.x < point.x) max.x = point.x;
                    if (max.y < point.y) max.y = point.y;
                }
            }
            bGeneratedBound = true;
            return (min + max) / 2.0;
        }
        Point2D<T> GetMassCentroid() const{
            T area = 0;
            Point2D<T> c(0, 0);
            for (auto& ring : rings) {
                const auto& points = ring.points;
                const size_t len = points.size();
                for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                    const Point2D<T>& a = points[j];
                    const Point2D<T>& b = points[i];
                    auto f = Cross(a, b);
                    /*c.x += (a.x + b.x) * f;
                    c.y += (a.y + b.y) * f;*/
                    c += (a + b) * f;
                    area += f * 6.0;
                }
            }
            if (std::fabs(area) < EPS) return rings.at(0).points.at(0);
            return c / area;
        }
        //删除共线段的中间点
        void Simplify()
        {
            for (auto& poly : rings) {
                poly.Simplify();
            }
        }
        bool IsRectangle() const
        {
            size_t len = rings.size();
            if (len == 1) {
                for (auto& poly : rings) {
                    if (poly.IsRectangle()) {
                        return true;
                    }
                }
            }
            return false;
        }
        T ComputeArea() const
        {
            T area = 0.0;
            for (auto& ring : rings) {
                size_t len = ring.points.size();
                for (size_t i = 0, j = len - 1; i < len; j = i++) {
                    Point2D<T> a = ring.points[j];
                    Point2D<T> b = ring.points[i];
                    area += Cross(a, b);
                }
            }
            return area / 2.0;
        }
    };
    template<typename T>
    class Circle :Ring<T> {
    public:
        T radius = 0;
        Point2D<T> center;
        Circle(){}
        Circle(const Point2D<T>& c, T r) :center(c), radius(r) {}
    };
    template<typename T>
    class Rectangle :Ring<T> {
    public:
        std::vector<Point2D<T>> points;
        Rectangle() {}
        Rectangle(const std::vector<Point2D<T>>&pts):points(pts){}
        Rectangle(const Ring<T>& poly)
        {
            auto pts = poly.points;
            for (auto& pt : pts) {
                points.push_back(pt);
            }
        }
        bool ConstructFromPolygon(const Polygon<T>& polys)
        {
            if (polys.IsRectangle()) {
                auto ring = polys.rings.front();
                for (auto& pt : ring.points) {
                    points.push_back(pt);
                }
                return true;
            }
            return false;
        }
        Circle<T> GetInscribedCircle() const
        {
            if (points.size() == 0) return Circle<T>();
            Point2D<T> ab = points[1] - points[0];
            Point2D<T> ad = points[3] - points[0];
            Point2D<T> center = (points[0] + points[2]) / 2.0;
            T radius = Min(ab.Magnitude(), ad.Magnitude()) / 2.0;
            return Circle<T>(center, radius);
        }
    };
    //计算多边形最近的两条边及对应的两个点
    template <typename T>
    Point2D<T> UpdateCircleCenter(const Point2D<T>& point, Polygon<T>& polys, T& miniDist){
        bool inside = false;
        std::vector<Ring<T>> rings = polys.rings;
        std::vector<std::vector<T>> sdlists;
        for (auto& poly : rings) {
            const auto& points = poly.points;
            const size_t len = points.size();
            std::vector<T> sdDist;
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                const auto& a = points[i];
                const auto& b = points[j];
                if (((a.y > point.y) != (b.y > point.y)) &&
                    ((point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x))) inside = !inside;
                sdDist.push_back(sdSegment(point, a, b));
            }
            sdlists.push_back(sdDist);
        }
        T r1 = std::numeric_limits<T>::infinity();
        T r2 = std::numeric_limits<T>::infinity();
        size_t ps1 = 0, index1 = 0, index2 = 0;
        for (int k = 0; k < sdlists.size(); ++k) {
            auto sdDist = sdlists[k];
            const size_t len = sdDist.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                T dist = sdDist[i];
                if (dist < r1) {
                    r1 = dist;
                    index1 = i;
                    index2 = j;
                    ps1 = k;
                }
            }
        }
        size_t ps2 = 0, index3 = 0, index4 = 0;
        for (int k = 0; k < sdlists.size(); ++k) {
            auto sdDist = sdlists[k];
            const size_t len = sdDist.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                if (k == ps1 && i == index1) continue;
                T dist = sdDist[i];
                if (dist < r2) {
                    r2 = dist;
                    index3 = i;
                    index4 = j;
                    ps2 = k;
                }
            }
        }
        miniDist = (inside ? 1 : -1) * std::sqrt(r1);
        T k = std::sqrt(r1 / r2);
        Point2D<T> p = GetSegmentProject(point, rings[ps1].points[index1], rings[ps1].points[index2]);
        Point2D<T> q = GetSegmentProject(point, rings[ps2].points[index3], rings[ps2].points[index4]);
        /*if (p == q) {
            Point2D<T> u = (point - p).Unit();
            return point - Point2D<T>(u.y, -u.x);
        }
        if (Collinear(point, p, q)) {
            Point2D<T> u = (q - p).Unit();
            return point - Point2D<T>(u.y, -u.x);
        }*/
        if (Collinear(point, p, q)) return p;
        return (p + q * k) / (1 + k);
    }

    //判断点是否在多边形轮廓内部
    template<typename T>
    bool IsPointInPolygons(const Point2D<T>& point, const Polygon<T>& polys)
    {
        bool inside = false;
        Point2D<T> min = polys.Min(), max = polys.Max();
        if (point.x <= min.x || point.x >= max.x || point.y <= min.y || point.y >= max.y) {
            return inside;
        }
        std::vector<Ring<T>> rings = polys.rings;
        for (auto& poly : rings) {
            const auto& points = poly.points;
            const size_t len = points.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                const auto& a = points[i];
                const auto& b = points[j];
                if (((a.y > point.y) != (b.y > point.y)) &&
                    ((point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x))) inside = !inside;
            }
        }
        return inside;
    }

    //获取多边形轮廓内部某一点
    template<typename T>
    Point2D<T> GetPointInPolygons(const Polygon<T>& polys) {
        Point2D<T> pt = (polys.Min() + polys.Max()) / 2.0;
        std::vector<T> leftXs, rightXs;
        bool inside = false;
        for (auto& poly : polys.rings) {
            const auto& points = poly.points;
            const size_t len = points.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                const auto& a = points[i];
                const auto& b = points[j];
                if (a.y == b.y) continue;
                if (pt.y < Min(a.y, b.y)) continue;
                if (pt.y >= Max(a.y, b.y)) continue;
                // 求交点的x坐标（由直线两点式方程转化而来）  
                double x = (double)(pt.y - a.y) * (double)(b.x - a.x) / (double)(b.y - a.y) + a.x;
                // 统计p1p2与p向右射线的交点及左射线的交点  
                if (pt.x < x) {
                    rightXs.push_back(x);
                    inside = !inside;
                } else {
                    leftXs.push_back(x);
                }
            }
        }
        if (inside) return pt;
        std::sort(leftXs.begin(), leftXs.end());
        std::sort(rightXs.begin(), rightXs.end());
        leftXs.insert(leftXs.end(), rightXs.begin(), rightXs.end());
        pt.x = (leftXs[0] + leftXs[1]) / 2.0;
        return pt;
    }

    template <typename T>
    Point2D<T> UpdateCircleRoutes(const Point2D<T>& point, const Polygon<T>& polys, T& miniDist)
    {
        std::vector<Ring<T>> rings = polys.rings;
        std::vector<std::vector<T>> sdlists;
        for (auto& poly : rings) {
            const auto& points = poly.points;
            const size_t len = points.size();
            std::vector<T> sdDist;
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                const auto& a = points[i];
                const auto& b = points[j];
                sdDist.push_back(sdSegment(point, a, b));
            }
            sdlists.push_back(sdDist);
        }
        T r1 = std::numeric_limits<T>::infinity();
        T r2 = std::numeric_limits<T>::infinity();
        size_t ps1 = 0, index1 = 0, index2 = 0;
        for (int k = 0; k < sdlists.size(); ++k) {
            auto& sdDist = sdlists[k];
            const size_t len = sdDist.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                T dist = sdDist[i];
                if (dist < r1) {
                    r1 = dist;
                    index1 = i;
                    index2 = j;
                    ps1 = k;
                }
            }
        }
        size_t ps2 = 0, index3 = 0, index4 = 0;
        for (int k = 0; k < sdlists.size(); ++k) {
            auto& sdDist = sdlists[k];
            const size_t len = sdDist.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                if (k == ps1 && i == index1) continue;
                T dist = sdDist[i];
                if (dist < r2) {
                    r2 = dist;
                    index3 = i;
                    index4 = j;
                    ps2 = k;
                }
            }
        }
        miniDist = std::sqrt(r1);
        T k = std::sqrt(r1 / r2);
        Point2D<T> p = GetSegmentProject(point, rings[ps1].points[index1], rings[ps1].points[index2]);
        Point2D<T> q = GetSegmentProject(point, rings[ps2].points[index3], rings[ps2].points[index4]);
        if (p == q) {
            Point2D<T> u = (point - p).Unit();
            return u;
        }
        if (Collinear(point, p, q)) {
            Point2D<T> u = (q - p).Unit();
            return Point2D<T>(u.y, -u.x);
        }
        Point2D<T> t = kEqualPoint(p, q, k);
        Point2D<T> u = (point - t).Unit();
        return u;
    }
    // get squared distance from a point to a segment
    /*template <typename T>
    constexpr inline T sdSegment2(const Point2D<T>& p, const Point2D<T>& a, const Point2D<T>& b)
    {
        T distAB2 = GetDistance2(a, b);
        T distAP2 = GetDistance2(a, p);
        T distBP2 = GetDistance2(b, p);
        T dist2 = GetDistance2(p, a, b);
        if (Max(distAP2, distBP2) - dist2 < distAB2) {
            return dist2;
        } else {
            return Min(distAP2, distBP2);
        }
    }*/
    template <typename T>
    inline T sdSegment(const Point2D<T>& p, const Point2D<T>& a, const Point2D<T>& b)
    {
        auto x = a.x;
        auto y = a.y;
        auto dx = b.x - x;
        auto dy = b.y - y;
        if (dx != 0 || dy != 0) {
            auto t = ((p.x - x) * dx + (p.y - y) * dy) / (dx * dx + dy * dy);
            if (t > 1) {
                x = b.x;
                y = b.y;

            } else if (t > 0) {
                x += dx * t;
                y += dy * t;
            }
        }
        dx = p.x - x;
        dy = p.y - y;
        return dx * dx + dy * dy;
    }

    // signed distance from point to poly outline (negative if point is outside)
    template <typename T>
    inline T sdPolygon(const Point2D<T>& pt, const Polygon<T>& polys)
    {
        bool inside = false;
        T minDistSq = std::numeric_limits<T>::infinity();
        for (auto& poly : polys.rings) {
            const auto& points = poly.points;
            const size_t len = points.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                const auto& a = points[i];
                const auto& b = points[j];
                if (((a.y > pt.y) != (b.y > pt.y)) &&
                    ((pt.x < (b.x - a.x) * (pt.y - a.y) / (b.y - a.y) + a.x))) inside = !inside;
                minDistSq = Min(minDistSq, sdSegment(pt, a, b));
            }
        }
        return (inside ? 1 : -1) * std::sqrt(minDistSq);
    }
    template <typename T>
    class Cell {
    public:
        constexpr Cell():c(0,0),w(0),h(0),d(0),max(0) {}
        constexpr Cell(const Point2D<T>& c_, T w_, T h_, const Polygon<T>& polys)
            : c(c_),
            w(w_),
            h(h_),
            d(sdPolygon(c, polys)),
            max(d + std::sqrt(w * w + h * h))
        {
        }
        constexpr Cell(const Point2D<T>& c_, T w_, T h_, T d_)
            :c(c_),
            w(w_),
            h(h_),
            d(d_),
            max(d+ std::sqrt(w * w + h * h))
        {

        }
        Point2D<T> c; // cell center
        T w; // half the cell wide size
        T h; // half the cell height size
        T d; // distance from cell center to polys
        T max; // max distance to poly within a cell
        
    };
    
    template<typename T>
    bool operator<(const Cell<T>& lhs, const Cell<T>& rhs)
    {
        return lhs.max < rhs.max;
    }
    template<typename T>
    bool operator==(const Cell<T>& lhs, const Cell<T>& rhs)
    {
        return lhs.max == rhs.max;
    }
    template<typename T>
    bool operator>(const Cell<T>& lhs, const Cell<T>& rhs)
    {
        return lhs.max > rhs.max;
    }
    template<typename T>
    std::vector<Point2D<T>> GetFourCorners(const Cell<T>& cell) 
    {
        T w = cell.w;
        T h = cell.h;
        const Point2D<T>& c = cell.c;
        std::vector<Point2D<T>> corners;
        corners.push_back(c + Point2D<T>(w, h));
        corners.push_back(c + Point2D<T>(-w, h));
        corners.push_back(c + Point2D<T>(-w, -h));
        corners.push_back(c + Point2D<T>(w, -h));
        return corners;
    }
    template<typename T>
    Cell<T> GetPolyCentroidCell(const Polygon<T>& polys)
    {
        Point2D<T> size = polys.Max() - polys.Min();
        Point2D<T> c = polys.GetMassCentroid();
        return Cell<T>(c, size.x / 2, size.y / 2, polys);
    }
    template<typename T>
    Cell<T> GetPolyCentroidCell(const Polygon<T>& polys, T w, T h)
    {
        Point2D<T> c = polys.GetMassCentroid();
        return Cell<T>(c, w, h, polys);
    }

    //生成[0,1)之间的随机数
    inline double Random()
    {
        return rand() / (double)RAND_MAX;
    }
    //获取[0,max)之间的随机整数
    inline int Range(const int max)
    {
        return rand() % max;
    }
    //获取[min,max)之间的随机整数
    inline int Range(const int min, const int max)
    {
        return rand() % (max - min) + min;
    }
    //生成矩形框内一随机点
    template<typename T>
    inline Point2D<T> RangeRectangle(const int width, const int height, T cell_size)
    {
        T x = Random() * width * cell_size;
        T y = Random() * height * cell_size;
        return Point2D<T>(x, y);
    }
    //生成半径radius圆内一随机点
    template<typename T>
    inline Point2D<T> RangeCircle(const T radius)
    {
        double theta = Random() * 2 * M_PI;
        T x = Random() * radius * std::cos(theta);
        T y = Random() * radius * std::sin(theta);
        return Point2D<T>(x, y);
    }
    //生成半径r1,r2圆环内一随机点
    template<typename T>
    inline Point2D<T> RangeCircular(const T r1, const T r2)
    {
        Point2D<T> pt = RangeCircle(r1);
        return pt + pt.Unit() * (r2 - r1);
    }

    /*
    泊松采样原理参考：https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
    */
    template<typename T>
    std::vector<Point2D<T>> possionSample(const Point2D<T>& range, const T threshold = 100, const int max_retry = 30)
    {
        std::vector<Point2D<T>> result_pts;
        // n 的值等于 sqrt(维度)
        // threshold 除上 sqrt(2)，可用保证每一个区块的斜边长为 threshold，
        // 从而确保一个区块内的任意两个点的距离都是小于 threshold 的。
        // 利用这个特性可用快速的确定两个采样点的间距小于 threshold。
        T cell_size = threshold / SQRT2;
        // 划分区块
        int32_t cols = std::ceil(range.x / cell_size);
        int32_t rows = std::ceil(range.y / cell_size);
        // 区块矩阵
        std::vector<std::vector<int32_t>> grids;
        grids.resize(rows);
        for (auto& row : grids) {
            row.resize(cols, -1);
        }
        // 开始
        // 随机选一个初始点
        auto start = Point2D<T>(Range(range.x), Range(range.y));
        int32_t col = std::floor(start.x / cell_size);
        int32_t row = std::floor(start.y / cell_size);
        auto start_key = result_pts.size();
        result_pts.push_back(start);
        grids[row][col] = start_key;
        std::vector<int> active_list;
        active_list.push_back(start_key);
        T r1 = threshold, r2 = 2 * threshold;
        while (active_list.size() > 0) {
            // 在已经有的采样集合中取一个点, 在这个点周围生成新的采样点
            auto key = active_list[Range(active_list.size())];
            // auto key = active_list[0];
            auto point = result_pts[key];
            bool found = false;
            for (int32_t i = 0; i < max_retry; i++) {
                auto direct = RangeCircle(r1);
                // 给原有的采样点 point 加上一个距离 [r, 2r) 的随机向量，成为新的采样点
                auto new_point = point + direct.Unit() * (r2 - r1) + direct;
                if ((new_point.x < 0 || new_point.x >= range.x) ||
                    (new_point.y < 0 || new_point.y >= range.y)) {
                    continue;
                }
                col = std::floor(new_point.x / cell_size);
                row = std::floor(new_point.y / cell_size);
                if (grids[row][col] != -1) // 区块内已经有采样点类
                {
                    continue;
                }
                // 检查新采样点周围区块是否存在距离小于 threshold 的点
                bool ok = true;
                int32_t min_r = std::floor((new_point.y - threshold) / cell_size);
                int32_t max_r = std::floor((new_point.y + threshold) / cell_size);
                int32_t min_c = std::floor((new_point.x - threshold) / cell_size);
                int32_t max_c = std::floor((new_point.x + threshold) / cell_size);
                [&]() {
                    for (int32_t r = min_r; r <= max_r; r++) {
                        if (r < 0 || r >= rows) {
                            continue;
                        }
                        for (int32_t c = min_c; c <= max_c; c++) {
                            if (c < 0 || c >= cols) {
                                continue;
                            }
                            int32_t point_key = grids[r][c];
                            if (point_key != -1) {
                                auto round_point = result_pts[point_key];
                                auto dist = (round_point - new_point).Magnitude();
                                if (dist < threshold) {
                                    ok = false;
                                    return;
                                }
                                // 当 ok 为 false 后，后续的循环检测都没有意义的了，
                                // 使用 return 跳出两层循环。
                            }
                        }
                    }
                }();
                // 新采样点成功采样
                if (ok) {
                    auto new_point_key = result_pts.size();
                    result_pts.push_back(new_point);
                    grids[row][col] = new_point_key;
                    active_list.push_back(new_point_key);
                    found = true;
                    break;
                }
            }
            if (!found) {
                auto iter = std::find(active_list.begin(), active_list.end(), key);
                if (iter != active_list.end()) {
                    active_list.erase(iter);
                }
            }
        }
        return result_pts;
    }

    // 计算水平线与多边形的交线段，并在相交线段中局部均匀采样
    template<typename T>
    std::vector<Cell<T>> crossCellsInPolygon(const Point2D<T>& pt, const Polygon<T>& polys, T delta = 1.0, int nxs = 20)
    {
        std::vector<Cell<T>> crossCells;
        std::vector<T> leftXs, rightXs;
        bool inside = false;
        for (auto& poly : polys.rings) {
            const auto& points = poly.points;
            const size_t len = points.size();
            for (std::size_t i = 0, j = len - 1; i < len; j = i++) {
                const auto& a = points[i];
                const auto& b = points[j];
                if (a.y == b.y) continue;
                if (pt.y < Min(a.y, b.y)) continue;
                if (pt.y >= Max(a.y, b.y)) continue;
                // 求交点的x坐标（由直线两点式方程转化而来）  
                double x = (double)(pt.y - a.y) * (double)(b.x - a.x) / (double)(b.y - a.y) + a.x;
                // 统计p1p2与p向右射线的交点及左射线的交点  
                if (pt.x < x) {
                    rightXs.push_back(x);
                    inside = !inside;
                } else {
                    leftXs.push_back(x);
                }
            }
        }
        std::sort(leftXs.begin(), leftXs.end());
        std::sort(rightXs.begin(), rightXs.end());
        const size_t lns = leftXs.size();
        const size_t rns = rightXs.size();
        const T h = delta / 2;
        const T y = pt.y;
        if (inside) {
            leftXs.insert(leftXs.end(), rightXs.begin(), rightXs.end());
            for (size_t i = 0; i < lns + rns; i += 2) {
                T xl = leftXs[i], xr = leftXs[i + 1];
                T w = (xr - xl) / (double)(2 * nxs + 1);
                for (T x = xr - w / 2; x >= xl; x -= 2 * w) {
                    crossCells.push_back(Cell<T>(Point2D<T>(x, y), w, h, polys));
                }
            }
        } else {
            // left part
            for (size_t i = 0; i < lns; i += 2) {
                T xl = leftXs[i], xr = leftXs[i + 1];
                T w = (xr - xl) / (double)(2 * nxs + 1);
                for (T x = xr - w / 2; x >= xl; x -= 2 * w) {
                    crossCells.push_back(Cell<T>(Point2D<T>(x, y), w, h, polys));
                }
            }
            // right part
            for (size_t i = 0; i < rns; i += 2) {
                T xl = rightXs[i], xr = rightXs[i + 1];
                T w = (xr - xl) / (double)(2 * nxs + 1);
                for (T x = xr - w / 2; x >= xl; x -= 2 * w) {
                    crossCells.push_back(Cell<T>(Point2D<T>(x, y), w, h, polys));
                }
            }
        }
        return crossCells;
    }
    template<typename T>
    std::vector<Cell<T>> conditionalSample(const Polygon<T>& polys, int nxs = 20, int nys = 20)
    {
        const Point2D<T>& min = polys.Min();
        const Point2D<T>& max = polys.Max();
        const T yMin = min.y, yMax = max.y;
        T delta = (yMax - yMin) / (double)(2.0 * nys + 1);
        T x = (max.x + min.x) / 2.0;
        std::vector<Cell<T>> sampleCells;
        for (T y = yMax - delta / 2; y >= yMin; y -= delta) {
            std::vector<Cell<T>> cs = crossCellsInPolygon(Point2D<T>(x, y), polys, delta, nxs);
            //在多边形内部横纵轴分别均匀采样
            sampleCells.insert(sampleCells.end(), cs.begin(), cs.end());
        }
        return sampleCells;
    }
    /*
    计算多边形的极心
    */
    template <typename T>
    Cell<T> sdPolygonPole(Polygon<T>& polys, T precision = 1, const poleAlgo type = poleAlgo::REGIONAL_SAMPLE)
    {
        polys.Simplify();
        if (polys.IsRectangle()) {
            Rectangle<T> rect(polys.rings.front());
            Circle<T> circle = rect.GetInscribedCircle();
            Cell<T> cell(circle.center, 0, 0, circle.radius);
            return cell;
        }
        const Point2D<T> center = polys.GetBoundCenter();
        polys.Translate(center);
        BoundBox<T> bound = polys.GetBoundBox();
        const Point2D<T>& maxPt = bound.max;
        const Point2D<T>& minPt = bound.min;
        const Point2D<T>& size = maxPt - minPt;
        T w = size.x / 2, h = size.y / 2;
        const T cellSize = Min(w, h);
        // a priority queue of cells in order of their "potential" (max distance to poly)
        std::priority_queue<Cell<T>, std::vector<Cell<T>>, std::less<Cell<T>>> cellQueue;
        if (cellSize == 0) {
            return Cell<T>(bound.min, w, h, polys);
        }
        // take centroid as the first best guess
        Cell<T> bestCell = GetPolyCentroidCell(polys, w, h);
        // second guess: bounding box centroid
        Point2D<T> boundCenroid = (maxPt + minPt) * 0.5;
        Cell<T> bboxCell(boundCenroid, w, h, polys);
        if (bboxCell.d > bestCell.d) {
            bestCell = bboxCell;
        }
        // select different coverage strategies according to the split methods.
        switch (type) {
        case poleAlgo::QUARDTER_COVER:
            // cover poly with initial cells
            {
                for (T x = minPt.x; x < maxPt.x; x += w) {
                    for (T y = minPt.y; y < maxPt.y; y += h) {
                        cellQueue.push(Cell<T>(Point2D<T>(x + w / 2, y + h / 2), w, h, polys));
                    }
                }
                auto numProbes = cellQueue.size();
                while (!cellQueue.empty()) {
                    if (cellSize - bestCell.d < precision) break;
                    // pick the most promising cell from the queue
                    auto cell = cellQueue.top();
                    h = cell.h / 2, w = cell.w / 2;
                    cellQueue.pop();
                    // update the best cell if we found a better one
                    if (cell.d > bestCell.d) {
                        bestCell = cell;
                    }
                    // do not drill down further if there's no chance of a better solution
                    if (cell.max - bestCell.d <= precision) break;
                    // split the cell into four cells
                    cellQueue.push(Cell<T>(Point2D<T>(cell.c.x - w, cell.c.y - h), w, h, polys));
                    cellQueue.push(Cell<T>(Point2D<T>(cell.c.x + w, cell.c.y - h), w, h, polys));
                    cellQueue.push(Cell<T>(Point2D<T>(cell.c.x - w, cell.c.y + h), w, h, polys));
                    cellQueue.push(Cell<T>(Point2D<T>(cell.c.x + w, cell.c.y + h), w, h, polys));
                    numProbes += 4;
                }
            }
            break;
        case poleAlgo::REGIONAL_SAMPLE:
            // conditional sample
            // 在多边形内部间隔采样，尽量保留两条相交线段的中间部分
            {
                std::vector<Cell<T>> cells = conditionalSample(polys, 20, 20);
                std::sort(cells.begin(), cells.end(), std::greater<Cell<T>>());
                bestCell = cells.front();
            }
            break;
        case poleAlgo::POISSON_SAMPLE:
        {
            std::vector<Point2D<T>> samplePoints = possionSample(size, 1000 * precision);
            std::vector<Cell<T>>cells;
            for (auto& pt : samplePoints) {
                cells.push_back(Cell<T>(pt, precision, precision, polys));
            }
            std::sort(cells.begin(), cells.end(), std::greater<Cell<T>>());
            bestCell = cells.front();
        }
            break;
        case poleAlgo::INSCRIBED_CIRCLE:
        {
            //计算原理参考https://www.docin.com/p-1221389860.html
            //初始步长，最短距离初始化
            T a = 1200, lastDist = 0, curDist = 0;
            //初始圆心坐标
            Point2D<T> pc = (maxPt + minPt) / 2.0;
            //步骤1，计算角平分线与底边交点的圆心坐标
            Point2D<T> pt = UpdateCircleCenter(pc, polys, lastDist);
            double flag = 1.0;
            while (a > precision) {
                //步骤2，根据步长，更新的圆心坐标
                //注意，判断当前点在内外部，及更新内外角平分线上圆心坐标
                flag = lastDist > 0 ? 1.0 : -1.0;
                Point2D<T> vec = (pc - pt).Unit();
                Point2D<T> curPc = pc + vec * a * flag;
                Point2D<T> curPt= UpdateCircleCenter(curPc, polys, curDist);
                if (curDist > lastDist) {
                    pc = curPc;
                    pt = curPt;
                    lastDist = curDist;
                } else {
                    a *= 0.618;
                    continue;
                }
            }
            bestCell.c = pc;
            bestCell.d = sdPolygon(pc, polys);
        }
        break;
        default:
            break;
        }
        polys.Translate(-center);
        bestCell.c += center;
        return bestCell;
    }

    template <typename T>
    Cell<T> GetIterationCircles(Polygon<T>& polys, topomesh::LightOffCircle& result, topomesh::LightOffDebugger* debugger = nullptr,
        ccglobal::Tracer* tracer = nullptr)
    {
        polys.Simplify();
        if (polys.IsRectangle()) {
            Rectangle<T> rect(polys.rings.front());
            Circle<T> circle = rect.GetInscribedCircle();
            result.point.X = circle.center.x;
            result.point.Y = circle.center.y;
            result.radius = circle.radius;
            Cell<T> cell(circle.center, 0, 0, circle.radius);
            return cell;
        }
        T precision = 1e3; ///<默认乘以1000
#ifdef DLP_USE_UM
        precision /= 1e-3;
#endif
        BoundBox<T> bound = polys.GetBoundBox();
        const Point2D<T>& maxPt = bound.max;
        const Point2D<T>& minPt = bound.min;
        const Point2D<T>& size = maxPt - minPt;
        const T side = Min(size.x, size.y) / 2;
        //计算原理参考https://www.docin.com/p-1221389860.html
            //初始步长，最短距离初始化
        T a = side, lastDist = 0, curDist = 0;
        //初始圆心坐标
        Point2D<T> pc = (maxPt + minPt) * 0.5;
        //步骤1，计算角平分线与底边交点的圆心坐标
        Point2D<T> pt = UpdateCircleCenter(pc, polys, lastDist);
        double flag = 1.0;
        bool incident = false;
        int num = 0;
        while (a > precision) {
            if (debugger) {
                result.point.X = pc.x;
                result.point.Y = pc.y;
                result.radius = std::fabs(lastDist);
                debugger->onIteration(result);
            }
            //步骤2，根据步长，更新的圆心坐标
            //注意，判断当前点在内外部，及更新内外角平分线上圆心坐标
            flag = lastDist > 0 ? 1.0 : -1.0;
            Point2D<T> vec = (pc - pt).Unit();
            Point2D<T> curPc = pc + vec * a * flag;
            Point2D<T> curPt = UpdateCircleCenter(curPc, polys, curDist);
            T offset = (pc - curPt).Magnitude();
            ++num;
            if (offset < precision && incident) break;
            if (curDist > lastDist+precision) {
                pc = curPc;
                pt = curPt;
                incident = true;
                lastDist = curDist;
            } else {
                incident = false;
                a *= 0.618;
                continue;
            }
        }
        Cell<T> res_cell;
        res_cell.c = pc;
        res_cell.d = lastDist;
        result.point.X = res_cell.c.x;
        result.point.Y = res_cell.c.y;
        result.radius = std::fabs(res_cell.d);
        return res_cell;
    }

    template <typename T>
    Cell<T> GetIterationCircleRoutes(Polygon<T>& polys, topomesh::LightOffCircle& result, topomesh::LightOffDebugger* debugger = nullptr,
        ccglobal::Tracer* tracer = nullptr)
    {
        polys.Simplify();
        if (polys.IsRectangle()) {
            Rectangle<T> rect(polys.rings.front());
            Circle<T> circle = rect.GetInscribedCircle();
            result.point.X = circle.center.x;
            result.point.Y = circle.center.y;
            result.radius = circle.radius;
            Cell<T> cell(circle.center, 0, 0, circle.radius);
            return cell;
        }
        T precision = 1e3; ///<默认乘以1000
#ifdef DLP_USE_UM
        precision /= 1e-3;
#endif
        BoundBox<T> bound = polys.GetBoundBox();
        const Point2D<T>& maxPt = bound.max;
        const Point2D<T>& minPt = bound.min;
        const Point2D<T>& size = maxPt - minPt;
        const T side = Min(size.x, size.y) / 2;
        //计算原理参考https://www.docin.com/p-1221389860.html
            //初始步长，最短距离初始化
        T a = side, lastDist = 0, curDist = 0;
        T flag = 1.0;
        //初始圆心坐标
        Point2D<T> pc = (maxPt + minPt) * 0.5;
        Point2D<T> vec = UpdateCircleRoutes(pc, polys, curDist);
        Point2D<T> curPc;
        lastDist = curDist;
        while (a > precision) {
            if (debugger) {
                result.point.X = pc.x;
                result.point.Y = pc.y;
                result.radius = std::fabs(lastDist);
                debugger->onIteration(result);
            }
            //步骤2，根据步长，更新的圆心坐标
            //注意，判断当前点在内外部，及更新内外角平分线上圆心坐标
            if (PointInPolygons(pc, polys)) {
                flag = 1.0;
                curPc = pc + vec * a * flag;
                vec = UpdateCircleRoutes(curPc, polys, curDist);
                curDist *= flag;
                if (curDist > lastDist) {
                    pc = curPc;
                    lastDist = curDist;
                } else {
                    a *= 0.618;
                }
            } else {
                flag = -1.0;
                curPc = pc + vec * a * flag;
                vec = UpdateCircleRoutes(curPc, polys, curDist);
                curDist *= flag;
                if (curDist > lastDist || lastDist > 0) {
                    pc = curPc;
                    lastDist = curDist;
                } else {
                    a *= 0.618;
                }
            }
        }
        Cell<T> res_cell;
        res_cell.c = pc;
        res_cell.d = lastDist;
        result.point.X = res_cell.c.x;
        result.point.Y = res_cell.c.y;
        result.radius = std::fabs(res_cell.d);
        return res_cell;
    }
} // namespace polygonPole
