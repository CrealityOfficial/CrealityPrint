#include "printerschangecommand.h"
#include "data/modeln.h"
#include "interface/printerinterface.h"
#include "interface/modelinterface.h"
#include "internal/multi_printer/printermanager.h"

namespace creative_kernel
{
	PrintersChangeCommand::PrintersChangeCommand(PrinterCmdOpType opType, int printerIndex, Printer* aPrinter)
		: m_opType(opType)
		, m_printerIndex(printerIndex)
		, m_isUndo(false)
	{
		if (PrinterCmdOpType::INSERT_PRINTER == opType)
		{
			m_addedPrinter = aPrinter;
			m_removedPrinter = nullptr;
		}
		else if(PrinterCmdOpType::REMOVE_PRINTER == opType)
		{
			m_removedPrinter = aPrinter;
			m_addedPrinter = nullptr;
		}
	}
	
	PrintersChangeCommand::~PrintersChangeCommand()
	{
	}

	void PrintersChangeCommand::undo()
	{
		m_isUndo = true;

		if (PrinterCmdOpType::REMOVE_PRINTER == m_opType)
		{
			m_addedPrinter = m_removedPrinter;
			this->_insertPrinter();
		}
		else if (PrinterCmdOpType::INSERT_PRINTER == m_opType)
		{
			m_removedPrinter = m_addedPrinter;
			this->_removePrinter();
		}
	}

	void PrintersChangeCommand::redo()
	{
		m_isUndo = false;

		if (PrinterCmdOpType::REMOVE_PRINTER == m_opType)
		{
			this->_removePrinter();
		}
		else if(PrinterCmdOpType::INSERT_PRINTER == m_opType)
		{
			this->_insertPrinter();
		}
	}

	void PrintersChangeCommand::_removePrinter()
	{
		if (!m_removedPrinter)
			return;

		auto pm = getPrinterMangager();

		QList<QString> outMoveModelSerialNames;
		QList<QVector3D> outStartPoses;
		QList<QVector3D> ouEndPoses;

		pm->deletePrinterEx(m_removedPrinter, outMoveModelSerialNames, outStartPoses, ouEndPoses, false);

		QList<ModelN*> modelsToMove;

		if (!m_isUndo)
		{
			m_moveModelSerialNames = outMoveModelSerialNames;
			m_startPoses = outStartPoses;
			m_endPoses = outStartPoses;

			modelsToMove = getModelnsBySerialName(outMoveModelSerialNames);

			moveModels(modelsToMove, outStartPoses, ouEndPoses, false);
		}
		else
		{
			QList<QVector3D> startPoses;
			QList<QVector3D> endPoses;

			for (int i = 0; i < m_moveModelSerialNames.size(); i++)
			{
				ModelN* mo = getModelNBySerialName(m_moveModelSerialNames[i]);
				if (mo)
				{
					modelsToMove.append(mo);
					startPoses.append(m_endPoses[i]);
					endPoses.append(m_startPoses[i]);
				}
			}

			moveModels(modelsToMove, startPoses, endPoses, false);

			m_removedPrinter->setParent(this);
		}

	}

	void PrintersChangeCommand::_insertPrinter()
	{
		if (!m_addedPrinter)
			return;

		auto pm = getPrinterMangager();

		QList<QString> outMoveModelSerialNames;
		QList<QVector3D> outStartPoses;
		QList<QVector3D> ouEndPoses;

		pm->insertPrinterEx(m_printerIndex, m_addedPrinter, outMoveModelSerialNames, outStartPoses, ouEndPoses);

		QList<ModelN*> modelsToMove;

		if (!m_isUndo)
		{
			m_moveModelSerialNames = outMoveModelSerialNames;
			m_startPoses = outStartPoses;
			m_endPoses = outStartPoses;

			modelsToMove = getModelnsBySerialName(outMoveModelSerialNames);

			moveModels(modelsToMove, outStartPoses, ouEndPoses, false);
		}
		else
		{
			QList<QVector3D> startPoses;
			QList<QVector3D> endPoses;

			for (int i = 0; i < m_moveModelSerialNames.size(); i++)
			{
				ModelN* mo = getModelNBySerialName(m_moveModelSerialNames[i]);
				if (mo)
				{
					modelsToMove.append(mo);
					startPoses.append(m_endPoses[i]);
					endPoses.append(m_startPoses[i]);
				}
			}

			moveModels(modelsToMove, startPoses, endPoses, false);
		}

	}

}