#include "topomesh/interface/concave.h"
#include "internal/polygon/slicehelper.h"

namespace topomesh
{
	void meshConcave(trimesh::TriMesh* mesh, std::vector<trimesh::vec3>& concave,
		const trimesh::quaternion& rotation, const trimesh::vec3& scale)
	{
		concave.clear();

		if (mesh)
		{
			SliceHelper helper;
			helper.prepare(mesh);
			helper.generateConcave(concave, &rotation, scale);
		}
	}
}