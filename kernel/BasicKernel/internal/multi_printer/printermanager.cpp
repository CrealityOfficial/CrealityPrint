#include  <cfloat>
#include "printermanager.h"
#include "external/kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "external/data/modeln.h"
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
#include "interface/modelinterface.h"
#include "internal/utils/byobjecteventhandler.h"
#include "internal/utils/bylayereventhandler.h"
#include "interface/machineinterface.h"
#include "interface/eventinterface.h"
#include "kernel/kernelui.h"
#include "external/message/stayonbordermessage.h"
#include "interface/renderinterface.h"
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

		emit signalAddPrinter(p);

		groupModelsByPrinterGlobalBox();

		onPrintersCountChanged();

		viewAllPrintersFromTop();

		requestVisUpdate(true);
		return true;
	}

	bool PrinterMangager::addPrinter(Printer* p)
	{
		int index = printersCount();
		return insertPrinter(index, p);
	}

	bool PrinterMangager::insertPrinterEx(int index, Printer* p, QList<QString>& moveModelSerialNames, QList<QVector3D>& starts, QList<QVector3D>& ends)
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

				QList<ModelN*> sub = prt->modelsInside();
				for (ModelN* m : sub)
				{
					QVector3D startPos = m->localPosition();
					QVector3D offset = startPos - printerStartPostion;

					//modelsToMove << m;
					moveModelSerialNames << m->getSerialName();
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

		emit signalAddPrinter(p);

		groupModelsByPrinterGlobalBox();
		
		onPrintersCountChanged();

		viewAllPrintersFromTop();
		
		requestVisUpdate(true);		
		return true;
	}

	void PrinterMangager::deletePrinterEx(Printer* p, QList<QString>& moveModelSerialNames, QList<QVector3D>& starts, QList<QVector3D>& ends, bool isCompletely)
	{
		if (!p)
			return;

		int deleteIndex = m_printers.indexOf(p);
		if (deleteIndex == -1)
		{
			return;
		}
		
		

		//QList<ModelN*> modelsToMove;
		//QList<QVector3D> starts, ends;
		int sizeBeforeDelete = m_printers.size();
		int sizeAfterDelete = sizeBeforeDelete - 1;

		//before delete

		//要删除的盘
		p->addTracer(nullptr);
		bool resetSelect = p->selected();
		QList<ModelN*> modelsOfDeletePrinter = p->modelsInside();
		QVector3D printerPosition = p->position();
		QVector3D takeInPosition = QVector3D(0.0, m_printers.first()->box().size().y()+50.0, 0.0);
		for (ModelN* m : modelsOfDeletePrinter)
		{
			//移动之前的位置
			QVector3D startPos = m->localPosition();
			QVector3D offset = startPos - printerPosition;

			//modelsToMove << m;
			moveModelSerialNames << m->getSerialName();
			starts << startPos;
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

			QList<ModelN*> sub = prt->modelsInside();
			for (ModelN* m : sub)
			{
				QVector3D startPos = m->localPosition();
				QVector3D offset = startPos - printerStartPostion;

				//modelsToMove << m;
				moveModelSerialNames << m->getSerialName();
				
				starts << startPos;
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
		//moveModels(modelsToMove, starts, ends, true);  // do this in undo command
		
		//visCamera()->fittingBoundingBox(m_boundingBox);
		requestVisUpdate(true);

		//groupModelsByPrinterGlobalBox();
		
		emit signalDeletePrinter(p);
		onPrintersCountChanged();
		
		if (isCompletely)
			p->deleteLater();
		else 
			p->setVisibility(false);
	}

	void PrinterMangager::updatePrinterContent()
	{
		groupModelsByPrinterGlobalBox();
		updateWipeTowers();

		checkSelectedPrinterModelsOnBorder();
		//checkModelsInSelectedPrinterCollision();
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

	void PrinterMangager::checkModelsToMove(int estimateBoxSize, QList<ModelN*>& modelsToMove, QList<QVector3D>& startPoses, QList<QVector3D>& endPoses)
	{
		int curSize = m_printers.size();

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

				QVector3D printerEndPosition = moveBoxs.at(i).min;

				QList<ModelN*> sub = prt->modelsInside();
				for (ModelN* m : sub)
				{
					QVector3D startPos = m->localPosition();
					QVector3D offset = startPos - printerStartPostion;

					modelsToMove << m;
					startPoses << startPos;
					endPoses << (printerEndPosition + offset);
				}
			}
		}
	}

	bool PrinterMangager::resizePrinters(int num)
	{
		int size = m_printers.size();
		if (size == num)
			return true;
		
		if (size == 0 || num == 0)
			return false;

		QList<ModelN*> modelsToMove;
		QList<QVector3D> startPoses;
		QList<QVector3D> endPoses;
		checkModelsToMove(num, modelsToMove, startPoses, endPoses);

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
				emit signalAddPrinter(p);
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
				emit signalDeletePrinter(p);
				p->deleteLater();

				/*if (isCompletely)
					p->deleteLater();
				else
					p->setVisibility(false);*/
			}
		}

		//after layout, moving models
		moveModels(modelsToMove, startPoses, endPoses, false);
		
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

		QList<ModelN*> modelsToMove;
		QList<QVector3D> starts, ends;
		int sizeBeforeDelete = m_printers.size();
		int sizeAfterDelete = sizeBeforeDelete - 1;

		//before delete

		//要删除的盘
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
		moveModels(modelsToMove, starts, ends, true);

		//visCamera()->fittingBoundingBox(m_boundingBox);
		requestVisUpdate(true);

		//groupModelsByPrinterGlobalBox();

		emit signalDeletePrinter(p);
		onPrintersCountChanged();

		if (isCompletely)
		{
			p->setParent(nullptr);
			p->deleteLater();
		}
		else
			p->setVisibility(false);
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

	bool PrinterMangager::selectPrinter(int index, bool exclude)
	{
		if (0 <= index && index < m_printers.size())
		{
			setContinousRender();

			for (size_t i = 0; i < m_printers.size(); i++)
			{
				Printer* p = m_printers.at(i);
				if (i == index)
				{
					p->setSelected(true);
				}
				else if (exclude)
				{
					p->setSelected(false);
				}
			}

			//groupModelsByPrinterGlobalBox();

			unselectAll();

			setCommandRender();

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

		emit signalDidSelectPrinter(p);

		//check models collision
		localPrintSequenceChanged(p, p->getLocalPrintSequence());
		p->checkWipeTowerCollide();

		checkSelectedPrinterModelsOnBorder();
		p->checkModelInsideVisibleLayers(true);

		p->checkSliceError();
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
			if (m_tracer) {
				m_tracer->message(p, 0, QString("auto layout while locking"));
			}
			return;
		}
		if (m_tracer)
			m_tracer->willAutolayoutForPrinter(p);
	}

	void  PrinterMangager::willPickBottomForPrinter(Printer* p)
	{
		QList<ModelN*> selections = selectionms();
		if (selections.empty())
		{
			creative_kernel::setModelsMaxFaceBottom(p->modelsInside());
		}
		else
		{
			creative_kernel::setModelsMaxFaceBottom(selections);
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

	void  PrinterMangager::willModifyPrinter(Printer* p)
	{
		if (m_tracer)
			m_tracer->willModifyPrinter(p);
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
		visCamera()->fittingBoundingBox(extendBox());
		Qt3DRender::QCamera* camera = visCamera()->camera();
		camera->setFieldOfView(30.0f);
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

	void PrinterMangager::groupModelsByPrinterGlobalBox()
	{
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
					Qt3DCore::QEntity* entity = model->getModelEntity();
					ModelNEntity* modelE = qobject_cast<ModelNEntity*>(entity);
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

	void PrinterMangager::onBoxChanged(const qtuser_3d::Box3D& box)
	{
	}

	void PrinterMangager::onSceneChanged(const qtuser_3d::Box3D& box) 
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

	void PrinterMangager::updateWipeTowersPositions(float x, float y)
	{
		for (size_t i = 0; i < m_printers.size(); i++)
		{
			creative_kernel::Printer* sub = m_printers.at(i);
			sub->updateWipeTowerState();
			sub->entity()->wipeTowerEntity()->setLeftBottomPosition(x, y);
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
			controller->viewFromTop();

			screenCamera->fittingBoundingBox(extendBox());
			
			Qt3DRender::QCamera* camera = screenCamera->camera();
			camera->setFieldOfView(30.0f);
			screenCamera->notifyCameraChanged();
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

		checkModelsInSelectedPrinterCollision();
	}

	void PrinterMangager::checkModelsInSelectedPrinterCollision()
	{
		if (m_collisionCheckHandler && !m_collisionCheckHandler->processing())
		{
			m_collisionCheckHandler->checkModels(getSelectedPrinter()->modelsInside());
		}
	}

	void PrinterMangager::checkSelectedPrinterModelsOnBorder()
	{
		Printer* p = getSelectedPrinter();
		const QList<ModelN*> list = p->modelsOnBorder();
		
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
			signalAddPrinter(pr);
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
			emit signalDeletePrinter(pr);
		}
		

		onPrintersCountChanged();

		for (Printer* pr : needDeletePrinters)
		{
			if (isCompletely)
				pr->deleteLater();
			else
				pr->setVisibility(false);
		}

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