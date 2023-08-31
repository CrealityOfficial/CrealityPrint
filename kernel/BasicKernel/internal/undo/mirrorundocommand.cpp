#include "mirrorundocommand.h"
#include "internal/data/_modelinterface.h"

namespace creative_kernel
{
	MirrorUndoCommand::MirrorUndoCommand(QObject* parent)
		:QObject(parent)
	{
	}
	
	MirrorUndoCommand::~MirrorUndoCommand()
	{
	}

	void MirrorUndoCommand::setMirrors(const QList<NMirrorStruct>& mirrorStructs)
	{
		m_mirrors = mirrorStructs;
	}

	void MirrorUndoCommand::undo()
	{
		call(false);
	}

	void MirrorUndoCommand::redo()
	{
		call(true);
	}

	void MirrorUndoCommand::call(bool _redo)
	{
		QList<NMirrorStruct> changes = m_mirrors;
		if (!_redo)
		{
			for (NMirrorStruct& change : changes)
			{
				QMatrix4x4 temp = change.start;
				change.start = change.end;
				change.end = temp;
			}
		}

		_mirrorModels(changes);
	}
}