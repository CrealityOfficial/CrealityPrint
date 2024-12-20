#include "reliefrotateundocommand.h"
#include "reliefoperatemode.h"

using namespace creative_kernel;

ReliefRotateUndoCommand::ReliefRotateUndoCommand(QObject* parent)
	:QObject(parent)
{
}

ReliefRotateUndoCommand::~ReliefRotateUndoCommand()
{
}

void ReliefRotateUndoCommand::undo()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	m_operater->rotateRelief(relief, m_oldAngle);
}

void ReliefRotateUndoCommand::redo()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	m_operater->rotateRelief(relief, m_newAngle);
}

void ReliefRotateUndoCommand::setOperater(ReliefOperateMode* operater)
{
	m_operater = operater;
}

void ReliefRotateUndoCommand::setRelief(ModelN* relief)
{
	if (relief)
		m_reliefId = relief->getFixedId();
	else
		m_reliefId = -1;
}

void ReliefRotateUndoCommand::setAngle(float oldAngle, float newAngle)
{
	m_oldAngle = oldAngle;
	m_newAngle = newAngle;
}