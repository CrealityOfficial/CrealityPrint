#include "nest2djobex.h"
#include "layoutitemex.h"
#include "multibinextendstrategy.h"
#include "layoutmanager.h"
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelgroupinterface.h"
#include "interface/machineinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "interface/layoutinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "qtuser3d/trimesh2/conv.h"
#include "kernel/kernel.h"
#include "nestplacer/placer.h"
#include "internal/multi_printer/printermanager.h"
#include "qtusercore/module/progressortracer.h"
#include "cxkernel/interface/constinterface.h"
#include "cxkernel/interface/cacheinterface.h"
#include "cxkernel/wrapper/placeitem.h"
#include <QtCore/QDateTime>

namespace creative_kernel
{
    Nest2DJobEx::Nest2DJobEx(QObject* parent)
		: Job(parent)
		, m_selectAll(false)
		, m_layoutOnePrinterFlag(false)
		, m_priorBinIndex(0)
		, m_needPrintersCount(0)
		, m_strategy(nullptr)
		, m_doExtraFlag(false)
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

	void Nest2DJobEx::work(qtuser_core::Progressor* progressor)
	{
		beforeWork();

		qtuser_core::ProgressorTracer tracer(progressor);

		doLayout(tracer);
	}


	void Nest2DJobEx::doLayout(ccglobal::Tracer& tracer)
	{
		nestplacer::PlacerParameter parameter;

		/*
		@param align_mode:
		0 - CENER, 1 - BOTTOM_LEFT, 2 - BOTTOM_RIGHT,
		3 - TOP_LEFT, 4 - TOP_RIGHT, 5 -  DONT_ALIGN;
		if is DONT_ALIGN, is same with no needAlign.
		*/
		parameter.startPoint = 0;

		parameter.itemGap = m_modelspacing;
		parameter.binItemGap = m_platformSpacing;
		parameter.rotate = (m_angle == 0 ? false : true);
		parameter.rotateAngle = m_angle;
		parameter.needAlign = m_alignMove;
		parameter.concaveCal = m_outlineConcave;
		parameter.box = m_box;
		parameter.curBinId = m_curBinIndex;
		parameter.tracer = &tracer;

		if (!cxkernel::isReleaseVersion())
		{
			QString cacheName = cxkernel::createNewAlgCache("autolayout");
			parameter.fileName = cacheName.toLocal8Bit().constData();
		}

		std::vector<nestplacer::PlacerItem*> fixed;
		std::vector<nestplacer::PlacerItem*> actives;
		std::vector<nestplacer::PlacerResultRT> results;

		for (int i = 0; i < m_fixInfos.size(); i++)
		{
			cxkernel::PlaceItemEx* fixPlaceItem = new cxkernel::PlaceItemEx(m_fixInfos[i].fixOutline);
			fixPlaceItem->setFixPanIndex(m_fixInfos[i].fixBinId);
			fixed.push_back(fixPlaceItem);
		}

		for (int k = 0; k < m_activeOutlines.size(); k++)
		{
			actives.push_back(new cxkernel::PlaceItemEx(m_activeOutlines[k]));
		}

#ifdef DEBUG
		qint64 t1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
#endif // DEBUG 

		place(fixed, actives, parameter, results, *m_strategy);


#ifdef DEBUG
		qint64 t1_1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
		qInfo() << "==== place Time use " << (t1_1 - t1) << " ms";
#endif // DEBUG

		QList<int> diffBinIndexList;

		Q_ASSERT(m_activeObjects.size() == results.size());
		for (int i = 0; i < results.size(); i++)
		{

			if ( (!diffBinIndexList.contains(results[i].binIndex)) && results[i].binIndex >= 0)
				diffBinIndexList.push_back(results[i].binIndex);

			LayoutNestResultEx aResult;
			aResult.binIndex = results[i].binIndex;

			if (results[i].binIndex < 0)
			{
				// fix bug : [ID1027475]
				qtuser_3d::Box3D pbox = getPrinter(0)->globalBox();
				QVector3D boxMin = pbox.min;
				aResult.rt.x = results[i].rt.x - boxMin.x() - 10.0f;
				aResult.rt.y = results[i].rt.y - boxMin.y();
				aResult.rt.z = results[i].rt.z;
			}
			else
			{
				aResult.rt = results[i].rt;
			}
			

			m_activeObjects[i]->nestResult = aResult;
		}

		if (!m_doExtraFlag)
		{
			getActualNeedPrintersNum(diffBinIndexList);
		}
		else
		{
			m_needPrintersCount = m_strategy->m_estimateBoxSizes;
		}
		

		for (int i = 0; i < fixed.size(); i++)
		{
			if (fixed[i])
			{
				delete fixed[i];
				fixed[i] = nullptr;
			}
		}

		for (int j = 0; j < actives.size(); j++)
		{
			if (actives[j])
			{
				delete actives[j];
				actives[j] = nullptr;
			}
		}

	}

