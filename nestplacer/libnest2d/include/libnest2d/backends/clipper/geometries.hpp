#ifndef CLIPPER_BACKEND_HPP
#define CLIPPER_BACKEND_HPP

#include <sstream>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <iostream>

#include <libnest2d/geometry_traits.hpp>
#include <libnest2d/geometry_traits_nfp.hpp>

#include "clipper_polygon.hpp"

namespace libnest2d {

// Aliases for convinience
using PointImpl = Clipper3r::IntPoint;
using PathImpl  = Clipper3r::Path;
using HoleStore = Clipper3r::Paths;
using PolygonImpl = Clipper3r::Polygon;

template<> struct ShapeTag<PolygonImpl> { using Type = PolygonTag; };
template<> struct ShapeTag<PathImpl>    { using Type = PathTag; };
template<> struct ShapeTag<PointImpl>   { using Type = PointTag; };

// Type of coordinate units used by Clipper. Enough to specialize for point,
// the rest of the types will work (Path, Polygon)
template<> struct CoordType<PointImpl> { using Type = Clipper3r::cInt; };

// Enough to specialize for path, it will work for multishape and Polygon
template<> struct PointType<PathImpl> { using Type = PointImpl; };

// This is crucial. CountourType refers to itself by default, so we don't have
// to secialize for clipper Path. ContourType<PathImpl>::Type is PathImpl.
template<> struct ContourType<PolygonImpl> { using Type = PathImpl; };

// The holes are contained in Clipper::Paths
template<> struct HolesContainer<PolygonImpl> { using Type = Clipper3r::Paths; };

namespace pointlike {

// Tell libnest2d how to extract the X coord from a ClipperPoint object
template<> inline Clipper3r::cInt x(const PointImpl& p)
{
    return p.X;
}

// Tell libnest2d how to extract the Y coord from a ClipperPoint object
template<> inline Clipper3r::cInt y(const PointImpl& p)
{
    return p.Y;
}

// Tell libnest2d how to extract the X coord from a ClipperPoint object
template<> inline Clipper3r::cInt& x(PointImpl& p)
{
    return p.X;
}

// Tell libnest2d how to extract the Y coord from a ClipperPoint object
template<> inline Clipper3r::cInt& y(PointImpl& p)
{
    return p.Y;
}

}

// Using the libnest2d default area implementation
#define DISABLE_BOOST_AREA

namespace shapelike {

template<>
inline void offset(PolygonImpl& sh, TCoord<PointImpl> distance, const PolygonTag&)
{
    #define DISABLE_BOOST_OFFSET

    using Clipper3r::ClipperOffset;
    using Clipper3r::jtMiter;
    using Clipper3r::etClosedPolygon;
    using Clipper3r::Paths;

    Paths result;
    
    try {
        ClipperOffset offs;
        offs.AddPath(sh.Contour, jtMiter, etClosedPolygon);
        offs.AddPaths(sh.Holes, jtMiter, etClosedPolygon);
        offs.Execute(result, static_cast<double>(distance));
    } catch (Clipper3r::clipperException &) {
        throw GeometryException(GeomErr::OFFSET);
    }

    // Offsetting reverts the orientation and also removes the last vertex
    // so boost will not have a closed polygon.

    bool found_the_contour = false;
    for(auto& r : result) {
        if(Clipper3r::Orientation(r)) {
            // We don't like if the offsetting generates more than one contour
            // but throwing would be an overkill. Instead, we should warn the
            // caller about the inability to create correct geometries
            if(!found_the_contour) {
                sh.Contour = std::move(r);
                Clipper3r::ReversePath(sh.Contour);
                auto front_p = sh.Contour.front();
                sh.Contour.emplace_back(std::move(front_p));
                found_the_contour = true;
            } else {
                dout() << "Warning: offsetting result is invalid!";
                /* TODO warning */
            }
        } else {
            // TODO If there are multiple contours we can't be sure which hole
            // belongs to the first contour. (But in this case the situation is
            // bad enough to let it go...)
            sh.Holes.emplace_back(std::move(r));
            Clipper3r::ReversePath(sh.Holes.back());
            auto front_p = sh.Holes.back().front();
            sh.Holes.back().emplace_back(std::move(front_p));
        }
    }
}

template<>
inline void offset(PathImpl& sh, TCoord<PointImpl> distance, const PathTag&)
{
    PolygonImpl p(std::move(sh));
    offset(p, distance, PolygonTag());
    sh = p.Contour;
}

// Tell libnest2d how to make string out of a ClipperPolygon object
template<> inline std::string toString(const PolygonImpl& sh)
{
    std::stringstream ss;

    ss << "Contour {\n";
    for(auto p : sh.Contour) {
        ss << "\t" << p.X << " " << p.Y << "\n";
    }
    ss << "}\n";

    for(auto& h : sh.Holes) {
        ss << "Holes {\n";
        for(auto p : h)  {
            ss << "\t{\n";
            ss << "\t\t" << p.X << " " << p.Y << "\n";
            ss << "\t}\n";
        }
        ss << "}\n";
    }

    return ss.str();
}

template<>
inline PolygonImpl create(const PathImpl& path, const HoleStore& holes)
{
    PolygonImpl p;
    p.Contour = path;
    p.Holes = holes;
   
    return p;
}

template<> inline PolygonImpl create( PathImpl&& path, HoleStore&& holes) {
    PolygonImpl p;
    p.Contour.swap(path);
    p.Holes.swap(holes);
    
    return p;
}

template<>
inline const THolesContainer<PolygonImpl>& holes(const PolygonImpl& sh)
{
    return sh.Holes;
}

template<> inline THolesContainer<PolygonImpl>& holes(PolygonImpl& sh)
{
    return sh.Holes;
}

template<>
inline TContour<PolygonImpl>& hole(PolygonImpl& sh, unsigned long idx)
{
    return sh.Holes[idx];
}

template<>
inline const TContour<PolygonImpl>& hole(const PolygonImpl& sh,
                                            unsigned long idx)
{
    return sh.Holes[idx];
}

template<> inline size_t holeCount(const PolygonImpl& sh)
{
    return sh.Holes.size();
}

template<> inline PathImpl& contour(PolygonImpl& sh)
{
    return sh.Contour;
}

template<>
inline const PathImpl& contour(const PolygonImpl& sh)
{
    return sh.Contour;
}

#define DISABLE_BOOST_TRANSLATE
template<>
inline void translate(PolygonImpl& sh, const PointImpl& offs)
{
    for(auto& p : sh.Contour) { p += offs; }
    for(auto& hole : sh.Holes) for(auto& p : hole) { p += offs; }
}

#define DISABLE_BOOST_ROTATE
template<>
inline void rotate(PolygonImpl& sh, const Radians& rads)
{
    using Coord = TCoord<PointImpl>;

    auto cosa = rads.cos();
    auto sina = rads.sin();

    for(auto& p : sh.Contour) {
        p = {
                static_cast<Coord>(p.X * cosa - p.Y * sina),
                static_cast<Coord>(p.X * sina + p.Y * cosa)
            };
    }
    for(auto& hole : sh.Holes) for(auto& p : hole) {
        p = {
                static_cast<Coord>(p.X * cosa - p.Y * sina),
                static_cast<Coord>(p.X * sina + p.Y * cosa)
            };
    }
}

} // namespace shapelike

#define DISABLE_BOOST_NFP_MERGE
inline TMultiShape<PolygonImpl> clipper_execute(
        Clipper3r::Clipper& clipper,
        Clipper3r::ClipType clipType,
        Clipper3r::PolyFillType subjFillType = Clipper3r::pftEvenOdd,
        Clipper3r::PolyFillType clipFillType = Clipper3r::pftEvenOdd)
{
    TMultiShape<PolygonImpl> retv;

    Clipper3r::PolyTree result;
    clipper.Execute(clipType, result, subjFillType, clipFillType);

    retv.reserve(static_cast<size_t>(result.Total()));

    std::function<void(Clipper3r::PolyNode*, PolygonImpl&)> processHole;

    auto processPoly = [&retv, &processHole](Clipper3r::PolyNode * pptr) {
        PolygonImpl poly;
        poly.Contour.swap(pptr->Contour);

        assert(!pptr->IsHole());

        if (!poly.Contour.empty()) {
            const auto& front_p = poly.Contour.front();
            const auto& back_p = poly.Contour.back();
            if (front_p.X != back_p.X || front_p.Y != back_p.Y)
                poly.Contour.emplace_back(front_p);
        }

        for (auto& h : pptr->Childs)
            processHole(h, poly);

        retv.push_back(poly);
    };

    processHole = [&processPoly](const Clipper3r::PolyNode * pptr, PolygonImpl & poly) {
        poly.Holes.emplace_back(std::move(pptr->Contour));

        assert(pptr->IsHole());

        if (!poly.Contour.empty()) {
            const auto& front_p = poly.Contour.front();
            const auto& back_p = poly.Contour.back();
            if (front_p.X != back_p.X || front_p.Y != back_p.Y)
                poly.Contour.emplace_back(front_p);
        }

        for (auto& c : pptr->Childs)
            processPoly(c);
    };

    auto traverse = [&processPoly] (Clipper3r::PolyNode *node)
    {
        for(auto& ch : node->Childs) 
            processPoly(ch);
    };

    traverse(&result);

    return retv;
}

namespace nfp {

template<> inline TMultiShape<PolygonImpl>
merge(const TMultiShape<PolygonImpl>& shapes)
{
    Clipper3r::Clipper clipper(Clipper3r::ioReverseSolution);

    bool closed = true;
    bool valid = true;

    for(auto& path : shapes) {
        valid &= clipper.AddPath(path.Contour, Clipper3r::ptSubject, closed);

        for(auto& h : path.Holes)
            valid &= clipper.AddPath(h, Clipper3r::ptSubject, closed);
    }

    if(!valid) throw GeometryException(GeomErr::MERGE);

    return clipper_execute(clipper, Clipper3r::ctUnion, Clipper3r::pftNegative);
}

}

}

#define DISABLE_BOOST_CONVEX_HULL

//#define DISABLE_BOOST_SERIALIZE
//#define DISABLE_BOOST_UNSERIALIZE

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)
#endif
// All other operators and algorithms are implemented with boost
#include <libnest2d/utils/boost_alg.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // CLIPPER_BACKEND_HPP
