#include "adaptiveundocommand.h"
#include "data/modeln.h"
#include "adaptivelayermode.h"
#include "interface/spaceinterface.h"

AdaptiveUndoCommand::AdaptiveUndoCommand(QObject* parent)
	:QObject(parent)
{
}

AdaptiveUndoCommand::~AdaptiveUndoCommand()
{
}

void AdaptiveUndoCommand::undo()
{
	creative_kernel::ModelN* model = creative_kernel::findModelBySerialName(m_serialName);
	if (!model)
		return;
	m_operateMode->restore(model, m_oldData);
}

void AdaptiveUndoCommand::redo()
{
	if (!m_needRedoOnFirst)
	{
		m_needRedoOnFirst = true;
		return;
	}
	
	creative_kernel::ModelN* model = creative_kernel::findModelBySerialName(m_serialName);
	if (!model)
		return;

	m_operateMode->restore(model, m_newData);
}

void AdaptiveUndoCommand::setObjects(AdaptiveLayerMode* colorOperateMode, creative_kernel::ModelN* model)
{
	m_operateMode = colorOperateMode;
	m_serialName = model->getSerialName();
}

void AdaptiveUndoCommand::setData(const std::vector<double>& oldColorData, const std::vector<double>& newColorData)
{
	m_oldData = oldColorData;
	m_newData = newColorData;
}
