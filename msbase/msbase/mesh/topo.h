#ifndef MSBASE_TOPO_1695108550958_H
#define MSBASE_TOPO_1695108550958_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
    struct EEdge {
        int a, b; ///< a < b
        EEdge(int a0, int b0) :a(a0), b(b0) {}
        inline bool operator<(const EEdge& e) const
        {
            return a < e.a || (a == e.a && b < e.b);
        };
        inline bool operator==(const EEdge& e) const
        {
            return a == e.a && b == e.b;
        };
    };

    MSBASE_API void generateFaceEdgeAdjacency(trimesh::TriMesh* mesh, 
        std::vector<EEdge>& edges, std::vector<std::vector<int>>& faceEdges,
        std::vector<std::vector<int>>* edgeFaces = nullptr);
}

#endif // MSBASE_TOPO_1695108550958_H