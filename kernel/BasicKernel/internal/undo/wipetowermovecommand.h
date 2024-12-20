#ifndef CREATIVE_KERNEL_WIPE_TOWER_MOVE_COMMAND_202407102005_H
#define CREATIVE_KERNEL_WIPE_TOWER_MOVE_COMMAND_202407102005_H
#include <QtWidgets/QUndoCommand>
#include "data/undochange.h"

namespace creative_kernel
{
	class WipeTowerMoveCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		WipeTowerMoveCommand(const WipeTowerChange& change);
		virtual ~WipeTowerMoveCommand();

		void undo() override;
		void redo() override;
	protected:
		WipeTowerChange m_change;
	};
}
#endif // CREATIVE_KERNEL_WIPE_TOWER_MOVE_COMMAND_202407102005_H