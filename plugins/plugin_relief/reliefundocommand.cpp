#include "reliefundocommand.h"
#include "reliefoperatemode.h"

using namespace creative_kernel;

ReliefUndoCommand::ReliefUndoCommand(QObject* parent)
	:QObject(parent)
{
}

ReliefUndoCommand::~ReliefUndoCommand()
{
}

void ReliefUndoCommand::undo()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	m_operater->applyFontConfigToRelief(relief, m_old);
}

void ReliefUndoCommand::redo()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	m_operater->applyFontConfigToRelief(relief, m_new);
}

void ReliefUndoCommand::setOperater(ReliefOperateMode* operater)
{
	m_operater = operater;
}

void ReliefUndoCommand::setTarget(ModelN* target)
{
	if (target)
		m_reliefId = target->getFixedId();
	else
		m_reliefId = -1;
}

void ReliefUndoCommand::setUndoConfig(ReliefUndoConfig oldConfig, ReliefUndoConfig newConfig)
{
	m_old = oldConfig;
	m_new = newConfig;
}