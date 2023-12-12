#include "trimesh2qgeometryrenderer.h"

#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/math/angles.h"
#include <Qt3DRender/QBuffer>

namespace qtuser_3d
{
	Qt3DRender::QGeometry* trimesh2Geometry(trimesh::TriMesh* mesh, int vflag, Qt3DCore::QNode* parent)
	{
		if (!mesh)
			return nullptr;

		std::vector<trimesh::TriMesh*> meshes;
		meshes.push_back(mesh);
		return trimeshes2Geometry(meshes, vflag, parent);
	}

	Qt3DRender::QGeometry* trimeshes2Geometry(std::vector<trimesh::TriMesh*>& meshes, int vflag, Qt3DCore::QNode* parent)
	{
		int count = 0;
		for (trimesh::TriMesh* mesh : meshes)
			count += (int)mesh->faces.size() * 3;

		if (count <= 0)
			return nullptr;

		QByteArray position;
		QByteArray normal;
		position.resize(count * 3 * sizeof(float));
		normal.resize(count * 3 * sizeof(float));
		QByteArray flag;

		if (vflag >= 0)
		{
			flag.resize(count * sizeof(float));
			flag.fill(0);
		}

		trimesh::vec3* vertexData = (trimesh::vec3*)position.data();
		trimesh::vec3* normalData = (trimesh::vec3*)normal.data();
		float* flagData = (float*)flag.data();
		int index = 0;
		for (trimesh::TriMesh* mesh : meshes)
		{
			for (trimesh::TriMesh::Face& f : mesh->faces)
			{
				const trimesh::vec3& v0 = mesh->vertices.at(f[0]);
				const trimesh::vec3& v1 = mesh->vertices.at(f[1]);
				const trimesh::vec3& v2 = mesh->vertices.at(f[2]);

				trimesh::vec3 v01 = v1 - v0;
				trimesh::vec3 v02 = v2 - v0;
				trimesh::vec3 n = v01 TRICROSS v02;
				trimesh::normalize(n);

				for (int j = 0; j < 3; ++j)
				{
					vertexData[index] = mesh->vertices.at(f[j]);
					normalData[index] = n;
					if (vflag >= 0)
					{
						flagData[index] = (float)vflag;
					}
					++index;
				}
			}
		}

		Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		Qt3DRender::QBuffer* normalBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		Qt3DRender::QBuffer* flagBuffer = nullptr;
		if (vflag >= 0)
			flagBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		positionBuffer->setData(position);
		normalBuffer->setData(normal);
		if (flagBuffer)
			flagBuffer->setData(flag);
		Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(positionBuffer, Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, 3, count);
		Qt3DRender::QAttribute* normalAttribute = new Qt3DRender::QAttribute(normalBuffer, Qt3DRender::QAttribute::defaultNormalAttributeName(), Qt3DRender::QAttribute::Float, 3, count);
		Qt3DRender::QAttribute* flagAttribute = nullptr;
		if (vflag >= 0)
			flagAttribute = new Qt3DRender::QAttribute(flagBuffer, "vertexFlag", Qt3DRender::QAttribute::Float, 1, count);

		return qtuser_3d::GeometryCreateHelper::create(parent, positionAttribute, normalAttribute, flagAttribute);
	}
}



