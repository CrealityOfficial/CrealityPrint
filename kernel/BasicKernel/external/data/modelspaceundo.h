#ifndef CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H
#define CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoStack>
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "data/undochange.h"

namespace creative_kernel
{
	class Printer;
	class BASIC_KERNEL_API ModelSpaceUndo : public QUndoStack
	{
		Q_OBJECT
	public:
		ModelSpaceUndo(QObject* parent = nullptr);
		virtual ~ModelSpaceUndo();

		void insertPrinter(int index, Printer* printer);
		void removePrinter(int index, Printer* printer);

		void modifySpace(const QList<ModelN*>& models, const QList<ModelN*>& removeModels);

		void mix(const QList<NUnionChangedStruct>& mixChange);
		void mirror(const QList<ModelN*>& models, int mode);
		void push(QUndoCommand* cmd);
		time_t getActiveTime() const { return m_activeTime; }

		void layoutChangeScene(const LayoutChangeInfo& changeInfo);
	private:
		time_t m_activeTime{ 0 };
	};
}
#endif // CREATIVE_KERNEL_MODELSPACEUNDO_1592743196335_H