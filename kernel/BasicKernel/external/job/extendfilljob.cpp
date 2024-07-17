#include "extendfilljob.h"
#include "nest2djobex.h"
#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "interface/machineinterface.h"
#include "interface/selectorinterface.h"
#include "interface/printerinterface.h"
#include "utils/modelpositioninitializer.h"
#include "internal/multi_printer/printermanager.h"
#include "qtuser3d/trimesh2/conv.h"
#include "kernel/kernel.h"
#include "nestplacer/placer.h"
#include "interface/printerinterface.h"

namespace creative_kernel
{
    ExtendFillJob::ExtendFillJob(QObject* parent)
        : cxkernel::Nest2DJobEx(parent)
		, m_fillNum(0)
		, m_extendModel(nullptr)
    {
    }

    ExtendFillJob::~ExtendFillJob()
    {
    }

	void ExtendFillJob::setExtendModel(ModelN* extendModel)
	{
		Q_ASSERT(extendModel);
		m_extendModel = extendModel;
	}

	void ExtendFillJob::setExtendBinIndex(int binIndex)
	{
		Q_ASSERT(binIndex >= 0 && binIndex < printersCount());

		m_curBinIndex = binIndex;

		trimesh::box3 pBox = qtuser_3d::qBox32box3(getPrinter(binIndex)->globalBox());
		setBounding(pBox);

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
		cxkernel::Nest2DJobEx::beforeWork();

		doPrepare();

	}

	void ExtendFillJob::afterWork()
	{
		if (m_results.size() <= 0)
			return;

		const float EPSINON = 0.00001f;

		auto it = std::remove_if(m_results.begin(), m_results.end(), [&](cxkernel::NestResultEx res) { return res.rt.x >= -EPSINON && res.rt.x <= EPSINON && res.rt.y >= -EPSINON && res.rt.y <= EPSINON; });
		m_results.erase(it, m_results.end());

		m_fillNum = m_results.size();

	}

	void ExtendFillJob::doPrepare()
	{
		// process the wipe tower outline in current bin 
		buildFixWipeTowerInfo();

		// process the outline of models in current bin
		buildFixModelInfo();

		LayoutItemEx activeItem(m_extendModel, m_outlineConcave);

		m_activeOutlines.push_back(activeItem.outLine(false, false));

	}


	void ExtendFillJob::buildFixWipeTowerInfo()
	{
		Printer* printerPtr = getPrinter(m_curBinIndex);

		if (!printerPtr || !printerPtr->checkWipeTowerShouldShow())
			return;

		cxkernel::NestFixedItemInfo tmpFixItemInfo;

		tmpFixItemInfo.fixOutline = getPrinterWipeTowerOutline(m_curBinIndex);

		tmpFixItemInfo.fixBinId = m_curBinIndex;
		tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(printerPtr->globalBox());

		m_fixInfos.push_back(tmpFixItemInfo);

	}

	void ExtendFillJob::buildFixModelInfo()
	{
		cxkernel::NestFixedItemInfo tmpFixItemInfo;

		QList<ModelN*> inPrinterModels = getPrinterModels(m_curBinIndex);

		for (ModelN* aModel : inPrinterModels)
		{
			LayoutItemEx fixitem(aModel, m_outlineConcave);

			//fixs use global outline
			tmpFixItemInfo.fixOutline = fixitem.outLine(true, true);
			tmpFixItemInfo.fixBinId = m_curBinIndex;
			tmpFixItemInfo.fixBinGlobalBox = qtuser_3d::qBox32box3(getPrinter(m_curBinIndex)->globalBox());

			m_fixInfos.push_back(tmpFixItemInfo);
		}
	}

    // invoke from main thread
    void ExtendFillJob::successed(qtuser_core::Progressor* progressor)
    {
		Q_ASSERT(m_extendModel);

		int nameIndex = 0;
		QString sname;
		QList<ModelN*> newModels;

		QString objectName = m_extendModel->objectName();
		objectName.chop(4);

		float zPos = m_extendModel->localPosition().z();
		trimesh::quaternion q = m_extendModel->nestRotation();

		for (int i = 0; i < m_fillNum; ++i)
		{
			creative_kernel::ModelN* model = new creative_kernel::ModelN();
			model->setRenderData(m_extendModel->renderData());
			// model->setData(m->data());

			model->copyNestData(m_extendModel);

			nameIndex = i;
			QString name = QString("%1-%2").arg(objectName).arg(nameIndex) + ".stl";
			//---                
			QList<ModelN*> models = modelns();
			for (int k = 0; k < models.size(); ++k)
			{
				sname = models[k]->objectName();
				if (name == sname)
				{
					nameIndex++;
					name = QString("%1-%2").arg(objectName).arg(nameIndex) + ".stl";
					k = -1;
				}
			}
			//---
			model->setObjectName(name);

			newModels << model;

			if (1)
			{

				trimesh::vec3 resPos = m_results[i].rt;

				Printer* printerPtr = getPrinter(m_curBinIndex);
				Q_ASSERT(printerPtr);

				QVector3D qBinPos = printerPtr->position();

				// the position of the first printer
				QVector3D printer0Pos = getPrinter(0)->position();

				// the global box of the first printer
				qtuser_3d::Box3D printer0Box = getPrinter(0)->globalBox();

				// if priorBinIndex IS NOT "0", the layout algothym will return the actual globalBox(the prior Box)
				QVector3D offset = QVector3D(0.0f, 0.0f, 0.0f);
				if (m_curBinIndex == 0 && printer0Box.contains(qtuser_3d::vec2qvector(resPos)))
				{
					offset = qBinPos - printer0Pos;
				}

				QVector3D newCopyPos(resPos.x + offset.x(), resPos.y + offset.y(), zPos);

				QVector3D axis(0.0f, 0.0f, 1.0f);
				trimesh::quaternion dq = trimesh::quaternion::fromAxisAndAngle(qtuser_3d::qVector3D2Vec3(axis.normalized()), resPos.z);
				trimesh::quaternion rq = dq * q;

				model->setLocalPosition(newCopyPos, true);
				model->setLocalScale(m_extendModel->localScale(), true);
				model->setLocalQuaternion(qtuser_3d::tqua2qqua(rq), true);
			}

			model->updateMatrix();
		}

		addModels(newModels, true);

		checkModelRange();

		emit jobSuccessed();

     }

}