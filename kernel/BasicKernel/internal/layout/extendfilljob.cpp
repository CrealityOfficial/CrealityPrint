#include "extendfilljob.h"
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/machineinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "qtuser3d/trimesh2/conv.h"
#include "kernel/kernel.h"
#include "nestplacer/placer.h"
#include "interface/printerinterface.h"
#include "qtusercore/module/progressortracer.h"
#include "cxkernel/interface/constinterface.h"
#include "cxkernel/interface/cacheinterface.h"
#include "cxkernel/wrapper/placeitem.h"
#include "internal/layout/layoutitemex.h"
#include <QtCore/QDateTime>

namespace creative_kernel
{
    ExtendFillJob::ExtendFillJob(QObject* parent)
		: Job(parent)
		, m_fillNum(0)
		, m_extendModelGroupId(-1)
    {
    }

    ExtendFillJob::~ExtendFillJob()
    {
    }

	void ExtendFillJob::setLayoutParamInfo(const LayoutParameterInfo& paramInfo)
	{
		Q_ASSERT(paramInfo.needLayoutGroupIds.size() == 1);
		m_extendModelGroupId = paramInfo.needLayoutGroupIds[0];

		Q_ASSERT(paramInfo.priorBinIndex >= 0 && paramInfo.priorBinIndex < printersCount());
		m_curBinIndex = paramInfo.priorBinIndex;
		trimesh::box3 pBox = qtuser_3d::qBox32box3(getPrinter(m_curBinIndex)->globalBox());
		setBounding(pBox);

		m_modelspacing = paramInfo.modelSpacing;
		m_platformSpacing = paramInfo.platfromSpacing;
		m_angle = paramInfo.angle;
		m_alignMove = paramInfo.alignMove;
		m_outlineConcave = paramInfo.outlineConcave;

	}

	void ExtendFillJob::setBounding(const trimesh::box3& box)
	{
		m_box = box;
		m_box.valid = true;
	}

	void ExtendFillJob::work(qtuser_core::Progressor* progressor)
	{
		beforeWork();

		qtuser_core::ProgressorTracer tracer(progressor);

		doExtendFill(tracer);

		afterWork();
	}

	void ExtendFillJob::doExtendFill(ccglobal::Tracer& tracer)
	{
		nestplacer::PlacerParameter parameter;
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

		Q_ASSERT(actives.size() == 1);
		extendFill(fixed, actives[0], parameter, results);


#ifdef DEBUG
		qint64 t1_1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
		qInfo() << "==== place Time use " << (t1_1 - t1) << " ms";
#endif // DEBUG

		for (int i = 0; i < results.size(); i++)
		{
			LayoutNestResultEx aResult;
			aResult.binIndex = results[i].binIndex;
			aResult.rt = results[i].rt;
			m_results.push_back(aResult);
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

    void ExtendFillJob::prepare()
    {

    }

    void ExtendFillJob::failed()
    {
		qWarning() << Q_FUNC_INFO;
    }

	void ExtendFillJob::beforeWork()
	{
		doPrepare();
	}

	void ExtendFillJob::afterWork()
	{
		if (m_results.size() <= 0)
			return;

		m_fillNum = m_results.size();

	}

	void ExtendFillJob::doPrepare()
	{
		// process the wipe tower outline in current bin 
		buildFixWipeTowerInfo();

		// process the outline of models in current bin
		buildFixModelInfo();

		LayoutItemEx activeItem(m_extendModelGroupId, m_outlineConcave);

		m_activeOutlines.push_back(activeItem.outLine());

	}


	void ExtendFillJob::buildFixWipeTowerInfo()
	{
		Printer* printerPtr = getPrinter(m_curBinIndex);

		if (!printerPtr || !printerPtr->checkWipeTowerShouldShow())
			return;

		creative_kernel::LayoutNestFixedItemInfo tmpFixItemInfo;

		tmpFixItemInfo.fixOutline = getPrinterWipeTowerOutline(m_curBinIndex);

		tmpFixItemInfo.fixBinId = m_curBinIndex;
		tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(printerPtr->globalBox());

		m_fixInfos.push_back(tmpFixItemInfo);

	}

	void ExtendFillJob::buildFixModelInfo()
	{
		creative_kernel::LayoutNestFixedItemInfo tmpFixItemInfo;

		QList<int> inPrinterGroupIds = getPrinterModelGroupIds(m_curBinIndex);

		for (int gid : inPrinterGroupIds)
		{
			LayoutItemEx fixitem(gid, m_outlineConcave);

			//fixs use global outline
			tmpFixItemInfo.fixOutline = fixitem.outLine();
			tmpFixItemInfo.fixBinId = m_curBinIndex;
			tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(m_curBinIndex)->globalBox());

			m_fixInfos.push_back(tmpFixItemInfo);
		}
	}

	QString ExtendFillJob::getExtendGroupName(const QString& exGroupName, int nameIdx)
	{
		// fix bug : [ID1027766] �Ҽ��˵�ִ��������ӡ������������б��е�ģ�Ͳ�Ӧ���б��
		return QString("%1").arg(exGroupName);
	}

    // invoke from main thread
    void ExtendFillJob::successed(qtuser_core::Progressor* progressor)
    {
		int nameIndex = 0;
		QString sname;
		QList<ModelGroup*> newModelGroups;

		ModelGroup* extendGroup = getModelGroupByObjectId(m_extendModelGroupId);
		Q_ASSERT(extendGroup);
		QString objectName = extendGroup->groupName();

		objectName.chop(4);

		float zPos = extendGroup->localPosition().z();
		trimesh::quaternion q = extendGroup->nestRotation();

		creative_kernel::SceneCreateData scene_create_data;
		QList<GroupCreateData> extend_group_datas;

		QMatrix4x4 tmpMatrix;
		for (int i = 0; i < m_fillNum; ++i)
		{
			GroupCreateData one_extend_group_data;
			one_extend_group_data.reset();
			one_extend_group_data.name = getExtendGroupName(extendGroup->groupName(), i);
			one_extend_group_data.outlinePath = extendGroup->rsPath();  // cache the group outlinePath for speed
			one_extend_group_data.outlinePathInitPos = extendGroup->getNodeOffset();

			if (1)
			{

				trimesh::vec3 resPos = m_results[i].rt;

				QVector3D copyOffset(resPos.x, resPos.y, zPos);

				QVector3D axis(0.0f, 0.0f, 1.0f);
				trimesh::quaternion dq = trimesh::quaternion::fromAxisAndAngle(qtuser_3d::qVector3D2Vec3(axis.normalized()), resPos.z);
				trimesh::quaternion rq = dq * q;

				one_extend_group_data.xf = trimesh::xform::trans(copyOffset) * extendGroup->getMatrix();
			}

			for (int k = 0; k < extendGroup->models().size(); k++)
			{
				ModelN* model = extendGroup->models().at(k);
				if (model)
				{
					ModelCreateData model_create_data;
					model_create_data.model_id = -1;
					model_create_data.fixed_id = model->getFixedId();
					model_create_data.dataID = model->sharedDataID();
					model_create_data.name = model->name();

					model_create_data.xf = model->getMatrix();

					one_extend_group_data.models.append(model_create_data);
				}
			}

			extend_group_datas.append(one_extend_group_data);
		}

		scene_create_data.add_groups = extend_group_datas;
		modifyScene(scene_create_data);

		emit jobSuccessed();

     }

}