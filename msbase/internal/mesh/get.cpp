#include "msbase/mesh/get.h"
#include "ccglobal/log.h"

#include <limits.h>
#include <math.h>
#include <set>

namespace msbase
{
	trimesh::vec3 getFaceNormal(trimesh::TriMesh* mesh, int faceIndex, bool normalized)
	{
		trimesh::vec3 n = trimesh::vec3(0.0f, 0.0f, 0.0f);
		if (mesh && faceIndex >= 0 && faceIndex < (int)mesh->faces.size())
		{
			const trimesh::TriMesh::Face& f = mesh->faces.at(faceIndex);

			trimesh::vec3 v01 = mesh->vertices.at(f[1]) - mesh->vertices.at(f[0]);
			trimesh::vec3 v02 = mesh->vertices.at(f[2]) - mesh->vertices.at(f[0]);
			n = v01 TRICROSS v02;
			if(normalized)
				trimesh::normalize(n);
		}
		else
		{
			LOGW("msbase::getFaceNormal : error input.");
		}
		return n;
	}

	void calculateFaceNormalOrAreas(trimesh::TriMesh* mesh, 
		std::vector<trimesh::vec3>& normals, std::vector<float>* areas)
	{
		if (!mesh || mesh->faces.size() == 0)
		{
			LOGW("msbase::getFaceNormal : error input.");
			return;
		}

		const int faceNum = mesh->faces.size();
		normals.resize(faceNum);

		for (int i = 0; i < faceNum; ++i) {
			normals.at(i) = getFaceNormal(mesh, i, false);
		}

		//std::transform
		if (areas) {
			areas->resize(faceNum, 0.0f);
			for (int i = 0; i < faceNum; ++i) {
				areas->at(i) = (trimesh::len(normals.at(i)) / 2.0);
			}
		}

		for (auto& n : normals) {
			trimesh::normalize(n);
		}
	}


	double count_triangle_area(const trimesh::point& a, const trimesh::point& b, const trimesh::point& c) {
		double area = -1;

		double side[3];

		side[0] = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
		side[1] = sqrt(pow(a.x - c.x, 2) + pow(a.y - c.y, 2) + pow(a.z - c.z, 2));
		side[2] = sqrt(pow(c.x - b.x, 2) + pow(c.y - b.y, 2) + pow(c.z - b.z, 2));

		if (side[0] + side[1] <= side[2] || side[0] + side[2] <= side[1] || side[1] + side[2] <= side[0]) return area;

		double p = (side[0] + side[1] + side[2]) / 2;
		area = sqrt(p * (p - side[0]) * (p - side[1]) * (p - side[2]));

		return area;
	}

	double getFacetArea(trimesh::TriMesh* mesh, int facetId)
	{
		if (!mesh || mesh->faces.size() == 0)
		{
			return 0.0f;
		}

		double s = 0.0f;
		std::vector<trimesh::point>& vs = mesh->vertices;
		if (mesh->faces.size() <= facetId)
			return 0.0f;

		trimesh::TriMesh::Face& face = mesh->faces[facetId];

		if (vs.size() <= face[0] || vs.size() <= face[1] || vs.size() <= face[2])
			return 0.0f;

		return count_triangle_area(vs[face[0]], vs[face[1]], vs[face[2]]);
	}
}