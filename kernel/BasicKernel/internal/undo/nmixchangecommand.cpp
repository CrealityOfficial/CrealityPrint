#include "nmixchangecommand.h"
#include "interface/spaceinterface.h"

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
		getModelSpace()->_mixUnions(m_changes, redo);
	}
}