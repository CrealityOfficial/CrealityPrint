#include "trimeshrender.h"
#include "ccglobal/log.h"

namespace qtuser_3d
{
	void traitTriangles(trimesh::TriMesh* mesh, const std::vector<int>& indices, std::vector<trimesh::vec3>& tris)
	{
		if (!mesh)
			return;

		int nFace = (int)mesh->faces.size();
		int nVert = (int)mesh->vertices.size();
		for (int i : indices)
		{
			if (i < 0 || i > nFace)
			{
				LOGW("traitTriangles invalid face index [%d]", i);
				continue;
			}

			const trimesh::TriMesh::Face& f = mesh->faces.at(i);
			for (int j = 0; j < 3; ++j)
				tris.push_back(mesh->vertices.at(f[j]));
		}
	}

	void traitTriangles(trimesh::TriMesh* mesh, std::vector<trimesh::vec3>& tris)
	{
		if (!mesh)
			return;

		std::vector<int> indices;
		for (int i = 0; i < (int)mesh->faces.size(); ++i)
			indices.push_back(i);

		return traitTriangles(mesh, indices, tris);
	}

	void traitTrianglesFromMeshes(const std::vector<trimesh::TriMesh*>& meshes, std::vector<trimesh::vec3>& tris)
	{
		for (const trimesh::TriMesh* mesh : meshes)
		{
			if (!mesh)
				continue;

			if (mesh->faces.size() > 0)
			{
				for (const trimesh::TriMesh::Face& f : mesh->faces)
				{
					tris.push_back(mesh->vertices.at(f.x));
					tris.push_back(mesh->vertices.at(f.y));
					tris.push_back(mesh->vertices.at(f.z));
				}
			}
			else
			{
				tris.insert(tris.end(), mesh->vertices.begin(), mesh->vertices.end());
			}
		}
	}

	void loopPolygons2Lines(const std::vector<std::vector<trimesh::vec3>>& polygons, std::vector<trimesh::vec3>& lines, bool loop)
	{
		for (const std::vector<trimesh::vec3>& polygon : polygons)
			loopPolygon2Lines(polygon, lines, loop, true);
	}

	void loopPolygon2Lines(const std::vector<trimesh::vec3>& polygon, std::vector<trimesh::vec3>& lines, bool loop, bool append)
	{
		if (!append)
			lines.clear();

		size_t size = polygon.size();
		if (size <= 1)
			return;

		int end = loop ? size : size - 1;
		for (size_t i = 0; i < end; ++i)
		{
			const trimesh::vec3& point = polygon.at(i);
			const trimesh::vec3& point1 = polygon.at((i + 1) % size);
			lines.push_back(point);
			lines.push_back(point1);
		}
	}

	void traitLines(trimesh::TriMesh* mesh, const std::vector<int>& indices, std::vector<trimesh::vec3>& lines)
	{
		if (!mesh)
			return;

		lines.clear();
		int count = (int)indices.size();
		if (count >= 2)
		{
			lines.reserve(2 * count);
			for (int i = 0; i < count; ++i)
			{
				lines.push_back(mesh->vertices.at(indices.at(i)));
				lines.push_back(mesh->vertices.at(indices.at((i + 1)%count)));
			}
		}
	}

	void box2Lines(const trimesh::box2& box, std::vector<trimesh::vec3>& lines)
	{
		const trimesh::vec2& v0 = box.max;
		const trimesh::vec2& v1 = box.min;

		lines.clear();
		lines.push_back(trimesh::vec3(v0.x, v0.y, 0.0f));
		lines.push_back(trimesh::vec3(v1.x, v0.y, 0.0f));
		lines.push_back(trimesh::vec3(v1.x, v0.y, 0.0f));
		lines.push_back(trimesh::vec3(v1.x, v1.y, 0.0f));
		lines.push_back(trimesh::vec3(v1.x, v1.y, 0.0f));
		lines.push_back(trimesh::vec3(v0.x, v1.y, 0.0f));
		lines.push_back(trimesh::vec3(v0.x, v1.y, 0.0f));
		lines.push_back(trimesh::vec3(v0.x, v0.y, 0.0f));
	}

	void box3esLines(const std::vector<trimesh::box3>& boxes, std::vector<trimesh::vec3>& lines)
	{
		lines.clear();
		int size = (int)boxes.size();
		if (size > 0)
		{
			lines.resize(12 * 2 * size);
			for (int i = 0; i < size; ++i)
				box3Lines(boxes.at(i), lines.begin() + i * 24);
		}
	}

	void box3Lines(const trimesh::box3& box, std::vector<trimesh::vec3>::iterator begin)
	{
		trimesh::vec3 v[8];
		v[0] = box.min;
		v[6] = box.max;

		v[1] = trimesh::vec3(v[0].x, v[0].y, v[6].z);
		v[2] = trimesh::vec3(v[6].x, v[0].y, v[6].z);
		v[3] = trimesh::vec3(v[6].x, v[0].y, v[0].z);
		v[4] = trimesh::vec3(v[0].x, v[6].y, v[0].z);
		v[5] = trimesh::vec3(v[0].x, v[6].y, v[6].z);
		v[7] = trimesh::vec3(v[6].x, v[6].y, v[0].z);

		auto add = [&box, &begin, &v](int i0, int i1) {
			*begin++ = v[i0];
			*begin++ = v[i1];
		};

		int idx = 0;
		add(idx, idx + 1);
		// edge 2
		add(idx + 1, idx + 2);
		// edge 3
		add(idx + 2, idx + 3);
		// edge 4
		add(idx + 3, idx);
		// edge 5
		add(idx + 4, idx + 5);
		// edge 6
		add(idx + 5, idx + 6);
		// edge 7
		add(idx + 6, idx + 7);
		// edge 8
		add(idx + 7, idx + 4);
		// edge 9
		add(idx, idx + 4);
		// edge 10
		add(idx + 1, idx + 5);
		// edge 11
		add(idx + 2, idx + 6);
		// edge 12
		add(idx + 3, idx + 7);
	}

	void box3Lines(const trimesh::box3& box, std::vector<trimesh::vec3>& lines)
	{
		lines.resize(12 * 2);
		box3Lines(box, lines.begin());
	}
}