#ifndef CREATIVE_KERNEL_GROUPMIXCHANGECOMMAND_202404152006_H
#define CREATIVE_KERNEL_GROUPMIXCHANGECOMMAND_202404152006_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>

#include "external/data/undochange.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API GroupMixChangeCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		GroupMixChangeCommand(QObject* parent = nullptr);
		virtual ~GroupMixChangeCommand();

		void undo() override;
		void redo() override;

		void setChanges(const QList<NUnionChangedStruct>& changes);
	private:
		void call(bool redo);
	protected:
		QList<NUnionChangedStruct> m_changes;
	};
}
#endif // CREATIVE_KERNEL_GROUPMIXCHANGECOMMAND_202404152006_H