#include "topomesh/interface/hex.h"

#include "internal/data/const.h"
#include "internal/alg/fillhoneycombs.h"

namespace topomesh
{
    enum class HexDirection :int {
        NO_NEIGHBOR = -1,
        X_NEGATIVEZ = 0, ///<(1,0,-1)
        Y_NEGATIVEZ = 1, ///<(0,1,-1)
        Y_NEGATIVEX = 2, ///<(-1,1,0)
        Z_NEGATIVEX = 3, ///<(-1,0,1)
        Z_NEGATIVEY = 4, ///<(0,-1,1)
        X_NEGATIVEY = 5, ///<(1,-1,0)
    };
    HexDirection hex_neighbor(const trimesh::ivec3& va, const trimesh::ivec3& vb)
    {
        const trimesh::ivec3& hex = vb - va;
        const int dist = int(abs(hex.x) + abs(hex.y) + abs(hex.z));
        if (dist == 2) {
            if (hex.x == 1) {
                if (hex.y == 0) return HexDirection::X_NEGATIVEZ;
                else return HexDirection::X_NEGATIVEY;
            }
            else if (hex.y == 1) {
                if (hex.x == 0) return HexDirection::Y_NEGATIVEZ;
                else return HexDirection::Y_NEGATIVEX;
            }
            else if (hex.z == 1) {
                if (hex.x == 0) return HexDirection::Z_NEGATIVEY;
                else return HexDirection::Z_NEGATIVEX;
            }
        }
        return HexDirection::NO_NEIGHBOR;
    };

    void translateTriPolygon(TriPolygon& poly, const trimesh::vec3& trans)
    {
        for (auto& p : poly) {
            p += trans;
        }
    }
    TriPolygons convertFromHexaPolygons(const HexaPolygons& hexas)
    {
        TriPolygons polys;
        polys.reserve(hexas.polys.size());
        for (const auto& h : hexas.polys) {
            polys.emplace_back(std::move(h.poly));
        }
        return polys;
    }
    HexaPolygons convertFromTriPolygons(const TriPolygons& polys, const trimesh::ivec3& coords)
    {
        HexaPolygons hexas;
        hexas.polys.reserve(polys.size());
        const int size = polys.size();
        if (polys.size() == coords.size()) {
            for (int i = 0; i < size; ++i) {
                hexas.polys.emplace_back();
                auto& hexa = hexas.polys.back();
                hexa.poly = polys[i];
                hexa.coord = coords[i];
            }
        } else {
            for (int i = 0; i < size; ++i) {
                hexas.polys.emplace_back();
                auto& hexa = hexas.polys.back();
                hexa.poly = polys[i];
            }
        }
        return hexas;
    }

