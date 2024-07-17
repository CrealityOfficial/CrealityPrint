#include "nest2djobex.h"
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "interface/machineinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "interface/layoutinterface.h"
#include "interface/undoredointerface.h"
#include "utils/modelpositioninitializer.h"
#include "internal/multi_printer/printermanager.h"
#include "qtuser3d/trimesh2/conv.h"
#include "kernel/kernel.h"
#include "nestplacer/placer.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/undo/layoutmodelsmovecommand.h"
#include "internal/data/_modelinterface.h"

namespace creative_kernel
{
	LayoutItemEx::LayoutItemEx(ModelN* _model, bool outlineConcave, QObject* parent)
		: QObject(parent)
		, model(_model)
		, m_outlineConcave(outlineConcave)
	{

	}

	LayoutItemEx::~LayoutItemEx()
	{
#ifdef DEBUG
		qInfo() << Q_FUNC_INFO;
#endif // DEBUG

	}

	trimesh::vec3 LayoutItemEx::position()
	{
		return qtuser_3d::qVector3D2Vec3(model->localPosition());
	}

	trimesh::box3 LayoutItemEx::globalBox()
	{
		return qtuser_3d::qBox32box3(model->globalSpaceBox());
	}

	std::vector<trimesh::vec3> LayoutItemEx::outLine(bool global, bool bTranslate)
	{
		Q_ASSERT(model);

		if(m_outlineConcave)
			//return model->concave_path(global, bTranslate);
			return model->concave_path();

		return model->outline_path(global, bTranslate);
	}


	// ====  another multiBin layout strategy ====

