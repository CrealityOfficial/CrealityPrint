#include "paintcommand.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include <QDebug>
#include <QPoint>
#include "paintoperatemode.h"

PaintCommand::PaintCommand(QObject* parent) : ToolCommand(parent)
{
	orderindex = 0;
}	

PaintCommand::~PaintCommand()
{
}

void PaintCommand::init()
{
	if (!m_operateMode)
		return;

	connect(m_operateMode, &PaintOperateMode::posChanged, this, &PaintCommand::posChanged);
	connect(m_operateMode, &PaintOperateMode::scaleChanged, this, &PaintCommand::scaleChanged);
	connect(m_operateMode, &PaintOperateMode::sectionRateChanged, this, &PaintCommand::sectionRateChanged);
	connect(m_operateMode, &PaintOperateMode::colorRadiusChanged, this, &PaintCommand::colorRadiusChanged);
	connect(m_operateMode, &PaintOperateMode::colorIndexChanged, this, &PaintCommand::colorIndexChanged);
	connect(m_operateMode, &PaintOperateMode::highlightOverhangsDegChanged, this, &PaintCommand::highlightOverhangsDegChanged);
	connect(m_operateMode, &PaintOperateMode::smartFillAngleChanged, this, &PaintCommand::smartFillAngleChanged);
	connect(m_operateMode, &PaintOperateMode::heightRangeChanged, this, &PaintCommand::heightRangeChanged);
	connect(m_operateMode, &PaintOperateMode::gapAreaChanged, this, &PaintCommand::gapAreaChanged);
	connect(m_operateMode, &PaintOperateMode::sphereSizeChanged, this, &PaintCommand::sphereSizeChanged);
	connect(m_operateMode, &PaintOperateMode::enter, this, &PaintCommand::isRunningChanged);
	connect(m_operateMode, &PaintOperateMode::quit, this, &PaintCommand::isRunningChanged);
	connect(m_operateMode, &PaintOperateMode::screenRadiusChanged, this, &PaintCommand::screenRadiusChanged);
}
	
void PaintCommand::setOperateMode(PaintOperateMode* operateMode)
{
	m_operateMode = operateMode;
	init();
}

void PaintCommand::execute()
{
	//qDebug() << "color command execute";
	creative_kernel::setVisOperationMode(m_operateMode);
}

bool PaintCommand::checkSelect() 
{
	return true;
}

void PaintCommand::resetDirection()
{
	m_operateMode->resetDirection();
}
	
void PaintCommand::clearAllColor()
{
	m_operateMode->clearAllColor();
}

void PaintCommand::setOperateModeEnable(bool enable)
{
	m_operateMode->setEnabled(enable);
}

QPoint PaintCommand::pos()
{
	return m_operateMode->pos();
}

float PaintCommand::scale()
{
	return m_operateMode->scale();
}

float PaintCommand::sectionRate()
{
	return m_operateMode->sectionRate();
}

void PaintCommand::setSectionRate(float rate)
{
	m_operateMode->setSectionRate(rate);
}

float PaintCommand::colorRadius()
{
	return m_operateMode->colorRadius();
}

float PaintCommand::screenRadius()
{
	return m_operateMode->screenRadius();
}

void PaintCommand::setColorRadius(float radius)
{
	m_operateMode->setColorRadius(radius);
}

int PaintCommand::colorMethod()
{
	return m_operateMode->colorMethod(); 
}

void PaintCommand::setColorMethod(int method)
{
	if (colorMethod() == method)
		return;

	m_operateMode->setColorMethod(method);
	emit colorMethodChanged();
}

int PaintCommand::colorIndex()
{
	return m_operateMode->colorIndex();
}

void PaintCommand::setColorIndex(int index)
{
	m_operateMode->setColorIndex(index);
}

QVariantList PaintCommand::colorsList()
{
	return m_operateMode->colorsList();
}

void PaintCommand::setColorsList(const QVariantList& datasList)
{
	m_operateMode->setColorsList(datasList);
	emit colorsListChanged();
}

float PaintCommand::highlightOverhangsDeg()
{
	return m_operateMode->highlightOverhangsDeg();
}

void PaintCommand::setHighlightOverhangsDeg(float deg)
{
	m_operateMode->setHighlightOverhangsDeg(deg);
}

bool PaintCommand::overHangsEnable()
{
	return m_operateMode->overHangsEnable();
}

void PaintCommand::setOverHangsEnable(bool enable)
{
	m_operateMode->setOverHangsEnable(enable);
}

float PaintCommand::smartFillAngle()
{
	return m_operateMode->smartFillAngle();
}

void PaintCommand::setSmartFillAngle(float angle)
{
	m_operateMode->setSmartFillAngle(angle);
}

bool PaintCommand::smartFillEnable()
{
	return m_operateMode->smartFillEnable();
}

void PaintCommand::setSmartFillEnable(bool enable)
{
	m_operateMode->setSmartFillEnable(enable);
}

float PaintCommand::heightRange()
{
	return m_operateMode->heightRange();
}

void PaintCommand::setHeightRange(float heightRange)
{
	m_operateMode->setHeightRange(heightRange);
}

float PaintCommand::gapArea()
{
	return m_operateMode->gapArea();
}

void PaintCommand::setGapArea(float gapArea)
{
	m_operateMode->setGapArea(gapArea);
}

float PaintCommand::sphereSize()
{
	return m_operateMode->sphereSize();
}

void PaintCommand::setSphereSize(float size)
{
	m_operateMode->setSphereSize(size);
}

bool PaintCommand::isRunning()
{
	return m_operateMode->isRunning();
}