#include "create.h"

namespace qtuser_3d
{
	trimesh::TriMesh* createCube(const trimesh::box3& box, float gap)
	{
		trimesh::TriMesh* boxMesh = new trimesh::TriMesh();
		trimesh::point point0(box.min.x -gap, box.min.y -gap, box.min.z);
		trimesh::point point1(box.max.x + gap, box.min.y - gap, box.min.z);
		trimesh::point point2(box.max.x + gap, box.min.y - gap, box.max.z + gap);
		trimesh::point point3(box.min.x - gap, box.min.y - gap, box.max.z + gap);
		trimesh::point point4(box.min.x - gap, box.max.y + gap, box.min.z);
		trimesh::point point5(box.max.x + gap, box.max.y + gap, box.min.z);
		trimesh::point point6(box.max.x + gap, box.max.y + gap, box.max.z + gap);
		trimesh::point point7(box.min.x - gap, box.max.y + gap, box.max.z + gap);
		boxMesh->vertices.push_back(point0);
		boxMesh->vertices.push_back(point1);
		boxMesh->vertices.push_back(point2);
		boxMesh->vertices.push_back(point3);
		boxMesh->vertices.push_back(point4);
		boxMesh->vertices.push_back(point5);
		boxMesh->vertices.push_back(point6);
		boxMesh->vertices.push_back(point7);

		//正面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(0, 1, 2));
		//正面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(2, 3, 0));

		//右侧面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(1, 5, 6));
		//右侧面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(6, 2, 1));

		//背面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(4, 6, 5));
		//背面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(6, 4, 7));

		//左侧面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(4, 0, 3));
		//左侧面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(3, 7, 4));

		//顶面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(3, 2, 6));
		//顶面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(6, 7, 3));

		//底面add face 1
		boxMesh->faces.push_back(trimesh::TriMesh::Face(5, 1, 0));
		//底面 add face 2
		boxMesh->faces.push_back(trimesh::TriMesh::Face(5, 0, 4));

		return boxMesh;
	}

	void boxLineIndices(const trimesh::box3& box, std::vector<trimesh::vec3>& corners, std::vector<int>& indices)
	{
		const trimesh::vec3& cmin = box.min;
		const trimesh::vec3& cmax = box.max;

		corners.clear();
		corners.resize(8);

		corners[0] = trimesh::vec3(cmin.x, cmin.y, cmin.z);
		corners[1] = trimesh::vec3(cmax.x, cmin.y, cmin.z);
		corners[2] = trimesh::vec3(cmax.x, cmax.y, cmin.z);
		corners[3] = trimesh::vec3(cmin.x, cmax.y, cmin.z);
		corners[4] = trimesh::vec3(cmin.x, cmin.y, cmax.z);
		corners[5] = trimesh::vec3(cmax.x, cmin.y, cmax.z);
		corners[6] = trimesh::vec3(cmax.x, cmax.y, cmax.z);
		corners[7] = trimesh::vec3(cmin.x, cmax.y, cmax.z);

		indices.clear();
		indices.reserve(24);

		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(3);
		indices.push_back(0);
		indices.push_back(0);
		indices.push_back(4);
		indices.push_back(3);
		indices.push_back(7);
		indices.push_back(1);
		indices.push_back(5);
		indices.push_back(2);
		indices.push_back(6);
		indices.push_back(4);
		indices.push_back(5);
		indices.push_back(5);
		indices.push_back(6);
		indices.push_back(6);
		indices.push_back(7);
		indices.push_back(7);
		indices.push_back(4);
	}

	void boxLines(const trimesh::box3& box, std::vector<trimesh::vec3>& lines)
	{
		lines.clear();

		std::vector<trimesh::vec3> corners;
		std::vector<int> indices;
		boxLineIndices(box, corners, indices);

		lines.reserve(indices.size());
		for (int index : indices)
			lines.push_back(corners.at(index));
	}

	void offsetPoints(std::vector<trimesh::vec3>& points, const trimesh::vec3& offset)
	{
		for (trimesh::vec3& point : points)
			point += offset;
	}
}