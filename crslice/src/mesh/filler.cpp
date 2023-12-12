#include "filler.h"
#include "trimesh2/quaternion.h"
#include <vector>

namespace crslice
{
	void fillCylinderSoup(trimesh::vec3* data, float radius1, const trimesh::vec3& center1,
		float radius2, const trimesh::vec3& center2, int num, float theta)
	{
		int hPart = num;

		trimesh::vec3 dir = center2 - center1;
		trimesh::normalize(dir);
		trimesh::quaternion q = trimesh::quaternion::fromDirection(dir, trimesh::vec3(0.0f, 0.0f, 1.0f));

		theta *= M_PIf / 180.0f;
		float deltaTheta = M_PIf * 2.0f / (float)(hPart);
		std::vector<float> cosValue;
		std::vector<float> sinValue;
		for (int i = 0; i < hPart; ++i)
		{
			cosValue.push_back(cosf(deltaTheta * (float)i + theta));
			sinValue.push_back(sinf(deltaTheta * (float)i + theta));
		}

		std::vector<trimesh::vec3> baseNormals;
		for (int i = 0; i < hPart; ++i)
		{
			baseNormals.push_back(trimesh::vec3(cosValue[i], sinValue[i], 0.0f));
		}

		int vertexNum = 2 * hPart;
		std::vector<trimesh::vec3> points(vertexNum);

		int vertexIndex = 0;
		for (int i = 0; i < hPart; ++i)
		{
			trimesh::vec3 n = q * baseNormals[i];
			trimesh::vec3 s = center1 + n * radius1;
			points.at(vertexIndex) = trimesh::vec3(s.x, s.y, s.z);
			++vertexIndex;
			trimesh::vec3 e = center2 + n * radius2;
			points.at(vertexIndex) = trimesh::vec3(e.x, e.y, e.z);
			++vertexIndex;
		}

		auto fvindex = [&hPart](int layer, int index)->int {
			return layer + 2 * index;
		};

		for (int i = 0; i < hPart; ++i)
		{
			int v1 = fvindex(0, i);
			int v2 = fvindex(1, i);
			int v3 = fvindex(0, (i + 1) % hPart);
			int v4 = fvindex(1, (i + 1) % hPart);
			*data++ = points.at(v1);
			*data++ = points.at(v3);
			*data++ = points.at(v2);
			*data++ = points.at(v2);
			*data++ = points.at(v3);
			*data++ = points.at(v4);
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(0);
			*data++ = points.at(fvindex(0, i + 1));
			*data++ = points.at(fvindex(0, i));
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(1);
			*data++ = points.at(fvindex(1, i));
			*data++ = points.at(fvindex(1, i + 1));
		}
	}
}