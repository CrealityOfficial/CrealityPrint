#include "topomesh/interface/utils.h"
#include "internal/alg/earclipping.h"
#include "internal/data/CMesh.h"
#include "msbase/mesh/get.h"
#include <queue>

namespace topomesh
{
	TriPolygons GetOpenMeshBoundarys(trimesh::TriMesh* triMesh)
	{
		CMesh mesh(triMesh);
		TriPolygons polys;
		std::vector<int> edges;
		mesh.SelectIndividualEdges(edges);
		//CMesh edgeMesh = mesh.SaveEdgesToMesh(edges);
		//edgeMesh.WriteSTLFile("edges.stl");
		//第0步，底面轮廓边界所有边排序
		std::vector<std::vector<int>>sequentials;
        if (!mesh.GetSequentialPoints(edges, sequentials))
            return polys;
		//第1步，构建底面边界轮廓多边形
		polys.reserve(sequentials.size());
		const auto& points = mesh.mpoints;
		for (const auto& seq : sequentials) {
			TriPolygon border;
			border.reserve(seq.size());
			for (const auto& v : seq) {
				const auto& p = points[v];
				border.emplace_back(p);
			}
			polys.emplace_back(border);
		}
		return polys;
	}

	void findNeignborFacesOfSameAsNormal(trimesh::TriMesh* trimesh, int indicate, float angle_threshold, std::vector<int>& faceIndexs)
	{
		if (!trimesh)
			return;

		trimesh->need_across_edge();
		faceIndexs.push_back(indicate);		
		trimesh::point normal = msbase::getFaceNormal(trimesh, indicate);
		std::vector<int> vis(trimesh->faces.size(), 0);
		std::queue<int> facequeue;
		facequeue.push(indicate);
		while (!facequeue.empty())
		{
			vis[facequeue.front()] = 1;
			for (int i = 0; i < trimesh->across_edge[facequeue.front()].size(); i++)
			{
				int face = trimesh->across_edge[facequeue.front()][i];
				if (face==-1||vis[face]) continue;				
				trimesh::point nor = msbase::getFaceNormal(trimesh, face);
				float arc = nor ^ normal;
				arc = arc >= 1.f ? 1.f : arc;
				arc = arc <= -1.f ? -1.f : arc;
				float ang = std::acos(arc) * 180 / M_PI;
				if (ang < angle_threshold)
				{
					vis[face] = 1;
					facequeue.push(face);
					faceIndexs.push_back(face);
				}
			}
			facequeue.pop();
		}
	}

	void findBoundary(trimesh::TriMesh* trimesh)
	{

	}

	void triangulate(trimesh::TriMesh* trimesh, std::vector<int>& sequentials)
	{
		std::vector<std::pair<trimesh::point, int>> lines;
		for (int i = 0; i < sequentials.size(); i++)
		{
			lines.push_back(std::make_pair(trimesh->vertices[sequentials[i]], sequentials[i]));
		}
		topomesh::EarClipping earclip(lines);
		std::vector<trimesh::ivec3> result = earclip.getResult();
		for (int fi = 0; fi < result.size(); fi++)
		{
			trimesh->faces.push_back(result[fi]);
		}
	}
}