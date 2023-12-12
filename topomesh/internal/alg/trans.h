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