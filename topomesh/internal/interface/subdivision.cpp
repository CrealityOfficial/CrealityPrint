#include "topomesh/interface/subdivision.h"

#include "msbase/mesh/dumplicate.h"
#include "trimesh2/TriMesh_algo.h"

#include <set>

namespace topomesh {
	void SimpleMidSubdivision(trimesh::TriMesh* mesh, std::vector<int>& faceindexs)
	{		
		for (int fi : faceindexs)
		{			
			trimesh::point c = (mesh->vertices[mesh->faces[fi][0]] + mesh->vertices[mesh->faces[fi][1]] + mesh->vertices[mesh->faces[fi][2]]) / 3.f;
			mesh->vertices.push_back(c);
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][0], mesh->faces[fi][1], mesh->vertices.size() - 1));
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][1], mesh->faces[fi][2], mesh->vertices.size() - 1));
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][2], mesh->faces[fi][0], mesh->vertices.size() - 1));
		}
		std::vector<bool> deletefaces(mesh->faces.size(), false);
		for (int fi : faceindexs)		
			deletefaces[fi] = true;
		trimesh::remove_faces(mesh, deletefaces);
		
	}

	void loopSubdivision(trimesh::TriMesh* mesh, std::vector<int>& faceindexs,std::vector<int>& outfaces)
	{
		mesh->need_across_edge();
		std::vector<bool> is_visit(mesh->faces.size(), false);
		std::vector<bool> is_subdiv(mesh->faces.size(), false);
		for (int fi : faceindexs)
			is_subdiv[fi] = true;
		std::vector<int> out(mesh->faces.size(), 0);
		for (int fi : faceindexs)
		{
			for (int i = 0; i < mesh->across_edge[fi].size(); i++)
			{
				if (mesh->across_edge[fi][i] == -1) continue;
				if (!is_subdiv[mesh->across_edge[fi][i]])
				{
					out[mesh->across_edge[fi][i]]++;
				}
			}
			trimesh::point v1 = (mesh->vertices[mesh->faces[fi][0]] + mesh->vertices[mesh->faces[fi][1]]) / 2.f;
			trimesh::point v2 = (mesh->vertices[mesh->faces[fi][1]] + mesh->vertices[mesh->faces[fi][2]]) / 2.f;
			trimesh::point v3 = (mesh->vertices[mesh->faces[fi][2]] + mesh->vertices[mesh->faces[fi][0]]) / 2.f;
			mesh->vertices.push_back(v1);
			mesh->vertices.push_back(v2);
			mesh->vertices.push_back(v3);
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->vertices.size()-3, mesh->vertices.size()-2, mesh->vertices.size()-1));
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][0], mesh->vertices.size() - 3, mesh->vertices.size() - 1));
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][1], mesh->vertices.size() - 2, mesh->vertices.size() - 3));
			mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[fi][2], mesh->vertices.size() - 1, mesh->vertices.size() - 2));
			outfaces.push_back(mesh->faces.size() - 1);
			outfaces.push_back(mesh->faces.size() - 2);
			outfaces.push_back(mesh->faces.size() - 3);
			outfaces.push_back(mesh->faces.size() - 4);
		}	
		std::vector<int> outindex;
		for (int i = 0; i < out.size(); i++)
		{
			if(out[i]>0)
				outindex.push_back(i);
			if (out[i] == 1)
			{
				int ei = 0;
				for (; ei < mesh->across_edge[i].size(); ei++)
					if (mesh->across_edge[i][ei]!=-1&&is_subdiv[mesh->across_edge[i][ei]])
						break;
				mesh->vertices.push_back((mesh->vertices[mesh->faces[i][ei+1]] + mesh->vertices[mesh->faces[i][(ei + 2) % 3]]) / 2.f);
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][ei], mesh->faces[i][(ei + 1) % 3],mesh->vertices.size()-1));
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][ei], mesh->vertices.size() - 1, mesh->faces[i][(ei + 2) % 3]));
			}
			else if(out[i]==2)
			{
				int ei = 0;
				for (; ei < mesh->across_edge[i].size(); ei++)
					if (mesh->across_edge[i][ei] != -1 && !is_subdiv[mesh->across_edge[i][ei]])
						break;
				int next = (ei + 1) % 3;
				int pri = (ei + 2) % 3;
				mesh->vertices.push_back((mesh->vertices[mesh->faces[i][ei]]+mesh->vertices[mesh->faces[i][next]])/2.f);
				mesh->vertices.push_back((mesh->vertices[mesh->faces[i][ei]] + mesh->vertices[mesh->faces[i][pri]]) / 2.f);
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][ei], mesh->vertices.size() - 2,mesh->vertices.size()-1));
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->vertices.size() - 1, mesh->vertices.size() - 2, mesh->faces[i][next]));
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][next], mesh->faces[i][pri], mesh->vertices.size() - 1));
			}
			else if (out[i] == 3)
			{
				mesh->vertices.push_back((mesh->vertices[mesh->faces[i][0]] + mesh->vertices[mesh->faces[i][1]]) / 2.f);
				mesh->vertices.push_back((mesh->vertices[mesh->faces[i][1]] + mesh->vertices[mesh->faces[i][2]]) / 2.f);
				mesh->vertices.push_back((mesh->vertices[mesh->faces[i][2]] + mesh->vertices[mesh->faces[i][0]]) / 2.f);
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->vertices.size()-3, mesh->vertices.size() - 2, mesh->vertices.size() - 1));
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][0], mesh->vertices.size() - 3, mesh->vertices.size() - 1));
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][1], mesh->vertices.size() - 2, mesh->vertices.size() - 3));
				mesh->faces.push_back(trimesh::TriMesh::Face(mesh->faces[i][2], mesh->vertices.size() - 1, mesh->vertices.size() - 2));
			}
		}
		std::vector<bool> deletefaces(mesh->faces.size(), false);
		for (int fi : faceindexs)
			deletefaces[fi] = true;
		for (int fi : outindex)
			deletefaces[fi] = true;
		for (int &fi : outfaces)
		{
			fi -= (faceindexs.size()+outindex.size());
		}
		trimesh::remove_faces(mesh, deletefaces);
		trimesh::remove_unused_vertices(mesh);
		msbase::dumplicateMesh(mesh);
	}
}