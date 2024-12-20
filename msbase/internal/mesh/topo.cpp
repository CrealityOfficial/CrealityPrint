#include "msbase/mesh/topo.h"
#include <unordered_map>

namespace msbase
{
    void generateFaceEdgeAdjacency(trimesh::TriMesh* mesh,
        std::vector<EEdge>& edges, std::vector<std::vector<int>>& faceEdges,
        std::vector<std::vector<int>>* edgeFaces)
    {
        if (!mesh)
            return;

        edges.clear();
        faceEdges.clear();

        const size_t facenums = mesh->faces.size();
        struct EEdgeHash {
            size_t operator()(const EEdge& e) const
            {
                return int((e.a * 99989)) ^ (int(e.b * 99991) << 2);
            }
        };
        struct EEdgeEqual {
            bool operator()(const EEdge& e1, const EEdge& e2) const
            {
                return e1.a == e2.a && e1.b == e2.b;
            }
        };
        std::unordered_map<EEdge, size_t, EEdgeHash, EEdgeEqual> edgeIndexMap;
        edges.reserve(3 * facenums);
        edgeIndexMap.reserve(3 * facenums);
        faceEdges.resize(facenums);
        for (size_t i = 0; i < facenums; ++i) {
            std::vector<int> elist;
            elist.reserve(3);
            auto& vertexs = mesh->faces[i];
            std::sort(vertexs.begin(), vertexs.end());
            for (size_t j = 0; j < 2; ++j) {
                EEdge e(vertexs[j], vertexs[j + 1]);
                const auto& itr = edgeIndexMap.find(e);
                if (itr != edgeIndexMap.end()) {
                    elist.emplace_back(itr->second);
                }
                if (itr == edgeIndexMap.end()) {
                    const size_t edgeIndex = edgeIndexMap.size();
                    edgeIndexMap.emplace(std::move(e), edgeIndex);
                    edges.emplace_back(std::move(e));
                    elist.emplace_back(edgeIndex);
                }
            }
            // 
            EEdge e(vertexs[0], vertexs[2]);
            const auto& itr = edgeIndexMap.find(e);
            if (itr != edgeIndexMap.end()) {
                elist.emplace_back(itr->second);
            }
            if (itr == edgeIndexMap.end()) {
                const size_t edgeIndex = edgeIndexMap.size();
                edgeIndexMap.emplace(std::move(e), edgeIndex);
                edges.emplace_back(std::move(e));
                elist.emplace_back(edgeIndex);
            }
            faceEdges[i].swap(elist);
        }
        if (edgeFaces) {
            const size_t edgenums = edges.size();
            edgeFaces->resize(edgenums);
            for (size_t i = 0; i < edgenums; ++i) {
                edgeFaces->at(i).reserve(2);
            }
            for (size_t i = 0; i < facenums; ++i) {
                const auto& es = std::move(faceEdges[i]);
                for (int j = 0; j < 3; ++j) {
                    edgeFaces->at(es[j]).emplace_back(i);
                }
            }
        }
    }
}