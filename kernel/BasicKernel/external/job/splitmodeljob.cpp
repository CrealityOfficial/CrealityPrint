#include "splitmodeljob.h"

#include <QSettings>

#include "data/modeln.h"

#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"

#include "utils/modelpositioninitializer.h"
#include "utils/convexhullcalculator.h"

#include <QtCore/QDebug>

#include "interface/spaceinterface.h"
#include "data/modeln.h"
#include "data/modelgroup.h"

#include "mmesh/trimesh/meshtopo.h"
#include "mmesh/trimesh/trimeshutil.h"

#include "interface/visualsceneinterface.h"
#include "qcxutil/trimesh2/conv.h"

#include "qtusercore/module/progressortracer.h"

using namespace creative_kernel;


SplitModelJob::SplitModelJob(QObject* parent)
	:Job(parent)
	, m_bSplit(false)
	, m_pnewMesh(NULL)
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
	if (m_bSplit)
	{
		AddSplitModel(progressor);
	}
	else
	{
		if (m_pnewMesh)
		{
			if (m_models.size() > 0)
			{
				QList<ModelN*> nlist;
				modifySpace(m_models, nlist, true);
			}

			TriMeshPtr mesh(m_pnewMesh);
			creative_kernel::ModelN* pNewModel = new creative_kernel::ModelN();
			if (pNewModel)
			{
				QString name = QString("%1#%2").arg(m_SplitModelInfor.objectName).arg(0) + ".stl";
				pNewModel->setObjectName(name);

				cxkernel::ModelCreateInput input;
				input.mesh = mesh;
				input.name = name;
				cxkernel::ModelNDataCreateParam param;
				param.toCenter = false;

				cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

				pNewModel->setData(data);
				pNewModel->setLocalPosition(m_SplitModelInfor.localPos);
				pNewModel->SetInitPosition(pNewModel->center());

				addModel(pNewModel, true);
			}
		}

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
	m_pnewMesh = NULL;
	m_models.clear();
	m_models = selectionms();
	if (m_models.size() < 2)
	{
		qDebug() << "merge invalid num.";
		return;
	}

	//for making the name of this model
	ModelN* mFirst = m_models.at(0);

	if (mFirst)
	{
		m_SplitModelInfor.objectName = mFirst->objectName();
	}
	m_SplitModelInfor.objectName.chop(4);

	//create the new triangle mesh data and it will store all the submodels triangle mesh data into this triangle mesh data
	
	m_pnewMesh = TriMeshPointer(new trimesh::TriMesh());

	if (m_pnewMesh)
	{
		std::vector<TriMeshPointer> vtTriMesh;
		//merging all the sub models triangle mesh data together.
		for (size_t i = 0; i < m_models.size(); i++)
		{
			ModelN* pCurModel = m_models.at(i);

			if (pCurModel && m_pnewMesh)
			{
				QMatrix4x4 globalMatrix = pCurModel->globalMatrix();

				trimesh::fxform gxf = qcxutil::qMatrix2Xform(globalMatrix);
				if (pCurModel->mesh())
				{
					vtTriMesh.push_back(pCurModel->meshptr());

					//mmesh::mergeTriMesh(m_pnewMesh, vtTriMesh, gxf, pCurModel->isFanZhuan());
					qtuser_core::ProgressorTracer tracer(progressor);
					mmesh::meshMerge(m_pnewMesh,vtTriMesh, gxf, pCurModel->isFanZhuan(), &tracer);
					vtTriMesh.clear();
				}
			}
		}

		m_pnewMesh->need_bbox();
		ConvexHullCalculator::calculate(m_pnewMesh.get(), nullptr);
	}
}


void SplitModelJob::AddSplitModel(qtuser_core::Progressor* progressor)
{
	for (int iSubModels = 0; iSubModels < m_meshes.size(); iSubModels++)
	{
		ModelN* pModel = m_models[iSubModels];
		if (NULL == pModel)
		{
			continue;
		}

		creative_kernel::ModelGroup* group = qobject_cast<creative_kernel::ModelGroup*>(pModel->parent());
		if (m_meshes.size() > 0 && group)
		{
			QList<ModelN*> newModels;
			QString name = pModel->objectName();
			int id = 0;

			std::vector<TriMeshPointer> subMeshs = m_meshes[iSubModels];
			for (auto mesh : subMeshs)
			{
				if (mesh->vertices.size() == 0 || mesh->faces.size() == 0)
				{
					//delete mesh;
					continue;
				}
				++id;

				creative_kernel::ModelN* m = new creative_kernel::ModelN();
				QString subName = QString("%1-split-parts-%2").arg(name).arg(id);
				m->setObjectName(subName);

				trimesh::fxform xf = qcxutil::qMatrix2Xform(pModel->globalMatrix());
				size_t vertexCount = mesh->vertices.size();
				for (size_t i = 0; i < vertexCount; ++i)
				{
					trimesh::vec3 v = mesh->vertices.at(i);
					mesh->vertices.at(i) = xf * v;
				}

				mesh->need_bbox();
				QVector3D localPos = qcxutil::vec2qvector(mmesh::moveToOriginal(mesh.get()));

				TriMeshPtr ptr(mesh);
				cxkernel::ModelCreateInput input;
				input.mesh = ptr;
				input.name = subName;

				cxkernel::ModelNDataCreateParam param;
				param.toCenter = false;
				cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input, nullptr, param));

				m->setData(data);
				m->setLocalPosition(localPos);
				m->SetInitPosition(localPos);
				newModels.push_back(m);
			}
			m_meshes.clear();

			if (newModels.size() > 0)
			{
				QList<ModelN*> removes;
				removes << pModel;

				modifySpace(removes, newModels, true);
			}

			requestVisUpdate(true);
		}
	}

}

void SplitModelJob::SplitModel(qtuser_core::Progressor* progressor)
{
	std::vector<TriMeshPointer> meshes;
	for (int iSubModel = 0; iSubModel < m_models.size(); iSubModel++)
	{
		ModelN* pModel = m_models.at(iSubModel);
		if (!pModel)
		{
			continue;
		}
		meshes.push_back(pModel->meshptr());
	}

	qtuser_core::ProgressorTracer tracer(progressor);
	m_meshes = mmesh::meshSplit(meshes, &tracer);

	if (progressor)
	{
		progressor->progress(1);
	}

}

void SplitModelJob::setModel(QList<creative_kernel::ModelN*> model)
{
	m_models = model;
}

