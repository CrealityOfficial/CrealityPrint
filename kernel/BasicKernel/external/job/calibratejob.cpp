#include "calibratejob.h"
#include "qtusercore/module/progressortracer.h"
//#include "fdm52flow.h"

#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "slice/sliceflow.h"
#include "interface/appsettinginterface.h"

QString generateTempGCodeFileName()
{
	QString fileName = QString("%1/temp.gcode").arg(SLICE_PATH);
	QFile::remove(fileName);

	return fileName;
}

Calibratejob::Calibratejob(creative_kernel::SliceFlow* _sliceflow, QObject* parent)
	: Job(parent)
	,m_sliceflow(_sliceflow)
	, m_creator(nullptr)
{
}

Calibratejob::~Calibratejob()
{
}

void Calibratejob::setFileName(const QString& fileName)
{
	m_fileName = fileName;
}

void Calibratejob::setSceneCreator(crslice::SceneCreator* creator)
{
	m_creator = creator;
}

QString Calibratejob::name()
{
	return "CalibrateJob";
}

QString Calibratejob::description()
{
	return "CalibrateJob.";
}

void Calibratejob::failed()
{
}

void Calibratejob::successed(qtuser_core::Progressor* progressor)
{
	m_sliceflow->loadGCode(m_gcodeFile);
}



void Calibratejob::work(qtuser_core::Progressor* progressor)
{
	qtuser_core::ProgressorTracer tracer(progressor);
	CrScenePtr scene;

	if (m_creator)
	{
		scene.reset(m_creator->create(&tracer));
		tracer.resetProgressScope();
	}

	if (!scene)
	{
		tracer.failed("empty scene.");
		return;
	}

	QString fileName = generateTempGCodeFileName();
	scene->setOutputGCodeFileName(fileName.toLocal8Bit().data());
	m_gcodeFile = fileName;

	crslice::CrSlice slice;
	slice.sliceFromScene(scene, &tracer);
}

