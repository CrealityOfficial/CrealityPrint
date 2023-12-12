#include "_modelinterface.h"

#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/modelspace.h"
#include "data/modelspaceundo.h"
#include "job/modeldeletejob.h"

#include "kernel/kernel.h"
#include "internal/data/cusModelListModel.h"

#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/modelinterface.h"
#include "cxkernel/interface/jobsinterface.h"

#include "qtusercore/string/resourcesfinder.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/tinymodify.h"


namespace creative_kernel
{
	void _addModel(ModelN* model, bool update)
	{
		getKernel()->listModel()->addModel(model);
		getModelSpace()->addModel(model);

		tracePickable(model);
		if (update)
			_requestUpdate();
	}

	void _modifySpace(const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		for (ModelN* model : adds)
			_addModel(model);
		for (ModelN* model : removes)
			_removeModel(model);
		for (ModelN* model : adds)
			getModelSpace()->uniformName(model);
		_requestUpdate();
	}

	void batchModels(QList<ModelN*>& removes, QList<ModelN*>& adds, bool reversible)
	{
		if (removes.size() == 0 && adds.size() == 0)
			return;

		//_modifySpace(removes, adds);
		modifySpace(removes, adds, reversible);

		if (adds.size() > 0)
		{
			creative_kernel::selectOne(adds.last());
		}
	}

	void _setModelVisible(ModelN* model, bool visible)
	{
		model->setVisibility(visible);
	}

	void _removeModel(ModelN* model, bool update)
	{
		getModelSpace()->removeModel(model);
		getKernel()->listModel()->removeModel(model);

		unTracePickable(model);
		
		ModelDeleteJob* job = new ModelDeleteJob();
		job->setModel(model);
		cxkernel::executeJob(job);

		if (update)
			_requestUpdate();
	}

	void _mixUnions(const QList<NUnionChangedStruct>& structs, bool redo)
	{
		for (const NUnionChangedStruct& changeStruct : structs)
		{
			ModelN* model = getModelNBySerialName(changeStruct.serialName);
			if (changeStruct.rotateActive)
			{
				model->updateDisplayRotation(redo);

				QQuaternion q = redo ? changeStruct.rotateChange.end : changeStruct.rotateChange.start;
				_setModelRotation(model, q, false);
			}

			if (changeStruct.scaleActive)
			{
				QVector3D scale = redo ? changeStruct.scaleChange.end : changeStruct.scaleChange.start;
				_setModelScale(model, scale, false);
			}

			if (changeStruct.posActive)
			{
				QVector3D pos = redo ? changeStruct.posChange.end : changeStruct.posChange.start;
				_setModelPosition(model, pos, false);
			}

			_updateModel(model);
		}

		_requestUpdate();
	}

	void _setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		model->setLocalPosition(end, update);

		if (update)
			_requestUpdate();
	}

	void _setModelsInitPosition(const QList<ModelN*>& models, const QList<QVector3D>& tEnds)
	{
		for (int i = 0, count = models.count(); i < count; ++i)
		{
			auto m = models[i];
			auto end = tEnds[i];
			if (m->needInit())
				m->SetInitPosition(end);
		}
	}

	void _setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		model->setLocalQuaternion(end, update);

		if (update)
			_requestUpdate();
	}

	void _setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		model->setLocalScale(end, update);

		if (update)
			_requestUpdate();
	}

	void _updateModel(ModelN* model)
	{
		if (!model)
			return;

		model->updateMatrix();
		selectedPickableInfoChanged(model);
	}

	void _requestUpdate()
	{
		checkModelRange();
		checkBedRange();
		dirtyModelSpace();
		requestVisUpdate(true);
	}

	void _mirrorModels(const QList<NMirrorStruct>& changes)
	{
		for (const NMirrorStruct& mirrorChange : changes)
		{
			ModelN* model = getModelNBySerialName(mirrorChange.serialName);
			MirrorOperation operation = mirrorChange.operation;

			_batchMirrorModel(model, mirrorChange.operation);
		}

		_requestUpdate();
	}

	void _batchMirrorModel(ModelN* model, const MirrorOperation& operation)
	{
		if (!model)
			return;

		if (!model->modelNData())
		{
			qDebug("mirrored model null data.");
			return;
		}

		auto f = [](const creative_kernel::MirrorOperation& operation)->trimesh::fxform {

			trimesh::fxform xf;
			QMatrix4x4 m;
			m.setToIdentity();

			switch (operation)
			{
			case creative_kernel::MirrorOperation::mo_x:
				m(0, 0) = -1;
				break;
			case creative_kernel::MirrorOperation::mo_y:
				m(1, 1) = -1;
				break;
			case creative_kernel::MirrorOperation::mo_z:
				m(2, 2) = -1;
				break;
			default:
				break;

			}

			xf = qtuser_3d::qMatrix2Xform(m);
			return xf;
		};

		TriMeshPtr mesh(new trimesh::TriMesh());
		*mesh = *(model->modelNData()->mesh);

		trimesh::apply_xform(mesh.get(), trimesh::xform(f(operation)));

		msbase::reverseTriMesh(mesh.get());

		cxkernel::ModelNDataPtr newMeshdata;
		if (mesh)
		{
			mesh->need_bbox();

			cxkernel::ModelCreateInput input;
			input.mesh = mesh;
			input.name = model->objectName();

			cxkernel::ModelNDataCreateParam param;
			param.toCenter = true;

			newMeshdata = cxkernel::createModelNData(input, nullptr, param);
		}

		if (newMeshdata)
		{
			ModelN* mirrorModel = nullptr;

			QList<creative_kernel::ModelN*> models;
			models.push_back(model);

			mirrorModel = creative_kernel::createModelFromData(newMeshdata);
			mirrorModel->setLocalData(qtuser_3d::qVector3D2Vec3(model->localPosition()), model->localQuaternion(), qtuser_3d::qVector3D2Vec3(model->localScale()));

			//if (!mirrorModel->hasSupport()) {
			//	mirrorModel->setLowerZPosition(0.0f);
			//}

			QList<creative_kernel::ModelN*> newModels;
			newModels.push_back(mirrorModel);
			batchModels(models, newModels, true);
		}
	}

	void _mirrorX(ModelN* model, bool update)
	{
		model->mirrorX();
	
		if (update)
			_requestUpdate();
	}

	void _mirrorY(ModelN* model, bool update)
	{
		model->mirrorY();
	
		if (update)
			_requestUpdate();
	}
	
	void _mirrorZ(ModelN* model, bool update)
	{
		model->_mirrorZ();
	
		if (update)
			_requestUpdate();
	}
	
	void _mirrorSet(ModelN* model, const QMatrix4x4& matrix, bool update)
	{
		model->mirrorSet(matrix);
	
		if (update)
			_requestUpdate();
	}

}
