#include "volumeMesh.h"
#include "msbase/mesh/get.h"

namespace topomesh {
	float getMeshTotalVolume(trimesh::TriMesh* mesh)
	{
		if (!mesh)
			return 0.0f;
		
		std::vector<int> indices;
		int size = (int)mesh->faces.size();
		for (int i = 0; i < size; ++i)
			indices.push_back(i);

		return getMeshVolume(mesh, indices);
	}

	float getMeshVolume(trimesh::TriMesh* mesh, std::vector<int>& faces)
	{
		float vol = 0.f;
		for (int fi = 0; fi < faces.size(); fi++)
		{
			trimesh::point v0 = mesh->vertices[mesh->faces[fi][0]];
			trimesh::point v1 = mesh->vertices[mesh->faces[fi][1]];
			trimesh::point v2 = mesh->vertices[mesh->faces[fi][2]];

			float v = (-v2.x * v1.y * v0.z + v1.x * v2.y * v0.z + v2.x * v0.y * v1.z - v0.x * v2.y * v1.z - v1.x * v0.y * v2.z + v0.x * v1.y * v2.z)/6.f;
			vol += v;
		}
		return vol;
	}

	//edge cutting born vertex to line
	float getPointCloudVolume(trimesh::TriMesh* mesh, std::vector<int>& vexter)
	{
		mesh->write("ori.ply");
		mesh->need_bbox();
		mesh->need_neighbors();
		mesh->need_adjacentfaces();
		float vol = 0.f;
		std::vector<bool> is_botm(mesh->faces.size(), false);
		for (int fi = 0; fi < mesh->faces.size(); fi++)
		{
			trimesh::point n(0, 0, 0);
			n = msbase::getFaceNormal(mesh, fi);
			if (std::abs(n.z - 1.f) < 1e-4f || std::abs(n.z + 1.f) < 1e-4f)
			{
				is_botm[fi] = true;
			}
		}
		const int N = 100;
		float span = (mesh->bbox.max.z - mesh->bbox.min.z) / (N * 1.f);

		std::vector<int> edge;		
		std::vector<std::vector<int>> vexter_container(N,std::vector<int>());

		for (int vi = 0; vi < mesh->vertices.size(); vi++)
		{
			float z = mesh->vertices[vi].z;
			int cen = (z - mesh->bbox.min.z) / span;
			if (cen >= N) cen = N-1;
			vexter_container[cen].push_back(vi);
		}
		trimesh::TriMesh* newmesh = new trimesh::TriMesh();
		for (int c = 0; c < vexter_container.size(); c++)
		{
			if (c == 10)
			{
				std::vector<int> faces;
				for (int cvi = 0; cvi < vexter_container[c].size(); cvi++)
				{
					int v = vexter_container[c][cvi];
					for (int vf = 0; vf < mesh->adjacentfaces[v].size(); vf++)
					{
						faces.push_back(mesh->adjacentfaces[v][vf]);
					}
				}

				for (int fi = 0; fi < faces.size(); fi++)
				{
					newmesh->vertices.push_back(trimesh::point(mesh->vertices[mesh->faces[faces[fi]][0]].x, mesh->vertices[mesh->faces[faces[fi]][0]].y, mesh->vertices[mesh->faces[faces[fi]][0]].z));
					newmesh->vertices.push_back(trimesh::point(mesh->vertices[mesh->faces[faces[fi]][1]].x, mesh->vertices[mesh->faces[faces[fi]][1]].y, mesh->vertices[mesh->faces[faces[fi]][1]].z));
					newmesh->vertices.push_back(trimesh::point(mesh->vertices[mesh->faces[faces[fi]][2]].x, mesh->vertices[mesh->faces[faces[fi]][2]].y, mesh->vertices[mesh->faces[faces[fi]][2]].z));
					newmesh->faces.push_back(trimesh::ivec3(3*fi,3*fi+1,3*fi+2));
				}
			}
		}
		newmesh->write("savenewmesh.ply");
		return vol;
	}
}