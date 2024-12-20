#include "opti.h"
#include <cmath>
namespace {
	float creatObjectFun(trimesh::TriMesh* mesh,trimesh::vec3 dir)
	{
		mesh->need_normals();
		float total_area = 0.f;
		for (int fi = 0; fi < mesh->faces.size(); fi++)
		{
			trimesh::vec3 v0(mesh->vertices[mesh->faces[fi][0]]);
			trimesh::vec3 v1(mesh->vertices[mesh->faces[fi][1]]);
			trimesh::vec3 v2(mesh->vertices[mesh->faces[fi][2]]);

			trimesh::vec3 c = (v1 - v0).cross(v2 - v0);
			trimesh::vec3 f = trimesh::normalized(c);
			float arc = f ^ dir;
			arc = arc >= 1.f ? 1.f : arc;
			arc = arc <= -1.f ? -1.f : arc;
			if (arc < 0.f)
				continue;
			float _area = sqrtf(c.sumsqr())/2.f;
			total_area += _area * arc;
		}
		return total_area;

	}

	void descent(trimesh::TriMesh* mesh,float rate,float iterations)
	{
		trimesh::vec3 dx = trimesh::vec3(0, 0, -1);
		float w = creatObjectFun(mesh, dx);
		for (int i = 1; i < iterations; i++)
		{
			
			//float loss= w
		}
	}

	

	void PSO()
	{

	}

	void GA()
	{

	}
}