	void Nest2DJobEx::getActualNeedPrintersNum(const QList<int>& binIndexList)
	{
		int extraPrintersCount = 0;

		// the locked printer box DO NOT involves layout
		int lockedNum = getLockedPrintersNum();
		
		int currentPrintersNum = printersCount();

		int remainNum = currentPrintersNum - lockedNum;

		if (binIndexList.size() > remainNum)
		{
			extraPrintersCount = binIndexList.size() - remainNum;
			m_needPrintersCount = currentPrintersNum + extraPrintersCount;
		}
		else if(binIndexList.size() == remainNum)
		{
			m_needPrintersCount = currentPrintersNum;	
		}
		else
		{
			if (m_layoutOnePrinterFlag)
			{
				m_needPrintersCount = currentPrintersNum;
			}
			else if(m_selectAll || m_activeObjects.size() == modelGroups().size())
				m_needPrintersCount = binIndexList.size() + lockedNum;
			else
			{
				m_needPrintersCount = m_strategy->getPriorIndexArray().size() + lockedNum;
			}
			
		}

	}



    void Nest2DJobEx::setSpaceBox(const qtuser_3d::Box3D& box)
    {
        trimesh::box3 bbx3;
        bbx3.min = box.min;
        bbx3.max = box.max;
        //setBounding(bbx3);
    }

	void Nest2DJobEx::setDoExtraFlag(bool extraFlag, int estimatePrinterSize)
	{
		m_doExtraFlag = extraFlag;

		if (m_doExtraFlag && m_strategy)
		{
			m_strategy->setEstimateBoxSize(estimatePrinterSize);
		}
	}

	void Nest2DJobEx::setLayoutParameterInfo(const LayoutParameterInfo& paramInfo)
	{
		if (paramInfo.priorBinIndex < 0 || paramInfo.priorBinIndex >= printersCount())
		{
			qWarning() << Q_FUNC_INFO << "==== priorBinIndex out of range;====";
			return;
		}

		m_priorBinIndex = paramInfo.priorBinIndex;
		m_strategy = new creative_kernel::MultiBinExtendStrategy(m_priorBinIndex);

		m_activeLayoutGroupIds = paramInfo.needLayoutGroupIds;
		m_layoutOnePrinterFlag = m_activeLayoutGroupIds.size() > 0 ? true : false;

		m_selectAll = paramInfo.layoutType <= 0 ? true : false;

		m_modelspacing = paramInfo.modelSpacing;
		m_platformSpacing = paramInfo.platfromSpacing;
		m_angle = paramInfo.angle;
		m_alignMove = paramInfo.alignMove;
		m_outlineConcave = paramInfo.outlineConcave;

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
		prepareBySelectFlag();

	}

