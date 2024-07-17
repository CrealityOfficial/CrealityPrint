#ifndef TOPOMESH_HEX_1692753685514_H
#define TOPOMESH_HEX_1692753685514_H

#include <map>
#include "topomesh/interface/idata.h"

namespace topomesh
{
    struct HexaEdge {
        int associate = -1; ///<边的综合相邻边索引(下面两种合并)，-1表示这条边没有关联关系
        int neighbor = -1; ///<完整边的相邻边索引，-1表示这条边没有关联关系
        int relate = -1; ///<裁剪的相邻边索引，-1表示这条边没有关联关系
        bool canAdd = false;
        bool hasAdd = false;
        bool addRect = false;
        bool addTriangle = false;
        int holenums = -1;
        bool bmutihole = false;

        std::vector<int> starts;
        int lowerHoles = 0;
        std::vector<int> holeIndexs;
        std::vector<int> pointIndexs;
        std::vector<int> corners;
        float lowHeight = 0.f;
        float topHeight = 10.f;
    };
    struct Hexagon {
        trimesh::vec3 centroid;
        float radius = 1;
        std::vector<trimesh::vec3> border;
        Hexagon(const trimesh::vec3& c, const float& d)
        {
            centroid = c, radius = d;
            const auto& theta = 2.0 * 3.1415926 / 6.0;
            for (int i = 0; i < 6; ++i) {
                const auto& phi = float(i) * theta;
                const auto& p = centroid + trimesh::vec3(std::cos(phi), std::sin(phi), 0) * radius;
                border.emplace_back(std::move(p));
            }
        }
    };
    struct HexaPolygon {
        bool standard = true; ///< 是否为完整的六边形结构(指原始的六边形的6条边未被裁剪，若被裁剪都是不完整的六边形结构)
        trimesh::vec3 center; ///< 裁剪前的六边形网格中心
        TriPolygon poly; ///<
        TriPolygon hexagon; ///<
        int startIndex = 0; ///< 六棱柱第一个点的索引
        trimesh::ivec3 coord; ///<三轴坐标系下的坐标
        std::vector<HexaEdge> edges; ///<所有的边结构
        std::map<int, int> p2hPointMap; /// poly->hexagon顶点索引映射
        std::map<int, int> h2pPointMap; /// hexagon->poly顶点索引映射
        std::map<int, int> p2hEdgeMap; ///  poly->hexagon边索引映射
        std::map<int, int> h2pEdgeMap; ///  hexagon->poly边索引映射
    };
    struct HexaPolygons {
        bool bSewTop = true; ///棱柱的顶部是否需要缝合
        bool bSewBottom = true; ///<棱柱的底部连接部分是否需要缝合
        float side = 0.0f; ///< 每个棱柱底面六角网格的边长
        std::vector<HexaPolygon> polys;
        std::vector<int> topfaces;
    };

    TOPOMESH_API void translateTriPolygon(TriPolygon& poly, const trimesh::vec3& trans);
    TOPOMESH_API TriPolygons convertFromHexaPolygons(const HexaPolygons& hexas);
    TOPOMESH_API HexaPolygons convertFromTriPolygons(const TriPolygons& polys, const trimesh::ivec3& coords);
    TOPOMESH_API trimesh::TriMesh SaveTriPolygonToMesh(const TriPolygon& poly, double r = 0.01, size_t nslices = 20);
    TOPOMESH_API trimesh::TriMesh SaveTriPolygonsToMesh(const TriPolygons& polys, double r = 0.01, size_t nslices = 20);
    TOPOMESH_API trimesh::TriMesh SaveTriPolygonPointsToMesh(const TriPolygon& poly, double radius = 0.01, size_t nrows = 20, size_t ncolumns = 20);
    TOPOMESH_API trimesh::TriMesh SaveTriPolygonsPointsToMesh(const TriPolygons& polys, double radius = 0.01, size_t nrows = 20, size_t ncolumns = 20);

    TOPOMESH_API TriPolygons traitCurrentPolygons(const HexaPolygons& hexas, int index);
    TOPOMESH_API TriPolygons traitNeighborPolygons(const HexaPolygons& hexas, int index);
    TOPOMESH_API TriPolygons traitDirctionPolygon(const HexaPolygons& hexas, int index, int dir);

    struct HexagonArrayParam {
        trimesh::vec dir = trimesh::vec3(0, 0, 1);
        trimesh::vec3 pos; ///<第一个六角网格的中心坐标
        int nrows = 2;
        int ncols = 2;
        double radius = 2.0; ///<每个六边形半径
        double nestWidth = 0.3; ///<相邻六边形网格的壁厚
    };

    TOPOMESH_API HexaPolygons GenerateHexagonsGridArray(const HexagonArrayParam& hexagonparams = HexagonArrayParam());

    struct ColumnarHoleParam {
        int nslices = 17; ///<圆孔截面圆采样为正17边形
        float height = 5.0f; ///<圆孔圆心距底面的高度
        float ratio = 0.5f; ///<圆孔截面圆半径
        float delta = 1.0f; ///<上下两层相邻圆孔的偏移距离
        bool holeConnect = true; ///<是否启用圆孔相连
    };

    TOPOMESH_API void GenerateHexagonNeighbors(HexaPolygons& hexas, const ColumnarHoleParam& param = ColumnarHoleParam());
    TOPOMESH_API TopoTriMeshPtr generateHolesColumnar(HexaPolygons& hexas, const ColumnarHoleParam& param);

    TOPOMESH_API HexaPolygons generateEmbedHolesHexa(trimesh::TriMesh* mesh);
}

#endif // TOPOMESH_HEX_1692753685514_H