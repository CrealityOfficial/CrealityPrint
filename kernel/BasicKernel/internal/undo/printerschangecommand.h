#ifndef CREATIVE_KERNEL_PRINTERS_CHANGE_COMMAND_202403201720_H
#define CREATIVE_KERNEL_PRINTERS_CHANGE_COMMAND_202403201720_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>

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
		QStringList m_movesModelSerialNames;

		PrinterCmdOpType m_opType;
		int m_printerIndex;
		Printer* m_addedPrinter{ NULL };
		Printer* m_removedPrinter{ NULL };

		QList<QString> m_moveModelSerialNames;
		QList<QVector3D> m_startPoses;
		QList<QVector3D> m_endPoses;

		bool m_isUndo;

	};
}
#endif // CREATIVE_KERNEL_PRINTERS_CHANGE_COMMAND_202403201720_H