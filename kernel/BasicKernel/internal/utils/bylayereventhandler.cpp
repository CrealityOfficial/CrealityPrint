#include "bylayereventhandler.h"
#include "data/modeln.h"
#include "nestplacer/printpiece.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "kernel/kernelui.h"
#include "external/message/modelcollisionmessage.h"

namespace creative_kernel
{
	ByLayerEventHandler::ByLayerEventHandler(QObject* parent)
		:PMCollisionCheckHandler(parent)
	{
		
	}
	
	ByLayerEventHandler::~ByLayerEventHandler()
	{
		clearCollisionFlags();
	}

	void ByLayerEventHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{

		Printer* sel = getPrinterManager()->getSelectedPrinter();
		if (!sel)
		{
			return;
		}
		QList<ModelN*> models = sel->currentModelsInsideVisible();
		checkModels(models);
	}

	void ByLayerEventHandler::clearCollisionFlags()
	{
		getKernelUI()->destroyMessage(MessageType::ModelCollision);
	}

	QList<ModelN*> ByLayerEventHandler::checkModels(const QList<ModelN*>& models)
	{
		return QList<ModelN*>();

		if (models.isEmpty())
		{
			getKernelUI()->destroyMessage(MessageType::ModelCollision);
			return QList<ModelN*>();
		}

		std::vector<nestplacer::PR_Polygon> polys;
		std::vector<nestplacer::PR_RESULT> results;

		for (ModelN* m : models)
		{
			std::vector<trimesh::vec3> paths = m->rsPath();
			QVector3D pos = m->localPosition();
			trimesh::fxform xf = trimesh::fxform::trans(trimesh::vec3(pos.x(), pos.y(), 0.0f));
			for (trimesh::vec3& point : paths)
				point = xf * point;

			polys.emplace_back(paths);
		}

		//check collide
		//std::string str("D:\\result.out");
		nestplacer::collisionCheck(polys, results, true, false, nullptr);

		QSet<int> collideIndexs;

		for (size_t i = 0; i < results.size(); i++)
		{
			const nestplacer::PR_RESULT& res = results.at(i);
			if (res.state == nestplacer::ContactState::INTERSECT)
			{
				if (models.size() > res.first && models.size() > res.second)
				{
					collideIndexs.insert(res.first);
					collideIndexs.insert(res.second);
				}
			}
		}

		QList<int> indexList = collideIndexs.toList();
		qSort(indexList.begin(), indexList.end());


		QList<ModelN*> collideModels;
		for (QList<int>::iterator it = indexList.begin(); it != indexList.end(); ++it)
		{
			int idx = *it;
			if (0 <= idx && idx < models.size())
			{
				auto m = models.at(idx);
				collideModels.push_back(m);
			}
		}

		if (collideModels.size() > 0)
		{
			ModelNCollisionMessage* msg = new ModelNCollisionMessage(collideModels);
			msg->setLevel(1);
			getKernelUI()->requestMessage(msg);
		}
		else {
			getKernelUI()->destroyMessage(MessageType::ModelCollision);
		}

		return collideModels;
	}
}