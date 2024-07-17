#include "layoutmodelsmovecommand.h"
#include "data/modeln.h"
#include "interface/printerinterface.h"
#include "interface/modelinterface.h"
#include "internal/multi_printer/printermanager.h"

namespace creative_kernel
{
	LayoutModelsMoveCommand::LayoutModelsMoveCommand()
	{

	}
	
	LayoutModelsMoveCommand::~LayoutModelsMoveCommand()
	{
	}

	void LayoutModelsMoveCommand::setLayoutChangeInfo(const LayoutChangeInfo& changeInfo)
	{
		m_layoutChangeInfo = changeInfo;
	}

	void LayoutModelsMoveCommand::undo()
	{
		if(m_layoutChangeInfo.needPrinterLayout)
			resizePrinters(m_layoutChangeInfo.prevPrintersCount);

		QList<ModelN*> models = getModelnsBySerialName(m_layoutChangeInfo.moveModelsSerialNames);
		mixTRModels(models, m_layoutChangeInfo.endPoses, m_layoutChangeInfo.startPoses, m_layoutChangeInfo.endQuaternions, m_layoutChangeInfo.startQuaternions,  false);
	}

	void LayoutModelsMoveCommand::redo()
	{
		//if(m_layoutChangeInfo.needPrinterLayout || m_layoutChangeInfo.nowPrintersCount > m_layoutChangeInfo.prevPrintersCount)
		//	resizePrinters(m_layoutChangeInfo.nowPrintersCount);

		QList<ModelN*> models = getModelnsBySerialName(m_layoutChangeInfo.moveModelsSerialNames);
		mixTRModels(models, m_layoutChangeInfo.startPoses, m_layoutChangeInfo.endPoses, m_layoutChangeInfo.startQuaternions, m_layoutChangeInfo.endQuaternions, false);

		if (m_layoutChangeInfo.needPrinterLayout /*|| m_layoutChangeInfo.nowPrintersCount > m_layoutChangeInfo.prevPrintersCount*/)
			resizePrinters(m_layoutChangeInfo.nowPrintersCount);

		if (m_layoutChangeInfo.needPrinterLayout || m_layoutChangeInfo.nowPrintersCount > m_layoutChangeInfo.prevPrintersCount)
		{
			//checkEmptyPrintersToRemove();
		}
	}

}