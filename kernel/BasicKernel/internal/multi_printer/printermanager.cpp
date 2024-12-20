#include  <cfloat>
#include "printermanager.h"
#include "external/kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "external/data/modeln.h"
#include "external/data/modelgroup.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "internal/render/modelnentity.h"
#include "interface/camerainterface.h"
#include "interface/selectorinterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "internal/render/printerentity.h"
#include "internal/render/wipetowerentity.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/materialcenter.h"
#include "internal/utils/byobjecteventhandler.h"
#include "internal/utils/bylayereventhandler.h"
#include "interface/machineinterface.h"
#include "interface/eventinterface.h"
#include "kernel/kernelui.h"
#include "external/message/stayonbordermessage.h"
#include "interface/renderinterface.h"
#include "interface/displaywarninginterface.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace creative_kernel
{
	PrinterMangager::PrinterMangager(QObject* parent)
		: QObject(parent)
		, m_tracer(nullptr)
		, m_collisionCheckHandler(nullptr)
	{

	}

	PrinterMangager::~PrinterMangager()
	{
	}

	void PrinterMangager::initialize()
	{
		MaterialCenter* materialCenter = getKernel()->materialCenter();
		connect(materialCenter, &MaterialCenter::dirtied, this, &PrinterMangager::dirtyAllPrinters);

		ParameterManager* parameterManager = getKernel()->parameterManager();
		connect(parameterManager, &ParameterManager::parameterEdited, this, &PrinterMangager::onGlobalSettingsChanged);
		connect(parameterManager, &ParameterManager::currentColorsChanged, this, &PrinterMangager::updateWipeTowers);

		connect(parameterManager, &ParameterManager::enablePrimeTowerChanged, this, &PrinterMangager::onWipeTowerEnable);
		connect(parameterManager, &ParameterManager::currentProcessLayerHeightChanged, this, &PrinterMangager::onLayerHeightChanged);
		connect(parameterManager, &ParameterManager::currentProcessPrimeVolumeChanged, this, &PrinterMangager::onWipeTowerVolumeChanged);
		connect(parameterManager, &ParameterManager::currentProcessPrimeTowerWidthChanged, this, &PrinterMangager::onWipeTowerWidthChanged);
		connect(parameterManager, &ParameterManager::enableSupportChanged, this, &PrinterMangager::onSupportEnable);
		connect(parameterManager, &ParameterManager::currentProcessSupportFilamentChanged, this, &PrinterMangager::onSupportFilamentChanged);
		connect(parameterManager, &ParameterManager::currentProcessSupportInterfaceFilamentChanged, this, &PrinterMangager::onSupportInterfaceFilamentChanged);
		connect(parameterManager, &ParameterManager::printSequenceChanged, this, &PrinterMangager::onGlobalPrintSequenceChanged);
		connect(parameterManager, &ParameterManager::currBedTypeChanged, this, &PrinterMangager::onGlobalPlateTypeChanged);
		connect(parameterManager, &ParameterManager::currBedTypeChanged, this, &PrinterMangager::dirtyAllPrinters);

		m_wipeTowerSetting.enable = parameterManager->enablePrimeTower();
		m_wipeTowerSetting.volume = parameterManager->currentProcessPrimeVolume();
		m_wipeTowerSetting.width = parameterManager->currentProcessPrimeTowerWidth();
		m_wipeTowerSetting.layerHeight = parameterManager->currentProcessLayerHeight();
		m_wipeTowerSetting.enableSupport = parameterManager->enableSupport();
		m_wipeTowerSetting.supportFilamentColorIndex = parameterManager->currentProcessSupportFilament();
		m_wipeTowerSetting.supportInterfaceFilamentColorIndex = parameterManager->currentProcessSupportInterfaceFilament();

		onGlobalPlateTypeChanged(parameterManager->currBedType());
		onGlobalPrintSequenceChanged(globalPrintSequence());
	}

	void PrinterMangager::setTracer(PrinterMangagerTracer* tracer)
	{
		m_tracer = tracer;
	}

	Printer* PrinterMangager::calculateModelGroupPrinter(ModelGroup* _model_group)
	{
		return nullptr;
	}

	QList<ModelGroup*> PrinterMangager::getPrinterModelGroups(Printer* printer)
	{
		QList<ModelGroup*> groups;
		return groups;
	}

	QList<QList<ModelGroup*>> PrinterMangager::getEachPrinterModelsGroups()
	{
		QList<QList<ModelGroup*>> all;

		for (Printer* printer : m_printers)
		{
			all << printer->modelGroupsInside();
		}

		return all;
	}

	QList<ModelGroup*> PrinterMangager::getAllPrinterModelsGroups()
	{
		QList<ModelGroup*> modelGroups;
		for (Printer* printer : m_printers)
		{
			modelGroups += printer->modelGroupsInside();
		}
		return modelGroups;
	}

	bool PrinterMangager::insertPrinter(int index, Printer* p)
	{
		if (!p) return false;
		if (m_printers.contains(p) || printerEnough())
		{
			return false;
		}

		int insertIndex = index;
		QList<ModelN*> modelsToMove;
		QList<QVector3D> starts, ends;
		int sizeBeforeInsert = m_printers.size();
		int sizeAfterInsert = sizeBeforeInsert + 1;

		if (!m_printers.isEmpty())
		{
			//要移动的盘
			QList<Printer*> printersNeedMove;  //所有要移动的盘
			QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
			QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(p->box(), sizeAfterInsert);
			for (size_t i = 0; i < sizeBeforeInsert; i++)
			{
				Printer* prt = m_printers.at(i);

				int indexOfAllBox = (i >= insertIndex ? i + 1 : i);

				if (prt->globalBox() != allBoxs.at(indexOfAllBox))
				{
					printersNeedMove << prt;
					moveBoxs << allBoxs.at(indexOfAllBox);
				}
			}

			//找到所有需要移动的模型，提前计算好它们需要移动到的位置
			for (size_t i = 0; i < printersNeedMove.size(); i++)
			{
				Printer* prt = printersNeedMove.at(i);
				QVector3D printerStartPostion = prt->position();

				QVector3D printerEndPosition = moveBoxs.at(i).min;

				QList<ModelN*> sub = prt->modelsInside();
				for (ModelN* m : sub)
				{
					QVector3D startPos = m->localPosition();
					QVector3D offset = startPos - printerStartPostion;

					modelsToMove << m;
					starts << startPos;
					ends << (printerEndPosition + offset);
				}
			}
		}

		//p->setSelected(m_printers.isEmpty());
		p->addTracer(this);
		p->setVisibility(true);
		p->setParent(this);
		p->onGlobalPlateTypeChanged(getKernel()->parameterManager()->currBedType());
		m_printers.insert(index, p);

		layout();

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* prt = m_printers.at(i);
			prt->setIndex(i);
			prt->setSelected(prt == p);
		}

		//after layout, moving models
		moveModels(modelsToMove, starts, ends, false);

		notifyAddedPrinter(p);

		groupModelsByPrinterGlobalBox();

		onPrintersCountChanged();

		viewAllPrintersFromTop();

		requestVisUpdate(true);
		return true;
	}

	QList<int> PrinterMangager::slicedPrinters()
	{
		QList<int> slicedPrinters;

		int count = printersCount();
		for (int index = 0; index < count; ++index)
		{
			Printer* pt = getPrinter(index);
			if (pt->isSliced())
			{
				slicedPrinters << index;
			}
		}

		return slicedPrinters;

		
	}

	bool PrinterMangager::addPrinter(Printer* p)
	{
		int index = printersCount();
		return insertPrinter(index, p);
	}

	bool PrinterMangager::insertPrinterEx(int index, Printer* p, QList<int64_t>& moveGroupIds, QList<trimesh::dvec3>& offsets)
	{
		if (!p) return false;
		if (m_printers.contains(p) || printerEnough())
		{
			return false;
		}

		int insertIndex = index;
		int sizeBeforeInsert = m_printers.size();
		int sizeAfterInsert = sizeBeforeInsert + 1;

		if (!m_printers.isEmpty())
		{
			//要移动的盘
			QList<Printer*> printersNeedMove;  //所有要移动的盘
			QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
			QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(p->box(), sizeAfterInsert);
			for (size_t i = 0; i < sizeBeforeInsert; i++)
			{
				Printer* prt = m_printers.at(i);

				int indexOfAllBox = (i >= insertIndex ? i + 1 : i);

				if (prt->globalBox() != allBoxs.at(indexOfAllBox))
				{
					printersNeedMove << prt;
					moveBoxs << allBoxs.at(indexOfAllBox);
				}
			}

			//找到所有需要移动的模型，提前计算好它们需要移动到的位置
			for (size_t i = 0; i < printersNeedMove.size(); i++)
			{
				Printer* prt = printersNeedMove.at(i);
				QVector3D printerStartPostion = prt->position();
				QVector3D printerEndPosition = moveBoxs.at(i).min;
				QVector3D offfset_printer = printerEndPosition - printerStartPostion;

				QList<ModelGroup*> sub = prt->modelGroupsInside();
				for (ModelGroup* mg : sub)
				{
					moveGroupIds << mg->getObjectId();
					offsets << trimesh::dvec3(offfset_printer.x(), offfset_printer.y(), offfset_printer.z());
				}
			}
		}

		//p->setSelected(m_printers.isEmpty());
		p->addTracer(this);
		p->setVisibility(true);
		p->setParent(this);
		p->onGlobalPlateTypeChanged(getKernel()->parameterManager()->currBedType());
		m_printers.insert(index, p);

		layout();

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* prt = m_printers.at(i);
			prt->setIndex(i);
			prt->setSelected(prt == p);
		}

		notifyAddedPrinter(p);

		groupModelsByPrinterGlobalBox();

		onPrintersCountChanged();

		viewAllPrintersFromTop();

		requestVisUpdate(true);
		return true;
	}

	void PrinterMangager::deletePrinterEx(Printer* p, QList<int64_t>& moveGroupIds, QList<trimesh::dvec3>& offsets, bool isCompletely)
	{
		if (!p)
			return;

		int deleteIndex = m_printers.indexOf(p);
		if (deleteIndex == -1)
		{
			return;
		}

		int sizeBeforeDelete = m_printers.size();
		int sizeAfterDelete = sizeBeforeDelete - 1;

		//before delete

		//要删除的盘
		p->addTracer(nullptr);
		bool resetSelect = p->selected();
		QList<ModelGroup*> groupsOfDeletePrinter = p->modelGroupsInside();
		QVector3D printerPosition = p->position();
		QVector3D takeInPosition = QVector3D(0.0, m_printers.first()->box().size().y()+50.0, 0.0);
		for (ModelGroup* mg : groupsOfDeletePrinter)
		{
			moveGroupIds << mg->getObjectId();

			QVector3D offfset_printer = takeInPosition - printerPosition;
			offsets << trimesh::dvec3(offfset_printer.x(), offfset_printer.y(), offfset_printer.z());
		}

		//要移动的盘
		QList<Printer*> printersNeedMove;  //所有要移动的盘
		QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
		QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(p->box(), sizeAfterDelete);
		for (size_t i = 0; i < sizeAfterDelete; i++)
		{
			Printer* prt;
			if (i >= deleteIndex)
			{
				prt = m_printers.at(i + 1);
			}
			else {
				prt = m_printers.at(i);
			}

			if (prt->globalBox() != allBoxs.at(i))
			{
				printersNeedMove << prt;
				moveBoxs << allBoxs.at(i);
			}
		}

		//找到所有需要移动的模型，提前计算好它们需要移动到的位置
		for (size_t i = 0; i < printersNeedMove.size(); i++)
		{
			Printer* prt = printersNeedMove.at(i);
			QVector3D printerStartPostion = prt->position();

			//QVector3D printerEndPosition = moveBoxs.at(i).min;

			QVector3D offfset_printer = moveBoxs.at(i).min - printerStartPostion;

			QList<ModelGroup*> sub = prt->modelGroupsInside();
			for (ModelGroup* mg : sub)
			{
				moveGroupIds << mg->getObjectId();
				offsets << trimesh::dvec3(offfset_printer.x(), offfset_printer.y(), offfset_printer.z());
			}
		}

		QList<ModelGroup*> groups;
;		p->replaceModelGroupsInside(groups);
		//layout
		m_printers.removeOne(p);

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* prt = m_printers.at(i);
			prt->setIndex(i);
		}

		if (resetSelect)
		{
			m_printers.first()->setSelected(true);
		}

		layout();

		//after layout, moving models
		//moveModels(modelsToMove, starts, ends, true);  // do this in undo command

		//visCamera()->fittingBoundingBox(m_boundingBox);
		requestVisUpdate(true);

		//groupModelsByPrinterGlobalBox();
		notifyDeletePrinter(p);
		onPrintersCountChanged();

		if (isCompletely)
			p->deleteLater();
		else
			p->setVisibility(false);
	}

	void PrinterMangager::checkGroupsToMove(int estimateBoxSize, QList<int64_t>& groupIds, QList<trimesh::dvec3>& offsets)
	{
		int curSize = m_printers.size();

		if (estimateBoxSize == curSize)
			return;

		if (!m_printers.isEmpty())
		{
			//要移动的盘
			QList<Printer*> printersNeedMove;  //所有要移动的盘
			QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
			QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->box(), estimateBoxSize);

			if (estimateBoxSize > curSize)
			{
				for (size_t i = 0; i < curSize; i++)
				{
					Printer* prt = m_printers.at(i);

					if (prt->globalBox() != allBoxs.at(i))
					{
						printersNeedMove << prt;
						moveBoxs << allBoxs.at(i);
					}
				}
			}
			else
			{
				for (size_t i = 0; i < estimateBoxSize; i++)
				{
					Printer* prt = m_printers.at(i);

					if (prt->globalBox() != allBoxs.at(i))
					{
						printersNeedMove << prt;
						moveBoxs << allBoxs.at(i);
					}
				}
			}


			//找到所有需要移动的模型，提前计算好它们需要移动到的位置
			for (size_t i = 0; i < printersNeedMove.size(); i++)
			{
				Printer* prt = printersNeedMove.at(i);
				QVector3D printerStartPostion = prt->position();
				QVector3D offfset_printer = moveBoxs.at(i).min - printerStartPostion;

				QList<ModelGroup*> sub = prt->modelGroupsInside();
				for (ModelGroup* mg : sub)
				{
					groupIds << mg->getObjectId();
					offsets << trimesh::dvec3(offfset_printer.x(), offfset_printer.y(), offfset_printer.z());
				}
			}
		}
	}

	int PrinterMangager::getPrinterIndexByGroupId(int groupId)
	{
		ModelGroup* mg = getModelGroupByObjectId(groupId);
		if (!mg)
			return -1;

		int printerSize = printersCount();
		Printer* printerPtr = nullptr;
		for (int i = 0; i < printerSize; i++)
		{
			printerPtr = getPrinter(i);
			Q_ASSERT(printerPtr);
			qtuser_3d::Box3D printerBox = printerPtr->globalBox();
			printerBox.min.setZ(0.0f);
			printerBox.max.setZ(0.0f);

			qtuser_3d::Box3D modelBox = mg->globalSpaceBox();
			modelBox.min.setZ(0.0f);
			modelBox.max.setZ(0.0f);

			if (printerBox.contains(modelBox))
				return printerPtr->index();
		}

		return -1;
	}

	void PrinterMangager::updatePrinterContent()
	{
		groupModelsByPrinterGlobalBox();
		updateWipeTowers();

		checkSelectedPrinterModelsOnBorder();
		checkModelsInSelectedPrinterCollision();
		checkGroupHigherThanBottom();
	}

	void PrinterMangager::layout()
	{
		if (m_printers.isEmpty())
			return;

		int size = m_printers.size();
		int w = ceilf(sqrt(size));
		int h = ceilf(size / (float)w);
		// w * h
		float margin = 50.0f;
		float cellX = FLT_MIN;
		float cellY = FLT_MIN;
		float cellZ = FLT_MIN;

		for (auto p : m_printers)
		{
			const qtuser_3d::Box3D box = p->box();

			cellX = std::max(box.size().x(), cellX);
			cellY = std::max(box.size().y(), cellY);
			cellZ = std::max(box.size().z(), cellZ);
		}

		QVector3D holdSize = QVector3D(cellX * w + margin * (w-1),
			cellY * h + margin * (h-1),
			cellZ);

		QVector3D firstSize = m_printers.first()->box().size();

		QVector3D gOffset = QVector3D((cellX - firstSize.x()) / 2.0, (cellY - firstSize.y()) / 2.0, 0.0f);

		QVector3D minPos = QVector3D(0.0f, (h - 1) * (cellY + margin) * -1.0f, 0.0f) - gOffset;
		qtuser_3d::Box3D holdBox = qtuser_3d::Box3D(minPos, minPos + holdSize);

		for (size_t i = 0; i < h; i++)
		{
			for (size_t j = 0; j < w; j++) {
				int idx = (i * w + j);
				if (idx < m_printers.size())
				{
					Printer* sub = m_printers.at(idx);
					QVector3D subSize = sub->box().size();

					QVector3D subOffset = QVector3D((cellX - subSize.x()) / 2.0, (cellY - subSize.y()) / 2.0, 0.0f);

					QVector3D cellOrigin = QVector3D(j * (cellX + margin), i * (cellY + margin) * -1.0f, 0.0f);
					QVector3D pOrigin = cellOrigin + subOffset - gOffset;

					sub->setPosition(pOrigin);
				}
			}
		}

		m_boundingBox = holdBox;
	}

	bool PrinterMangager::appendPrinter()
	{
		if (m_printers.isEmpty())
		{
			return false;
		}

		if (printerEnough())
		{
			return false;
		}

		Printer* last = m_printers.last();

		Printer* newOne = new Printer(*last);

		bool added = addPrinter(newOne);

		if (added)
		{
			return true;
		}
		else {
			delete newOne;
		}

		return false;
	}

	bool PrinterMangager::resizePrinters(int num)
	{
		int size = m_printers.size();
		if (size == num)
			return true;

		if (size == 0 || printerEnough() || num == 0)
			return false;

		if (size < num)
		{
			// add
			const Printer* templatePrinter = m_printers.constLast();

			for (size_t i = size; i < num; i++)
			{
				Printer* p = new Printer(*templatePrinter);
				p->setSelected(m_printers.isEmpty());
				p->addTracer(this);
				p->setVisibility(true);
				p->setParent(this);
				m_printers.push_back(p);
			}

			for (size_t i = 0; i < m_printers.size(); i++)
			{
				Printer* prt = m_printers.at(i);
				prt->setIndex(i);
			}

			layout();

			visCamera()->fittingBoundingBox(m_boundingBox);

			for (size_t i = size; i < num; i++)
			{
				Printer* p = m_printers.at(i);
				notifyAddedPrinter(p);
			}
		}
		else {
		//remove

			QList<Printer*> willDelete;
			bool resetSelect = false;
			for (size_t i = num; i < size; i++)
			{
				Printer* p = m_printers.at(i);
				p->addTracer(nullptr);
				if (p->selected())
				{
					resetSelect = true;
				}

				willDelete.push_back(p);
			}

			if (resetSelect)
			{
				m_printers.first()->setSelected(true);
			}

			QList<Printer*>::iterator iter = m_printers.begin() + num;
			m_printers.erase(iter, m_printers.end());

			layout();

			visCamera()->fittingBoundingBox(m_boundingBox);

			for (auto p : willDelete)
			{
				notifyDeletePrinter(p);
				p->deleteLater();

				/*if (isCompletely)
					p->deleteLater();
				else
					p->setVisibility(false);*/
			}
		}

		groupModelsByPrinterGlobalBox();

		onPrintersCountChanged();

		requestVisUpdate(true);

		return true;
	}

	void PrinterMangager::deletePrinter(int idx, bool isCompletely)
	{
		if (0 <= idx && idx < m_printers.size())
		{
			Printer* p = m_printers.at(idx);
			deletePrinter(p, isCompletely);
		}
	}

	void PrinterMangager::deletePrinter(Printer* p, bool isCompletely)
	{
		if (!p)
			return;

		int deleteIndex = m_printers.indexOf(p);
		if (deleteIndex == -1)
		{
			return;
		}

		QList<ModelGroup*> groupsToMove;
		QList<QVector3D> ends;
		int sizeBeforeDelete = m_printers.size();
		int sizeAfterDelete = sizeBeforeDelete - 1;

		//before delete

		//要删除的盘
		p->addTracer(nullptr);
		bool resetSelect = p->selected();
		QList<ModelGroup*> groupsOfDeletePrinter = p->modelGroupsInside();
		QVector3D printerPosition = p->position();
		QVector3D takeInPosition = QVector3D(0.0, m_printers.first()->box().size().y() + 50.0, 0.0);
		for (ModelGroup* mg : groupsOfDeletePrinter)
		{
			//移动之前的位置
			QVector3D startPos = mg->localPosition();
			QVector3D offset = startPos - printerPosition;

			groupsToMove << mg;
			ends << (takeInPosition + offset);
		}

		//要移动的盘
		QList<Printer*> printersNeedMove;  //所有要移动的盘
		QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
		QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(p->box(), sizeAfterDelete);
		for (size_t i = 0; i < sizeAfterDelete; i++)
		{
			Printer* prt;
			if (i >= deleteIndex)
			{
				prt = m_printers.at(i + 1);
			}
			else {
				prt = m_printers.at(i);
			}

			if (prt->globalBox() != allBoxs.at(i))
			{
				printersNeedMove << prt;
				moveBoxs << allBoxs.at(i);
			}
		}

		//找到所有需要移动的模型，提前计算好它们需要移动到的位置
		for (size_t i = 0; i < printersNeedMove.size(); i++)
		{
			Printer* prt = printersNeedMove.at(i);
			QVector3D printerStartPostion = prt->position();

			QVector3D printerEndPosition = moveBoxs.at(i).min;

			QList<ModelGroup*> sub = prt->modelGroupsInside();
			for (ModelGroup* mg : sub)
			{
				QVector3D startPos = mg->localPosition();
				QVector3D offset = startPos - printerStartPostion;

				groupsToMove << mg;

				ends << (printerEndPosition + offset);
			}
		}

		//layout
		m_printers.removeOne(p);

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* prt = m_printers.at(i);
			if (resetSelect)
			{
				prt->setSelected(i == 0);
			}
			prt->setIndex(i);
		}

		layout();

		//after layout, moving models
		for (int i = 0; i < groupsToMove.size(); i++)
		{
			groupsToMove[i]->setLocalPosition(ends[i]);
		}

		updateSpaceNodeRender(groupsToMove);

		//visCamera()->fittingBoundingBox(m_boundingBox);
		requestVisUpdate(true);

		//groupModelsByPrinterGlobalBox();

		notifyDeletePrinter(p);

		onPrintersCountChanged();

		if (isCompletely)
		{
			p->setParent(nullptr);
			p->deleteLater();
		}
		else
			p->setVisibility(false);
	}

	bool PrinterMangager::hasPrinterSliced()
	{
		int count = printersCount();
		bool isSliced = false;
		for (int index = 0; index < count; ++index)
		{
			Printer* pt = getPrinter(index);
			bool sliced = pt->isSliced();
			if (sliced)
			{
				isSliced = true;
				break;
			}
		}
		return isSliced;
	}

	int PrinterMangager::printersCount()
	{
		return m_printers.size();
	}

	Printer* PrinterMangager::getPrinter(int idx)
	{
		if (0 <= idx && idx < m_printers.size())
		{
			Printer* p = m_printers.at(idx);
			return p;
		}
		return nullptr;
	}

	void PrinterMangager::selectPrinter(Printer* p)
	{
		willSelectPrinter(p);
	}

	bool PrinterMangager::selectPrinter(int index, bool exclude)
	{
		if (0 <= index && index < m_printers.size())
		{
			Printer* newSelectPrinter = nullptr;
			requestRender(this);

			for (size_t i = 0; i < m_printers.size(); i++)
			{
				Printer* p = m_printers.at(i);
				if (i == index)
				{
					//p->setSelected(true);
					newSelectPrinter = p;
				}
				else if (exclude)
				{
					p->setSelected(false);
				}
			}

			if (newSelectPrinter)
			{
				newSelectPrinter->setSelected(true);
			}
			//groupModelsByPrinterGlobalBox();

			endRequestRender(this);

			return true;
		}

		return false;
	}

	Printer* PrinterMangager::getSelectedPrinter()
	{
		for (Printer* p : m_printers)
		{
			if (p->selected())
			{
				return p;
			}
		}
		return nullptr;
	}

	QObject* PrinterMangager::getSelectedPrinterObject()
	{
		return (QObject*)getSelectedPrinter();
	}

	int PrinterMangager::selectedPrinterIndex()
	{
		Printer* pt = getSelectedPrinter();
		int index = indexOfPrinter(pt);
		return index;
	}

	void PrinterMangager::setSelectedPrinterIndex(int index)
	{
		Printer* pt = getPrinter(index);
		emit signalDidSelectPrinter(pt);
	}

	int PrinterMangager::indexOfPrinter(Printer* printer)
	{
		if (!m_printers.contains(printer)) {
			return -1;
		}

		return m_printers.indexOf(printer);
	}

	void PrinterMangager::allPrinterUpdateBox(const qtuser_3d::Box3D& box)
	{
		for (Printer* p : m_printers)
		{
			//p->addTracer(nullptr);
			p->updateBox(box);
			//p->addTracer(this);
		}

		layout();

		checkModelsInSelectedPrinterCollision();

		requestVisUpdate(true);
	}

	QList<int> PrinterMangager::usedExtruders()
	{
		QSet<int> ret;
		for (Printer* p : m_printers)
		{
			ret.unite(p->getUsedExtruders());
		}
		return ret.toList();
	}

	bool PrinterMangager::hasSliced()
	{
		for (Printer* p : m_printers)
		{
			if (p->isSliced())
			{
				return true;
			}
		}
		return false;
	}

	QList<ModelN*> PrinterMangager::getPrinterModels(int index)
	{
		QList<ModelN*> models;
		if (index < 0 || index >= m_printers.size())
			return models;

		Printer* printer = m_printers[index];

		return printer->modelsInside();
	}

	bool PrinterMangager::hasModelsOnBorder(int index)
	{
		if (0 <= index && index < m_printers.size())
		{
			return (m_printers.at(index)->hasModelsOnBorder());
		}
		return false;
	}

	QList<ModelN*> PrinterMangager::getAllModelsOutOfPrinter()
	{
		return m_modelsOutOfPrinter;
	}

	QList<ModelGroup*> PrinterMangager::getAllModelGroupsOutOfPrinter()
	{
		return m_modelGroupsOutOfPrinter;
	}

	QList<ModelGroup*> PrinterMangager::getPrinterModelGroups(int index)
	{
		QList<ModelGroup*> modelGroups;
		if (index < 0 || index >= m_printers.size())
			return modelGroups;

		Printer* printer = m_printers[index];

		return printer->modelGroupsInside();
	}

	QList<int> PrinterMangager::getSlicedPrinterIndice()
	{
		QList<int> indice;
		for (const auto& printer : m_printers)
		{
			if (printer->isSliced())
			{
				indice.append(printer->index());
			}
		}
		return indice;
	}

	bool PrinterMangager::shouldDeletePrinter(Printer* p)
	{
		return (m_printers.size() > 1);
	}

	void PrinterMangager::willDeletePrinter(Printer* p)
	{
		if (!p) return;

		//deletePrinter(p);
	}

	void PrinterMangager::willSelectPrinter(Printer* p)
	{
		int index = m_printers.indexOf(p);
		if (index != -1)
		{
			selectPrinter(index, true);
		}
	}

	void PrinterMangager::didSelectPrinter(Printer* p)
	{
		groupModelsByPrinterGlobalBox();

		//check models collision
		localPrintSequenceChanged(p, p->getLocalPrintSequence());

		checkSelectedPrinterModelsOnBorder();

		checkGroupHigherThanBottom();

		p->checkModelInsideVisibleLayers(true);
		p->checkSliceError();

		emit signalDidSelectPrinter(p);
		if (m_tracer)
		{
			m_tracer->didSelectPrinter(p);
		}
	}

	void PrinterMangager::didLockPrinterOrNot(Printer* p, bool lockOrNot)
	{

	}

	/*void PrinterMangager::didBoundingBoxChange(Printer* p, const qtuser_3d::Box3D& newBox)
	{
		layout();
	}*/

	void PrinterMangager::willSettingPrinter(Printer* p)
	{
		if (m_tracer)
			m_tracer->willSettingPrinter(p);
	}

	void  PrinterMangager::willAutolayoutForPrinter(Printer* p)
	{
		if (p->lock())
		{
			return;
		}
		if (m_tracer)
			m_tracer->willAutolayoutForPrinter(p);
	}

	void  PrinterMangager::willPickBottomForPrinter(Printer* p)
	{
		QList<ModelGroup*> modelGroups = selectedGroups();
		if (modelGroups.empty())
		{
			creative_kernel::setModelGroupsMaxFaceBottom(p->modelGroupsInside());
		}
		else
		{
			creative_kernel::setModelGroupsMaxFaceBottom(modelGroups);
		}
		/*if (p->lock())
		{
			if (m_tracer) {
				m_tracer->message(p, 0, QString("pick bottom while locking"));
			}
			return;
		}
		if (m_tracer)
			m_tracer->willPickBottomForPrinter(p);*/

	}

	void  PrinterMangager::willEditNameForPrinter(Printer* p, QString from)
	{
		if (m_tracer)
			m_tracer->willEditNameForPrinter(p, from);
	}

	const WipeTowerGlobalSetting& PrinterMangager::wipeTowerCurrentSetting()
	{
		return m_wipeTowerSetting;
	}

	void PrinterMangager::localPrintSequenceChanged(Printer* p, QString newSequence)
	{
		updateWipeTowers();

		if (!p->selected())
			return;

		if (newSequence.isEmpty())
		{
			handlePrintSequence(globalPrintSequence());
		}
		else {
			handlePrintSequence(newSequence);
		}
	}

	QString PrinterMangager::globalPlateType()
	{
		return getKernel()->parameterManager()->currBedType();
	}

	QString PrinterMangager::globalPrintSequence()
	{
		return getKernel()->parameterManager()->printSequence();
	}

	void PrinterMangager::nameChanged(Printer* p, QString newName)
	{
		if (m_tracer)
		{
			m_tracer->printerNameChanged(p, newName);
		}
	}

	void PrinterMangager::printerIndexChanged(Printer* p, int newIndex)
	{
		if (m_tracer)
		{
			m_tracer->printerIndexChanged(p, newIndex);
		}
	}

	void PrinterMangager::modelGroupsInsideChange(Printer* p, const QList<ModelGroup*>& list)
	{
		if (m_tracer)
		{
			m_tracer->printerModelGroupsInsideChange(p, list);
		}
	}

	qtuser_3d::Box3D PrinterMangager::extendBox()
	{
		if (!m_boundingBox.valid)
		{
			return qtuser_3d::Box3D();
		}

		QVector3D size = m_boundingBox.size();
		size /= 2.0;
		QVector3D center = m_boundingBox.center();
		qtuser_3d::Box3D extendBox;
		extendBox += (center - size * 1.25);
		extendBox += (center + size * 1.25);
		return extendBox;
	}

	void PrinterMangager::refittingExtendBox()
	{
		layout();

		qtuser_3d::ScreenCamera* screenCamera = visCamera();
		Qt3DRender::QCamera* camera = screenCamera->camera();
		camera->setFieldOfView(30.0f);

		cameraController()->home(m_boundingBox, 1);

		requestVisUpdate(true);
	}

	void PrinterMangager::relayout()
	{
		layout();
	}

	const qtuser_3d::Box3D& PrinterMangager::boundingBox()
	{
		return m_boundingBox;
	}

	bool PrinterMangager::printerEnough()
	{
		//return printersCount() >= 16;
		return false;
	}


	void PrinterMangager::setTheme(int theme)
	{
		for (Printer *p : m_printers)
		{
			p->setTheme(theme);
		}
	}

	void PrinterMangager::divideModelGroupsForPrinter(Printer* printer, QList<ModelGroup*>& groups, QList<int>& groupedFlags)
	{
		int flag = 1;
		float error = 0.0f;

		qtuser_3d::Box3D printerBox = printer->globalBox();

		QList<ModelGroup*> groupsInside;
		QList<ModelGroup*> groupsOnBorder;

		for (size_t j = 0; j < groups.size(); j++)
		{
			int grouped = groupedFlags[j];

			if (grouped == flag)
				continue;
			ModelGroup* m = groups.at(j);
			qtuser_3d::Box3D modelBox = m->globalSpaceBox();

			if (printerBox.intersects(modelBox))
			{
				/*if (printerBox.contains(modelBox))
				{
					modelsInside.push_back(m);
				}*/
				if (modelBox.min.x() >= printerBox.min.x() - error &&
					modelBox.min.y() >= printerBox.min.y() - error &&
					//modelBox.min.z() >= printerBox.min.z() - error &&
					modelBox.max.x() <= printerBox.max.x() + error &&
					modelBox.max.y() <= printerBox.max.y() + error &&
					modelBox.max.z() <= printerBox.max.z() + error)
				{
					groupsInside.push_back(m);
				}
				else {
					groupsOnBorder.push_back(m);
				}
				groupedFlags[j] = flag;
			}
		}

		printer->replaceModelGroupsInside(groupsInside);
		printer->replaceModelGroupsOnBorder(groupsOnBorder);
		printer->updateAssociateModelsState();
	}

	void PrinterMangager::divideModelGroupsByPrinterGlobalBox()
	{
		int flag = 1;
		QList<ModelGroup*> groups = modelGroups();
		QList<ModelGroup*> exceptiveGroups;
		QList<int> groupedFlags;
		for (size_t i = 0; i < groups.size(); i++)
		{
			groupedFlags.push_back(0);
		}

		Printer* thePrinter = getSelectedPrinter();
		if (thePrinter)
		{
			divideModelGroupsForPrinter(thePrinter, groups, groupedFlags);
			exceptiveGroups.append(thePrinter->modelGroupsOnBorder());
		}

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* pt = m_printers.at(i);
			if (pt != thePrinter)
			{
				divideModelGroupsForPrinter(pt, groups, groupedFlags);
				exceptiveGroups.append(pt->modelGroupsOnBorder());
			}
		}

		//游离的模型组
		{
			qtuser_3d::Box3D box(QVector3D(-100000.0f, -100000.0f, -100000.0f), QVector3D(100000.0f, 100000.0f, 100000.0f));

			for (size_t j = 0; j < groups.size(); j++)
			{
				int grouped = groupedFlags[j];

				if (grouped != flag) {
					ModelGroup* group = groups.at(j);

					for (ModelN* model : group->models())
					{
						ModelNEntity* modelE = model->entity();
						if (modelE)
						{
							modelE->setAssociatePrinterBox(box);
						}
					}

					exceptiveGroups.push_back(group);
				}
			}
		}

		bool equal = Printer::checkTwoGroupListEqual(exceptiveGroups, m_modelGroupsOutOfPrinter);
		if (!equal)
		{
			m_modelGroupsOutOfPrinter = exceptiveGroups;

			m_modelsOutOfPrinter.clear();
			for (ModelGroup* group : m_modelGroupsOutOfPrinter)
			{
				m_modelsOutOfPrinter += group->models();
			}

			if (m_tracer)
			{
				m_tracer->modelGroupsOutOfPrinterChange(exceptiveGroups);
			}
		}
	}

	void PrinterMangager::groupModelsByPrinterGlobalBox()
	{
		divideModelGroupsByPrinterGlobalBox();
		return;
		int flag = 1;
		QList<ModelN*> models = modelns();
		QList<ModelN*> exceptiveModels;
		QList<int> groupedFlags;
		for (size_t i = 0; i < models.size(); i++)
		{
			groupedFlags.push_back(0);
		}

		Printer* thePrinter = getSelectedPrinter();
		if (thePrinter)
		{
			groupModelsForPrinter(thePrinter, models, groupedFlags);
			exceptiveModels.append(thePrinter->modelsOnBorder());
		}

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* pt = m_printers.at(i);
			if (pt != thePrinter)
			{
				groupModelsForPrinter(pt, models, groupedFlags);
				exceptiveModels.append(pt->modelsOnBorder());
			}
		}

		//游离的模型
		{
			qtuser_3d::Box3D box(QVector3D(-100000.0f, -100000.0f, -100000.0f), QVector3D(100000.0f, 100000.0f, 100000.0f));

			for (size_t j = 0; j < models.size(); j++)
			{
				int grouped = groupedFlags[j];

				if (grouped != flag) {
					ModelN* model = models.at(j);
					ModelNEntity* modelE = model->entity();
					if (modelE)
					{
						modelE->setAssociatePrinterBox(box);
					}

					exceptiveModels.push_back(model);
				}
			}
		}

		bool equal = Printer::checkTwoListEqual(exceptiveModels, m_modelsOutOfPrinter);
		if (!equal)
		{
			m_modelsOutOfPrinter = exceptiveModels;
			emit signalModelsOutOfPrinterChange(exceptiveModels);
		}
	}

	void PrinterMangager::groupModelsForPrinter(Printer* printer, QList<ModelN*>& models, QList<int>& groupedFlags)
	{
		int flag = 1;
		float error = 0.5;

		qtuser_3d::Box3D printerBox = printer->globalBox();

		QList<ModelN*> modelsInside;
		QList<ModelN*> modelsOnBorder;

		for (size_t j = 0; j < models.size(); j++)
		{
			int grouped = groupedFlags[j];

			if (grouped == flag)
				continue;
			ModelN* m = models.at(j);
			qtuser_3d::Box3D modelBox = m->globalSpaceBox();

			if (printerBox.intersects(modelBox))
			{
				/*if (printerBox.contains(modelBox))
				{
					modelsInside.push_back(m);
				}*/
				if (modelBox.min.x() > printerBox.min.x() - error &&
					modelBox.min.y() > printerBox.min.y() - error &&
					//modelBox.min.z() > printerBox.min.z() - error &&
					modelBox.max.x() < printerBox.max.x() + error &&
					modelBox.max.y() < printerBox.max.y() + error &&
					modelBox.max.z() < printerBox.max.z() + error)
				{
					modelsInside.push_back(m);
				}
				else {
					modelsOnBorder.push_back(m);
				}
				groupedFlags[j] = flag;
			}
		}

		printer->replaceModelsInside(modelsInside);
		printer->replaceModelsOnBorder(modelsOnBorder);
	}

	void PrinterMangager::setPrinterVisibility(int printerIndex, bool visibility, bool exclude)
	{
		if (0 <= printerIndex && printerIndex < m_printers.size())
		{
			for (size_t i = 0; i < m_printers.size(); i++)
			{
				Printer* pt = m_printers.at(i);
				if (i == printerIndex) {
					pt->setVisibility(visibility);
				}
				else if (exclude)
				{
					pt->setVisibility(!visibility);
				}
			}
		}
	}

	void PrinterMangager::showPrinter(int printerIndex)
	{
		if (printerIndex == -1)
		{
			for (auto p : m_printers)
				p->setVisibility(true);
		}
		else
		{
			for (auto p : m_printers)
			{
				if (p->index() == printerIndex)
					p->setVisibility(true);
			}
		}
	}

	void PrinterMangager::hidePrinter(int printerIndex)
	{
		if (printerIndex == -1)
		{
			for (auto p : m_printers)
				p->setVisibility(false);
		}
		else
		{
			for (auto p : m_printers)
			{
				if (p->index() == printerIndex)
					p->setVisibility(false);
			}
		}
	}

	PrinterMangager* getPrinterMangager()
	{
		return getKernel()->reuseableCache()->getPrinterMangager();
	}

	QList<WipeTowerEntity*>PrinterMangager::wipeTowerEntities()
	{
		QList<WipeTowerEntity*> list;

		for (auto p : m_printers)
		{
			WipeTowerEntity* tower = p->entity()->wipeTowerEntity();
			if (tower)
			{
				list.push_back(tower);
			}
		}
		return list;
	}

	Printer* PrinterMangager::findPrinterFromWipeTower(WipeTowerEntity* t)
	{
		Printer* ptr = nullptr;
		if (t == nullptr)
		{
			return ptr;
		}

		for (auto p : m_printers)
		{
			WipeTowerEntity* tower = p->entity()->wipeTowerEntity();
			if (tower == t)
			{
				ptr = p;
				break;
			}
		}
		return ptr;
	}

	void PrinterMangager::onSceneChanged(const trimesh::dbox3& box)
	{
		updatePrinterContent();
	}

	void PrinterMangager::updateWipeTowers()
	{
		for (auto p : m_printers)
		{
			bool show = p->checkAndUpdateWipeTowerState();
			if (p->selected())
				p->checkWipeTowerCollide();
		}
	}

	void PrinterMangager::updateWipeTowersPositions(const std::vector<float> x, const std::vector<float> y)
	{
		if (x.empty() || y.empty())
		{
			return;
		}
		for (size_t i = 0; i < m_printers.size(); i++)
		{
			creative_kernel::Printer* sub = m_printers.at(i);
			sub->updateWipeTowerState();
			if(i < x.size() && i < y.size())
				sub->entity()->wipeTowerEntity()->setLeftBottomPosition(x[i], y[i]);
		}
	}


	void PrinterMangager::setPrinterPreviewMode(int printerIndex)
	{
		if (printerIndex == -1)
		{
			for (auto p : m_printers)
				p->setPrinterPreviewMode();
		}
		else
		{
			for (auto p : m_printers)
			{
				if (p->index() == printerIndex)
					p->setPrinterPreviewMode();
			}
		}
	}

	void PrinterMangager::setPrinterEditMode(int printerIndex)
	{
		if (printerIndex == -1)
		{
			for (auto p : m_printers)
				p->setPrinterEditMode();

		}
		else
		{
			for (auto p : m_printers)
			{
				if (p->index() == printerIndex)
					p->setPrinterEditMode();
			}
		}
	}

	QString PrinterMangager::getAllWipeTowerPosition(int axis, QString separate)
	{
		QString str;
		if (m_printers.isEmpty())
			return str;

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			auto sub = m_printers.at(i);
			sub->adjustWipeTowerPositionWhenInvalid();
			QVector3D pos = sub->entity()->wipeTowerEntity()->positionOfLeftBottom();
			QString xString = QString(std::to_string(pos[axis]).c_str());
			if (i != m_printers.size() - 1)
			{
				str += xString + separate;
			}
			else {
				str += xString;
			}
		}

		return str;
	}

	void PrinterMangager::dirtyAllPrinters()
	{
		int count = printersCount();
		for (int i = 0; i < count; ++i)
			getPrinter(i)->dirty();
	}

	void PrinterMangager::onGlobalSettingsChanged(QObject* parameterBase, const QString& key)
	{
		int count = printersCount();
		for (int i = 0; i < count; ++i)
		{
			Printer* printer = getPrinter(i);
			for (ModelN* model : printer->modelsInside())
			{
				if (!model->hasParameter(key))
				{// 模型对象设置不存在Key时跟随全局参数置脏。
					printer->dirty();
					// printer->clearSliceCache();
					break;
				}
			}
		}
	}

	void PrinterMangager::onWipeTowerEnable(bool enable)
	{
		if (m_wipeTowerSetting.enable != enable)
		{
			m_wipeTowerSetting.enable = enable;
			updateWipeTowers();

			requestVisUpdate(true);
		}
	}

	void PrinterMangager::onLayerHeightChanged(float height)
	{
		if (m_wipeTowerSetting.layerHeight != height)
		{
			m_wipeTowerSetting.layerHeight = height;
			updateWipeTowers();
		}
	}

	void PrinterMangager::onWipeTowerVolumeChanged(float volume)
	{
		if (m_wipeTowerSetting.volume != volume)
		{
			m_wipeTowerSetting.volume = volume;
			updateWipeTowers();
		}
	}

	void PrinterMangager::onWipeTowerWidthChanged(float width)
	{
		if (m_wipeTowerSetting.width != width)
		{
			m_wipeTowerSetting.width = width;
			updateWipeTowers();
		}
	}

	void PrinterMangager::onSupportEnable(bool enable)
	{
		if (m_wipeTowerSetting.enableSupport != enable)
		{
			m_wipeTowerSetting.enableSupport = enable;
			updateWipeTowers();
		}
	}

	void PrinterMangager::onSupportFilamentChanged(int index)
	{
		if (m_wipeTowerSetting.supportFilamentColorIndex != index)
		{
			m_wipeTowerSetting.supportFilamentColorIndex = index;
			updateWipeTowers();
		}
	}

	void PrinterMangager::onSupportInterfaceFilamentChanged(int index)
	{
		if (m_wipeTowerSetting.supportInterfaceFilamentColorIndex != index)
		{
			m_wipeTowerSetting.supportInterfaceFilamentColorIndex = index;
			updateWipeTowers();
		}
	}

	//全局
	void PrinterMangager::onGlobalPrintSequenceChanged(const QString& str)
	{
		QString seq = getSelectedPrinter()->getLocalPrintSequence();
		if (seq.isEmpty())
		{
			seq = str;
		}

		handlePrintSequence(seq);

		updateWipeTowers();
	}

	void PrinterMangager::handlePrintSequence(const QString& str)
	{
		qDebug() << "PrintSequence" << str;

		if (str.toLower() == "by layer")
		{
			if (m_collisionCheckHandler == nullptr)
			{
				m_collisionCheckHandler = new ByLayerEventHandler(this);
				addLeftMouseEventHandler(m_collisionCheckHandler);
			}
			else
			{
				ByLayerEventHandler* bylayer = qobject_cast<ByLayerEventHandler*>(m_collisionCheckHandler);
				if (bylayer == nullptr)
				{
					removeLeftMouseEventHandler(m_collisionCheckHandler);
					delete m_collisionCheckHandler;
					m_collisionCheckHandler = new ByLayerEventHandler(this);
					addLeftMouseEventHandler(m_collisionCheckHandler);
				}
			}

			checkModelsInSelectedPrinterCollision();

		}
		else if (str.toLower() == "by object") {

			if (m_collisionCheckHandler == nullptr)
			{
				m_collisionCheckHandler = new ByObjectEventHandler(this);
				addLeftMouseEventHandler(m_collisionCheckHandler);
			}
			else
			{
				ByObjectEventHandler* byobject = qobject_cast<ByObjectEventHandler*>(m_collisionCheckHandler);
				if (byobject == nullptr)
				{
					removeLeftMouseEventHandler(m_collisionCheckHandler);
					delete m_collisionCheckHandler;
					m_collisionCheckHandler = new ByObjectEventHandler(this);
					addLeftMouseEventHandler(m_collisionCheckHandler);
				}
			}

			checkModelsInSelectedPrinterCollision();

		}
	}

	void PrinterMangager::onGlobalPlateTypeChanged(const QString& str)
	{
		for (auto p : m_printers)
		{
			p->onGlobalPlateTypeChanged(str);
		}
	}

	QList<qtuser_3d::Box3D> PrinterMangager::estimateGlobalBoxBeenLayout(const qtuser_3d::Box3D& box, int size)
	{
		if (size <= 0 || box.valid == false)
		{
			return QList<qtuser_3d::Box3D>();
		}

		int w = ceilf(sqrt(size));
		int h = ceilf(size / (float)w);
		// w * h
		float margin = 50.0f;
		float cellX = box.size().x();
		float cellY = box.size().y();
		float cellZ = box.size().z();

		QVector3D holdSize = QVector3D(cellX * w + margin * (w - 1),
			cellY * h + margin * (h - 1),
			cellZ);

		QVector3D gOffset = QVector3D(0.0f, 0.0f, 0.0f);

		QVector3D minPos = QVector3D(0.0f, (h - 1) * (cellY + margin) * -1.0f, 0.0f) - gOffset;
		qtuser_3d::Box3D holdBox = qtuser_3d::Box3D(minPos, minPos + holdSize);

		QList<qtuser_3d::Box3D> result;

		for (size_t i = 0; i < h; i++)
		{
			for (size_t j = 0; j < w; j++) {
				int idx = (i * w + j);
				if (idx < size)
				{
					QVector3D subSize = box.size();

					QVector3D subOffset = QVector3D((cellX - subSize.x()) / 2.0, (cellY - subSize.y()) / 2.0, 0.0f);

					QVector3D cellOrigin = QVector3D(j * (cellX + margin), i * (cellY + margin) * -1.0f, 0.0f);
					QVector3D pOrigin = cellOrigin + subOffset - gOffset;

					qtuser_3d::Box3D box = qtuser_3d::Box3D(pOrigin, pOrigin + subSize);
					result.push_back(box);
				}
			}
		}

		return result;
	}

	void PrinterMangager::viewAllPrintersFromTop()
	{
		qtuser_3d::ScreenCamera* screenCamera = visCamera();
		qtuser_3d::CameraController* controller = cameraController();
		if (screenCamera && controller)
		{
			Qt3DRender::QCamera* camera = screenCamera->camera();
			camera->setFieldOfView(30.0f);
			screenCamera->fittingBoundingBox(extendBox());
			controller->viewFromTop();

		}
	}

	void PrinterMangager::onPrintersCountChanged()
	{
		if (printersCount() == 1)
		{
			Printer* p = getPrinter(0);
			if (p)
			{
				p->setCloseEnable(false);
			}
		}
		else {
			for (Printer* p : m_printers)
			{
				p->setCloseEnable(true);
			}
		}

		emit signalPrintersCountChanged();
		if (m_tracer)
		{
			m_tracer->printersCountChanged(printersCount());
		}

		checkModelsInSelectedPrinterCollision();
	}

	void PrinterMangager::checkModelsInSelectedPrinterCollision()
	{
		if (m_collisionCheckHandler && !m_collisionCheckHandler->processing())
		{
			m_collisionCheckHandler->checkModelGroups(getSelectedPrinter()->modelGroupsInside());
		}
	}

	void PrinterMangager::checkSelectedPrinterModelsOnBorder()
	{
		Printer* p = getSelectedPrinter();
		if (!p)
			return;
		const QList<ModelGroup*> list = p->modelGroupsOnBorder();

		if (list.size() > 0)
		{
			StayOnBorderMessage* message = new StayOnBorderMessage(list);
			getKernelUI()->requestMessage(message);
		}
		else
		{
			getKernelUI()->destroyMessage(MessageType::StayOnBorder);
		}
	}

	bool PrinterMangager::appendPrinters(int count)
	{
		if (count <= 0)
			return false;

		QList<Printer*> addedPrinters;
		for (int i = 0; i < count; i++)
		{
			Printer* newPrinter = new Printer(*getPrinter(0));
			Q_ASSERT(newPrinter);
			addedPrinters << newPrinter;
		}

		int insertIndex = this->printersCount();

		QList<ModelN*> modelsToMove;
		QList<QVector3D> starts, ends;

		int sizeBeforeInsert = this->printersCount();

		int sizeAfterInsert = sizeBeforeInsert + count;

		if (1)
		{
			//要移动的盘
			QList<Printer*> printersNeedMove;  //所有要移动的盘
			QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
			QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->globalBox(), sizeAfterInsert);
			for (size_t i = 0; i < sizeBeforeInsert; i++)
			{
				Printer* prt = getPrinter(i);

				int indexOfAllBox = (i >= insertIndex ? i + 1 : i);

				if (prt->globalBox() != allBoxs.at(indexOfAllBox))
				{
					printersNeedMove << prt;
					moveBoxs << allBoxs.at(indexOfAllBox);
				}
			}

			//找到所有需要移动的模型，提前计算好它们需要移动到的位置
			for (size_t i = 0; i < printersNeedMove.size(); i++)
			{
				Printer* prt = printersNeedMove.at(i);
				QVector3D printerStartPostion = prt->position();

				QVector3D printerEndPosition = moveBoxs.at(i).min;

				QList<ModelN*> sub = prt->modelsInside();
				for (ModelN* m : sub)
				{
					QVector3D startPos = m->localPosition();
					QVector3D offset = startPos - printerStartPostion;

					modelsToMove << m;
					starts << startPos;
					ends << (printerEndPosition + offset);
				}
			}
		}

		for (int k = 0; k < addedPrinters.size(); k++)
		{
			Printer* p = addedPrinters.at(k);
			p->addTracer(this);
			p->setVisibility(true);
			p->setParent(this);
			p->onGlobalPlateTypeChanged(getKernel()->parameterManager()->currBedType());
			m_printers.append(p);
		}


		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* prt = m_printers.at(i);
			prt->setIndex(i);
			if(i == (m_printers.size() - 1) )
				prt->setSelected(true);
			else
			{
				prt->setSelected(false);
			}
		}

		layout();

		//after layout, moving models
		moveModels(modelsToMove, starts, ends, false);

		groupModelsByPrinterGlobalBox();

		for (Printer* pr : addedPrinters)
		{
			notifyAddedPrinter(pr);
		}

		onPrintersCountChanged();

		viewAllPrintersFromTop();

		requestVisUpdate(true);

		return true;
	}

	//
	void PrinterMangager::removePrinters(int removeCount, bool isCompletely)
	{
		if (removeCount <= 0 || removeCount >= m_printers.size())
			return;

		//int deleteIndex = m_printers.indexOf(p);
		//if (deleteIndex == -1)
		//{
		//	return;
		//}

		QList<ModelN*> modelsToMove;
		QList<QVector3D> starts, ends;

		int sizeBeforeDelete = m_printers.size();
		int sizeAfterDelete = sizeBeforeDelete - removeCount;

		QList<Printer*> needDeletePrinters;
		for (int i = 0; i < removeCount; i++)
		{
			Printer* pr = m_printers.at(sizeBeforeDelete - i - 1);
			needDeletePrinters << pr;
		}

		//before delete

		//要删除的盘

		for (Printer* p : needDeletePrinters)
		{
			p->addTracer(nullptr);
			bool resetSelect = p->selected();
			QList<ModelN*> modelsOfDeletePrinter = p->modelsInside();
			QVector3D printerPosition = p->position();
			QVector3D takeInPosition = QVector3D(0.0, m_printers.first()->box().size().y() + 50.0, 0.0);
			for (ModelN* m : modelsOfDeletePrinter)
			{
				//移动之前的位置
				QVector3D startPos = m->localPosition();
				QVector3D offset = startPos - printerPosition;

				modelsToMove << m;
				starts << startPos;
				ends << (takeInPosition + offset);
			}
		}


		//要移动的盘
		QList<Printer*> printersNeedMove;  //所有要移动的盘
		QList<qtuser_3d::Box3D> moveBoxs;  //这些盘目标位置
		QList<qtuser_3d::Box3D> allBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->box(), sizeAfterDelete);

		for (size_t i = 0; i < sizeAfterDelete; i++)
		{
			Printer* prt;
			prt = m_printers.at(i);
			//if (i >= deleteIndex)
			//{
			//	prt = m_printers.at(i + 1);
			//}
			//else {
			//	prt = m_printers.at(i);
			//}

			if (prt->globalBox() != allBoxs.at(i))
			{
				printersNeedMove << prt;
				moveBoxs << allBoxs.at(i);
			}
		}

		//找到所有需要移动的模型，提前计算好它们需要移动到的位置
		for (size_t i = 0; i < printersNeedMove.size(); i++)
		{
			Printer* prt = printersNeedMove.at(i);
			QVector3D printerStartPostion = prt->position();

			QVector3D printerEndPosition = moveBoxs.at(i).min;

			QList<ModelN*> sub = prt->modelsInside();
			for (ModelN* m : sub)
			{
				QVector3D startPos = m->localPosition();
				QVector3D offset = startPos - printerStartPostion;

				modelsToMove << m;
				starts << startPos;
				ends << (printerEndPosition + offset);
			}
		}

		//layout
		//m_printers.removeOne(p);
		for (Printer* pr : needDeletePrinters)
		{
			pr->disconnectModels();
			m_printers.removeOne(pr);
		}

		for (size_t i = 0; i < m_printers.size(); i++)
		{
			Printer* prt = m_printers.at(i);
			prt->setSelected(0 == i);
			prt->setIndex(i);
		}

		layout();

		//after layout, moving models
		moveModels(modelsToMove, starts, ends, true);

		//visCamera()->fittingBoundingBox(m_boundingBox);
		requestVisUpdate(true);

		//groupModelsByPrinterGlobalBox();

		for (Printer* pr : needDeletePrinters)
		{
			notifyDeletePrinter(pr);
		}


		onPrintersCountChanged();

		for (Printer* pr : needDeletePrinters)
		{
			if (isCompletely)
				delete pr;
			else
				pr->setVisibility(false);
		}

	}

	void PrinterMangager::checkEmptyPrintersToRemove()
	{
		int count = printersCount();

		QList<Printer*> removes;

		for (size_t i = 0; i < count; i++)
		{
			Printer* prt = getPrinter(i);

			if (prt && prt->modelGroupsInside().size() <= 0 && !prt->lock())
			{
				removes << prt;
			}
		}

		int index = 0;
		for (Printer* printer : removes)
		{
			deletePrinter(printer);
		}

		updatePrinterContent();
	}

	void PrinterMangager::notifyAddedPrinter(Printer* p)
	{
		emit signalAddPrinter(p);
		if (m_tracer)
		{
			m_tracer->didAddPrinter(p);
			notifyPrinter(p);
		}
	}

	void PrinterMangager::notifyDeletePrinter(Printer* p)
	{
		emit signalDeletePrinter(p);
		if (m_tracer)
		{
			m_tracer->didDeletePrinter(p);
			disNodifyPrinter(p);
		}
	}

	void PrinterMangager::notifySelectPrinter(Printer* p)
	{
		if (m_tracer)
		{
			m_tracer->didSelectPrinter(p);
		}
	}

	void PrinterMangager::notifyPrinter(Printer* p)
	{
		connect(p, &Printer::signalSlicedStateChanged, this, &PrinterMangager::sigHasPrinterSlicedChanged);
	}

	void PrinterMangager::disNodifyPrinter(Printer* p)
	{
		disconnect(p, &Printer::signalSlicedStateChanged, this, &PrinterMangager::sigHasPrinterSlicedChanged);
	}

	QByteArray PrinterMangager::getSerialData()
	{
		QJsonObject rootObj;
		QJsonArray printer_array;
		for (const auto printer : m_printers)
		{
			QJsonObject printerObj;
			printerObj.insert("index", printer->index());
			printerObj.insert("name", printer->name());
			printerObj.insert("x", printer->position().x());
			printerObj.insert("y", printer->position().y());
			printerObj.insert("z", printer->position().z());
			printer_array.append(printerObj);
		}
		rootObj.insert("printers", printer_array);
		QJsonDocument doc(rootObj);
		return doc.toJson(QJsonDocument::Compact);
	}
}
