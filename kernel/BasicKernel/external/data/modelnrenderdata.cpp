#include "modelnrenderdata.h"
#include "kernel/kernel.h"
#include "kernel/reuseablecache.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "cxkernel/data/trimeshutils.h"

namespace creative_kernel
{
	ModelNRenderData::ModelNRenderData(cxkernel::ModelNDataPtr data)
		:m_data(data)
		, m_geometry(nullptr)
	{
		if (data->mesh->colors.size() == 0)
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

		Qt3DCore::QNode* parent = getKernel()->reuseableCache();

		const cxkernel::GeometryData& data = m_geometryData;
		if (data.indiceCount > 0)
		{
			m_geometry = qtuser_3d::GeometryCreateHelper::indexCreate(data.vcount, data.position, data.color, 3 * data.fcount, data.indices, parent);
		}
		else {
			m_geometry = qtuser_3d::GeometryCreateHelper::create(data.vcount, data.position, data.normal,
				data.texcoord, data.color, parent);
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
		if (m_data->mesh && ((int)m_data->mesh->faces.size() != m_geometryData.fcount))
		{
			cxkernel::generateIndexGeometryDataFromMesh(m_data->mesh.get(), m_geometryData);
			createGeometry();
		}
	}

	void ModelNRenderData::updateRenderData()
	{
		if (m_data->mesh && ((int)m_data->mesh->faces.size() != m_geometryData.fcount))
		{
			cxkernel::generateGeometryDataFromMesh(m_data->mesh.get(), m_geometryData);
			createGeometry();
		}
	}

	void ModelNRenderData::updateRenderDataForced()
	{
		cxkernel::generateGeometryDataFromMesh(m_data->mesh.get(), m_geometryData);
		createGeometry();
	}
};
