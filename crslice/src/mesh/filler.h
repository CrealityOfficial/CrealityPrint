#ifndef TRIMESH_FILLER_1604124753258_H
#define TRIMESH_FILLER_1604124753258_H
#include "trimesh2/Vec.h"

namespace crslice
{
	void fillCylinderSoup(trimesh::vec3* data, float radius1, const trimesh::vec3& center1,
		float radius2, const trimesh::vec3& center2, int num, float theta);
}

#endif // TRIMESH_FILLER_1604124753258_H