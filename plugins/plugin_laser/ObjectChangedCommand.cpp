#include "ObjectChangedCommand.h"
#include "laserscene.h"

ObjectChangedCommand::ObjectChangedCommand(LaserScene* laser, QObject* obj, long long oldX, long long oldY,
	double oldWidth, double oldHeight, double oldRatation, long long newX, long long newY,
	double newWidth, double newHeight, double newRatation, bool undo)
{
	m_laserScene = laser;
	m_isUndo = undo;
	m_obj = obj;
	m_oldX = oldX;
	m_oldY = oldY;
	m_oldWidth = oldWidth;
	m_oldHeight = oldHeight;
	m_oldRatation = oldRatation;
	m_newX = newX;
	m_newY = newY;
	m_newWidth = newWidth;
	m_newHeight = newHeight;
	m_newRatation = newRatation;
}

ObjectChangedCommand::~ObjectChangedCommand()
{
}

void ObjectChangedCommand::undo()
{
	m_laserScene->setObjectChangedData(m_obj, m_oldX, m_oldY, m_oldWidth, m_oldHeight, m_oldRatation);
}

void ObjectChangedCommand::redo()
{
	if (m_isUndo)
	{
		m_isUndo = false;
	}
	else
	{
		m_laserScene->setObjectChangedData(m_obj, m_newX, m_newY, m_newWidth, m_newHeight, m_newRatation);
	}
}
