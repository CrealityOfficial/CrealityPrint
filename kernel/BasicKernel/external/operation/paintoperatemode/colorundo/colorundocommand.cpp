#include "colorundocommand.h"
#include "data/modeln.h"
#include "../paintoperatemode.h"
#include "interface/spaceinterface.h"

ColorUndoCommand::ColorUndoCommand(QObject* parent)
	:QObject(parent)
{
}

ColorUndoCommand::~ColorUndoCommand()
{
}

void ColorUndoCommand::undo()
{
	creative_kernel::ModelN* model = creative_kernel::findModelBySerialName(m_serialName);
	if (!model)
		return;
	m_colorOperateMode->restore(model, m_oldData);
}

void ColorUndoCommand::redo()
{
	if (!m_needRedoOnFirst)
	{
		m_needRedoOnFirst = true;
		return;
	}
	
	creative_kernel::ModelN* model = creative_kernel::findModelBySerialName(m_serialName);
	if (!model)
		return;

	m_colorOperateMode->restore(model, m_newData);
}

void ColorUndoCommand::setObjects(PaintOperateMode* colorOperateMode, creative_kernel::ModelN* model)
{
	m_colorOperateMode = colorOperateMode;
	m_serialName = model->getSerialName();
	// m_model = model;
}

void ColorUndoCommand::setData(const std::vector<std::string>& oldColorData, const std::vector<std::string>& newColorData)
{
	m_oldData = oldColorData;
	m_newData = newColorData;
}
