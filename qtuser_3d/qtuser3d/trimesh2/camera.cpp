#include "camera.h"
#include <math.h>

namespace qtuser_3d
{
	trimesh::fxform ScreenCameraMeta::viewMatrix()
	{
		return trimesh::fxform::lookat(position, viewCenter, upVector);
	}

	trimesh::fxform ScreenCameraMeta::posMatrix()
	{
		//trimesh::vec3 dir = viewCenter - position;
		//trimesh::normalize(dir);
		//trimesh::vec3 x = -(upVector TRICROSS dir);
		//trimesh::normalize(x);
		//
		//trimesh::quaternion q = trimesh::quaternion::fromAxes(x, upVector, dir);
		//q.normalize();
		//
		//trimesh::fxform rot = mmesh::fromQuaterian(q);
		trimesh::fxform view = viewMatrix();
		//trimesh::fxform rot = trimesh::rot_only(view);
		//return trimesh::fxform::trans(position) * rot;
		return trimesh::inv(view);
	}

	trimesh::fxform ScreenCameraMeta::projectionMatrix()
	{
		trimesh::fxform xf = trimesh::fxform::identity();
		if (type == ScreenCameraProjectionType::ePerspective) {
			xf = trimesh::fxform::perspective(fovy, aspectRatio, fNear, fFar);
		}
		else {
			xf = trimesh::fxform::ortho(left, right, bottom, top, fNear, fFar);
		}
		return xf;
	}

	//把box当做一个球来处理，直径是box.length()，相机放在球心位置往dir方向移动distance的位置
	ScreenCameraMeta viewAllByProjection(const trimesh::box3& box, const trimesh::vec3& viewDir, float fovy)
	{
		ScreenCameraMeta meta;
		memset(&meta, 0x0, sizeof(meta));

		trimesh::vec3 bsize = box.size();
		trimesh::vec3 ViewCenter = box.center(); //视点
		float r = trimesh::length(bsize) / 2.0f;
		
		trimesh::vec3 dir = trimesh::normalized(viewDir);
		float distance = r / (float)sinf(M_PI * fovy / 2.0 / 180.0);
		trimesh::vec3 cameraPosition = ViewCenter + dir * distance;
		trimesh::vec3 upVector = trimesh::vec3(0.0, 0.0, 1.0);
		
		float nearPlane = distance - 1.1f * r;
		float farPlane = distance + 1.1f * r;

		meta.type = ScreenCameraProjectionType::ePerspective;
		meta.viewCenter = ViewCenter;
		meta.upVector = upVector;
		meta.position = cameraPosition;

		meta.fNear = nearPlane;
		meta.fFar = farPlane;
		meta.fovy = fovy;

		return meta;
	}

	void createCameraPoints(ScreenCameraMeta* meta, std::vector<trimesh::vec3>& positions)
	{
		positions.resize(9);
		positions.at(0) = trimesh::vec3(0.0f, 0.0f, 0.0f);
		float nZ = - meta->fNear;
		float fZ = - meta->fFar;
		float nY = meta->fNear * tanf(meta->fovy * M_PIf / 360.0f);
		float fY = meta->fFar * tanf(meta->fovy * M_PIf / 360.0f);
		float nX = meta->aspectRatio * nY;
		float fX = meta->aspectRatio * fY;
		
		positions.at(1) = trimesh::vec3(nX, nY, nZ);
		positions.at(2) = trimesh::vec3(-nX, nY, nZ);
		positions.at(3) = trimesh::vec3(-nX, -nY, nZ);
		positions.at(4) = trimesh::vec3(nX, -nY, nZ);
		positions.at(5) = trimesh::vec3(fX, fY, fZ);
		positions.at(6) = trimesh::vec3(-fX, fY, fZ);
		positions.at(7) = trimesh::vec3(-fX, -fY, fZ);
		positions.at(8) = trimesh::vec3(fX, -fY, fZ);
	}
}
