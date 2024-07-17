#include "topomesh/interface/poly.h"
#include "internal/polygon/conv1.h"

#include "msbase/mesh/tinymodify.h"
#include "msbase/mesh/topo.h"

namespace topomesh
{
	void simplifyPolygons(TriPolygons& polys)
	{
		ClipperLib::Paths paths;
		convertRaw(polys, paths);

        ClipperLib::Paths outPath;
        ClipperLib::Clipper clipper;
        for(const ClipperLib::Path& path : paths)
            clipper.AddPath(path, ClipperLib::ptSubject, true);
        clipper.Execute(ClipperLib::ctUnion, outPath, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

		ClipperLib::SimplifyPolygons(outPath);
		convertRaw(outPath, polys);
	}

	void generateEdgePolygons(trimesh::TriMesh* mesh, const std::vector<int>& removedFaces, TriPolygons& polys)
	{
        if (!mesh)
            return;

        //拷贝一份数据
        trimesh::TriMesh nMesh;
        nMesh.faces = mesh->faces;
        nMesh.vertices = mesh->vertices;
        //第3步，剪切掉底面得边界轮廓
        msbase::removeMeshFaces(&nMesh, removedFaces);
        
        std::vector<msbase::EEdge> edges;
        std::vector<std::vector<int>> faceEdges;
        std::vector<std::vector<int>> edgeFaces;
        msbase::generateFaceEdgeAdjacency(mesh, edges, faceEdges, &edgeFaces);

        std::vector<int> edgeIndexes;
        const size_t edgenums = edgeFaces.size();
        for (int i = 0; i < edgenums; ++i) {
            const auto& neighbor = edgeFaces[i];
            if (neighbor.size() == 1) {
                edgeIndexes.emplace_back(i);
            }
        }
        //第4步，底面轮廓边界所有边排序
        std::vector<std::vector<int>> sequentials;
        //cutMesh.GetSequentialPoints(edges, sequentials);
        // 
        //第5步，构建底面边界轮廓多边形
        polys.reserve(sequentials.size());
        for (const auto& seq : sequentials) {
            TriPolygon border;
            border.reserve(seq.size());
            for (const auto& v : seq) {
                const auto& p = mesh->vertices[v];
                border.emplace_back(p);
            }
            polys.emplace_back(border);
        }
	}
}