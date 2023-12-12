#ifndef QTUSER_3D_MMESH_SCREENCAMERA_1619922752767_H
#define QTUSER_3D_MMESH_SCREENCAMERA_1619922752767_H
#include "qtuser3d/qtuser3dexport.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"
#include "trimesh2/Box.h"
#include <vector>

namespace qtuser_3d
{
	enum class ScreenCameraProjectionType
	{
		ePerspective,
		eOrth
	};

	struct ScreenCameraMeta
	{
		ScreenCameraProjectionType type;

		trimesh::vec3 viewCenter;
		trimesh::vec3 upVector;
		trimesh::vec3 position;

		float fNear;
		float fFar;
		float fovy;
		float aspectRatio;

		float top;
		float bottom;
		float left;
		float right;

		QTUSER_3D_API trimesh::fxform viewMatrix();
		QTUSER_3D_API trimesh::fxform posMatrix();
		QTUSER_3D_API trimesh::fxform projectionMatrix();
	};

	QTUSER_3D_API ScreenCameraMeta viewAllByProjection(const trimesh::box3& box, const trimesh::vec3& viewDir, float fovy);

	QTUSER_3D_API void createCameraPoints(ScreenCameraMeta* meta, std::vector<trimesh::vec3>& positions);
}

#endif // QTUSER_3D_MMESH_SCREENCAMERA_1619922752767_H
