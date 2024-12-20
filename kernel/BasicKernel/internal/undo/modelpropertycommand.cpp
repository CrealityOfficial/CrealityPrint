#include "modelpropertycommand.h"
#include "interface/spaceinterface.h"
#include "data/shareddatamanager/shareddatatracer.h"

namespace creative_kernel
{
	ModelNPropertyMeshCommand::ModelNPropertyMeshCommand(const QList<ModelNPropertyMeshUndo>& changes, QObject* parent)
		:QObject(parent)
		, m_changes(changes)
	{
		for (ModelNPropertyMeshUndo undo : changes)
		{
			SharedDataTracer* tracer = new SharedDataTracer;
			tracer->setSharedDataID(undo.start_data_id);
			m_dataSustainers << tracer;

			tracer = new SharedDataTracer;
			tracer->setSharedDataID(undo.end_data_id);
			m_dataSustainers << tracer;
		}
	}

	ModelNPropertyMeshCommand::~ModelNPropertyMeshCommand()
	{
		qDeleteAll(m_dataSustainers);
	}

	void ModelNPropertyMeshCommand::undo()
	{
		ModelSpace* space = getModelSpace();
		for (ModelNPropertyMeshUndo& mesh_undo : m_changes)
		{
			ModelN* model = space->getModelNByObjectId(mesh_undo.model_id);
			if (model)
			{
				model->setName(mesh_undo.start_name);
				model->setSharedData(mesh_undo.start_data_id);
			}
		}

		space->notifySpaceChange(true);
	}

	void ModelNPropertyMeshCommand::redo()
	{
		ModelSpace* space = getModelSpace();
		for (ModelNPropertyMeshUndo& mesh_undo : m_changes)
		{
			ModelN* model = space->getModelNByObjectId(mesh_undo.model_id);
			if (model)
			{
				model->setName(mesh_undo.end_name);
				model->setSharedData(mesh_undo.end_data_id);
			}
		}

		space->notifySpaceChange(true);
	}
}