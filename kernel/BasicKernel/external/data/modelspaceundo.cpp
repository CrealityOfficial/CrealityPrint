#include "modelspaceundo.h"
#include "internal/undo/spacemodifycommand.h"
#include "internal/undo/nmixchangecommand.h"
#include "internal/undo/mirrorundocommand.h"
#include "internal/undo/nmixchangecommand.h"
#include "internal/undo/printerschangecommand.h"
#include "internal/undo/layoutmodelsmovecommand.h"

namespace creative_kernel
{
	ModelSpaceUndo::ModelSpaceUndo(QObject* parent)
		:QUndoStack(parent)
	{
        setUndoLimit(5);
	}
	
	ModelSpaceUndo::~ModelSpaceUndo()
	{
	}

	void ModelSpaceUndo::insertPrinter(int index, Printer* printer)
	{
		//SpaceModifyCommand* command = new SpaceModifyCommand();
		//command->insertPrinter(index, printer);

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

	void ModelSpaceUndo::modifySpace(const QList<ModelN*>& models, const QList<ModelN*>& removeModels)
	{
		SpaceModifyCommand* command = new SpaceModifyCommand();
		command->setModels(models);
		command->setRemoveModels(removeModels);

		push(command);
	}

	void ModelSpaceUndo::mix(const QList<NUnionChangedStruct>& mixChange)
	{
		NMixChangeCommand* command = new NMixChangeCommand();
		command->setChanges(mixChange);

		push(command);
	}

	void ModelSpaceUndo::mirror(const QList<ModelN*>& models, int mode)
	{
		MirrorUndoCommand* command = new MirrorUndoCommand();
		command->setModels(models);
		command->setMirrorMode(mode);

		push(command);
	}
	void ModelSpaceUndo::push(QUndoCommand* cmd)
	{
		QUndoStack::push(cmd);
		m_activeTime = time(nullptr);
	}

	void ModelSpaceUndo::layoutChangeScene(const LayoutChangeInfo& changeInfo)
	{
		LayoutModelsMoveCommand* command = new LayoutModelsMoveCommand;
		command->setLayoutChangeInfo(changeInfo);
		push(command);
	}
}