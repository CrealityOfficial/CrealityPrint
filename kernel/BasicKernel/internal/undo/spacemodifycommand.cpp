#include "spacemodifycommand.h"
#include "internal/data/_modelinterface.h"
#include "data/modeln.h"
#include "internal/multi_printer/printermanager.h"

namespace creative_kernel
{
	SpaceModifyCommand::SpaceModifyCommand(Qt3DCore::QNode* parent)
		:QNode(parent)
	{
	}
	
	SpaceModifyCommand::~SpaceModifyCommand()
	{
	}

	void SpaceModifyCommand::undo()
	{
		deal(false);
	}

	void SpaceModifyCommand::redo()
	{
		deal(true);
	}

	void SpaceModifyCommand::setModels(const QList<ModelN*>& models)
	{
		m_modelList = models;
		m_type = MODIFY_MODELS;
	}

	void SpaceModifyCommand::setRemoveModels(const QList<ModelN*>& models)
	{
		m_modelRemoveList = models;
		m_type = MODIFY_MODELS;
	}

	void SpaceModifyCommand::insertPrinter(int index, Printer* printer)
	{
		m_printerIndex = index;
		m_addedPrinter = printer;
		m_type = INSERT_PRINTER;
	}

	void SpaceModifyCommand::removePrinter(int index, Printer* printer)
	{
		m_printerIndex = index;
		m_removedPrinter = printer;
		m_type = REMOVE_PRINTER;
	}

	void SpaceModifyCommand::deal(bool redo)
	{
		if (m_type == INSERT_PRINTER)
		{
			auto pm = getPrinterMangager();
			if (redo)
				pm->insertPrinter(m_printerIndex, m_addedPrinter);
			else 
			{
				pm->deletePrinter(m_addedPrinter, false);
				m_addedPrinter->setParent(this);
			}
		}
		else if (m_type == REMOVE_PRINTER) 
		{
			auto pm = getPrinterMangager();
			if (redo) 
			{
				pm->deletePrinter(m_removedPrinter, false);
				m_removedPrinter->setParent(this);
			}
			else 
				pm->insertPrinter(m_printerIndex, m_removedPrinter);
		}
		else 
		{
			QList<ModelN*> removes = redo ? m_modelRemoveList : m_modelList;
			QList<ModelN*> adds = redo ? m_modelList : m_modelRemoveList;
			
			_modifySpace(removes, adds);

			for (ModelN* model : removes)
				model->setParent(this);
		}
	}
}