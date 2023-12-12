#include "crslice/support.h"

namespace crslice
{
	trimesh::fxform beltXForm(const trimesh::vec3& offset, float angle, int beltType)
	{
		float theta = angle * M_PIf / 180.0f;

		trimesh::fxform xf0 = trimesh::fxform::trans(offset);

		trimesh::fxform xf1 = trimesh::fxform::identity();
		xf1(2, 2) = 1.0f / sinf(theta);
		xf1(1, 2) = -1.0f / tanf(theta);

		trimesh::fxform xf2 = trimesh::fxform::identity();
		xf2(2, 2) = 0.0f;
		xf2(1, 1) = 0.0f;
		xf2(2, 1) = -1.0f;
		xf2(1, 2) = 1.0f;

		if (1 == beltType)
		{
			trimesh::fxform xf3 = trimesh::fxform::trans(0.0f, 0.0f, 0.0f);

			trimesh::fxform xf = xf3 * xf2 * xf1 * xf0;
			//trimesh::fxform xf = xf3 * xf0;
			return xf;
		}
		else
		{
			trimesh::fxform xf3 = trimesh::fxform::trans(0.0f, 0.0f, 200.0f);
			trimesh::fxform xf = xf3 * xf2 * xf1 * xf0;
			return xf;
		}

	}
}