	void Nest2DJobEx::prepareBySelectFlag()
	{
		buildNestFixWipeTowerInfo();

		if (m_layoutOnePrinterFlag)  // if m_layoutOnePrinterFlag is true, meaning just layout models in one printer
		{
			 for (int gid : m_activeLayoutGroupIds)
			 {
			 	LayoutItemEx* item = new LayoutItemEx(gid, m_outlineConcave);
			 	m_activeObjects.append(item);
			 	m_activeOutlines.push_back(item->outLine());
			 }

			 buildNestFixModelGroupInfo();

			 return;

		}

		if(m_selectAll)  // layout all groups in scene
		{
			buildSelectAllNestInfo();
		}
		else  // layout partial groups in scene
		{
			QList<int> tmpLockedModelGroupIds;
			QList< QPair<int, int> > lockedModelGroupIds = getAllLockedPrinterModelGroupIds();

			for (auto pa : lockedModelGroupIds)
			{
				tmpLockedModelGroupIds.append(pa.first);
			}

			QList<ModelGroup*> needLayoutGroups = selectedGroups();
			if (needLayoutGroups.size() <= 0)
			{
				// fix bug:[ID1027665]
				for (ModelN* mo : selectionms())
				{
					if (!needLayoutGroups.contains(mo->getModelGroup()))
					{
						needLayoutGroups.append(mo->getModelGroup());
					}
				}
			}

			for (ModelGroup* mg : needLayoutGroups)
			{
				if (tmpLockedModelGroupIds.contains(mg->getObjectId()))
					continue;

				LayoutItemEx* item = new LayoutItemEx(mg->getObjectId(), m_outlineConcave);
				m_activeObjects.append(item);

				//actives use local outline
				m_activeOutlines.push_back(item->outLine());
			}

			buildNestFixModelGroupInfo();

		}

	}


	void Nest2DJobEx::buildNestFixWipeTowerInfo()
	{
		creative_kernel::LayoutNestFixedItemInfo tmpFixItemInfo;
		int oriPrinterIndex = 0;
		int priorPrinterIndex = 0;

		Q_ASSERT(m_strategy);
		std::vector<int> priorIndexArray = m_strategy->getPriorIndexArray();

		m_strategy->m_hasWipeTowerFlag = false;
		for (int i = 0; i < priorIndexArray.size(); i++)
		{
			priorPrinterIndex = priorIndexArray[i];

			Printer* printerPtr = getPrinterMangager()->getPrinter(priorPrinterIndex);

			if (!printerPtr || printerPtr->lock() || !printerPtr->checkWipeTowerShouldShow())
				continue;

			// fix bug:[ID1027408]开启擦拭塔，此时点击盘上的布局，模型靠近擦拭塔，拖动擦拭塔后再点击布局，模型跟着擦拭塔移动不合理
			creative_kernel::LayoutNestFixedItemInfo tmpFakeFixItemInfo;
			tmpFakeFixItemInfo.fixBinId = i;
			tmpFakeFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(priorPrinterIndex)->globalBox());
			trimesh::vec3 boxCenter = tmpFakeFixItemInfo.fixBinGlobalBox.center();
			std::vector<trimesh::vec3> fakeOutline;
			float centerOffset = 0.02f;
			fakeOutline.push_back(trimesh::vec3(boxCenter.x - centerOffset, boxCenter.y - centerOffset, 0.0f));
			fakeOutline.push_back(trimesh::vec3(boxCenter.x, boxCenter.y + centerOffset, 0.0f));
			fakeOutline.push_back(trimesh::vec3(boxCenter.x + centerOffset, boxCenter.y - centerOffset, 0.0f));
			tmpFakeFixItemInfo.fixOutline = fakeOutline;
			m_fixInfos.push_back(tmpFakeFixItemInfo);

			tmpFixItemInfo.fixOutline = getPrinterWipeTowerOutline(priorPrinterIndex);
			if (tmpFixItemInfo.fixOutline[0] == tmpFixItemInfo.fixOutline[1])
				continue;

			m_strategy->m_hasWipeTowerFlag = true;
			tmpFixItemInfo.fixBinId = i;
			tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(priorPrinterIndex)->globalBox());

