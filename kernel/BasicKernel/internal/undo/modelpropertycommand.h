#ifndef CREATIVE_KERNEL_MODEL_PROPERTY_COMMAND_1592874176854_H
#define CREATIVE_KERNEL_MODEL_PROPERTY_COMMAND_1592874176854_H
#include <QtWidgets/QUndoCommand>
#include "data/undochange.h"

namespace creative_kernel
{
	class SharedDataTracer;
	class ModelNPropertyMeshCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT
	public:
		ModelNPropertyMeshCommand(const QList<ModelNPropertyMeshUndo>& changes, QObject* parent = nullptr);
		virtual ~ModelNPropertyMeshCommand();

		void undo() override;
		void redo() override;
	protected:
		QList<ModelNPropertyMeshUndo> m_changes;
		QList<SharedDataTracer*> m_dataSustainers;


	};
}
#endif // CREATIVE_KERNEL_MODEL_PROPERTY_COMMAND_1592874176854_H