#include "shareddatamanager.h"
#include "cxkernel/data/trimeshutils.h"

#include "qtusercore/module/progressortracer.h"

//#include <QtCore/QDebug>
//#include <QtCore/QFile>
//#include <QtCore/QDataStream>

#include "meshdatawrapper.h"
#include "spreaddatawrapper.h"
#include "idgenerator.h"
#include "shareddatatracer.h"

namespace creative_kernel
{
	SharedDataManager::SharedDataManager(Qt3DCore::QNode* parent) : Qt3DCore::QNode(parent)
	{
		m_generator = new IdGenerator(this);
		m_deleteDelayTimer.setSingleShot(true);
		connect(&m_deleteDelayTimer, &QTimer::timeout, this, [=] ()
		{
			m_deleteRenderDatas.clear();
		});
	}

	SharedDataManager::~SharedDataManager()
	{

	}

	SharedDataID SharedDataManager::registerModelData(ModelDataPtr modelData)
	{
		if (!modelData)
			return SharedDataID();

		int meshId = registerMeshData(modelData->mesh, true);
		int colorId = registerColor(modelData->colors);
		int seamId = registerSeam(modelData->seams);
		int supportId = registerSupport(modelData->supports);

		SharedDataID id(meshId, colorId, seamId, supportId, modelData->defaultColor);
		return id;
	}
	
    int SharedDataManager::registerMeshData(cxkernel::MeshDataPtr meshData, bool reuse)
	{
		if (meshData->mesh->vertices.empty())
			return -1;
		int id = m_generator->generateNewId("mesh");
		MeshDataWrapper* wrapper = new MeshDataWrapper(id, meshData);

		if (reuse)
		{
			for (DataSerial* serial : m_meshMap)
			{
				if (*serial == *wrapper)
				{
					delete wrapper;
					return serial->ID();
				}
			}
		}

		wrapper->store();
		wrapper->checkMesh();
		m_meshMap[id] = wrapper;
		return id;
	}

    int SharedDataManager::registerColor(const std::vector<std::string>& data)
	{
		if (data.empty())
			return -1;
		int id = m_generator->generateNewId("color");
		SpreadDataWrapper* wrapper = new SpreadDataWrapper(id, data, "color_spread_data");	// 颜色属性访问频率高，存放在缓存内
		for (DataSerial* serial : m_colorsMap)
		{
			if (*serial == *wrapper)
			{
				delete wrapper;
				return serial->ID();
			}
		}
		wrapper->store();
		m_colorsMap[id] = wrapper;
		return id;
	}

    int SharedDataManager::registerSeam(const std::vector<std::string>& data)
	{
		if (data.empty())
			return -1;
		int id = m_generator->generateNewId("seam");
		SpreadDataWrapper* wrapper = new SpreadDataWrapper(id, data, "seam_spread_data");
		for (DataSerial* serial : m_seamsMap)
		{
			if (*serial == *wrapper)
			{
				delete wrapper;
				return serial->ID();
			}
		}
		wrapper->store();
		m_seamsMap[id] = wrapper ;
		return id;
	}

    int SharedDataManager::registerSupport(const std::vector<std::string>& data)
	{
		if (data.empty())
			return -1;
		int id = m_generator->generateNewId("support");
		SpreadDataWrapper* wrapper = new SpreadDataWrapper(id, data, "support_spread_data");
		for (DataSerial* serial : m_supportsMap)
		{
			if (*serial == *wrapper)
			{
				delete wrapper;
				return serial->ID();
			}
		}
		wrapper->store();
		m_supportsMap[id] = wrapper;
		return id;
	}

	void SharedDataManager::coverMeshData(cxkernel::MeshDataPtr meshData, int meshID)
	{
		if (meshData->mesh->vertices.empty())
			return ;

		unregisterMeshData(meshID);
		int id = meshID;
		MeshDataWrapper* wrapper = new MeshDataWrapper(id, meshData);
		for (DataSerial* serial : m_meshMap)
		{
			if (*serial == *wrapper)
			{
				unregisterMeshData(serial->ID());
				break;
			}
		}
		wrapper->store();
		m_meshMap[id] = wrapper;

		SharedDataID dataID;
		dataID(MeshID) = meshID;
		clearRenderData(dataID);
	}