			m_fixInfos.push_back(tmpFixItemInfo);
		}

	}

	void Nest2DJobEx::buildNestFixModelGroupInfo()
	{
		creative_kernel::LayoutNestFixedItemInfo tmpFixItemInfo;
		int oriPrinterIndex = 0;
		int priorPrinterIndex = 0;
		int tmpFixBinIndex = 0;

		QList<int> activeGroupIds;
		for (LayoutItemEx* item : m_activeObjects)
		{
			activeGroupIds.append(item->modelGroupId);
		}
		

		Q_ASSERT(m_strategy);
		std::vector<int> priorIndexArray = m_strategy->getPriorIndexArray();

		for (int i = 0; i < priorIndexArray.size(); i++)
		{
			priorPrinterIndex = priorIndexArray[i];

			QList<ModelGroup*> inPrinterGroups = getPrinterModelGroups(priorPrinterIndex);

			tmpFixBinIndex = i;

			for (ModelGroup* aGroup : inPrinterGroups)
			{
				if (activeGroupIds.contains(aGroup->getObjectId()))
				{
					continue;
				}

				LayoutItemEx fixitem(aGroup->getObjectId(), m_outlineConcave);

				//fixs use global outline
				tmpFixItemInfo.fixOutline = fixitem.outLine();
				tmpFixItemInfo.fixBinId = tmpFixBinIndex;
				tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(priorPrinterIndex)->globalBox());

				m_fixInfos.push_back(tmpFixItemInfo);
			}
		}
	}

	void Nest2DJobEx::buildSelectAllNestInfo()
	{
		const QList<ModelGroup*>& inSceneGroups = modelGroups();

		QList<int> tmpLockedGroupIds;
		QList< QPair<int, int> > lockedGroupIds = getAllLockedPrinterModelGroupIds();

		cxkernel::NestFixedItemInfo tmpFixItemInfo;
		for (auto pa : lockedGroupIds)
		{
			tmpLockedGroupIds.append(pa.first);
		}

		for (ModelGroup* aGroup : inSceneGroups)
		{
			if (tmpLockedGroupIds.contains(aGroup->getObjectId()))
			{
				continue;
			}

			LayoutItemEx* item = new LayoutItemEx(aGroup->getObjectId() , m_outlineConcave, this);

			m_activeObjects.append(item);
			m_activeOutlines.push_back(item->outLine());

		}

	}

    // invoke from main thread
    void Nest2DJobEx::successed(qtuser_core::Progressor* progressor)
    {
		if (m_activeObjects.isEmpty())
			return;

		if ( (!m_doExtraFlag) && m_needPrintersCount != printersCount())
		{
			// the first time layout, retrive estimatePrinterSize;
			emit extraLayout(m_needPrintersCount);
			return;
		}

		creative_kernel::LayoutChangeInfo changeInfo;
		changeInfo.printersCount = m_needPrintersCount;

		 Q_ASSERT(m_strategy);
		 auto f = [this, &changeInfo](creative_kernel::LayoutItemEx* item) {
		 	const creative_kernel::LayoutNestResultEx& result = item->nestResult;

		 	ModelGroup* mg = getModelGroupByObjectId(item->modelGroupId);
			Q_ASSERT(mg);
		 	QVector3D lt = mg->localPosition();
		 	trimesh::quaternion q = mg->nestRotation();
			
			trimesh::vec3 modelPosition = result.rt;

		 	QVector3D t(modelPosition.x, modelPosition.y, lt.z());

		 	QVector3D axis(0.0f, 0.0f, 1.0f);
		 	trimesh::quaternion dq = trimesh::quaternion::fromAxisAndAngle(qtuser_3d::qVector3D2Vec3(axis.normalized()), result.rt.z);
		 	trimesh::quaternion rq = dq * q;

			changeInfo.moveModelGroupIds.push_back(item->modelGroupId);

			changeInfo.endPoses.push_back(t);
			changeInfo.endQuaternions.push_back(qtuser_3d::tqua2qqua(rq));

		 };

		 for (LayoutItemEx* item : m_activeObjects)
		 {
		 	f(item);
		 }

		 for (int i = 0; i < m_activeObjects.size(); i++)
		 {
		 	if (m_activeObjects[i])
		 	{
		 		delete m_activeObjects[i];
		 		m_activeObjects[i] = nullptr;
		 	}
		 }

		 emit layoutSuccessed(changeInfo);

     }

}