    trimesh::TriMesh SaveTriPolygonToMesh(const TriPolygon& poly, double r, size_t nslices)
    {
        trimesh::TriMesh edgeMesh;
        const size_t polysize = poly.size();
        std::vector<trimesh::vec3> points;
        std::vector<trimesh::ivec3> faces;
        points.reserve(2 * polysize * nslices);
        faces.reserve(2 * polysize * nslices);
        double delta = 2.0 * M_PI / nslices;
        trimesh::vec3 z(0, 0, 1);
        for (size_t i = 0; i < polysize; ++i) {
            const auto& a = poly[i];
            const auto& b = poly[(i + 1) % polysize];
            const auto& n = trimesh::normalized(b - a);
            auto && x = std::move(z.cross(n));
            if (trimesh::len(x) < EPS) {
                x = trimesh::vec3(1, 0, 0);
            }
            trimesh::normalize(x);
            const auto& y = n.cross(x);
            for (int j = 0; j < nslices; ++j) {
                const auto& theta = delta * j;
                const auto& p = b + x * r * std::cos(theta) + y * r * std::sin(theta);
                points.emplace_back(p);
            }
            for (int j = 0; j < nslices; ++j) {
                const auto& theta = delta * j;
                const auto& p = a + x * r * std::cos(theta) + y * r * std::sin(theta);
                points.emplace_back(p);
            }
        }
        for (size_t i = 0; i < polysize; ++i) {
            for (size_t j = 0; j < nslices; ++j) {
                const auto& i0 = j + 2 * nslices * i;
                const auto& i1 = (j + 1) % nslices + 2 * nslices * i;
                const auto & j0 = i0 + nslices;
                const auto & j1 = i1 + nslices;
                faces.emplace_back(j0, i1, i0);
                faces.emplace_back(j0, j1, i1);
            }
        }
        edgeMesh.faces.swap(faces);
        edgeMesh.vertices.swap(points);
        return edgeMesh;
    }
    trimesh::TriMesh SaveTriPolygonsToMesh(const TriPolygons & polys, double r, size_t nslices)
    {
        trimesh::TriMesh edgeMesh;
        size_t pointnums = 0;
        for (int i = 0; i < polys.size(); ++i) {
            pointnums += polys[i].size();
        }
        std::vector<trimesh::vec3> points;
        std::vector<trimesh::ivec3> faces;
        points.reserve(2 * pointnums * nslices);
        faces.reserve(2 * pointnums * nslices);
        double delta = 2.0 * M_PI / nslices;
        trimesh::vec3 z(0, 0, 1);
        int start = 0;
        for (size_t k = 0; k < polys.size(); ++k) {
            const auto& poly = polys[k];
            const int polysize = poly.size();
            for (size_t i = 0; i < polysize; ++i) {
                const auto& a = poly[i];
                const auto& b = poly[(i + 1) % polysize];
                const auto& n = trimesh::normalized(b - a);
                auto&& x = std::move(z.cross(n));
                if (trimesh::len(x) < EPS) {
                    x = trimesh::vec3(1, 0, 0);
                }
                trimesh::normalize(x);
                const auto& y = n.cross(x);
                for (int j = 0; j < nslices; ++j) {
                    const auto& theta = delta * j;
                    const auto& p = b + x * r * std::cos(theta) + y * r * std::sin(theta);
                    points.emplace_back(p);
                }
                for (int j = 0; j < nslices; ++j) {
                    const auto& theta = delta * j;
                    const auto& p = a + x * r * std::cos(theta) + y * r * std::sin(theta);
                    points.emplace_back(p);
                }
            }
            for (size_t i = 0; i < polysize; ++i) {
                for (size_t j = 0; j < nslices; ++j) {
                    const auto& i0 = start + j + 2 * nslices * i;
                    const auto& i1 = start + (j + 1) % nslices + 2 * nslices * i;
                    const auto & j0 = i0 + nslices;
                    const auto & j1 = i1 + nslices;
                    faces.emplace_back(j0, i1, i0);
                    faces.emplace_back(j0, j1, i1);
                }
            }
            start = points.size();
        }
        edgeMesh.faces.swap(faces);
        edgeMesh.vertices.swap(points);
        return edgeMesh;
    }
    trimesh::TriMesh SaveTriPolygonPointsToMesh(const TriPolygon& poly, double radius, size_t nrows, size_t ncolumns)
    {
        trimesh::TriMesh pointMesh;
        const size_t spherenums = poly.size();
        const size_t nums = nrows * ncolumns + 2;
        const size_t pointnums = nums * spherenums;
        const size_t facenums = 2 * (nums - 2) * spherenums;
        std::vector<trimesh::vec3> points;
        points.reserve(pointnums);
        std::vector<trimesh::ivec3> faces;
        faces.reserve(facenums);
        for (int k = 0; k < spherenums; ++k) {
            const auto& p = poly[k];
            points.emplace_back(p.x, p.y, p.z + radius);
            for (int i = 0; i < nrows; ++i) {
                const auto& phi = M_PI * (i + 1.0) / double(nrows + 1.0);
                const auto& z = radius * std::cos(phi);
                const auto& r = radius * std::sin(phi);
                for (int j = 0; j < ncolumns; ++j) {
                    const auto& theta = 2.0 * M_PI * j / ncolumns;
                    const auto& x = r * std::cos(theta);
                    const auto& y = r * std::sin(theta);
                    points.emplace_back(p.x + x, p.y + y, p.z + z);
                }
            }
            points.emplace_back(p.x, p.y, p.z - radius);
            const auto& maxInx = points.size() - 1;
            const auto& v0 = k * nums;
            //上下底部两部分
            for (size_t i = 0; i < ncolumns; ++i) {
                const auto& i0 = i + 1 + v0;
                const auto& i1 = (i + 1) % ncolumns + 1 + v0;
                faces.emplace_back(trimesh::ivec3(v0, i0, i1));
                const auto& j0 = i0 + (nrows - 1) * ncolumns;
                const auto& j1 = i1 + (nrows - 1) * ncolumns;
                faces.emplace_back(trimesh::ivec3(j1, j0, maxInx));
            }
            //中间部分
            for (size_t i = 0; i < nrows - 1; ++i) {
                const auto& j0 = i * ncolumns + 1 + v0;
                const auto& j1 = (i + 1) * ncolumns + 1 + v0;
                for (size_t j = 0; j < ncolumns; ++j) {
                    const auto& i0 = j0 + j;
                    const auto& i1 = j0 + (j + 1) % ncolumns;
                    const auto& i2 = j1 + j;
                    const auto& i3 = j1 + (j + 1) % ncolumns;
                    faces.emplace_back(trimesh::ivec3(i2, i1, i0));
                    faces.emplace_back(trimesh::ivec3(i2, i3, i1));
                }
            }
        }
        pointMesh.vertices.swap(points);
        pointMesh.faces.swap(faces);
        return pointMesh;
    }