	void SharedDataManager::unregisterMeshData(int meshID)
	{
		if (!m_meshMap.contains(meshID))
			return;

		delete m_meshMap[meshID];
		m_meshMap.remove(meshID);
	}

	void SharedDataManager::unregisterColor(int colorID)
	{
		if (!m_colorsMap.contains(colorID))
			return;

		delete m_colorsMap[colorID];
		m_colorsMap.remove(colorID);
	}

	void SharedDataManager::unregisterSeam(int seamID)
	{
		if (!m_seamsMap.contains(seamID))
			return;

		delete m_seamsMap[seamID];
		m_seamsMap.remove(seamID);
	}

	void SharedDataManager::unregisterSupport(int supportID)
	{
		if (!m_supportsMap.contains(supportID))
			return;

		delete m_supportsMap[supportID];
		m_supportsMap.remove(supportID);
	}

	void SharedDataManager::clear()
	{
		qDebug() << "clear SharedDataManager";
		for (MeshDataWrapper* wrapper : m_meshMap)
			delete wrapper;
		m_meshMap.clear();

		for (SpreadDataWrapper* wrapper : m_colorsMap)
			delete wrapper;
		m_colorsMap.clear();

		for (SpreadDataWrapper* wrapper : m_seamsMap)
			delete wrapper;
		m_seamsMap.clear();

		for (SpreadDataWrapper* wrapper : m_supportsMap)
			delete wrapper;
		m_supportsMap.clear();
		m_renderDatasMap.clear();
	}

	cxkernel::MeshDataPtr SharedDataManager::getMeshData(int id) const
	{
		if (m_meshMap.find(id) != m_meshMap.cend())
		{
			return m_meshMap[id]->data();
		}
		else 
		{
			assert(false);
			return NULL;
		}
	}

	cxkernel::MeshDataPtr SharedDataManager::getMeshData(SharedDataID id) const
	{
		if (m_meshMap.find(id.renderDataId.meshId) != m_meshMap.cend())
		{
			return m_meshMap[id.renderDataId.meshId]->data();
		}
		else 
		{
			return NULL;
		}
	}

	bool SharedDataManager::getMeshNeedRepair(int id)
	{
		if (m_meshMap.find(id) != m_meshMap.cend())
		{
			return m_meshMap[id]->needRepair();
		}
		else 
		{
			return false;
		}
	}

	bool SharedDataManager::getMeshNeedRepair(SharedDataID id)
	{
		if (m_meshMap.find(id.renderDataId.meshId) != m_meshMap.cend())
		{
			return m_meshMap[id.renderDataId.meshId]->needRepair();
		}
		else 
		{
			return false;
		}
	}

	std::vector<std::string> SharedDataManager::getColors(int id) const 
	{
		if (m_colorsMap.find(id) != m_colorsMap.cend())
		{
			return m_colorsMap[id]->data();
		}
		else 
		{
			return std::vector<std::string>();
		}
	}

	std::vector<std::string> SharedDataManager::getColors(SharedDataID id) const 
	{
		if (m_colorsMap.find(id.renderDataId.colorsId) != m_colorsMap.cend())
		{
			return m_colorsMap[id.renderDataId.colorsId]->data();
		}
		else 
		{
			return std::vector<std::string>();
		}
	}

	std::vector<std::string> SharedDataManager::getSeams(int id) const
	{
		if (m_seamsMap.find(id) != m_seamsMap.cend())
		{
			return m_seamsMap[id]->data();
		}
		else 
		{
			return std::vector<std::string>();
		}
	}

	std::vector<std::string> SharedDataManager::getSeams(SharedDataID id) const 
	{
		if (m_seamsMap.find(id.seamsId) != m_seamsMap.cend())
		{
			return m_seamsMap[id.seamsId]->data();
		}
		else 
		{
			return std::vector<std::string>();
		}
	}

