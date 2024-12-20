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
	m_colorOperateMode->restore(m_objectID, m_oldData);
}

void ColorUndoCommand::redo()
{
	if (!m_needRedoOnFirst)
	{
		m_needRedoOnFirst = true;
		return;
	}
	
	m_colorOperateMode->restore(m_objectID, m_newData);
}

void ColorUndoCommand::setObjects(PaintOperateMode* colorOperateMode, creative_kernel::ModelN* model)
{
	m_colorOperateMode = colorOperateMode;
	m_objectID = model->getObjectId();
}

void ColorUndoCommand::setObjects(PaintOperateMode* colorOperateMode, creative_kernel::ModelGroup* modelGroup)
{
	m_colorOperateMode = colorOperateMode;
	m_objectID = modelGroup->getObjectId();
}

void ColorUndoCommand::setData(const QList<std::vector<std::string>>& oldColorData, const QList<std::vector<std::string>>& newColorData)
{
	m_oldData = oldColorData;
	m_newData = newColorData;
}
