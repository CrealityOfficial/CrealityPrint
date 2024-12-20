#include "printerschangecommand.h"
#include "data/modeln.h"
#include "interface/printerinterface.h"
#include "interface/spaceinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "data/modelspace.h"
#include "kernel/kernel.h"
#include "interface/projectinterface.h"
#include "internal/project_cx3d/autosavejob.h"
namespace creative_kernel
{
	PrintersChangeCommand::PrintersChangeCommand(PrinterCmdOpType opType, int printerIndex, Printer* aPrinter)
		: m_opType(opType)
		, m_printerIndex(printerIndex)
		, m_isUndo(false)
		, m_firstRedo(true)
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
		m_firstRedo = false;

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
		triggerAutoSave(QList<ModelGroup*>(),AutoSaveJob::PLATE);
		notifySpaceChange(false);
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
		triggerAutoSave(QList<ModelGroup*>(),AutoSaveJob::PLATE);
		notifySpaceChange(false);
	}

	void PrintersChangeCommand::_removePrinter()
	{
		if (!m_removedPrinter)
			return;

		auto pm = getPrinterMangager();

		QList<int64_t> outMoveGroupIds;
		QList<trimesh::dvec3> outEndOffsets;

		pm->deletePrinterEx(m_removedPrinter, outMoveGroupIds, outEndOffsets, false);
		Q_ASSERT(outMoveGroupIds.size() == outEndOffsets.size());

		QList<ModelGroup*> modelGroupsToMove;

		if (!m_isUndo)
		{
			if (m_firstRedo)
			{
				m_moveGroupIds = outMoveGroupIds;
				m_endOffsets = outEndOffsets;
			}

			for(int i = 0; i < outMoveGroupIds.size(); i++)
			{
				ModelGroup* mg = getModelGroupByObjectId(outMoveGroupIds[i]);
				if (mg)
				{
					if(m_firstRedo)
						m_undoMatrixs << mg->getMatrix();

					mg->setMatrix(trimesh::xform::trans(outEndOffsets[i]) * mg->getMatrix());
					modelGroupsToMove << mg;
				}
			}
		}
		else
		{
			Q_ASSERT(m_moveGroupIds.size() == m_undoMatrixs.size());
			for (int i = 0; i < m_moveGroupIds.size(); i++)
			{
				ModelGroup* mg = getModelGroupByObjectId(m_moveGroupIds[i]);
				if (mg)
				{
					mg->setMatrix(m_undoMatrixs[i]);
					modelGroupsToMove << mg;
				}
			}

			m_removedPrinter->setParent(this);
		}

		updateSpaceNodeRender(modelGroupsToMove);

	}

	void PrintersChangeCommand::_insertPrinter()
	{
		if (!m_addedPrinter)
			return;

		auto pm = getPrinterMangager();

		QList<int64_t> outMoveGroupIds;
		QList<trimesh::dvec3> outEndOffsets;

		pm->insertPrinterEx(m_printerIndex, m_addedPrinter, outMoveGroupIds, outEndOffsets);
		Q_ASSERT(outMoveGroupIds.size() == outEndOffsets.size());

		QList<ModelGroup*> modelGroupsToMove;

		if (!m_isUndo)
		{
			if (m_firstRedo)
			{
				m_moveGroupIds = outMoveGroupIds;
				m_endOffsets = outEndOffsets;
			}

			for (int i = 0; i < outMoveGroupIds.size(); i++)
			{
				ModelGroup* mg = getModelGroupByObjectId(outMoveGroupIds[i]);
				if (mg)
				{
					if (m_firstRedo)
						m_undoMatrixs << mg->getMatrix();

					mg->setMatrix(trimesh::xform::trans(outEndOffsets[i]) * mg->getMatrix());
					modelGroupsToMove << mg;
				}
			}

		}
		else
		{
			Q_ASSERT(m_moveGroupIds.size() == m_undoMatrixs.size());

			for (int i = 0; i < m_moveGroupIds.size(); i++)
			{
				ModelGroup* mg = getModelGroupByObjectId(m_moveGroupIds[i]);
				if (mg)
				{
					mg->setMatrix(m_undoMatrixs[i]);
					modelGroupsToMove << mg;
				}
			}

		}

		updateSpaceNodeRender(modelGroupsToMove);

	}

}