	std::vector<std::string> SharedDataManager::getSupports(int id) const 
	{
		if (m_supportsMap.find(id) != m_supportsMap.cend())
		{
			return m_supportsMap[id]->data();
		}
		else 
		{
			return std::vector<std::string>();
		}
	}

	std::vector<std::string> SharedDataManager::getSupports(SharedDataID id) const
	{
		if (m_supportsMap.find(id.supportsId) != m_supportsMap.cend())
		{
			return m_supportsMap[id.supportsId]->data();
		}
		else 
		{
			return std::vector<std::string>();
		}
	}

	RenderDataPtr SharedDataManager::getRenderData(SharedDataID id)
	{
		if (m_renderDatasMap.contains(id.renderDataId))
		{
			return m_renderDatasMap[id.renderDataId];
		}
		else 
		{
			m_mutex.lock();

			if (m_renderDatasMap.contains(id.renderDataId))
			{
				m_mutex.unlock();
				return m_renderDatasMap[id.renderDataId];
			}

			cxkernel::MeshDataPtr meshData = getMeshData(id);
			if (!meshData)
			{
				m_mutex.unlock();
				return NULL;
			}
			TriMeshPtr mesh = meshData->mesh;
			
			SpreadDataWrapper* colorWrapper = NULL;
			std::vector<std::string> colors = getColors(id);
			if (!colors.empty())
				colorWrapper = m_colorsMap[id(ColorsID)];

			qDebug() << QString("create renderData: (%1 %2 %3)").arg(id(MeshID)).arg(id(ColorsID)).arg(id(MaterialID));
			RenderData* renderData = new RenderData(mesh, colors, id(MaterialID), (Qt3DCore::QNode*)this);
			m_renderDatasMap[id.renderDataId] = RenderDataPtr(renderData);

			if (colorWrapper)
				colorWrapper->setUsedIndex(renderData->colorIndexs());

			m_mutex.unlock();
			return m_renderDatasMap[id.renderDataId];
		}
	}

	void SharedDataManager::updateRenderData(bool regenerate)
	{
		if (regenerate)
			m_renderDatasMap.clear();
		
		for (SharedDataTracer* tracer : m_tracers)
		{
			tracer->beginUpdateRender();
			tracer->updateRender();
			tracer->endUpdateRender();
		}
	}

	void SharedDataManager::updateRenderData(QList<int> colorsIDs)
	{
		if (colorsIDs.empty())
			return;
			
		auto it = m_renderDatasMap.begin(), end = m_renderDatasMap.end();
		for (; it != end; )
		{
			RenderDataID renderDataID = it.key();
			auto tempIt = it;
			it++;
			if (colorsIDs.contains(renderDataID.colorsId))
				m_renderDatasMap.remove(renderDataID);
		}
		
		for (SharedDataTracer* tracer : m_tracers)
		{
			int colorID = tracer->colorsID(); 
			if (colorsIDs.contains(colorID))
			{
				tracer->beginUpdateRender();
				tracer->updateRender();
				tracer->endUpdateRender();
			}
		}
	}

	void SharedDataManager::updateRenderData(QList<RenderDataID> renderDataIDs)
	{
		if (renderDataIDs.empty())
			return;

		auto it = m_renderDatasMap.end() - 1, end = m_renderDatasMap.begin();
		for (; ; it--)
		{
			RenderDataID renderDataID = it.key();
			if (renderDataIDs.contains(renderDataID))
				m_renderDatasMap.remove(renderDataID);

			if (it == end)
				break;
		}
		
		for (SharedDataTracer* tracer : m_tracers)
		{
			RenderDataID renderDataID = tracer->renderDataID(); 
			if (renderDataIDs.contains(renderDataID))
			{
				tracer->beginUpdateRender();
				tracer->updateRender();
				tracer->endUpdateRender();
			}
		}
	}

	void SharedDataManager::discardColorIndex(int index)
	{
		QList<int> ids;
		for (SpreadDataWrapper* wrapper : m_colorsMap)
		{
			if (wrapper->hasIndex(index))
			{
				wrapper->discardIndex(index);
				ids << wrapper->ID();
			}
		}

		for (SharedDataTracer* tracer : m_tracers)
		{
			int materialID = tracer->materialID(); 
			if (materialID == index)
				tracer->onDiscardMaterial(index);
		}
	}

