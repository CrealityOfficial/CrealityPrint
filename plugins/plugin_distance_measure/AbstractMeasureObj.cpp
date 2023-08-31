#include "AbstractMeasureObj.h"
#include <QApplication>
#include <QDebug>

AbstractMeasureObj::AbstractMeasureObj(QObject* object)
	: m_attachUI(object)
{
}

AbstractMeasureObj::~AbstractMeasureObj()
{
}

int AbstractMeasureObj::measureType() const
{
	return m_measureType;
}

QString AbstractMeasureObj::measureValue() const
{
	return m_measureValue;
}




