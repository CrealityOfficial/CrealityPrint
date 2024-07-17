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

#include "external/kernel/modelnselector.h"
#include "interface/projectinterface.h"
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

	void _modifySpace(const QList<ModelN*>& removes, const QList<ModelN*>& adds) {
		setSelectorEnable(false);
		for (ModelN* model : removes) {
			_removeModel(model);
		}

		for (ModelN* model : adds) {
			_addModel(model);
		}
		setSelectorEnable(true);
		getModelSelect()->notifyTracers();
		
		triggerAutoSave();
		_requestUpdate();
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
		model->getModelEntity()->setEnabled(false);
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
				model->resetNestRotation();

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
			model->dirty();
		}

		_requestUpdate();
	}

	void _setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		if (model->needInit())
			model->SetInitPosition(end);
		model->setLocalPosition(end, update);
		model->dirty();

		if (update)
			_requestUpdate();
	}

	void _setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		model->setLocalQuaternion(end, update);
		model->dirty();

		if (update)
			_requestUpdate();
	}

	void _setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		model->setLocalScale(end, update);
		model->dirty();

		if (update)
			_requestUpdate();
	}

	void _updateModel(ModelN* model)
	{
		if (!model)
			return;

		model->updateMatrix();
		model->dirty();
		selectedPickableInfoChanged(model);
	}

	void _requestUpdate()
	{
		checkModelRange();
		checkBedRange();
		dirtyModelSpace();
		dirtyProjectSpace();
		requestVisUpdate(true);
	}

	void _mirrorModels(const QList<NMirrorStruct>& changes)
	{
		for (const NMirrorStruct& mirrorChange : changes)
		{
			ModelN* model = getModelNBySerialName(mirrorChange.serialName);
			MirrorOperation operation = mirrorChange.operation;
			switch (operation)
			{
			case creative_kernel::MirrorOperation::mo_x:
				_mirrorX(model, false);
				break;
			case creative_kernel::MirrorOperation::mo_y:
				_mirrorY(model, false);
				break;
			case creative_kernel::MirrorOperation::mo_z:
				_mirrorZ(model, false);
				break;
			case creative_kernel::MirrorOperation::mo_reset:
				_mirrorSet(model, mirrorChange.end, false);
				break;
			default:
				break;
			}
		}

		_requestUpdate();
	}

	void _mirrorX(ModelN* model, bool update)
	{
		model->mirrorX();
		model->dirty();

		if (update)
			_requestUpdate();
	}

	void _mirrorY(ModelN* model, bool update)
	{
		model->mirrorY();
		model->dirty();

		if (update)
			_requestUpdate();
	}

	void _mirrorZ(ModelN* model, bool update)
	{
		model->_mirrorZ();
		model->dirty();

		if (update)
			_requestUpdate();
	}

	void _mirrorSet(ModelN* model, const QMatrix4x4& matrix, bool update)
	{
		model->mirrorSet(matrix);
		model->dirty();

		if (update)
			_requestUpdate();
	}

}
