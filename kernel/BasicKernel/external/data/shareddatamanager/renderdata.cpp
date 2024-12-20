#include "renderdata.h"
#include "cxkernel/data/attribute.h"
#include "cxkernel/data/trimeshutils.h"

#include "msbase/mesh/deserializecolor.h" 

#include "interface/machineinterface.h"

#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/trimesh2/create.h"

namespace creative_kernel
{
	RenderData::RenderData(TriMeshPtr mesh, const std::vector<std::string>& colors, int materialIndex, Qt3DCore::QNode* parent) : Qt3DCore::QNode(parent)
    {
		generate(mesh, colors, materialIndex, parent);
    }

    RenderData::~RenderData()
    {
		m_geometry->deleteLater();
    }
    
    void RenderData::generate(TriMeshPtr mesh, const std::vector<std::string>& colors, int materialIndex, Qt3DCore::QNode* parent)
    {
		if (materialIndex < 0)
			materialIndex = 0;
			
		TriMeshPtr tempMesh(msbase::mergeColorMeshes(mesh.get(), colors, m_spreadFaces));
		trimesh::TriMesh* _mesh;
		if (tempMesh.get())
			_mesh = tempMesh.get();
		else
			_mesh = mesh.get();

		QSet<int> colorIndexs;
		std::vector<trimesh::vec3> tris;
		std::vector<float> flags;
		if (_mesh)
		{
			m_spreadFaceCount = _mesh->faces.size();
			tris.resize(m_spreadFaceCount * 3);
			flags.resize(m_spreadFaceCount * 3);

			if (_mesh->flags.size() == 0)
			{
				_mesh->flags.resize(m_spreadFaceCount);
			}

			for (int i = 0; i < m_spreadFaceCount; ++i)
			{
				int flag = _mesh->flags[i];
				if (flag == 0)
					colorIndexs.insert(materialIndex);
				else
					colorIndexs.insert(flag - 1);

				auto face = _mesh->faces[i];
				int triIndex = i * 3;
				tris[triIndex + 0] = _mesh->vertices[face[0]];
				tris[triIndex + 1] = _mesh->vertices[face[1]];
				tris[triIndex + 2] = _mesh->vertices[face[2]];
				flags[triIndex + 0] = flag;
				flags[triIndex + 1] = flag;
				flags[triIndex + 2] = flag;
			}
		}

		m_colorMap.clear();
		std::vector<trimesh::vec> colorsList = creative_kernel::currentColors();
		for (int index : colorIndexs)
		{
			m_colorMap[index] = colorsList[index];
		}
		m_colorIndexs = colorIndexs;
		//flags.clear();
		m_geometry = createTrianglesWithFlags(tris, flags, parent);
    }

	Qt3DRender::QGeometry* RenderData::createTrianglesWithFlags(const std::vector<trimesh::vec3>& tris, const std::vector<float>& flags, Qt3DCore::QNode* parent)
	{
		if (tris.size() <= 0 || flags.size() <= 0)
			return nullptr;

		int posNum = tris.size();
		int flagNum = flags.size();

		Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)tris.data(), posNum, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

		Qt3DRender::QAttribute* flagAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
			(const char*)flags.data(), flagNum, 1, "vertexFlag");

		Qt3DRender::QGeometry* geometry = qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttribute, flagAttribute);
		 geometry->setParent(parent);

		 connect(geometry, &QObject::destroyed, this, [=]()
			 {
				 qDebug() << "geometry destroyed";
			 });

		return geometry;
	}
}