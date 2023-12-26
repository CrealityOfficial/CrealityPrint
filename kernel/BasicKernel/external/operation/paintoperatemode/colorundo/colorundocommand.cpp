#include "colorundocommand.h"
#include "data/modeln.h"
#include "../paintoperatemode.h"

ColorUndoCommand::ColorUndoCommand(QObject* parent)
	:QObject(parent)
{
}

ColorUndoCommand::~ColorUndoCommand()
{
}

void ColorUndoCommand::undo()
{
	m_colorOperateMode->restore(m_model, m_oldData);
}

void ColorUndoCommand::redo()
{
	if (!m_needRedoOnFirst)
	{
		m_needRedoOnFirst = true;
		return;
	}
	m_colorOperateMode->restore(m_model, m_newData);
}

void ColorUndoCommand::setObjects(PaintOperateMode* colorOperateMode, creative_kernel::ModelN* model)
{
	m_colorOperateMode = colorOperateMode;
	m_model = model;
}

void ColorUndoCommand::setData(const std::vector<std::string>& oldColorData, const std::vector<std::string>& newColorData)
{
	m_oldData = oldColorData;
	m_newData = newColorData;
}
