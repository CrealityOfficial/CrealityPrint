#include "msbase/interface.h"
#include "trimesh2/quaternion.h"
#include "trimesh2/XForm.h"

namespace msbase
{
	MSBASE_API float angleOfVector3D2(const trimesh::vec3& v1, const trimesh::vec3& v2);
	MSBASE_API float radianOfVector3D2(const trimesh::vec3& v1, const trimesh::vec3& v2);

	MSBASE_API trimesh::vec3 triangleNormal(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3);
}