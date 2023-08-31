#include "nmixchangecommand.h"
#include "internal/data/_modelinterface.h"

namespace creative_kernel
{
	NMixChangeCommand::NMixChangeCommand(QObject* parent)
		:QObject(parent)
	{
	}
	
	NMixChangeCommand::~NMixChangeCommand()
	{
	}

	void NMixChangeCommand::undo()
	{
		call(false);
	}

	void NMixChangeCommand::redo()
	{
		call(true);
	}

	void NMixChangeCommand::setChanges(const QList<NUnionChangedStruct>& changes)
	{
		m_changes = changes;
	}

	void NMixChangeCommand::call(bool redo)
	{
		_mixUnions(m_changes, redo);
	}

	MeshChangeCommand::MeshChangeCommand(QObject* parent)
		:QObject(parent)
	{
	}

	MeshChangeCommand::~MeshChangeCommand()
	{
	}

	void MeshChangeCommand::undo()
	{
		call(false);
	}

	void MeshChangeCommand::redo()
	{
		call(true);
	}

	void MeshChangeCommand::setChanges(const QList<MeshChange>& changes)
	{
		m_changes = changes;
	}

	void MeshChangeCommand::call(bool _redo)
	{
		QList<MeshChange> changes = m_changes;
		if (!_redo)
		{
			for (MeshChange& change : changes)
			{
				TriMeshPtr temp = change.start;
				change.start = change.end;
				change.end = temp;
				QString tn = change.startName;
				change.startName = change.endName;
				change.endName = tn;
			}
		}

		_replaceModelsMesh(changes);
	}
}