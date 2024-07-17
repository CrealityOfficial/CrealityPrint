#include "splitmodeljob.h"

#include "interface/modelinterface.h"
#include "interface/spaceinterface.h"

#include <QtCore/QDebug>

#include "msbase/mesh/merge.h"

#include "interface/visualsceneinterface.h"
#include "qtuser3d/trimesh2/conv.h"

#include "qtusercore/module/progressortracer.h"
#include "interface/uiinterface.h"
#include <QCoreApplication>

#include "kernel/kernelui.h"
namespace creative_kernel
{
	SplitModelJob::SplitModelJob(QObject* parent)
		:Job(parent)
		, m_bSplit(false)
	{
	}

	SplitModelJob::~SplitModelJob()
	{
	}

	void SplitModelJob::EnableSplit(bool bSplit)
	{
		m_bSplit = bSplit;
	}

	QString SplitModelJob::name()
	{
		return "Generate Geometry";
	}

	QString SplitModelJob::description()
	{
		return "Generating Geometry.";
	}

	void SplitModelJob::failed()
	{
	}

	void SplitModelJob::successed(qtuser_core::Progressor* progressor)
	{
		if (m_bSplit && m_datas.size() <= 1)
		{
			getKernelUI()->requestQmlTipDialog(QCoreApplication::translate("AllMenuDialog", "The currently selected model cannot be split"));
			return;
		}
		if (m_datas.size() > 0)
		{
			QList<ModelN*> removes;
			removes << m_models;

			QList<ModelN*> newModels;
			for (cxkernel::ModelNDataPtr data : m_datas)
			{
				ModelN* m = new ModelN();

				m->setData(data);
				m->setLocalPosition(-qtuser_3d::vec2qvector(data->offset));
				m->SetInitPosition(m->localPosition());
				newModels.append(m);
			}

			modifySpace(removes, newModels, true);
			requestVisUpdate(true);
		}
	}

	void SplitModelJob::work(qtuser_core::Progressor* progressor)
	{
		if (m_bSplit)
		{
			SplitModel(progressor);
		}
		else
		{
			MergeModel(progressor);
		}
	}


	void SplitModelJob::MergeModel(qtuser_core::Progressor* progressor)
	{
		if (m_models.size() < 2)
		{
			qDebug() << "merge invalid num.";
			return;
		}

		//for making the name of this model
		ModelN* mFirst = m_models.at(0);

		QString name;
		if (mFirst)
		{
			name = mFirst->objectName();
		}
		name.chop(4);
		name = QString("%1#%2").arg(name).arg(0) + ".stl";

		std::vector<TriMeshPtr> meshes;
		for (ModelN* model : m_models)
			meshes.push_back(TriMeshPtr(model->createGlobalMesh()));
		TriMeshPtr mesh(msbase::mergeMeshes(meshes));

		if (mesh)
		{
			cxkernel::ModelCreateInput input;
			cxkernel::ModelNDataCreateParam param;
			input.mesh = mesh;
			input.name = name;
			cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

			m_datas.push_back(data);
		}
	}

	void SplitModelJob::SplitModel(qtuser_core::Progressor* progressor)
	{
		std::vector<TriMeshPtr> meshes;
		for (int iSubModel = 0; iSubModel < m_models.size(); iSubModel++)
		{
			ModelN* pModel = m_models.at(iSubModel);
			meshes.push_back(TriMeshPtr(pModel->createGlobalMesh()));
		}

		qtuser_core::ProgressorTracer tracer(progressor);
		std::vector<std::vector<TriMeshPtr>> out = msbase::meshSplit(meshes, &tracer);

		if (progressor)
		{
			progressor->progress(1);
		}

		for (int iSubModel = 0; iSubModel < m_models.size(); iSubModel++)
		{
			ModelN* pModel = m_models.at(iSubModel);
			QString name = pModel->objectName();
			const std::vector<TriMeshPtr>& outMeshes = out.at(iSubModel);
			for (int id = 0; id < outMeshes.size(); ++id)
			{
				QString subName = QString("%1-split-parts-%2").arg(name).arg(id);

				cxkernel::ModelCreateInput input;
				cxkernel::ModelNDataCreateParam param;
				input.mesh = outMeshes.at(id);
				input.name = subName;
				cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

				m_datas.push_back(data);
			}
		}
	}

	void SplitModelJob::setModels(QList<ModelN*> model)
	{
		m_models = model;
	}

}
