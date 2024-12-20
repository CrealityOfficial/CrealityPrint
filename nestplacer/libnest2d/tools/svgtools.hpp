#ifndef SVGTOOLS_HPP
#define SVGTOOLS_HPP

#include <iostream>
#include <fstream>
#include <string>

#include <libnest2d/nester.hpp>
#include "svg/polygon.h"
#include "svg/aabb.h"
#include "svg/svg.h"

namespace libnest2d { namespace writer {
template<class RawShape>
class ItemWriter {
    using Item = _Item<RawShape>;
    using Vertex = TPoint<RawShape>;
    using Coord = TCoord<TPoint<RawShape>>;
    using Box = _Box<TPoint<RawShape>>;
    using ItemGroup = _ItemGroup<RawShape>;
    using PackGroup = _PackGroup<RawShape>;
    using Shapes = TMultiShape<RawShape>;
private:
    svg::Polygons convertPolygons(const RawShape& sh)
    {
        Clipper3r::Paths paths;
        const Clipper3r::Path& con= sh.Contour;
        const Clipper3r::Paths& holes = sh.Holes;
        paths.emplace_back(con);
        paths.insert(paths.end(), holes.begin(), holes.end());
        svg::Polygons polys;
        polys.paths.swap(paths);
        return polys;
    }

    svg::Polygons convertPolygons(const Shapes& shs)
    {
        Clipper3r::Paths paths;
        for (const auto& sh : shs) {
            const auto& polys = convertPolygons(sh);
            const auto& ps = polys.paths;
            paths.insert(paths.end(), ps.begin(), ps.end());
        }
        svg::Polygons polygons;
        polygons.paths.swap(paths);
        return polygons;
    }

    svg::Polygons convertPolygons(const Item& item)
    {
        const RawShape& sh = item.transformedShape();
        return convertPolygons(sh);
    }

    svg::Polygons convertPolygons(const ItemGroup& items)
    {
        Clipper3r::Paths paths;
        for (const auto& item : items) {
            const auto& polys = convertPolygons(item.get());
            const auto& ps = polys.paths;
            paths.insert(paths.end(), ps.begin(), ps.end());
        }
        svg::Polygons polygons;
        polygons.paths.swap(paths);
        return polygons;
    }

    svg::Polygons convertPolygons(const Box& box, bool inflate = false)
    {
        const auto& min = box.minCorner();
        const auto& max = box.maxCorner();
        Clipper3r::Path path;
        if (inflate) {
            const auto& w = box.width() / 2;
            const auto& h = box.height() / 2;
            path.emplace_back(min.X - w, min.Y - h);
            path.emplace_back(min.X - w, max.Y + h);
            path.emplace_back(max.X + w, max.Y + h);
            path.emplace_back(max.X + w, min.Y - h);
        } else {
            path.emplace_back(min.X, min.Y);
            path.emplace_back(min.X, max.Y);
            path.emplace_back(max.X, max.Y);
            path.emplace_back(max.X, min.Y);
        }
        svg::Polygons polys;
        polys.paths.emplace_back(path);
        return polys;
    }

    svg::AABB getAABB(const Item& item)
    {
        const Box& bin = item.boundingBox();
        const Coord& off = std::min(bin.width(), bin.height());
        const RawShape& rs = item.transformedShape();
        Item cpy(rs); cpy.inflate(off);
        svg::Polygons polys = convertPolygons(cpy.transformedShape());
        svg::AABB aabb(polys);
        return aabb;
    }

    svg::AABB getAABB(const Box& box, bool inflate = false)
    {
        svg::Polygons polys = convertPolygons(box, inflate);
        svg::AABB aabb(polys);
        return aabb;
    }

