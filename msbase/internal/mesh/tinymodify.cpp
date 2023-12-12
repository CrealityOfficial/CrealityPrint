#include "msbase/mesh/tinymodify.h"
#include "trimesh2/TriMesh_algo.h"

namespace msbase
{
	trimesh::vec3 moveTrimesh2Center(trimesh::TriMesh* mesh, bool zZero)
	{
		if (!mesh)
			return trimesh::vec3(0.0f, 0.0f, 0.0f);

		mesh->need_bbox();
		trimesh::box3 b = mesh->bbox;

		trimesh::vec3 size = b.size() / 2.0f;
		size.x = 0.0f;
		size.y = 0.0f;
		if (!zZero)
			size.z = 0.0f;

		trimesh::vec3 offset = size - b.center();
		trimesh::trans(mesh, offset);

		return offset;
	}

	void reverseTriMesh(trimesh::TriMesh* Mesh)
	{
		for (size_t i = 0; i < Mesh->faces.size(); i++)
		{
			int temp = Mesh->faces[i].at(1);
			Mesh->faces[i].at(1) = Mesh->faces[i].at(2);
			Mesh->faces[i].at(2) = temp;
		}
	}

	void fillTriangleSoupFaceIndex(trimesh::TriMesh* mesh)
	{
		if (!mesh || mesh->faces.size() != 0)
			return;

		size_t size = mesh->vertices.size();
		if (size % 3 || size < 3)
			return;

		size /= 3;
		mesh->faces.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			trimesh::TriMesh::Face& face = mesh->faces.at(i);
			face[0] = (int)(3 * i);
			face[1] = (int)(3 * i + 1);
			face[2] = (int)(3 * i + 2);
		}
	}

	void removeMeshFaces(trimesh::TriMesh* mesh, const std::vector<int>& removes)
	{
		if (!mesh || (mesh->vertices.size() == 0)
				|| (mesh->faces.size() == 0))
			return;

		int vcount = mesh->vertices.size();
		int fcount = mesh->faces.size();
		std::vector<int> vertexMap(vcount, -1);
		std::vector<bool> faceStates(fcount, true);
		for (int i : removes)
		{
			if (i >= 0 && i < fcount)
				faceStates.at(i) = false;
		}

		std::vector<trimesh::vec3> points;
		std::vector<trimesh::TriMesh::Face> faces;
		for (int i = 0; i < fcount; ++i)
		{
			if (faceStates.at(i))
			{
				const trimesh::TriMesh::Face& f = mesh->faces.at(i);
				trimesh::TriMesh::Face newFace;

				for(int j = 0; j < 3; ++j)
				{
					int index = f[j];
					if (vertexMap.at(index) >= 0)
						newFace[j] = vertexMap.at(index);
					else
					{
						points.push_back(mesh->vertices.at(index));
						int ind = (int)points.size() - 1;
						vertexMap.at(index) = ind;
						newFace[j] = ind;
					}
				}

				faces.push_back(newFace);
			}
		}
		mesh->vertices.swap(points);
		mesh->faces.swap(faces);
	}
}