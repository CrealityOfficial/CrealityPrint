#ifndef _NULLSPACE_UNDOCOMMAND_1589703676681_H
#define _NULLSPACE_UNDOCOMMAND_1589703676681_H
#include "qtusercore/plugin/toolcommand.h"

class UndoCommand : public ToolCommand
{
	Q_OBJECT
public:
	UndoCommand(QObject* parent = nullptr);
	virtual ~UndoCommand();

	Q_INVOKABLE void execute();
};
#endif // _NULLSPACE_UNDOCOMMAND_1589703676681_H
