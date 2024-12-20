#include "msbase/space/utils.h"
#include "msbase/space/angle.h"

namespace msbase {

	trimesh::vec3 centerBox(const trimesh::vec3& center, const std::vector<trimesh::box3>& boxes, bool ignoreZ)
	{
		trimesh::box3 allBox;
		for (const trimesh::box3& abox : boxes)
		{
			allBox += abox;
		}
		trimesh::vec3 offset = center - allBox.center();
		if (ignoreZ)
		{
			offset.z = 0.0;
		}
		return offset;
	}

	trimesh::fxform layMatrixFromPositionNormal(const trimesh::vec3& position, const trimesh::vec3& normal, const trimesh::vec3& scale, const trimesh::vec3& originNormal)
	{
		trimesh::fxform matrix = trimesh::fxform::identity();

		float angle = radianOfVector3D2(normal, originNormal);
		trimesh::vec3 axis = trimesh::cross(originNormal, normal);
		trimesh::normalize(axis);

		trimesh::fxform scaleMatrix = trimesh::fxform::scale(scale.x, scale.y, scale.z);
		trimesh::fxform transMatrix = trimesh::fxform::trans(position);
		if (abs(angle) < 0.001)
		{
			matrix = transMatrix * scaleMatrix;
		}
		else {
			trimesh::fxform rotateMatrix = trimesh::fxform::rot(angle, axis);
			matrix = transMatrix * rotateMatrix * scaleMatrix;
		}

		return matrix;
	}


	trimesh::fxform layArrowMatrix(const trimesh::vec3& position, const trimesh::vec3& normal, const trimesh::vec3& scale)
	{
		trimesh::vec3 originNormal(0.0f, 0.0f, 1.0f);
		return layMatrixFromPositionNormal(position, normal, scale, originNormal);
	}
}