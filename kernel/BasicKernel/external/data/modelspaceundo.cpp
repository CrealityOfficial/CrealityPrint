#include "modelspaceundo.h"
#include "internal/undo/nmixchangecommand.h"
#include "internal/undo/modelpropertycommand.h"
#include "internal/undo/nmixchangecommand.h"
#include "internal/undo/printerschangecommand.h"
#include "internal/undo/modelserialcommand.h"
#include "internal/undo/groupmixchangecommand.h"
#include "internal/undo/layoutmodelsmovecommand.h"
#include "internal/undo/nodechangecommand.h"
#include "internal/undo/wipetowermovecommand.h"

namespace creative_kernel
{
	ModelSpaceUndo::ModelSpaceUndo(QObject* parent)
		:QUndoStack(parent)
	{
	}
	
	ModelSpaceUndo::~ModelSpaceUndo()
	{
	}

	void ModelSpaceUndo::insertPrinter(int index, Printer* printer)
	{
		PrintersChangeCommand* command = new PrintersChangeCommand(PrinterCmdOpType::INSERT_PRINTER, index, printer);

		push(command);
	}

	void ModelSpaceUndo::removePrinter(int index, Printer* printer)
	{
		//PrintersChangeCommand* command = new PrintersChangeCommand();
		//command->removePrinter(index, printer);

		//push(command);

		PrintersChangeCommand* command = new PrintersChangeCommand(PrinterCmdOpType::REMOVE_PRINTER, index, printer);

		push(command);
	}

	void ModelSpaceUndo::changeMaterials(const QList<ModelN*>& models, int materialIndex)
	{
		QList<ModelNPropertyMeshUndo> propertyList;
		for (ModelN* model : models)
		{
			ModelNPropertyMeshUndo undo;
			undo.model_id = model->getObjectId();
			undo.start_name = model->name();
			undo.end_name = model->name();
			SharedDataID dataID = model->sharedDataID();
			undo.start_data_id = dataID;
			dataID(MaterialID) = materialIndex;
			undo.end_data_id = dataID;

			propertyList << undo;
		}

		ModelNPropertyMeshCommand* command = new ModelNPropertyMeshCommand(propertyList);
		push(command);
	}

	void ModelSpaceUndo::mix(const QList<NUnionChangedStruct>& mixChange)
	{
		NMixChangeCommand* command = new NMixChangeCommand();
		command->setChanges(mixChange);

		push(command);
	}

	void ModelSpaceUndo::push(QUndoCommand* cmd)
	{
		QUndoStack::push(cmd);
	}

	void ModelSpaceUndo::mixGroup(const QList<NUnionChangedStruct>& mixChange)
	{
		GroupMixChangeCommand* command = new GroupMixChangeCommand();
		command->setChanges(mixChange);

		push(command);
	}

	void ModelSpaceUndo::modifySpace(const SceneCreateData& scene_data)
	{
		SpaceSerialCommand* command = new SpaceSerialCommand(scene_data);
		push(command);
	}

	void ModelSpaceUndo::modifyModelsMeshProperty(const QList<ModelNPropertyMeshUndo>& changes, bool reversible)
	{
		if (changes.size() == 0)
			return;

		ModelNPropertyMeshCommand* command = new ModelNPropertyMeshCommand(changes);
		if(reversible)
			push(command);
		else {
			command->redo();
			command->deleteLater();
		}
	}

	void ModelSpaceUndo::modifyNodes(const QList<NodeChange>& changes)
	{
		if (changes.size() == 0)
			return;

		NodeChangeCommand* command = new NodeChangeCommand(changes);
		push(command);
	}

	void ModelSpaceUndo::modifyWipeTower(const WipeTowerChange& data)
	{
		if (data.index < 0)
		{
			return;
		}
		WipeTowerMoveCommand* command = new WipeTowerMoveCommand(data);
		push(command);
	}

	void ModelSpaceUndo::layoutChangeScene(const LayoutChangeInfo& changeInfo)
	{
		LayoutModelsMoveCommand* command = new LayoutModelsMoveCommand(changeInfo);
		push(command);
	}

	void ModelSpaceUndo::executeCommand(QUndoCommand* command)
	{
		if (!command)
			return;

		push(command);
	}
}