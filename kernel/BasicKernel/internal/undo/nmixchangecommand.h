#ifndef CREATIVE_KERNEL_NMIXCHANGECOMMAND_1592796635575_H
#define CREATIVE_KERNEL_NMIXCHANGECOMMAND_1592796635575_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>

#include "external/data/undochange.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API NMixChangeCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		NMixChangeCommand(QObject* parent = nullptr);
		virtual ~NMixChangeCommand();

		void undo() override;
		void redo() override;

		void setChanges(const QList<NUnionChangedStruct>& changes);
	private:
		void call(bool redo);
	protected:
		QList<NUnionChangedStruct> m_changes;
	};

	class BASIC_KERNEL_API MeshChangeCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		MeshChangeCommand(QObject* parent = nullptr);
		virtual ~MeshChangeCommand();

		void undo() override;
		void redo() override;

		void setChanges(const QList<MeshChange>& changes);
	private:
		void call(bool redo);
	protected:
		QList<MeshChange> m_changes;
	};
}
#endif // CREATIVE_KERNEL_NMIXCHANGECOMMAND_1592796635575_H