#include "shareddatatracer.h"
#include "shareddatamanager.h"
#include "kernel/kernel.h"

namespace creative_kernel
{
	SharedDataTracer::SharedDataTracer()
	{
		m_manager = getKernel()->sharedDataManager();
		m_manager->addTracer(this);
	}

	SharedDataTracer::~SharedDataTracer()
	{
		m_manager->removeTracer(this);
	}

	SharedDataID SharedDataTracer::sharedDataID()
	{
		return m_id;
	}

	void SharedDataTracer::setSharedDataID(SharedDataID id)
	{
		if (m_id != id)
		{
			SharedDataID oldID = m_id;
			m_id = id;
			m_manager->onSharedDataIDChanged(this, oldID, id);
		}
	}

	int SharedDataTracer::meshID()
	{
		return m_id(MeshID);
	}

	void SharedDataTracer::setMeshID(int meshID)
	{
		SharedDataID newID = m_id;
		newID(MeshID) = meshID;
		setSharedDataID(newID);
	}

	int SharedDataTracer::colorsID()
	{
		return m_id(ColorsID);
	}

	void SharedDataTracer::setColorsID(int colorsID)
	{
		SharedDataID newID = m_id;
		newID(ColorsID) = colorsID;
		setSharedDataID(newID);
	}

	int SharedDataTracer::seamsID()
	{
		return m_id(SeamsID);
	}

	void SharedDataTracer::setSeamsID(int seamsID)
	{
		SharedDataID newID = m_id;
		newID(SeamsID) = seamsID;
		setSharedDataID(newID);
	}
	
	int SharedDataTracer::supportsID()
	{
		return m_id(SupportsID);
	}

	void SharedDataTracer::setSupportsID(int supportsID)
	{
		SharedDataID newID = m_id;
		newID(SupportsID) = supportsID;
		setSharedDataID(newID);
	}
	
	int SharedDataTracer::materialID()
	{
		return m_id(MaterialID);
	}

	void SharedDataTracer::setMaterialID(int materialID)
	{
		SharedDataID newID = m_id;
		newID(MaterialID) = materialID;
		setSharedDataID(newID);
	}

	ModelNType SharedDataTracer::modelType()
	{
		return (ModelNType)m_id(ModelType);
	}

	void SharedDataTracer::setModelType(ModelNType type)
	{
		SharedDataID newID = m_id;
		newID(ModelType) = (int)type;
		setSharedDataID(newID);
	}

	void SharedDataTracer::setModelType(int type)
	{
		SharedDataID newID = m_id;
		newID(ModelType) = type;
		setSharedDataID(newID);
	}
	
	RenderDataID SharedDataTracer::renderDataID()
	{
		return m_id.renderDataId;
	}

	void SharedDataTracer::setRenderDataID(RenderDataID renderDataID)
	{
		SharedDataID newID = m_id;
		newID.renderDataId = renderDataID;
		setSharedDataID(newID);
	}

	void SharedDataTracer::beginUpdateRender()
	{

	}

	void SharedDataTracer::updateRender()
	{

	}

	void SharedDataTracer::endUpdateRender()
	{

	}

	void SharedDataTracer::onDiscardMaterial(int materialIndex)
	{

	}

}