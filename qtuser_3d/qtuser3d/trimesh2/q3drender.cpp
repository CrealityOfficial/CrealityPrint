#include "q3drender.h"

#include <Qt3DRender/QBuffer>
#include "qtuser3d/geometry/geometrycreatehelper.h"

namespace qtuser_3d
{
	Qt3DRender::QGeometry* trimesh2Geometry(trimesh::TriMesh* mesh, Qt3DCore::QNode* parent)
	{
		if (!mesh)
			return nullptr;

		std::vector<trimesh::TriMesh*> meshes;
		meshes.push_back(mesh);
		return trimeshes2Geometry(meshes, parent);
	}

	Qt3DRender::QGeometry* trimeshes2Geometry(const std::vector<trimesh::TriMesh*>& meshes, Qt3DCore::QNode* parent)
	{
		qtuser_3d::AttributeShade positionAttributeShade;
		qtuser_3d::AttributeShade normalAttributeShade;

		trimeshes2AttributeShade(meshes, positionAttributeShade, normalAttributeShade);

		if (positionAttributeShade.count <= 0 || normalAttributeShade.count <= 0)
			return nullptr;

		return qtuser_3d::GeometryCreateHelper::create(parent, &positionAttributeShade, &normalAttributeShade);
	}


	// deprecated
	QByteArray createFlagArray(int faceNum)
	{
		QByteArray flagBytes;
		if (faceNum > 0)
		{
			flagBytes.resize(faceNum * 3 * sizeof(float));
			float* fdata = (float*)flagBytes.data();
			float vd = 1.0f;
			for (int i = 0; i < faceNum * 3; ++i)
				*fdata++ = vd;
		}
		return flagBytes;
	}


	// deprecated
	QByteArray createPositionArray(trimesh::TriMesh* mesh)
	{
		QByteArray positionBytes;
		if (mesh && mesh->faces.size() > 0)
		{
			int facesN = (int)mesh->faces.size();
#if QT_USE_GLES
			int index = 0;
			positionBytes.resize(facesN * 3 * 4 * sizeof(float));
			trimesh::vec4* position = (trimesh::vec4*)positionBytes.data();
			for (trimesh::TriMesh::Face face : mesh->faces)
			{
				for (int i = 0; i < 3; ++i)
				{
					const trimesh::vec3& p = mesh->vertices.at(face[i]);
					*position++ = trimesh::vec4(p.x, p.y, p.z, (float)index);
					++index;
				}
			}
#else
			positionBytes.resize(facesN * 3 * 3 * sizeof(float));
			trimesh::vec3* position = (trimesh::vec3*)positionBytes.data();
			for (trimesh::TriMesh::Face face : mesh->faces)
			{
				*position++ = mesh->vertices.at(face[0]);
				*position++ = mesh->vertices.at(face[1]);
				*position++ = mesh->vertices.at(face[2]);
			}
#endif
		}
		return positionBytes;
	}

	void trimesh2SupportAttributeInfo(trimesh::TriMesh* mesh, qtuser_3d::SupportAttributeShade& supportChunkAttributeInfo)
	{
		if (!mesh)
			return;

		if (mesh->faces.size() <= 0)
			return;

		int count = (int)mesh->faces.size() * 3;

		trimesh::vec4* vertex4Data = nullptr;
		trimesh::vec3* vertex3Data = nullptr;

		if (qtuser_3d::isGles())
		{
			supportChunkAttributeInfo.positionAttribute.bytes.resize(count * 4 * sizeof(float));
			vertex4Data = (trimesh::vec4*)supportChunkAttributeInfo.positionAttribute.bytes.data();
		}
		else
		{
			supportChunkAttributeInfo.positionAttribute.bytes.resize(count * 3 * sizeof(float));
			vertex3Data = (trimesh::vec3*)supportChunkAttributeInfo.positionAttribute.bytes.data();
		}

		supportChunkAttributeInfo.normalAttribute.bytes.resize(count * 3 * sizeof(float));
		trimesh::vec3* normalData = (trimesh::vec3*)supportChunkAttributeInfo.normalAttribute.bytes.data();

		int index = 0;
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
				const trimesh::vec3& v = mesh->vertices.at(f[j]);
				if (qtuser_3d::isGles())
				{
					vertex4Data[index] = trimesh::vec4(v.x, v.y, v.z, (float)index);
				}
				else
				{
					vertex3Data[index] = v;
				}

				normalData[index] = n;
				++index;
			}
		}

		int faceNum = (int)mesh->faces.size();

		supportChunkAttributeInfo.flagAttribute.bytes.resize(faceNum * 3 * sizeof(float));
		float* fdata = (float*)supportChunkAttributeInfo.flagAttribute.bytes.data();
		float vd = 1.0f;
		for (int i = 0; i < faceNum * 3; ++i)
			*fdata++ = vd;

	}

	void trimeshes2AttributeShade(const std::vector<trimesh::TriMesh*>& meshes, qtuser_3d::AttributeShade& position, qtuser_3d::AttributeShade& normal)
	{
		int count = 0;
		for (trimesh::TriMesh* mesh : meshes)
			count += (int)mesh->faces.size() * 3;

		if (count <= 0)
			return;

		trimesh::vec4* vertex4Data = nullptr;
		trimesh::vec3* vertex3Data = nullptr;
		if (qtuser_3d::isGles())
		{
			position.stride = 4;
			position.bytes.resize(count * 4 * sizeof(float));
			vertex4Data = (trimesh::vec4*)position.bytes.data();
		}
		else
		{
			position.stride = 3;
			position.bytes.resize(count * 3 * sizeof(float));
			vertex3Data = (trimesh::vec3*)position.bytes.data();
		}

		normal.stride = 3;
		normal.bytes.resize(count * 3 * sizeof(float));
		trimesh::vec3* normalData = (trimesh::vec3*)normal.bytes.data();

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
					const trimesh::vec3& v = mesh->vertices.at(f[j]);
					if (qtuser_3d::isGles())
					{
						vertex4Data[index] = trimesh::vec4(v.x, v.y, v.z, (float)index);
					}
					else
					{
						vertex3Data[index] = v;
					}
					
					normalData[index] = n;
					++index;
				}
			}
		}

		position.count = count;
		position.name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		normal.count = count;
		normal.name = Qt3DRender::QAttribute::defaultNormalAttributeName();
	}

	void trimesh2AttributeShade(trimesh::TriMesh* mesh, qtuser_3d::AttributeShade& position, qtuser_3d::AttributeShade& normal)
	{
		if (!mesh)
			return;

		std::vector<trimesh::TriMesh*> meshes;
		meshes.push_back(mesh);
		return trimeshes2AttributeShade(meshes, position, normal);
	}
}