	void SharedDataManager::discardColorIndexMoreThan(int index)
	{
		QList<int> ids;
		for (SpreadDataWrapper* wrapper : m_colorsMap)
		{
			if (wrapper->hasIndexMoreThan(index))
			{
				wrapper->discardIndexMoreThan(index);
				ids << wrapper->ID();
			}
		}

		for (SharedDataTracer* tracer : m_tracers)
		{
			int materialID = tracer->materialID(); 
			if (materialID > index)
				tracer->setMaterialID(0);
		}
		
		updateRenderData(ids);
	}

	void SharedDataManager::clearExcessData()
	{
		QList<int> meshIDList, colorsIDList, seamsIDList, supportsIDList;
		auto meshIt = m_meshMap.begin(), meshEnd = m_meshMap.end();
		for (; meshIt != meshEnd; ++meshIt)
			meshIDList << meshIt.key();

		auto colorIt = m_colorsMap.begin(), colorEnd = m_colorsMap.end();
		for (; colorIt != colorEnd; ++colorIt)
			colorsIDList << colorIt.key();
			
		auto seamIt = m_seamsMap.begin(), seamEnd = m_seamsMap.end();
		for (; seamIt != seamEnd; ++seamIt)
			seamsIDList << seamIt.key();
			
		auto supportIt = m_supportsMap.begin(), supportEnd = m_supportsMap.end();
		for (; supportIt != supportEnd; ++supportIt)
			supportsIDList << supportIt.key();
		
		QList<RenderDataID> renderDataIDList;
		auto renderDataIt = m_renderDatasMap.begin(), renderDataEnd = m_renderDatasMap.end();
		for (; renderDataIt != renderDataEnd; ++renderDataIt)
			renderDataIDList << renderDataIt.key();
	
		for (SharedDataTracer* tracer : m_tracers)
		{
			meshIDList.removeOne(tracer->meshID());
			colorsIDList.removeOne(tracer->colorsID());
			seamsIDList.removeOne(tracer->seamsID());
			supportsIDList.removeOne(tracer->supportsID());
			renderDataIDList.removeOne(tracer->renderDataID());
		}

		for (int id : meshIDList)
			unregisterMeshData(id);

		for (int id : colorsIDList)
			unregisterColor(id);

		for (int id : seamsIDList)
			unregisterSeam(id);
			
		for (int id : supportsIDList)
			unregisterSupport(id);

		for (RenderDataID id : renderDataIDList)
		{
			if (!m_renderDatasMap.contains(id))
				continue;

			m_renderDatasMap.remove(id);
		}
	}

	void SharedDataManager::addTracer(SharedDataTracer* tracer)
	{
		m_tracers << tracer;
	}

	void SharedDataManager::removeTracer(SharedDataTracer* tracer)
	{
		m_tracers.removeOne(tracer);
		//clearExcessData();
	}
	
	void SharedDataManager::onSharedDataIDChanged(SharedDataTracer* tracer, SharedDataID oldID, SharedDataID newID)
	{
		//clearExcessData();
	}

	void SharedDataManager::clearRenderData(SharedDataID dataID)
	{
		int meshID = dataID(MeshID);
		//int colorID = dataID(ColorsID);
		//int materialID = dataID(MaterialID);
		//int type = dataID(ModelType);

		//bool clearAll = meshID < 0 || 
		//				colorID < 0 || 
		//				materialID < 0 ||
		//				type < 0;

		auto it = m_renderDatasMap.begin(), end = m_renderDatasMap.end();
		for (; it != end;)
		{
			RenderDataID id = it.key();
			++it;
			if (meshID >= 0 && id.meshId == meshID)
			{
				m_deleteRenderDatas << m_renderDatasMap[id];		
				m_renderDatasMap.remove(id);
				m_deleteDelayTimer.start(500);
			}
		}
	}

}