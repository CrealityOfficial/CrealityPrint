#include "calibratejob.h"
#include "qtusercore/module/progressortracer.h"
//#include "fdm52flow.h"

#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "slice/sliceflow.h"
#include "slice/ansycworker.h"
#include "qtusercore/module/systemutil.h"
#include <QtCore/QUuid>

#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "slice/calibratescenecreator.h"

Calibratejob::Calibratejob(creative_kernel::SliceFlow* _sliceflow, QObject* parent)
	: Job(parent)
	,m_sliceflow(_sliceflow)
	, m_creator(nullptr)
	, m_creatorOrca(nullptr)
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

void Calibratejob::setSceneCreatorOrca(crslice2::SceneCreator* creator)
{
	m_creatorOrca = creator;
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
	creative_kernel::FormatSlice tracer(progressor);
	if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
	{
		if (!m_creator)
		{
			tracer.failed("empty calib_creator.");
			return;
		}
		CrScenePtr scene;
		scene.reset(m_creator->create(&tracer));
		if (!scene)
		{
			tracer.failed("empty scene.");
			return;
		}
		float progressStep = 0.9f;
		tracer.resetProgressScope(0.0f, progressStep);
		m_gcodeFile = scene->m_gcodeFileName.c_str();

		crslice::CrSlice slice;
		slice.sliceFromScene(scene, &tracer);
	} 
	else
	{
		if (!m_creatorOrca)
		{
			tracer.failed("empty calib_creator.");
			return;
		}
		crslice2::CrScenePtr scene;
		scene.reset(m_creatorOrca->createOrca(&tracer));
		if (!scene)
		{
			tracer.failed("empty scene.");
			return;
		}
		float progressStep = 0.9f;
		tracer.resetProgressScope(0.0f, progressStep);

		m_gcodeFile = scene->m_gcodeFileName.c_str();

		scene->m_isBBLPrinter = true;
		scene->m_plate_index = 0;

		scene->m_settings->add("filament_deretraction_speed", "nil");
		scene->m_settings->add("filament_retract_before_wipe", "nil");
		scene->m_settings->add("filament_retract_lift_above", "nil");
		scene->m_settings->add("filament_retract_lift_below", "nil");
		scene->m_settings->add("filament_retract_restart_extra", "nil");
		scene->m_settings->add("filament_retract_when_changing_layer", "nil");
		scene->m_settings->add("filament_retraction_length", "nil");
		scene->m_settings->add("filament_retraction_minimum_travel", "nil");
		scene->m_settings->add("filament_retraction_speed", "nil");
		scene->m_settings->add("filament_wipe", "nil");
		scene->m_settings->add("filament_wipe_distance", "nil");
		scene->m_settings->add("filament_z_hop", "nil");
		
		crslice2::CrSlice slice;
		slice.sliceFromScene(scene, &tracer);
	}
}

