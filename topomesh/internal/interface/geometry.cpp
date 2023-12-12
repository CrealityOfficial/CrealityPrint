#include "topomesh/interface/geometry.h"
#include "internal/alg/volumeMesh.h"

namespace topomesh
{
	float quickCalculateVolume(trimesh::TriMesh* mesh)
	{
		if (!mesh)
			return 0.0f;

		float volume = getMeshTotalVolume(mesh);
		return volume;
	}
}