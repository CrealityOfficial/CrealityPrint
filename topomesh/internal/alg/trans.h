#ifndef TOPOMESH_UTIL_1694152396889_H
#define TOPOMESH_UTIL_1694152396889_H
#include <Eigen/Dense>
#include "internal/data/mmesht.h"

namespace topomesh
{
	struct CameraParam
	{
		trimesh::ivec2 ScreenSize;  // hw
		trimesh::ivec2 p1;
		trimesh::ivec2 p2;
		//

		CameraParam()
		{
			this->n = 0.f;
			this->f = 0.f;
			this->t = 0.f;
			this->b = 0.f;
			this->l = 0.f;
			this->r = 0.f;
			this->fov = 0.f;
			this->aspect = 0.f;
			this->pos = trimesh::point(0, 0, 0);
			this->lookAt = trimesh::point(0, 0, 0);
			this->right = trimesh::point(0, 0, 0);
			this->up = trimesh::point(0, 0, 0);
			this->dir = trimesh::point(0, 0, 0);
			this->ScreenSize = trimesh::ivec2(0, 0);
			this->p1 = trimesh::ivec2(0, 0);
			this->p2 = trimesh::ivec2(0, 0);
		};
		float n;
		float f;
		float t;
		float b;
		float l;
		float r;

		float fov, aspect;

		//
		trimesh::point pos;
		trimesh::point lookAt;
		trimesh::point right;
		trimesh::point up;
		trimesh::point dir;

		bool operator==(const CameraParam& b) const
		{
			if (this->n - b.n>=1e-6)
				return false;
			if (this->f - b.f >= 1e-6)
				return false;
			if (this->t - b.t >= 1e-6)
				return false;
			if (this->b - b.b >= 1e-6)
				return false;
			if (this->l - b.l >= 1e-6)
				return false;
			if (this->r - b.r >= 1e-6)
				return false;

			if (this->fov - b.fov >= 1e-6)
				return false;
			if (this->aspect - b.aspect >= 1e-6)
				return false;

			if (this->pos != b.pos)
				return false;
			if (this->lookAt != b.lookAt)
				return false;
			if (this->right != b.right)
				return false;
			if (this->up != b.up)
				return false;
			if (this->dir != b.dir)
				return false;

			return true;
		}
	};

	void getScreenWidthAndHeight(const CameraParam& camera, std::pair<float, float>& wh);
	void getViewMatrixAndProjectionMatrix(const CameraParam& camera, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix);
	void getEmbedingPoint(std::vector<std::vector<trimesh::point>>& lines, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix, std::vector<std::vector<trimesh::vec2>>& poly);
	void unTransformationMesh(MMeshT* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix);
	void unTransformationMesh(trimesh::TriMesh* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix);
	void TransformationMesh(MMeshT* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix);
	void TransformationMesh(trimesh::TriMesh* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix);
}

#endif // TOPOMESH_UTIL_1694152396889_H