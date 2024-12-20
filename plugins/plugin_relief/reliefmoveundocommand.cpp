#include "reliefmoveundocommand.h"
#include "reliefoperatemode.h"
#include "interface/spaceinterface.h"

using namespace creative_kernel;

ReliefMoveUndoCommand::ReliefMoveUndoCommand(QObject* parent)
	:QObject(parent)
{

}

ReliefMoveUndoCommand::~ReliefMoveUndoCommand()
{

}

void ReliefMoveUndoCommand::undo()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	if (m_isOldEmboss)
	{
		ModelN* target = getModelNByFixedId(m_oldTargetId);
		m_operater->moveRelief(relief, target, m_oldFaceId, m_oldCross, m_oldEmbossType);
	}
	else 
	{
		m_operater->initRelief(relief);
	}
}

void ReliefMoveUndoCommand::redo()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	ModelN* target = getModelNByFixedId(m_newTargetId);
	m_operater->moveRelief(relief, target, m_newFaceId, m_newCross, m_newEmbossType);
}

void ReliefMoveUndoCommand::setOperater(ReliefOperateMode* operater)
{
	m_operater = operater;
}

void ReliefMoveUndoCommand::needInit(bool need)
{
	m_isOldEmboss = !need;
	m_oldTargetId = -1;
	m_oldFaceId = -1;
	m_oldEmbossType = -1;
}

void ReliefMoveUndoCommand::setRelief(creative_kernel::ModelN* relief)
{
	if (relief)
		m_reliefId = relief->getFixedId();
	else 
		m_reliefId = -1;
}

void ReliefMoveUndoCommand::setOldEmbossInfo(int targetId, int faceId, QVector3D location, int embossType)
{
	m_oldTargetId = targetId;
	m_oldFaceId = faceId;
	m_oldCross = location;
	m_oldEmbossType = embossType;
}

void ReliefMoveUndoCommand::setNewEmbossInfo(int targetId, int faceId, QVector3D location, int embossType)
{
	m_newTargetId = targetId;
	m_newFaceId = faceId;
	m_newCross = location;
	m_newEmbossType = embossType;
}

bool ReliefMoveUndoCommand::valid()
{
	if (m_oldTargetId != m_newTargetId)
		return true;

	if (m_oldFaceId != m_newFaceId)
		return true;

	if (m_oldCross != m_newCross)
		return true;

	if (m_oldEmbossType != m_newEmbossType)
		return true;

	return false;
}