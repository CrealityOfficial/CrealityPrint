#include "modelnrenderdata.h"
#include "kernel/kernel.h"
#include "kernel/reuseablecache.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "cxkernel/data/trimeshutils.h"
#include "msbase/mesh/deserializecolor.h" 
#include "interface/modelinterface.h"
#include "external/data/modelnrenderdata.h"
#include "interface/machineinterface.h"

namespace creative_kernel
{
	ModelNRenderData::ModelNRenderData(cxkernel::ModelNDataPtr data)
		:m_data(data)
		, m_geometry(nullptr)
	{
		if (data->mesh->colors.size() == 0 && data->colors.size() == 0)
			updateIndexRenderData();
		else
			updateRenderData();
	}

	ModelNRenderData::~ModelNRenderData()
	{
		if (m_geometry)
		{
			m_geometry->deleteLater();
		}
	}

	int ModelNRenderData::checkMaterialChanged()
	{
		std::vector<trimesh::vec> colors = creative_kernel::currentColors();
		auto it = m_data->colorMap.begin(), end = m_data->colorMap.end();
		for (; it != end; ++it)
		{
			int index = it.key();
			if (index >= colors.size())
				return index;

			if (colors[index] != it.value())
			{
				return index;
			}
		}
		return -1;
	}

	void ModelNRenderData::discardMaterial(int materialIndex)
	{
		msbase::change_all_triangles_state(m_data->colors, materialIndex);
		// msbase::mergeColorMeshes(m_data->mesh.get(), m_data->colors, m_data->spreadFaces);
	}

	cxkernel::ModelNDataPtr ModelNRenderData::data()
	{
		return m_data;
	}

	Qt3DRender::QGeometry* ModelNRenderData::geometry()
	{
		return m_geometry;
	}

	void ModelNRenderData::createGeometry()
	{
		if (!m_data)
			return;

		destroyGeometry();

		cxkernel::GeometryData geometryData;
		auto tempMesh = msbase::mergeColorMeshes(m_data->mesh.get(), m_data->colors, m_data->spreadFaces);
		QSet<int> colorIndexs;
		if (tempMesh)
		{
			TriMeshPtr renderMesh(tempMesh);
			applyMaterialColorsToMesh(renderMesh.get(), m_data->defaultColor, colorIndexs);
			generateGeometryDataFromMesh(renderMesh.get(), geometryData);
			m_data->spreadFaceCount = renderMesh->faces.size();
		}
		else
		{
			applyMaterialColorsToMesh(m_data->mesh.get(), m_data->defaultColor, colorIndexs);
			generateGeometryDataFromMesh(m_data->mesh.get(), geometryData);
			m_data->spreadFaceCount = m_data->mesh->faces.size();
		}
		// colorIndexs.insert(m_data->defaultColor);

		m_data->colorMap.clear();
		std::vector<trimesh::vec> colors = creative_kernel::currentColors();
		for (int index : colorIndexs)
		{
			m_data->colorMap[index] = colors[index];
		}
		m_data->colorIndexs = colorIndexs;

		Qt3DCore::QNode* parent = getKernel()->reuseableCache();
		if (geometryData.indiceCount > 0)
		{
			m_geometry = qtuser_3d::GeometryCreateHelper::indexCreate(geometryData.vcount, geometryData.position, geometryData.color, 3 * geometryData.fcount, geometryData.indices, parent);
		}
		else
		{
			m_geometry = qtuser_3d::GeometryCreateHelper::create(geometryData.vcount, geometryData.position, geometryData.normal, geometryData.texcoord, geometryData.color, parent);
		}
	}

	void ModelNRenderData::destroyGeometry()
	{
		if (m_geometry)
		{
			delete m_geometry;
			m_geometry = nullptr;
		}
	}

	void ModelNRenderData::updateIndexRenderData()
	{
		if (m_data->mesh )
		{
			createGeometry();
		}
	}

	void ModelNRenderData::updateRenderData()
	{
		if (m_data->mesh )
		{
			createGeometry();
		}
	}

	void ModelNRenderData::updateRenderDataForced()
	{
		createGeometry();
	}
};