	MultiBinExtendStrategyEx::MultiBinExtendStrategyEx(int priorBin)
		: nestplacer::BinExtendStrategy()
		, m_hasWipeTowerFlag(false)
		, m_priorBinId(priorBin)
	{
		m_box0 = getPrinter(0)->box();
		m_estimateBoxSizes = printersCount();
		m_estimateBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->box(), m_estimateBoxSizes);
	}

	MultiBinExtendStrategyEx::~MultiBinExtendStrategyEx()
	{
	}

	// this func returns the box of the index layout printer, 
	// "index" is likely bigger than total printers count of current scene, when needing to generate a new bin
	trimesh::box3 MultiBinExtendStrategyEx::bounding(int index) const
	{
		if (index < 0)
		{
			// this case should never happen
			return qtuser_3d::qBox32box3(m_box0);
		}

		int firstUnlockIdx = getPriorPrinterIndex(index);

		Printer* firstUnlockPrinter = getPrinter(firstUnlockIdx);
		if (!firstUnlockPrinter)
		{
			//// need to generate new printer
			if (firstUnlockIdx >= 0 && firstUnlockIdx < m_estimateBoxs.size())
				return qtuser_3d::qBox32box3(m_estimateBoxs[firstUnlockIdx]);

			return qtuser_3d::qBox32box3(m_box0);

		}

		qtuser_3d::Box3D printerBox = getPrinter(firstUnlockIdx)->globalBox();
		return qtuser_3d::qBox32box3(printerBox);
		
	}

	// this func returns the box in which the fixItem resides 
	trimesh::box3 MultiBinExtendStrategyEx::fixBounding(int fixBinId) const
	{
		//Q_ASSERT(fixBinId >= 0 && fixBinId < printersCount());
		if (fixBinId < 0 || fixBinId >= printersCount())
		{
			return qtuser_3d::qBox32box3(m_box0);
		}

		int firstUnlockIdx = 0;

		firstUnlockIdx = getPriorPrinterIndex(fixBinId);

		//Q_ASSERT(firstUnlockIdx >= 0 && firstUnlockIdx < printersCount());
		if (firstUnlockIdx < 0 || firstUnlockIdx >= printersCount())
		{
			return qtuser_3d::qBox32box3(m_box0);
		}

		qtuser_3d::Box3D printerBox = getPrinter(firstUnlockIdx)->globalBox();
		return qtuser_3d::qBox32box3(printerBox);
	}

	// --
	std::vector<int> MultiBinExtendStrategyEx::getPriorIndexArray() const
	{
		int lockedPrintersNum = getLockedPrintersNum();

		std::vector<int> numbers;
		//int printerCount = printersCount() - lockedPrintersNum;
		int printerCount = m_estimateBoxSizes - lockedPrintersNum;
		if (printerCount <= 0)
		{
			// all printers could possibly be locked
			printerCount = printersCount() + 1;
			numbers.push_back(printerCount - 1);
		}
		else
		{
			numbers.reserve(printerCount);

			for (int i = 0; i < m_estimateBoxSizes; ++i)
			{
				if ( i < printersCount())
				{
					if (!getPrinter(i)->lock())
					{
						numbers.emplace_back(i);
					}	
				}
				else
				{
					numbers.emplace_back(i);
				}

			}
		}

		
		auto iter = std::find(numbers.begin(), numbers.end(), m_priorBinId);
		if (iter != numbers.end()) {
			std::rotate(numbers.begin(), iter, iter + 1);
		}

		return numbers;
	}

	int MultiBinExtendStrategyEx::getPriorPrinterIndex(int resultIndex) const
	{
		std::vector<int> numbers = getPriorIndexArray();

		if (resultIndex >= 0 && resultIndex < numbers.size())
			return numbers[resultIndex];

		if (resultIndex >= numbers.size())
			return 0;

		qWarning() << Q_FUNC_INFO << "resultIndex : " << resultIndex << "  out of range";

		return 0;
	}

	void MultiBinExtendStrategyEx::setEstimateBoxSize(int boxSize)
	{
		m_estimateBoxSizes = boxSize;
		
		m_estimateBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->box(), m_estimateBoxSizes);
	}

	//====  another multiBin layout strategy ====




    Nest2DJobEx::Nest2DJobEx(QObject* parent)
        : cxkernel::Nest2DJobEx(parent)
        , m_object(nullptr)
		, m_selectAll(false)
		, m_importFlag(false)
		, m_priorBinIndex(0)
		, m_needPrintersCount(0)
		, m_maxBinId(0)
		, m_strategy(nullptr)
		, m_printersNeedLayout(true)
		, m_needAddToScene(true)
		, m_doExtraFlag(false)
		, m_finalFlag(false)
    {
    }

    Nest2DJobEx::~Nest2DJobEx()
    {
		if (m_strategy)
		{
			delete m_strategy;
			m_strategy = nullptr;
		}
    }

    void Nest2DJobEx::setSpaceBox(const qtuser_3d::Box3D& box)
    {
        trimesh::box3 bbx3;
        bbx3.min = box.min;
        bbx3.max = box.max;
        setBounding(bbx3);
    }

	void Nest2DJobEx::setSelectAllFlag(bool bSelectAll)
	{
		m_selectAll = bSelectAll;
	}

	void Nest2DJobEx::setActiveLayoutModels(QList<ModelN*> activeModels)
	{
		m_activeLayoutModels = activeModels;

		if (m_activeLayoutModels.size() > 0)
			m_importFlag = true;
		else
			m_importFlag = false;
	}

	void Nest2DJobEx::setPriorBinIndex(int priorBinIndex)
	{
		Q_ASSERT(priorBinIndex >= 0 && priorBinIndex < printersCount());

		// what if the prior printer is locked ??
		m_priorBinIndex = priorBinIndex;

		m_strategy = new creative_kernel::MultiBinExtendStrategyEx(m_priorBinIndex);
		setStrategy(m_strategy);

	}

	void Nest2DJobEx::setPrintersNeedLayout(bool bNeedLayout)
	{
		m_printersNeedLayout = bNeedLayout;
	}

	void Nest2DJobEx::setNeedAddToScene(bool bNeedAddToScene)
	{
		m_needAddToScene = bNeedAddToScene;
	}

	void Nest2DJobEx::setDoExtraFlag(bool extraFlag, bool finalFlag, int estimatePrinterSize)
	{
		m_doExtraFlag = extraFlag;
		m_finalFlag = finalFlag;

		if (m_doExtraFlag && m_strategy)
		{
			m_strategy->setEstimateBoxSize(estimatePrinterSize);
		}
	}

	void Nest2DJobEx::setSavedSceneInfo(const LayoutChangeInfo& initInfo)
	{
		m_initSceneInfo = initInfo;
	}

    void Nest2DJobEx::prepare()
    {

    }

    void Nest2DJobEx::failed()
    {
		qWarning() << Q_FUNC_INFO;
    }

	void Nest2DJobEx::beforeWork()
	{
		cxkernel::Nest2DJobEx::beforeWork();

		prepareBySelectFlag();

	}

	void Nest2DJobEx::getActualNeedPrintersNum(const QList<int>& binIndexList)
	{
		int extraPrintersCount = 0;

		Q_ASSERT(m_strategy);
		std::vector<int> priorIndexArray = m_strategy->getPriorIndexArray();

		int maxBinId = 0;
		for (int binIdx : binIndexList)
		{
			if (binIdx >= priorIndexArray.size())
			{
				extraPrintersCount += 1;
			}
			else
			{
				int tmpIndex = priorIndexArray[binIdx];

				if (!getPrinter(tmpIndex))
				{
					extraPrintersCount += 1;
				}
				else
				{
					if (tmpIndex >= maxBinId)
						maxBinId = tmpIndex;
				}

			}
		}

		if (extraPrintersCount > 0)
		{
			m_needPrintersCount = printersCount() + extraPrintersCount;
		}
		else
		{
			m_needPrintersCount = maxBinId + 1;
		}

	}

	void Nest2DJobEx::afterWork()
	{
		const float EPSINON = 0.00001f;

		m_outResults.clear();
		//m_maxBinId = 0;

		QList<int> diffBinIndexList;
		for (int i = 0; i < m_results.size(); i++)
		{
			//if (m_results[i].binIndex >= m_maxBinId)
			//{
			//	m_maxBinId = m_results[i].binIndex;
			//}

			if (!diffBinIndexList.contains(m_results[i].binIndex))
				diffBinIndexList.push_back(m_results[i].binIndex);

			if (m_results[i].rt.x >= -EPSINON && m_results[i].rt.x <= EPSINON
				&& m_results[i].rt.y >= -EPSINON && m_results[i].rt.y <= EPSINON)
			{
				continue;
			}

			cxkernel::NestResultEx aResult;
			aResult.binIndex = m_results[i].binIndex;
			aResult.rt = m_results[i].rt;
			m_outResults.push_back(aResult);
		}

		getActualNeedPrintersNum(diffBinIndexList);

		if (m_outResults.size() <= 0)
			return;

		if (m_outResults.size() != m_activeObjects.size())
		{
			qWarning() << "m_outResults.size() != m_activeObjects.size()";
			return;
		}

		
		for (int i = 0; i < m_outResults.size(); i++)
		{
			m_activeObjects[i]->nestResult = m_outResults[i];
		}

		// the lock printers  do not involves in the layout algorithm
		//int lockedPrintersNum = getLockedPrintersNum();

		//m_needPrintersCount = m_maxBinId + 1 + lockedPrintersNum;

	}

    // invoke from main thread
    void Nest2DJobEx::successed(qtuser_core::Progressor* progressor)
    {
		if (m_activeObjects.isEmpty())
			return;

		if (!m_doExtraFlag)
		{
			// related to add or delete printers, or layout with wipeTower;
			// the first time layout, retrive estimatePrinterSize;
			activeModelsAddToScene();
			emit extraLayout(m_needPrintersCount);
			return;
		}

		QList<ModelN*> activeModels;
		 creative_kernel::LayoutChangeInfo changeInfo;
		 changeInfo.prevPrintersCount = printersCount();
		 changeInfo.nowPrintersCount = m_strategy->m_estimateBoxSizes;

		 Q_ASSERT(m_strategy);
		 auto f = [this,&changeInfo, &activeModels](LayoutItemEx* item) {
		 	const cxkernel::NestResultEx& result = item->nestResult;

		 	ModelN* m = item->model;
		 	QVector3D lt = m->localPosition();
		 	trimesh::quaternion q = m->nestRotation();
			
			trimesh::vec3 modelPosition = result.rt;

			//int priorPrinterIdx = m_strategy->getPriorPrinterIndex(result.binIndex);

			//if (priorPrinterIdx < 0 || priorPrinterIdx > m_strategy->m_estimateBoxs.size())
			//	return;

		 	QVector3D t(modelPosition.x, modelPosition.y, lt.z());

		 	QVector3D axis(0.0f, 0.0f, 1.0f);
		 	trimesh::quaternion dq = trimesh::quaternion::fromAxisAndAngle(qtuser_3d::qVector3D2Vec3(axis.normalized()), result.rt.z);
		 	trimesh::quaternion rq = dq * q;

			changeInfo.moveModelsSerialNames.push_back(m->getSerialName());

			t.setZ(m->zeroLocation().z());

			changeInfo.endPoses.push_back(t);
			changeInfo.endQuaternions.push_back(qtuser_3d::tqua2qqua(rq));

			changeInfo.startPoses.push_back(lt);
			changeInfo.startQuaternions.push_back(qtuser_3d::tqua2qqua(q));

			activeModels.push_back(m);
		 };

		 for (LayoutItemEx* item : m_activeObjects)
		 {
		 	f(item);
		 }

		 if (!m_finalFlag)
		 {
			 m_initSceneInfo.startPoses = changeInfo.startPoses;
			 m_initSceneInfo.startQuaternions = changeInfo.startQuaternions;
			 m_initSceneInfo.prevPrintersCount = changeInfo.prevPrintersCount;
			 m_initSceneInfo.nowPrintersCount = changeInfo.nowPrintersCount;

			 changeInfo.needPrinterLayout = !m_importFlag;
			 m_initSceneInfo.needPrinterLayout = changeInfo.needPrinterLayout;

			 layoutChangeScene(changeInfo, false);
		 }
		 else
		 {
			 changeInfo.startPoses = m_initSceneInfo.startPoses;
			 changeInfo.startQuaternions = m_initSceneInfo.startQuaternions;
			 changeInfo.prevPrintersCount = m_initSceneInfo.prevPrintersCount;
			 changeInfo.nowPrintersCount = m_initSceneInfo.nowPrintersCount;
			 changeInfo.needPrinterLayout = m_initSceneInfo.needPrinterLayout;
			 layoutChangeScene(changeInfo, !m_importFlag);

			 // the third time layout , set the newly imported models visual to  true
			 if (m_importFlag && m_needAddToScene)
			 {
				 for (ModelN* m : activeModels)
				 {
					 m->setVisibility(true);
				 }
			 }

		 }
		 

		 for (int i = 0; i < m_activeObjects.size(); i++)
		 {
		 	if (m_activeObjects[i])
		 	{
		 		delete m_activeObjects[i];
		 		m_activeObjects[i] = nullptr;
		 	}
		 }

		 creative_kernel::checkModelCollision();

		 if (!m_finalFlag)
		 {
			 emit secondLayoutSuccessed(m_initSceneInfo);
		 }
		 else
		 {
			 emit finalLayoutSuccessed();
		 }
			
     }


	 void Nest2DJobEx::prepareBySelectFlag()
	 {

		 buildNestFixWipeTowerInfo();

		 if (m_importFlag)  // if importFlag is true, means import models or copy models, or just layout models in one printer
		 {
			 for (ModelN* model : m_activeLayoutModels)
			 {
				 LayoutItemEx* item = new LayoutItemEx(model, m_outlineConcave);
				 m_activeObjects.append(item);
				 m_activeOutlines.push_back(item->outLine(false, false));
			 }

			 buildNestFixModelInfo(false);

			 return;

		 }

		 if (m_selectAll)  // layout all in scene models
		 {
			 buildSelectAllNestInfo();
		 }
		 else
		 {
			 QList<ModelN*> tmpLockedModels;
			 QList< QPair<ModelN*, int> > lockedModels = getAllLockedPrinterModels();

			 for (auto pa : lockedModels)
			 {
				 tmpLockedModels.append(pa.first);
			 }

			 for (ModelN* sm : selectionms())
			 {
				 if (tmpLockedModels.contains(sm))
					 continue;

				 LayoutItemEx* item = new LayoutItemEx(sm, m_outlineConcave);
				 m_activeObjects.append(item);

				 //actives use local outline
				 m_activeOutlines.push_back(item->outLine(false, false));
			 }

			 buildNestFixModelInfo(true);

		 }

	 }


	 void Nest2DJobEx::buildNestFixWipeTowerInfo()
	 {
		 cxkernel::NestFixedItemInfo tmpFixItemInfo;
		 int oriPrinterIndex = 0;
		 int priorPrinterIndex = 0;

		 Q_ASSERT(m_strategy);
		 std::vector<int> priorIndexArray = m_strategy->getPriorIndexArray();

		 m_strategy->m_hasWipeTowerFlag = false;
		 for (int i = 0; i < priorIndexArray.size(); i++)
		 {
			 priorPrinterIndex = priorIndexArray[i];

			 Printer* printerPtr = getPrinterMangager()->getPrinter(priorPrinterIndex);

			 if (!printerPtr || printerPtr->lock() || !printerPtr->checkWipeTowerShouldShow() )
				 continue;

			 tmpFixItemInfo.fixOutline = getPrinterWipeTowerOutline(priorPrinterIndex);
			 if (tmpFixItemInfo.fixOutline[0] == tmpFixItemInfo.fixOutline[1])
				 continue;

			 m_strategy->m_hasWipeTowerFlag = true;
			 tmpFixItemInfo.fixBinId = i;
			 tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(priorPrinterIndex)->globalBox());

			 m_fixInfos.push_back(tmpFixItemInfo);
		 }

	 }

	 void Nest2DJobEx::buildNestFixModelInfo(bool judgeSelect)
	 {
		 cxkernel::NestFixedItemInfo tmpFixItemInfo;
		 int oriPrinterIndex = 0;
		 int priorPrinterIndex = 0;
		 int tmpFixBinIndex = 0;

		 Q_ASSERT(m_strategy);
		 std::vector<int> priorIndexArray = m_strategy->getPriorIndexArray();

		 for (int i = 0; i < priorIndexArray.size(); i++)
		 {
			 priorPrinterIndex = priorIndexArray[i];

			 QList<ModelN*> inPrinterModels = getPrinterModels(priorPrinterIndex);

			 tmpFixBinIndex = i;

			 for (ModelN* aModel : inPrinterModels)
			 {
				 if (judgeSelect)
				 {
					 if (aModel->selected())
						 continue;
				 }

				 if (m_importFlag)
				 {
					 if (m_activeLayoutModels.contains(aModel))
						 continue;
				 }

				 LayoutItemEx fixitem(aModel, m_outlineConcave);

				 //fixs use global outline
				 tmpFixItemInfo.fixOutline = fixitem.outLine(true, true);
				 tmpFixItemInfo.fixBinId = tmpFixBinIndex;
				 tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(priorPrinterIndex)->globalBox());

				 m_fixInfos.push_back(tmpFixItemInfo);
			 }
		 }
	 }

	 void Nest2DJobEx::buildSelectAllNestInfo()
	 {
		 const QList<ModelN*>& inSceneModels = modelns();

		 QList<ModelN*> tmpLockedModels;
		 QList< QPair<ModelN*, int> > lockedModels = getAllLockedPrinterModels();

		 cxkernel::NestFixedItemInfo tmpFixItemInfo;
		 for (auto pa : lockedModels)
		 {
			 tmpLockedModels.append(pa.first);
		 }

		 for (ModelN* model : inSceneModels)
		 {
			 if (tmpLockedModels.contains(model))
			 {
				 continue;
			 }

			 LayoutItemEx* item = new LayoutItemEx(model, m_outlineConcave, this);

			 m_activeObjects.append(item);
			 m_activeOutlines.push_back(item->outLine(false, false));

			 m_md5NameList << model->modelNData()->input.fileName;

		 }

	 }

	 void Nest2DJobEx::activeModelsAddToScene()
	 {
		 if (m_importFlag && m_needAddToScene)
		 {
			 QList<ModelN*> addedModels;
			 for (auto obj : m_activeObjects)
			 {
				 addedModels << obj->model;

				 // fix bug : display error tip before layout
				 qtuser_3d::Box3D modelBox = obj->model->globalSpaceBox();
				 QVector3D newPos = getPrinter(0)->position();
				 newPos.setX(newPos.x() - modelBox.size().x());
				 newPos.setY(newPos.y() + modelBox.size().y() / 2);
				 _setModelPosition(obj->model, newPos, false);
			 }

			 addModels(addedModels, true);

			 // the first time layout , after retrive the estimate printer size, set model visual to false
			 for (ModelN* m : addedModels)
			 {
				 m->setVisibility(false);
			 }
		 }
	 }

}