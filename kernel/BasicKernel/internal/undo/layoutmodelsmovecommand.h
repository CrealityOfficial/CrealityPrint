#ifndef CREATIVE_KERNEL_LAYOUT_MODELS_MOVE_COMMAND_202404231440_H
#define CREATIVE_KERNEL_LAYOUT_MODELS_MOVE_COMMAND_202404231440_H
#include "basickernelexport.h"
#include "data/undochange.h"
#include <QtWidgets/QUndoCommand>

namespace creative_kernel
{
	class LayoutModelsMoveCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT

	public:
		LayoutModelsMoveCommand();
		virtual ~LayoutModelsMoveCommand();

		void setLayoutChangeInfo(const LayoutChangeInfo& changeInfo);

	protected:
		void undo() override;
		void redo() override;

	protected:
		LayoutChangeInfo m_layoutChangeInfo;
	};
}
#endif // CREATIVE_KERNEL_LAYOUT_MODELS_MOVE_COMMAND_202404231440_H