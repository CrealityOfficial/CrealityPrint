#include "undocommand.h"

#include "cxkernel/interface/undointerface.h"

UndoCommand::UndoCommand(QObject* parent)
	:ToolCommand(parent)
{
     orderindex=2;
	m_name = "Undo";
	m_enabledIcon = "qrc:/kernel/images/undo.png";
	m_pressedIcon = "qrc:/kernel/images/undo_d.png";
}

UndoCommand::~UndoCommand()
{
}

void UndoCommand::execute()
{
	cxkernel::undo();
}

