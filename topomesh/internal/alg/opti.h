#pragma once
#include "trimesh2/TriMesh.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"
#include "trimesh2/quaternion.h"


namespace topomesh {
	float creatObjectFun(trimesh::TriMesh* mesh);
	void descent(trimesh::TriMesh* mesh, float rate, float iterations=50);
	void gradient();
	void PSO();
	void GA();
}