#ifndef CREATIVE_KERNEL_NODDE_PROPERTY_COMMAND_1592874176854_H
#define CREATIVE_KERNEL_NODDE_PROPERTY_COMMAND_1592874176854_H
#include <QtWidgets/QUndoCommand>
#include "data/undochange.h"

namespace creative_kernel
{
	class NodeChangeCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		NodeChangeCommand(const QList<NodeChange>& changes, QObject* parent = nullptr);
		virtual ~NodeChangeCommand();

		void undo() override;
		void redo() override;
	protected:
		void invoke(bool redo);
	protected:
		QList<NodeChange> m_changes;
	};
}
#endif // CREATIVE_KERNEL_NODDE_PROPERTY_COMMAND_1592874176854_H