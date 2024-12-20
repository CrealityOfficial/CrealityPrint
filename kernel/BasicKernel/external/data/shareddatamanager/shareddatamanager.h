#ifndef CREATIVE_KERNEL_MODELNDATAMANAGER_1681019989200_H
#define CREATIVE_KERNEL_MODELNDATAMANAGER_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/rawdata.h"
#include <QHash>
#include <Qt3DCore/qnode.h>
#include <Qt3DRender/qgeometry.h>
#include "cxkernel/data/meshdata.h"
#include "shareddataid.h"
#include "renderdata.h"
#include <cereal/archives/binary.hpp>
#include <QTimer>
#include <QMutex>

namespace creative_kernel
{
	class IdGenerator;
	class MeshDataWrapper;
	class SpreadDataWrapper;
	class SharedDataTracer;
	class BASIC_KERNEL_API SharedDataManager : public Qt3DCore::QNode
	{
	public:
		SharedDataManager(Qt3DCore::QNode* parent);
		~SharedDataManager();

		SharedDataID registerModelData(ModelDataPtr modelData);
        int registerMeshData(cxkernel::MeshDataPtr meshData, bool reuse);
        int registerColor(const std::vector<std::string>& data);
        int registerSeam(const std::vector<std::string>& data);
        int registerSupport(const std::vector<std::string>& data);

        void coverMeshData(cxkernel::MeshDataPtr meshData, int meshID);	// cover or register MeshData

		void clear();

		cxkernel::MeshDataPtr getMeshData(int id) const;
		cxkernel::MeshDataPtr getMeshData(SharedDataID id) const;
		bool getMeshNeedRepair(int id);
		bool getMeshNeedRepair(SharedDataID id);

		std::vector<std::string> getColors(int id) const;
		std::vector<std::string> getColors(SharedDataID id) const;

		std::vector<std::string> getSeams(int id) const;
		std::vector<std::string> getSeams(SharedDataID id) const;

		std::vector<std::string> getSupports(int id) const;
		std::vector<std::string> getSupports(SharedDataID id) const;

		RenderDataPtr getRenderData(SharedDataID id);
		void updateRenderData(bool regenerate);
		void updateRenderData(QList<int> colorsIDs);
		void updateRenderData(QList<RenderDataID> renderDataIDs);
		void discardColorIndex(int index);
		void discardColorIndexMoreThan(int index);

		void clearExcessData();	// 删除没有被引用的数据

	/* tracer */
		void addTracer(SharedDataTracer* tracer);
		void removeTracer(SharedDataTracer* tracer);
		void onSharedDataIDChanged(SharedDataTracer* tracer, SharedDataID oldID, SharedDataID newID);


	private:
		void clearRenderData(SharedDataID dataID = SharedDataID());
        void unregisterMeshData(int meshID);
        void unregisterColor(int colorID);
        void unregisterSeam(int seamID);
        void unregisterSupport(int supportID);

	private:
		IdGenerator* m_generator;

		QHash<int, MeshDataWrapper*> m_meshMap;
		QHash<int, SpreadDataWrapper*> m_colorsMap;
		QHash<int, SpreadDataWrapper*> m_seamsMap;
		QHash<int, SpreadDataWrapper*> m_supportsMap;

		QMap<RenderDataID, RenderDataPtr> m_renderDatasMap;

		QList<SharedDataTracer*> m_tracers;

		QList<RenderDataPtr> m_deleteRenderDatas;
		QTimer m_deleteDelayTimer;

		QMutex m_mutex;

	};

}

#endif // CREATIVE_KERNEL_MODELNDATAMANAGER_1681019989200_H