#include "LaserAddCommand.h"
#include "laserscene.h"

LaserAddCommand::LaserAddCommand(LaserScene* laser, QObject* obj, QString sharpName, bool undo)
{
	m_laserScene = laser;
	m_isUndo = undo;
	m_obj = obj;
	m_sharpName = sharpName;
}

LaserAddCommand::~LaserAddCommand()
{
}

void LaserAddCommand::undo()
{
	if (m_isRemove)
	{
		m_laserScene->setAddRectObject(m_sharpName, m_obj);
	}
	else
	{
		m_laserScene->deleteUndoObject(m_obj);
	}
	
}

void LaserAddCommand::redo()
{
	if (m_isRemove)
	{
		if (m_isUndo) {
			m_isUndo = false;
		}
		else 
		{
			m_laserScene->deleteUndoObject(m_obj);
		}
	}
	else
	{
		if (m_isUndo) {

			m_isUndo = false;
		}
		else {
			m_laserScene->setAddRectObject(m_sharpName, m_obj);
		}
	}
}

void LaserAddCommand::setIsRemove(bool isRemove)
{
	m_isRemove = isRemove;
}