    svg::AABB getAABB(const svg::Polygons& polys, bool inflate = false)
    {
        svg::AABB aabb(polys);
        const auto& min = aabb.min;
        const auto& max = aabb.max;
        const auto& dist = std::min(max.X - min.X, max.Y - min.Y) / 2;
        aabb.expand(dist);
        return aabb;
    }
public:
    struct SVGData {
        SVGData():orsh(RawShape()){}
        SVGData(const Box& b, const ItemGroup& s) :bin(b), items(s) {}
        SVGData(const Box& b, const Item& o) :bin(b), orsh(o) {}
        SVGData(const ItemGroup& s, const Item& o, const Shapes& n) :items(s), orsh(o), nfps(n) {}
        SVGData(const Box& b, const ItemGroup& s, const Item& o, const Shapes& n) :bin(b), items(s), orsh(o), nfps(n) {}
        Vertex p;
        Box bin;
        ItemGroup items;
        Item orsh;
        Shapes nfps;
        Box getBound() const
        {
            const auto& min = bin.minCorner();
            const auto& max = bin.maxCorner();
            auto xmin = min.X, ymin = min.Y;
            auto xmax = max.X, ymax = max.Y;
            for (const auto& item : items) {
                const auto& b = item.get().boundingBox();
                const auto& p1 = b.minCorner();
                const auto& p2 = b.maxCorner();
                if (p1.X < xmin) xmin = p1.X;
                if (p1.Y < ymin) ymin = p1.Y;
                if (p2.X > xmax) xmax = p2.X;
                if (p2.Y > ymax) ymax = p2.Y;
            }
            for (const auto& nfp : nfps) {
                Item cpy(nfp);
                const auto& b = cpy.boundingBox();
                const auto& p1 = b.minCorner();
                const auto& p2 = b.maxCorner();
                if (p1.X < xmin) xmin = p1.X;
                if (p1.Y < ymin) ymin = p1.Y;
                if (p2.X > xmax) xmax = p2.X;
                if (p2.Y > ymax) ymax = p2.Y;
            }
            Box box({ xmin,ymin }, { xmax,ymax });
            const auto& obox = orsh.boundingBox();
            box = sl::boundingBox(box, obox);
            return box;
        }
    };
    void saveRawShape(const RawShape& sh, const std::string& filename, int color = 5, bool bWritePoints = false)
    {
        Item cpy(sh);
        saveItem(cpy, filename, color, bWritePoints);
    }

    void saveShapes(const Shapes& shapes, const std::string& filename, int color = 5, bool bWritePoints = false)
    {
        /*std::string filepath = "D:/test";
        std::ifstream file(filepath);
        if (!file.good()) return;*/
        std::string svgFile = filename + ".svg";
        svg::Polygons polys = convertPolygons(shapes);
        svg::AABB aabb = getAABB(polys);
        svg::SVG svg(svgFile, aabb);
        svg.setFlipY(true);
        if (bWritePoints) svg.writePoints(polys, true, 1.0, svg::SVG::Color::BLUE);
        auto co = svg::SVG::Color(color % 10);
        svg.writePolygons(polys, co, 0.2);
    }

    void saveShapes(const RawShape& rsh, const Shapes& shapesA, const Shapes& shapesB, const Shapes& merge, const std::string& filename, bool bWritePoints = false)
    {
        /*std::string filepath = "D:/test";
        std::ifstream file(filepath);
        if (!file.good()) return;*/
        std::string svgFile = filename + ".svg";
        svg::Polygons polyRs = convertPolygons(rsh);
        svg::Polygons polyAs = convertPolygons(shapesA);
        svg::Polygons polyBs = convertPolygons(shapesB);
        svg::Polygons polyMs = convertPolygons(merge);
        svg::Polygons polys;
        polys.paths.insert(polys.paths.end(), polyRs.paths.begin(), polyRs.paths.end());
        polys.paths.insert(polys.paths.end(), polyAs.paths.begin(), polyAs.paths.end());
        polys.paths.insert(polys.paths.end(), polyBs.paths.begin(), polyBs.paths.end());
        polys.paths.insert(polys.paths.end(), polyMs.paths.begin(), polyMs.paths.end());
        svg::AABB aabb = getAABB(polys);
        svg::SVG svg(svgFile, aabb);
        svg.setFlipY(true);
        svg.writePolygons(polyRs, svg::SVG::Color::RED, 0.3);
        svg.writePolygons(polyAs, svg::SVG::Color::BLACK, 0.2);
        //if (bWritePoints) svg.writePoints(shpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg.writePolygons(polyBs, svg::SVG::Color::GREEN, 0.2);
        //if (bWritePoints) svg.writePoints(shpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg.writePolygons(polyMs, svg::SVG::Color::BLUE, 0.2);
        //if (bWritePoints) svg.writePoints(orpolys, true, 1.0, svg::SVG::Color::RAINBOW);
    }

