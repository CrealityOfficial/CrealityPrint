#include "modelspaceundo.h"
#include "internal/undo/spacemodifycommand.h"
#include "internal/undo/nmixchangecommand.h"
#include "internal/undo/mirrorundocommand.h"
#include "internal/undo/nmixchangecommand.h"

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

	void ModelSpaceUndo::replaceModels(const QList<MeshChange>& meshChanges)
	{
		int count = meshChanges.size();
		if (count == 0)
			return;

		MeshChangeCommand* command = new MeshChangeCommand();
		command->setChanges(meshChanges);

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

	void ModelSpaceUndo::mirror(const QList<NMirrorStruct>& mirrors)
	{
		MirrorUndoCommand* command = new MirrorUndoCommand();
		command->setMirrors(mirrors);

		push(command);
	}
	void ModelSpaceUndo::push(QUndoCommand* cmd)
	{
		QUndoStack::push(cmd);
		m_activeTime = time(nullptr);
	}
}