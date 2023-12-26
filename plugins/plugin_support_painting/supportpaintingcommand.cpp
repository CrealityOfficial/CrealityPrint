#include "supportpaintingcommand.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "supportpaintingoperatemode.h"
#include <QDebug>
#include <QPoint>

SupportPaintingCommand::SupportPaintingCommand(QObject* parent) : ToolCommand(parent)
{
	m_operateMode = new SupportPaintingOperateMode(this);

	connect(m_operateMode, &PaintOperateMode::posChanged, this, &SupportPaintingCommand::posChanged);
	connect(m_operateMode, &PaintOperateMode::scaleChanged, this, &SupportPaintingCommand::scaleChanged);
	connect(m_operateMode, &PaintOperateMode::sectionRateChanged, this, &SupportPaintingCommand::sectionRateChanged);
	connect(m_operateMode, &PaintOperateMode::colorRadiusChanged, this, &SupportPaintingCommand::colorRadiusChanged);
	connect(m_operateMode, &PaintOperateMode::highlightOverhangsDegChanged, this, &SupportPaintingCommand::highlightOverhangsDegChanged);
	connect(m_operateMode, &PaintOperateMode::enter, this, &SupportPaintingCommand::isRunningChanged);
	connect(m_operateMode, &PaintOperateMode::quit, this, &SupportPaintingCommand::isRunningChanged);

}	

SupportPaintingCommand::~SupportPaintingCommand()
{
}

void SupportPaintingCommand::execute()
{
	qDebug() << "color command execute";
	creative_kernel::setVisOperationMode(m_operateMode);
}

void SupportPaintingCommand::setSupportEnabled(bool enabled)
{
	m_operateMode->setEnabled(enabled);
}

bool SupportPaintingCommand::checkSelect() 
{
	return true;
}

void SupportPaintingCommand::resetDirection()
{
	m_operateMode->resetDirection();
}
	
void SupportPaintingCommand::clearAllColor()
{
	m_operateMode->clearAllColor();
}

QPoint SupportPaintingCommand::pos()
{
	return m_operateMode->pos();
}

float SupportPaintingCommand::scale()
{
	return m_operateMode->scale();
}

float SupportPaintingCommand::sectionRate()
{
	return m_operateMode->sectionRate();
}

void SupportPaintingCommand::setSectionRate(float rate)
{
	m_operateMode->setSectionRate(rate);
}

float SupportPaintingCommand::colorRadius()
{
	return m_operateMode->colorRadius();
}

void SupportPaintingCommand::setColorRadius(float radius)
{
	m_operateMode->setColorRadius(radius);
}

int SupportPaintingCommand::colorMethod()
{
	return m_operateMode->colorMethod(); 
}

void SupportPaintingCommand::setColorMethod(int method)
{
	if (colorMethod() == method)
		return;

	m_operateMode->setColorMethod(method);
	emit colorMethodChanged();
}

int SupportPaintingCommand::colorIndex()
{
	return m_operateMode->colorIndex();
}

void SupportPaintingCommand::setColorIndex(int index)
{
	m_operateMode->setColorIndex(index);
}

QVariantList SupportPaintingCommand::colorsList()
{
	return m_operateMode->colorsList();
}

void SupportPaintingCommand::setColorsList(const QVariantList& datasList)
{
	m_operateMode->setColorsList(datasList);
	emit colorsListChanged();
}

float SupportPaintingCommand::highlightOverhangsDeg()
{
	return m_operateMode->highlightOverhangsDeg();
}

void SupportPaintingCommand::setHighlightOverhangsDeg(float deg)
{
	m_operateMode->setHighlightOverhangsDeg(deg);
}

bool SupportPaintingCommand::overHangsEnable()
{
	return m_operateMode->overHangsEnable();
}

void SupportPaintingCommand::setOverHangsEnable(bool enable)
{
	m_operateMode->setOverHangsEnable(enable);
}

bool SupportPaintingCommand::isRunning()
{
	return m_operateMode->isRunning();
}