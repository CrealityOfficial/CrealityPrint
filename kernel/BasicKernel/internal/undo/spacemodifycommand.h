#ifndef CREATIVE_KERNEL_MODELADD2GROUPCOMMAND_1592790419297_H
#define CREATIVE_KERNEL_MODELADD2GROUPCOMMAND_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include <Qt3DCore/QNode>

namespace creative_kernel
{
	class ModelN;
	class Printer;
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

		void insertPrinter(int index, Printer* printer);
		void removePrinter(int index, Printer* printer);

	private:
		void deal(bool redo);
	protected:
		enum OperateType
		{
			MODIFY_MODELS = 0,
			INSERT_PRINTER,
			REMOVE_PRINTER
		};

		OperateType m_type;

		QList<ModelN*> m_modelList;
		QList<ModelN*> m_modelRemoveList;

		int m_printerIndex;
		Printer* m_addedPrinter { NULL };
		Printer* m_removedPrinter { NULL };
	
	};
}
#endif // CREATIVE_KERNEL_MODELADD2GROUPCOMMAND_1592790419297_H