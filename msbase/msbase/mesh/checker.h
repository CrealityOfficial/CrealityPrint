#ifndef MSBASE_CHECKER_1691396111581_H
#define MSBASE_CHECKER_1691396111581_H
#include "msbase/interface.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	MSBASE_API bool checkDegenerateFace(trimesh::TriMesh* mesh, bool remove = false);

	MSBASE_API void checkLargetPlanar(trimesh::TriMesh* mesh, const std::vector<trimesh::vec3>& normals, const std::vector<float>& areas, float threshold,
		/*out*/std::vector<int>& faces);
}

#endif // MSBASE_CHECKER_1691396111581_H