    void saveItem(const Item& item, const std::string& filename, int color = 5, bool bWritePoints = false)
    {
        std::string filepath = "D:/test";
        std::ifstream file(filepath);
        if (!file.good()) return;
        std::string svgFile = filename + ".svg";
        svg::AABB aabb = getAABB(item);
        svg::SVG svg(svgFile, aabb);
        svg.setFlipY(true);
        svg::Polygons polys = convertPolygons(item);
        if (bWritePoints) svg.writePoints(polys, true, 1.0, svg::SVG::Color::BLUE);
        auto co = svg::SVG::Color(color % 10);
        svg.writePolygons(polys, co, 0.3);
    }

    /*void saveBox(const Box& box, const std::string& filename, int color = 5, bool bWritePoints = false)
    {
        svg::Polygons polys = convertPolygons(box, true);
        std::string svgFile = filename + ".svg";
        svg::AABB aabb = getAABB(box);
        svg::SVG svg(svgFile, aabb);
        svg.setFlipY(true);
        svg.writePolygons(polys, svg::SVG::Color::WHITE, 1);
    }*/

    void saveItems(const SVGData& datas, const std::string& filename, bool bWritePoints = false)
    {
        /*std::string filepath = "D:/test";
        std::ifstream file(filepath);
        if (!file.good()) return;*/
        std::string svgFile = filename + ".svg";
        svg::Polygons binpolys;
        Box fullbb = datas.getBound();
        binpolys = convertPolygons(fullbb, true);
        svg::AABB aabb = getAABB(binpolys, true);
        if (datas.bin.area()) {
            binpolys = convertPolygons(datas.bin);
        }
        svg::SVG svg(svgFile, aabb);
        svg.setFlipY(true);
        float linewidth = 0.5f;
        svg.writePolygons(binpolys, svg::SVG::Color::BLACK, linewidth);
        //if (bWritePoints) svg.writePoints(binpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg::Polygons shpolys = convertPolygons(datas.items);
        svg.writePolygons(shpolys, svg::SVG::Color::RED, linewidth);
        //if (bWritePoints) svg.writePoints(shpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg::Polygons orpolys = convertPolygons(datas.orsh);
        svg.writePolygons(orpolys, svg::SVG::Color::GREEN, linewidth);
        //if (bWritePoints) svg.writePoints(orpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg::Polygons nfppolys = convertPolygons(datas.nfps);
        svg.writePolygons(nfppolys, svg::SVG::Color::BLUE, linewidth);
        //if (bWritePoints) svg.writePoints(nfppolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg::Point point(datas.p.X, datas.p.Y);
        svg.writePoint(point, true, 1.0, svg::SVG::Color::BLUE);
    }

    void savePaths(const Clipper3r::Path& path1, const Clipper3r::Path& path2, const std::string& filename, bool bWritePoints = false)
    {
        /*std::string filepath = "D:/test";
        std::ifstream file(filepath);
        if (!file.good()) return;*/
        std::string svgFile = filename + ".svg";
        svg::Polygons binpolys;
        binpolys.paths.emplace_back(path1);
        binpolys.paths.emplace_back(path2);
        svg::AABB aabb = getAABB(binpolys, true);
        svg::SVG svg(svgFile, aabb);
        svg.setFlipY(true);
        float linewidth = 0.5f;
        svg::Polygons spolys, epolys;
        spolys.paths.emplace_back(path1);
        epolys.paths.emplace_back(path2);
        svg.writePolygons(binpolys, svg::SVG::Color::BLACK, linewidth);
        //if (bWritePoints) svg.writePoints(binpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg.writePolygons(spolys, svg::SVG::Color::RED, linewidth);
        //if (bWritePoints) svg.writePoints(shpolys, true, 1.0, svg::SVG::Color::RAINBOW);

        svg.writePolygons(epolys, svg::SVG::Color::GREEN, linewidth);
        //if (bWritePoints) svg.writePoints(orpolys, true, 1.0, svg::SVG::Color::RAINBOW);
    }
};

template<class RawShape>
class SVGWriter {
    using Item = _Item<RawShape>;
    using Coord = TCoord<TPoint<RawShape>>;
    using Box = _Box<TPoint<RawShape>>;
    using PackGroup = _PackGroup<RawShape>;

public:

