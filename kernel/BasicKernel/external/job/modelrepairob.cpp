#include "trimesh2/TriMesh_algo.h"
#include "trimesh2/XForm.h"
#include <stack>
#include "qtuser3d/math/const.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/tinymodify.h"
#include "modelrepairob.h"
#include "crslice2/repair.h"
#include "qtusercore/module/progressortracer.h"
#include "interface/shareddatainterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
	ModelRepairWinJob::ModelRepairWinJob(QObject* parent)
		: Job(parent)
	{
	}

	ModelRepairWinJob::~ModelRepairWinJob()
	{
	}

	QString ModelRepairWinJob::name()
	{
		return "repairt";
	}

	QString ModelRepairWinJob::description()
	{
		return "324234";
	}

	void ModelRepairWinJob::setParts(const QList<creative_kernel::ModelN*>& models, creative_kernel::ModelGroup* modelGroup)
	{

	}

	void ModelRepairWinJob::setModelGroups(const QList<creative_kernel::ModelGroup*>& modelgroup)
	{

	}

	void ModelRepairWinJob::work(qtuser_core::Progressor* progressor)
	{
		QList<creative_kernel::ModelN*> models = selectionms();
		crslice2::CheckInfo info;
		for (creative_kernel::ModelN* model : models)
		{
			TriMeshPtr input(model->globalMesh());
			trimesh::TriMesh* mesh = input.get();
			#ifdef Q_OS_WINDOWS
			// info = crslice2::checkMesh(mesh);

			// if (!info.warning_icon_name.empty())
			 {
				qtuser_core::ProgressorTracer tracer{ progressor };
				trimesh::TriMesh* mesh = model->mesh();
				QString name = model->name() + "    " + tr("Model is being repaired") + "...";
				progressor->message(name);
				
				trimesh::TriMesh* tm  = crslice2::repairMesh(mesh, &tracer);
				if (tm)
				{
					TriMeshPtr meshptr(tm);
					cxkernel::MeshData* meshData = new cxkernel::MeshData(meshptr, false);
					cxkernel::MeshDataPtr meshDataPtr(meshData);
					m_meshDatas.append(meshDataPtr);
				}
				
			 }
			#endif
		}
	}

	void ModelRepairWinJob::failed()
	{

	}

	void ModelRepairWinJob::successed(qtuser_core::Progressor* progressor)
	{
		QList<creative_kernel::ModelNPropertyMeshChange> changes;
		QList<creative_kernel::ModelN*> models = selectionms();
		//Q_ASSERT(m_meshDatas.size() == models.size());

		for (int i = 0; i < m_meshDatas.size(); i++)
		{
			creative_kernel::ModelNPropertyMeshChange change;
			change.model = models[i];
			change.name = QStringLiteral("%1").arg(change.model->name());
			change.mesh = m_meshDatas[i];
			changes.append(change);
		}

		creative_kernel::replaceModelNMeshes(changes, true);

		emit repairSuccessed();
	}
}