    trimesh::TriMesh SaveTriPolygonsPointsToMesh(const TriPolygons& polys, double radius, size_t nrows, size_t ncolumns)
    {
        trimesh::TriMesh pointMesh;
        TriPolygon poly;
        size_t spherenums = 0;
        for (const auto& pys : polys) {
            spherenums += pys.size();
        }
        poly.reserve(spherenums);
        for (const auto& pys : polys) {
            for (const auto& p : pys) {
                poly.emplace_back(p);
            }
        }
        return SaveTriPolygonPointsToMesh(poly, radius, nrows, ncolumns);
    }

    TriPolygons traitCurrentPolygons(const HexaPolygons& hexas, int index)
    {
        topomesh::TriPolygons polys;
        if (index >= 0 && index < (hexas.polys.size())) {
            polys.emplace_back(hexas.polys[index].poly);
        }
        return polys;
    }

    TriPolygons traitNeighborPolygons(const HexaPolygons& hexas, int index)
    {
        topomesh::TriPolygons polys;
        if (index >= 0 && index < (hexas.polys.size())) {
            const auto& edges = hexas.polys[index].edges;
            int nums = edges.size();
            polys.reserve(nums);
            for (int i = 0; i < nums; ++i) {
                if (edges[i].relate >= 0)
                    polys.emplace_back(hexas.polys[edges[i].relate].poly);
            }
        }
        return polys;
    }

    TriPolygons traitDirctionPolygon(const HexaPolygons& hexas, int index, int dir)
    {
        topomesh::TriPolygons polys;
        if (index >= 0 && index < (hexas.polys.size())) {
            if (dir >= 0 && dir <= 5) {
                const auto& edges = hexas.polys[index].edges;
                int val = edges[dir].relate;
                if (val >= 0) {
                    polys.emplace_back(hexas.polys[val].poly);
                }
            }
        }
        return polys;
    }

    HexaPolygons GenerateHexagonsGridArray(const HexagonArrayParam& hexagonparams)
    {
        const float radius = hexagonparams.radius;
        const float nestWidth = hexagonparams.nestWidth;
        const float xdelta = 3.0 / 2.0 * radius;
        const float ydelta = SQRT3 / 2.0 * radius;
        const float side = radius - nestWidth / SQRT3;
        const size_t nrows = hexagonparams.nrows;
        const size_t ncols = hexagonparams.ncols;
        const auto& p0 = hexagonparams.pos;
        std::vector<std::vector<trimesh::point>> gridPoints;
        for (size_t i = 0; i < 2 * nrows - 1; ++i) {
            std::vector<trimesh::point> crossPts;
            for (size_t j = 0; j < ncols; ++j) {
                const auto& pt = trimesh::vec3(p0.x + xdelta * j, p0.y - ydelta * i, p0.z);
                crossPts.emplace_back(pt);
            }
            gridPoints.emplace_back(crossPts);
        }
        const size_t nums = (2 * nrows - 1) * ncols;
        HexaPolygons polygons;
        polygons.polys.reserve(nums);
        polygons.side = side;
        //计算六角网格对应边界包围盒坐标奇偶性质
        for (int i = 0; i < 2 * nrows - 1; i += 2) {
            const auto& rowPoints = gridPoints[i];
            for (int j = 0; j < ncols; ++j) {
                const auto& pt = rowPoints[j];
                if (j % 2 == 0) {
                    const auto& center = trimesh::vec3(pt.x, pt.y, 0);
                    topomesh::Hexagon hexagon(center, side);
                    const auto& border = hexagon.border;
                    HexaPolygon hexa;
                    hexa.center = hexagon.centroid;
                    hexa.poly.reserve(border.size());
                    for (const auto& p : border) {
                        hexa.poly.emplace_back(trimesh::vec3((float)p.x, (float)p.y, p0.z));
                    }
                    hexa.coord = trimesh::ivec3(j, -(i + j) / 2, (i - j) / 2);
                    polygons.polys.emplace_back(std::move(hexa));
                }
                else {
                    const auto& center = trimesh::vec3(pt.x, (double)pt.y - ydelta, 0);
                    topomesh::Hexagon hexagon(center, side);
                    const auto& border = hexagon.border;
                    HexaPolygon hexa;
                    hexa.center = hexagon.centroid;
                    hexa.poly.reserve(border.size());
                    for (const auto& p : border) {
                        hexa.poly.emplace_back(trimesh::vec3((float)p.x, (float)p.y, p0.z));
                    }
                    hexa.coord = trimesh::ivec3(j, -(i + 1 + j) / 2, (i + 1 - j) / 2);
                    polygons.polys.emplace_back(std::move(hexa));
                }
            }
        }
        return polygons;
    }

