#include "seampaintingcommand.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "seampaintingoperatemode.h"
#include <QDebug>
#include <QPoint>

SeamPaintingCommand::SeamPaintingCommand(QObject* parent) : ToolCommand(parent)
{
	m_operateMode = new SeamPaintingOperateMode(this);

	connect(m_operateMode, &PaintOperateMode::posChanged, this, &SeamPaintingCommand::posChanged);
	connect(m_operateMode, &PaintOperateMode::scaleChanged, this, &SeamPaintingCommand::scaleChanged);
	connect(m_operateMode, &PaintOperateMode::sectionRateChanged, this, &SeamPaintingCommand::sectionRateChanged);
	connect(m_operateMode, &PaintOperateMode::colorRadiusChanged, this, &SeamPaintingCommand::colorRadiusChanged);
}

SeamPaintingCommand::~SeamPaintingCommand()
{
}

void SeamPaintingCommand::execute()
{
	qDebug() << "color command execute";
	creative_kernel::setVisOperationMode(m_operateMode);
}

bool SeamPaintingCommand::checkSelect() 
{
	return true;
}

void SeamPaintingCommand::resetDirection()
{
	m_operateMode->resetDirection();
}
	
void SeamPaintingCommand::clearAllColor()
{
	m_operateMode->clearAllColor();
}

QPoint SeamPaintingCommand::pos()
{
	return m_operateMode->pos();
}

float SeamPaintingCommand::scale()
{
	return m_operateMode->scale();
}

float SeamPaintingCommand::sectionRate()
{
	return m_operateMode->sectionRate();
}

void SeamPaintingCommand::setSectionRate(float rate)
{
	m_operateMode->setSectionRate(rate);
}

float SeamPaintingCommand::colorRadius()
{
	return m_operateMode->colorRadius();
}

void SeamPaintingCommand::setColorRadius(float radius)
{
	m_operateMode->setColorRadius(radius);
}

int SeamPaintingCommand::colorMethod()
{
	return m_operateMode->colorMethod(); 
}

void SeamPaintingCommand::setColorMethod(int method)
{
	if (colorMethod() == method)
		return;

	m_operateMode->setColorMethod(method);
	emit colorMethodChanged();
}

int SeamPaintingCommand::colorIndex()
{
	return m_operateMode->colorIndex();
}

void SeamPaintingCommand::setColorIndex(int index)
{
	m_operateMode->setColorIndex(index);
}

QVariantList SeamPaintingCommand::colorsList()
{
	return m_operateMode->colorsList();
}

void SeamPaintingCommand::setColorsList(const QVariantList& datasList)
{
	m_operateMode->setColorsList(datasList);
	emit colorsListChanged();
}