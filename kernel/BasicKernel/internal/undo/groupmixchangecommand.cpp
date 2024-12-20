#include "groupmixchangecommand.h"
#include "internal/data/_modelgroupinterface.h"
#include "interface/spaceinterface.h"

namespace creative_kernel
{
	GroupMixChangeCommand::GroupMixChangeCommand(QObject* parent)
		:QObject(parent)
	{
	}
	
	GroupMixChangeCommand::~GroupMixChangeCommand()
	{
	}

	void GroupMixChangeCommand::undo()
	{
		call(false);
		notifySpaceChange(true);
	}

	void GroupMixChangeCommand::redo()
	{
		call(true);
		notifySpaceChange(true);
	}

	void GroupMixChangeCommand::setChanges(const QList<NUnionChangedStruct>& changes)
	{
		m_changes = changes;
	}

	void GroupMixChangeCommand::call(bool redo)
	{
		_mixUnionsModelGroup(m_changes, redo);
	}
}