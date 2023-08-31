#ifndef CREATIVE_KERNEL_MODELADD2GROUPCOMMAND_1592790419297_H
#define CREATIVE_KERNEL_MODELADD2GROUPCOMMAND_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include <Qt3DCore/QNode>

namespace creative_kernel
{
	class ModelN;
	class SpaceModifyCommand : public Qt3DCore::QNode, public QUndoCommand
	{
		Q_OBJECT
	public:
		SpaceModifyCommand(Qt3DCore::QNode* parent = nullptr);
		virtual ~SpaceModifyCommand();

		void undo() override;
		void redo() override;

		void setModels(const QList<ModelN*>& models);
		void setRemoveModels(const QList<ModelN*>& models);
	private:
		void deal(bool redo);
	protected:
		QList<ModelN*> m_modelList;
		QList<ModelN*> m_modelRemoveList;
	};
}
#endif // CREATIVE_KERNEL_MODELADD2GROUPCOMMAND_1592790419297_H