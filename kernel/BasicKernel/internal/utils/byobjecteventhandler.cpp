#include "byobjecteventhandler.h"
#include "internal/multi_printer/printer.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/printerinterface.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "nestplacer/printpiece.h"
#include "kernel/kernelui.h"
#include "external/message/modelcollisionmessage.h"
#include "external/message/modeltootallmessage.h"
#include "external/kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "external/kernel/visualscene.h"

namespace creative_kernel {

	ByObjectEventHandler::ByObjectEventHandler(QObject* parent)
		:PMCollisionCheckHandler(parent)
		, m_enable(false)
		, m_processing(false)
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		connect(parameterManager, &ParameterManager::extruderClearanceHeightToRodChanged,
			this, &ByObjectEventHandler::onExtruderClearanceHeightToLodChanged);
		
		getKernel()->visScene()->heightReferEntity()->setEnabled(true);
	}

	ByObjectEventHandler::~ByObjectEventHandler()
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		disconnect(parameterManager, &ParameterManager::extruderClearanceHeightToLidChanged,
			this, &ByObjectEventHandler::onExtruderClearanceHeightToLodChanged);

		clearCollisionFlags();
		getKernelUI()->destroyMessage(MessageType::ModelCollision);
		getKernelUI()->destroyMessage(MessageType::ModelTooTall);

		getKernel()->visScene()->heightReferEntity()->setEnabled(false);
	}

	QList<ModelN*> ByObjectEventHandler::collisionCheckAndUpdateState(const QList<ModelN*>& models)
	{
		QList<ModelN*> list = _collisionCheck(models);
		setStateCollide(list);


		QList<ModelN*> tallList = _modelHeightCheck(models);
		setStateTooTall(tallList);

		return list + tallList;
	}

	/*void ByObjectEventHandler::initialize()
	{
		connect(getPrinterManager(), &PrinterMangager::signalDidSelectPrinter, this, &ByObjectEventHandler::slotDidSelectPrinter);
	}*/

	void ByObjectEventHandler::setStateNone(ModelN* m)
	{
		m->setOuterLinesVisibility(false);
		m->setNozzleRegionVisibility(false);
	}

	void ByObjectEventHandler::setStateCollide(ModelN* m)
	{
		m->setNozzleRegionVisibility(true);
		m->setOuterLinesVisibility(true);
		m->setOuterLinesColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
	}

	void ByObjectEventHandler::setStateCollide(const QList<ModelN*>& models)
	{
		for (auto m : models)
		{
			setStateCollide(m);
		}

		if (models.size() > 0)
		{
			/*ModelN* first = collideModels.first();
			QString str = first->objectName();*/

			ModelNCollisionMessage* msg = new ModelNCollisionMessage(models);
			msg->setLevel(2);
			getKernelUI()->requestMessage(msg);
		}
		else {

			getKernelUI()->destroyMessage(MessageType::ModelCollision);
		}
	}

	void ByObjectEventHandler::setStateTooTall(const QList<ModelN*>& tallList)
	{
		if (tallList.size() > 0)
		{
			ModelTooTallMessage* msg = new ModelTooTallMessage(tallList);
			getKernelUI()->requestMessage(msg);
		}
		else {
			getKernelUI()->destroyMessage(MessageType::ModelTooTall);
		}
	}


	void ByObjectEventHandler::setStateMoving(ModelN* m)
	{
		m->setOuterLinesVisibility(true);
		m->setOuterLinesColor(QVector4D(0.6f, 0.6f, 0.6f, 1.0f));
		m->setNozzleRegionVisibility(false);
	}

	void ByObjectEventHandler::clearCollisionFlags()
	{
		clearCollisionFlags(m_modelPtrs);
	}

	QList<ModelN*> ByObjectEventHandler::checkModels(const QList<ModelN*>& models)
	{
		updateHeightReferEntitySize();

		return collisionCheckAndUpdateState(models);
	}

	void ByObjectEventHandler::clearCollisionFlags(const QList<QPointer<ModelN>>& modelPtrs)
	{
		for (ModelN* m : modelPtrs)
		{
			if (m == nullptr)
			{
				continue;
			}
			setStateNone(m);
		}
	}

	QList<ModelN*> ByObjectEventHandler::_collisionCheck(const QList<ModelN*>& models)
	{
		clearCollisionFlags(m_modelPtrs);

		m_modelPtrs.clear();
		for (auto m : models)
		{
			m_modelPtrs.push_back(QPointer<ModelN>(m));
		}
		return _collisionCheck(m_modelPtrs);
	}

	QList<ModelN*> ByObjectEventHandler::_collisionCheck(const QList<QPointer<ModelN>>& modelPtrs)
	{
		QList<ModelN*> models;
		for (auto m : modelPtrs)
		{
			if (m.isNull())
			{
				continue;
			}
			models.push_back(m.data());
		}

		if (models.isEmpty())
		{
			return QList<ModelN*>();
		}

		return ByObjectEventHandler::collisionCheck(models);
	}

	void ByObjectEventHandler::onLeftMouseButtonPress(QMouseEvent* event)
	{
		m_processing = false;

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == nullptr/* || pickable->selected() == false*/)
			return;

		ModelN* pick = qobject_cast<ModelN*>(pickable);
		/*if (!pick)
		{
			return;
		}*/

		Printer* sel = getPrinterManager()->getSelectedPrinter();
		if (!sel)
		{
			return;
		}

		clearCollisionFlags(m_modelPtrs);
		m_modelPtrs.clear();

		QList<ModelN*> models = sel->currentModelsInsideVisible();
		if (pick && !models.contains(pick))
		{
			models.append(pick);
		}
		
		for (auto m : models)
		{
			QPointer<ModelN> p = m;
			m_modelPtrs.push_back(p);
			setStateMoving(m);
		}
		m_processing = true;
	}

	void ByObjectEventHandler::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		if (m_processing == false)
			return;

		clearCollisionFlags(m_modelPtrs);
		QList<ModelN*> list = _collisionCheck(m_modelPtrs);
		setStateCollide(list);

		_modelHeightCheckForSelectedPrinter();


		m_processing = false;
	}
	
	bool ByObjectEventHandler::processing()
	{
		return m_processing;
	}

	QList<ModelN*> ByObjectEventHandler::_modelHeightCheck(const QList<ModelN*>& inputModels)
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		float maxH = parameterManager->extruderClearanceHeightToRod();
		return ByObjectEventHandler::heightCheck(inputModels, maxH);
	}


	QList<ModelN*> ByObjectEventHandler::_modelHeightCheckForSelectedPrinter()
	{
		Printer* sel = getPrinterManager()->getSelectedPrinter();
		if (!sel)
		{
			return QList<ModelN*>();
		}
		QList<ModelN*> models = sel->currentModelsInsideVisible();
		QList<ModelN*> tallList = _modelHeightCheck(models);
		setStateTooTall(tallList);
		return tallList;

	}

	void ByObjectEventHandler::onExtruderClearanceHeightToLodChanged(float h)
	{
		updateHeightReferEntitySize();
		_modelHeightCheckForSelectedPrinter();
	}

	QList<ModelN*> ByObjectEventHandler::collisionCheck(const QList<ModelN*>& models)
	{
		return QList<ModelN*>();

		std::vector<nestplacer::PR_Polygon> polys;
		std::vector<nestplacer::PR_RESULT> results;

		for (ModelN* m : models)
		{
			std::vector<trimesh::vec3> paths = m->sweepAreaPath();
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

		return collideModels;
	}

	QList<ModelN*> ByObjectEventHandler::heightCheck(const QList<ModelN*>& inputModels, float heightToRod)
	{
		QList<ModelN*> models;
		for (auto m : inputModels)
		{
			qtuser_3d::Box3D modelBox = m->globalSpaceBox();
			if (heightToRod > 0 && modelBox.max.z() > heightToRod)
			{
				models.push_back(m);
			}
		}

		return models;
	}

	void ByObjectEventHandler::updateHeightReferEntitySize()
	{
		float maxH = extruderClearanceHeightToRod();

		qtuser_3d::Box3D box = getSelectedPrinter()->globalBox();

		QMatrix4x4 m;
		m.translate(box.min);

		QVector3D size = box.size();
		size.setZ(maxH);

		m.scale(size);

		getKernel()->visScene()->heightReferEntity()->setPose(m);
	}

	float extruderClearanceHeightToRod()
	{
		ParameterManager* parameterManager = getKernel()->parameterManager();
		float maxH = parameterManager->extruderClearanceHeightToRod();
		return maxH;
	}
}

