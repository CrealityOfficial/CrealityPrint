#include "redocommand.h"

#include "cxkernel/interface/undointerface.h"

RedoCommand::RedoCommand(QObject* parent)
	:ToolCommand(parent)
{
     orderindex=3;
	m_name = "Redo";
	m_enabledIcon = "qrc:/kernel/images/redo.png";
	m_pressedIcon = "qrc:/kernel/images/redo_d.png";
}

RedoCommand::~RedoCommand()
{
}

void RedoCommand::execute()
{
	cxkernel::redo();
}