    void GenerateHexagonNeighbors(HexaPolygons& hexas, const ColumnarHoleParam& param)
    {
        const int nums = hexas.polys.size();
        for (int i = 0; i < nums; ++i) {
            if (hexas.polys[i].standard) {
                for (int j = 0; j < 6; ++j) {
                    hexas.polys[i].h2pPointMap.emplace(j, j);
                    hexas.polys[i].p2hPointMap.emplace(j, j);
                    hexas.polys[i].h2pEdgeMap.emplace(j, j);
                    hexas.polys[i].p2hEdgeMap.emplace(j, j);
                }
            }
            if (hexas.polys[i].edges.size() == 0) hexas.polys[i].edges.resize(6);
        }
        std::vector<std::vector<int>> neighborhoods(nums, std::vector<int>(nums, 1));
        std::vector<std::vector<int>> associatehoods(nums, std::vector<int>(nums, 1));
        for (int i = 0; i < nums; ++i) {
            for (int j = 0; j < nums; ++j) {
                //更新完整边对应关系
                if (neighborhoods[i][j] && (j != i)) {
                    const auto& hexa = hexas.polys[i];
                    const auto& hexb = hexas.polys[j];
                    int res = int(hex_neighbor(hexa.coord, hexb.coord));
                    if (res >= 0) {
                        const auto& h2pmap1 = hexa.h2pEdgeMap;
                        auto itr1 = h2pmap1.find(res); ///<当前线段对应六角网格边的索引
                        if (itr1 != h2pmap1.end()) {
                            const auto& h2pmap2 = hexb.h2pEdgeMap;
                            const int inx = (res + 3) % 6; ///<临近线段对应六角网格边的索引
                            const auto& itr2 = h2pmap2.find(inx);
                            if (itr2 != h2pmap2.end()) {
                                hexas.polys[i].edges[itr1->second].neighbor = j;
                                hexas.polys[j].edges[itr2->second].neighbor = i;
                                neighborhoods[i][j] = 0;
                                neighborhoods[j][i] = 0;
                            }
                        }
                    }
                }
            }
        }
        for (int i = 0; i < nums; ++i) {
            for (int j = 0; j < nums; ++j) {
                //更新裁剪边对应关系
                if (associatehoods[i][j] && (neighborhoods[i][j]) && (j != i)) {
                    const auto& hexa = hexas.polys[i];
                    const auto& hexb = hexas.polys[j];
                    int res = int(hex_neighbor(hexa.coord, hexb.coord));
                    if (res >= 0) {
                        const auto& h2pmap1 = hexa.h2pPointMap;
                        auto itr1 = h2pmap1.find(res); ///<当前线段起始点为六角网格网格顶点
                        if (itr1 != h2pmap1.end()) {
                            const auto& h2pmap2 = hexb.h2pPointMap;
                            const int inx = (res + 3 + 1) % 6; ///<关联线段终点为六角网格网格顶点
                            const auto& itr2 = h2pmap2.find(inx);
                            if (itr2 != h2pmap2.end()) {
                                const int sizeb = hexb.poly.size();
                                hexas.polys[i].edges[itr1->second].associate = j;
                                hexas.polys[j].edges[(itr2->second + sizeb - 1) % sizeb].associate = i;
                                associatehoods[i][j] = 0;
                                associatehoods[j][i] = 0;
                            }
                        }
                    }
                }
            }
        }
        //更新内外轮廓之间的网格相邻关系
        for (int i = 0; i < nums; ++i) {
            for (int j = 0; j < nums; ++j) {
                if (associatehoods[i][j] && (neighborhoods[i][j]) && (j != i)) {
                    const auto& hexa = hexas.polys[i];
                    const auto& hexb = hexas.polys[j];
                    int res = int(hex_neighbor(hexa.coord, hexb.coord));
                    if (res >= 0) {
                        const auto& h2pmap1 = hexa.h2pPointMap;
                        auto itr1 = h2pmap1.find(res); ///<当前线段起始点为六角网格网格顶点
                        if (itr1 != h2pmap1.end()) {
                            const auto& h2pmap2 = hexb.h2pPointMap;
                            const int inx = (res + 3) % 6; ///<关联线段始点为六角网格网格顶点
                            const auto& itr2 = h2pmap2.find(inx);
                            if (itr2 != h2pmap2.end()) {
                                const int sizeb = hexb.poly.size();
                                hexas.polys[i].edges[itr1->second].associate = j;
                                hexas.polys[j].edges[itr2->second].associate = i;
                                associatehoods[i][j] = 0;
                                associatehoods[j][i] = 0;
                            }
                        }
                    }
                    if (res >= 0) {
                        const auto& h2pmap1 = hexa.h2pPointMap;
                        auto itr1 = h2pmap1.find((res + 1) % 6); ///<当前线段终点点为六角网格网格顶点
                        if (itr1 != h2pmap1.end()) {
                            const auto& h2pmap2 = hexb.h2pPointMap;
                            const int inx = (res + 3 + 1) % 6; ///<关联线终点为六角网格网格顶点
                            const auto& itr2 = h2pmap2.find(inx);
                            if (itr2 != h2pmap2.end()) {
                                const int sizea = hexa.poly.size();
                                const int sizeb = hexb.poly.size();
                                hexas.polys[i].edges[(itr1->second + sizea - 1) % sizea].associate = j;
                                hexas.polys[j].edges[(itr2->second + sizeb - 1) % sizeb].associate = i;
                                associatehoods[i][j] = 0;
                                associatehoods[j][i] = 0;
                            }
                        }
                    }
                }
            }
        }
        for (int i = 0; i < nums; ++i) {
            const int size = hexas.polys[i].poly.size();
            for (int j = 0; j < size; ++j) {
                const int associate = hexas.polys[i].edges[j].associate;
                const int neighbor = hexas.polys[i].edges[j].neighbor;
                hexas.polys[i].edges[j].relate = associate < 0 ? neighbor : associate;
            }
        }
        const float cradius = hexas.side * param.ratio * 0.5f;
        const float cdelta = param.delta + 2 * cradius;
        const float cheight = param.height + cradius;
        float cmax = param.height + 2 * cradius;
        //更新六棱柱每个可加孔洞侧面最低点最高点坐标值
        for (auto& hexa : hexas.polys) {
            const int polysize = hexa.poly.size();
            for (int j = 0; j < polysize; ++j) {
                const int res = hexa.p2hEdgeMap[j];
                if (hexa.edges[j].neighbor >= 0 && (!hexa.edges[j].canAdd) && (res >= 0)) {
                    auto& edge = hexa.edges[j];
                    auto& next = hexa.edges[(j + 1) % polysize];
                    auto& oh = hexas.polys[hexa.edges[j].neighbor];
                    const auto& h2pmap = oh.h2pEdgeMap;
                    const int inx = (res + 3) % 6;
                    auto itr = h2pmap.find(inx);
                    if (itr != h2pmap.end()) {
                        const int ind = itr->second;
                        auto& oe = oh.edges[ind];
                        const int osize = oh.poly.size();
                        auto& onext = oh.edges[(ind + 1) % osize];
                        edge.lowHeight = std::max(edge.lowHeight, next.lowHeight);
                        edge.topHeight = std::min(edge.topHeight, next.topHeight);
                        oe.lowHeight = std::max(oe.lowHeight, onext.lowHeight);
                        oe.topHeight = std::min(oe.topHeight, onext.topHeight);
                        if (param.holeConnect) {
                            float lowerlimit = std::max(edge.lowHeight, oe.lowHeight);
                            float upperlimit = std::min(edge.topHeight, oe.topHeight);
                            float medium = (lowerlimit + upperlimit) * 0.5;
                            float scope = (upperlimit - lowerlimit) * 0.5;
                            if (medium < cheight && (cmax - medium) < scope) {
                                hexa.edges[j].canAdd = true;
                                oh.edges[ind].canAdd = true;
                            }
                            else if (medium >= cheight) {
                                float appro = std::round((medium - cheight) / cdelta);
                                float dist = std::abs((medium - cheight) - cdelta * appro);
                                if (dist + cradius < scope) {
                                    hexa.edges[j].canAdd = true;
                                    oh.edges[ind].canAdd = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return;
    }

    HexaPolygons generateEmbedHolesHexa(trimesh::TriMesh* mesh)
    {
        topomesh::HoneyCombParam combParam;
        combParam.honeyCombRadius = 3.0f;
        combParam.shellThickness = 2.0f;
        combParam.nestWidth = 2.0f;
        return topomesh::generateEmbedHolesColumnar(mesh, combParam);
    }
}