#include "wipetowermovecommand.h"
#include "interface/spaceinterface.h"
#include "internal/multi_printer/printer.h"
#include "internal/multi_printer/printermanager.h"

namespace creative_kernel
{
	WipeTowerMoveCommand::WipeTowerMoveCommand(const WipeTowerChange& change)
		:QObject(nullptr)
		, m_change(change)
	{
		
	}

	WipeTowerMoveCommand::~WipeTowerMoveCommand()
	{
		
	}

	void WipeTowerMoveCommand::undo()
	{
		auto pm = getPrinterMangager();
		Printer* ptr = pm->getPrinter(m_change.index);
		if (ptr == nullptr)
		{
			return;
		}
		ptr->setWipeTowerPosition_LeftBottom(m_change.start);

		ModelSpace* space = getModelSpace();
		space->notifySpaceChange(true);
	}

	void WipeTowerMoveCommand::redo()
	{
		auto pm = getPrinterMangager();
		Printer* ptr = pm->getPrinter(m_change.index);
		if (ptr == nullptr)
		{
			return;
		}
		ptr->setWipeTowerPosition_LeftBottom(m_change.end);

		ModelSpace* space = getModelSpace();
		space->notifySpaceChange(true);
	}
}