    enum OrigoLocation {
        TOPLEFT,
        BOTTOMLEFT
    };

    struct Config {
        OrigoLocation origo_location;
        Coord mm_in_coord_units;
        double width, height;
        Config():
            origo_location(BOTTOMLEFT), mm_in_coord_units(10000),
            width(500), height(500) {}

    };

private:
    Config conf_;
    std::vector<std::string> svg_layers_;
    bool finished_ = false;
public:

    SVGWriter(const Config& conf = Config()):
        conf_(conf) {}

    void setSize(const Box& box) {
        conf_.height = static_cast<double>(box.height()) /
                conf_.mm_in_coord_units;
        conf_.width = static_cast<double>(box.width()) /
                conf_.mm_in_coord_units;
    }
    
    void writeShape(RawShape tsh) {
        if(svg_layers_.empty()) addLayer();
        if(conf_.origo_location == BOTTOMLEFT) {
            auto d = static_cast<Coord>(
                std::round(conf_.height * conf_.mm_in_coord_units) / 2.0);
            auto w = static_cast<Coord>(
                std::round(conf_.width * conf_.mm_in_coord_units) / 2.0);
            auto& contour = shapelike::contour(tsh);
            for (auto& v : contour) {
                setY(v, -getY(v) + d);
                setX(v, getX(v) + w);
            }
            
            auto& holes = shapelike::holes(tsh);
            for (auto& h : holes) {
                for (auto& v : h) {
                    setY(v, -getY(v) + d);
                    setX(v, getX(v) + w);
                }
            }
        }
        currentLayer() +=
            shapelike::serialize<Formats::SVG>(tsh,
                                               1.0 / conf_.mm_in_coord_units) +
            "\n";
    }

    void writeItem(const Item& item) {
        const Box& bin = item.boundingBox();
        const Coord& off = std::min(bin.width(), bin.height());
        const RawShape& rs = item.transformedShape();
        Item cpy(rs); cpy.inflate(off);
        const Box& box = cpy.boundingBox();
        setSize(box);
        writeShape(item.transformedShape());
    }

    void writePackGroup(const PackGroup& result) {
        for(auto r : result) {
            addLayer();
            for(Item& sh : r) {
                writeItem(sh);
            }
            finishLayer();
        }
    }

    void addLayer() {
        svg_layers_.emplace_back(header());
        finished_ = false;
    }

    void finishLayer() {
        currentLayer() += "\n</svg>\n";
        finished_ = true;
    }

    void save(const std::string& filepath) {
        size_t lyrc = svg_layers_.size() > 1? 1 : 0;
        size_t last = svg_layers_.size() > 1? svg_layers_.size() : 0;

        for(auto& lyr : svg_layers_) {
            std::fstream out(filepath + (lyrc > 0? std::to_string(lyrc) : "") +
                             ".svg", std::fstream::out);
            if(out.is_open()) out << lyr;
            if(lyrc == last && !finished_) out << "\n</svg>\n";
            out.flush(); out.close(); lyrc++;
        };
    }

private:

    std::string& currentLayer() { return svg_layers_.back(); }

    const std::string header() const {
        std::string svg_header =
R"raw(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.0//EN" "http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd">
<svg height=")raw";
        svg_header += std::to_string(conf_.height) + "\" width=\"" + std::to_string(conf_.width) + "\" ";
        svg_header += R"raw(xmlns="http://www.w3.org/2000/svg" xmlns:svg="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">)raw";
        return svg_header;
    }

};

}
}

#endif // SVGTOOLS_HPP
