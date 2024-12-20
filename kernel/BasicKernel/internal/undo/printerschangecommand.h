#ifndef CREATIVE_KERNEL_PRINTERS_CHANGE_COMMAND_202403201720_H
#define CREATIVE_KERNEL_PRINTERS_CHANGE_COMMAND_202403201720_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include "qtuser3d/trimesh2/conv.h"

namespace creative_kernel
{
	class Printer;

	enum PrinterCmdOpType
	{
		INSERT_PRINTER = 0,
		REMOVE_PRINTER
	};

	// add or delete printer
	class PrintersChangeCommand : public QObject, public QUndoCommand
	{
		Q_OBJECT

	public:
		PrintersChangeCommand(PrinterCmdOpType opType, int printerIndex, Printer* aPrinter);
		virtual ~PrintersChangeCommand();

	protected:
		void undo() override;
		void redo() override;

		void _removePrinter();
		void _insertPrinter();

	protected:
		PrinterCmdOpType m_opType;
		int m_printerIndex;
		Printer* m_addedPrinter{ NULL };
		Printer* m_removedPrinter{ NULL };

		QList<int64_t> m_moveGroupIds;

		QList<trimesh::dvec3> m_endOffsets;
		QList<trimesh::xform> m_undoMatrixs;

		bool m_isUndo;
		bool m_firstRedo;

	};
}
#endif // CREATIVE_KERNEL_PRINTERS_CHANGE_COMMAND_202403201720_H