#ifndef _NULLSPACE_REDOCOMMAND_1589703676663_H
#define _NULLSPACE_REDOCOMMAND_1589703676663_H
#include "qtusercore/plugin/toolcommand.h"

class RedoCommand : public ToolCommand
{
	Q_OBJECT
public:
	RedoCommand(QObject* parent = nullptr);
	virtual ~RedoCommand();

	Q_INVOKABLE void execute();
protected:
};
#endif // _NULLSPACE_REDOCOMMAND_1589703676663_H
