#include "trans.h"

namespace topomesh
{
	void getScreenWidthAndHeight(const CameraParam& camera, std::pair<float, float>& wh)
	{
		wh.first = 2.0f * camera.n * std::tan(camera.fov * M_PIf / 2.0f / 180.0f);
		wh.second = wh.first * camera.aspect;
	}

	void getViewMatrixAndProjectionMatrix(const CameraParam& camera, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix)
	{
		trimesh::point undir = -1.0f * camera.dir;
		ViewMatrix << camera.right.x, camera.right.y, camera.right.z, -1.0f * camera.pos DOT camera.right,
			camera.up.x, camera.up.y, camera.up.z, -1.0f * camera.pos DOT camera.up,
			undir.x, undir.y, undir.z, -1.0f * camera.pos DOT undir,
			0, 0, 0, 1;
		std::pair<float, float> ScreenSize;// h w
		getScreenWidthAndHeight(camera, ScreenSize);
		ProjectionMatrix << 2.0f * camera.n / (1.0f * ScreenSize.second), 0, 0, 0,
			0, 2.0f * camera.n / (1.0f * ScreenSize.first), 0, 0,
			0, 0, -1.0f * (camera.n + camera.f) / (1.0f * (camera.f - camera.n)), -2.0f * camera.n * camera.f / (1.0f * (camera.f - camera.n)),
			0, 0, -1, 0;

	}

	void getEmbedingPoint(std::vector<std::vector<trimesh::point>>& lines, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix, std::vector<std::vector<trimesh::vec2>>& poly)
	{
		poly.resize(lines.size());
		for (unsigned i = 0; i < lines.size(); i++)
		{
			for (trimesh::point& p : lines[i])
			{
				Eigen::Vector4f linePoint = { p.x,p.y,p.z,1.0 };
				Eigen::Vector4f point = ProjectionMatrix * ViewMatrix * linePoint;
				poly[i].push_back(trimesh::vec2(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w())));
			}
		}
	}

	void TransformationMesh(MMeshT* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix)
	{
		for (MMeshVertex& v : mesh->vertices)if (!v.IsD())
		{
			Eigen::Vector4f vPoint = { v.p.x,v.p.y,v.p.z,1.0 };
			Eigen::Vector4f point = ProjectionMatrix * ViewMatrix * vPoint;
			v.p = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
		}
	}

	void TransformationMesh(trimesh::TriMesh* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix)
	{
		for (trimesh::point& v : mesh->vertices)
		{
			Eigen::Vector4f vPoint = { v.x,v.y,v.z,1.0 };
			Eigen::Vector4f point = ProjectionMatrix * ViewMatrix * vPoint;
			v = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
		}
		Eigen::Vector4f vPoint = { mesh->bbox.max.x,mesh->bbox.max.y ,mesh->bbox.max.z ,1.0 };
		Eigen::Vector4f point = ProjectionMatrix * ViewMatrix * vPoint;
		mesh->bbox.max = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
		vPoint = { mesh->bbox.min.x,mesh->bbox.min.y ,mesh->bbox.min.z ,1.0 };
		point = ProjectionMatrix * ViewMatrix * vPoint;
		mesh->bbox.min = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
	}

	void unTransformationMesh(MMeshT* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix)
	{
		for (MMeshVertex& v : mesh->vertices)if (!v.IsD())
		{
			Eigen::Vector4f vPoint = { v.p.x,v.p.y,v.p.z,1.0 };
			Eigen::Vector4f point = ViewMatrix.inverse() * ProjectionMatrix.inverse() * vPoint;
			v.p = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
		}
	}

	void unTransformationMesh(trimesh::TriMesh* mesh, Eigen::Matrix4f& ViewMatrix, Eigen::Matrix4f& ProjectionMatrix)
	{
		for (trimesh::point& v : mesh->vertices)
		{
			Eigen::Vector4f vPoint = { v.x,v.y,v.z,1.0 };
			Eigen::Vector4f point = ViewMatrix.inverse() * ProjectionMatrix.inverse() * vPoint;
			v = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
		}
		Eigen::Vector4f vPoint = { mesh->bbox.max.x,mesh->bbox.max.y ,mesh->bbox.max.z ,1.0 };
		Eigen::Vector4f point = ViewMatrix.inverse() * ProjectionMatrix.inverse() * vPoint;
		mesh->bbox.max = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
		vPoint = { mesh->bbox.min.x,mesh->bbox.min.y ,mesh->bbox.min.z ,1.0 };
		point = ViewMatrix.inverse() * ProjectionMatrix.inverse() * vPoint;
		mesh->bbox.min = trimesh::point(point.x() * (1.0f / point.w()), point.y() * (1.0f / point.w()), point.z() * (1.0f / point.w()));
	}
}