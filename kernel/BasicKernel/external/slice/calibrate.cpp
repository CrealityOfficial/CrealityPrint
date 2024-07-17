#include "calibrate.h"
#include "job/calibratejob.h"
#include "interface/appsettinginterface.h"

namespace creative_kernel
{
	Calibrate::Calibrate(creative_kernel::SliceFlow* _flow,QObject* parent)
	:m_flow(_flow)
	{

	}


	Calibrate::~Calibrate()
	{

	}

	void Calibrate::temp(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_temperature);
		m_creator.setValue(_start, _end, _step);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		}
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
		//getKernel()->sliceFlow()->calibrate(_statrt, _end, _step);
	}

	void Calibrate::lowflow()
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_lowflow);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		}
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	Q_INVOKABLE void Calibrate::highflow(int value)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_highflow);
		//m_creator.setHighFlowValue(value);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		} 
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	void Calibrate::maxFlowValume(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_maxvolumetric);
		m_creator.setValue(_start, _end, _step);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		}
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	void Calibrate::setVFA(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_VFA);
		m_creator.setValue(_start, _end, _step);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		}
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	Q_INVOKABLE void Calibrate::setPA(float _start, float _end, float _step, int _type)
	{
		//m_creator.m_paType = (PA_type)_type;
		Calibratejob* job = new Calibratejob(m_flow);
		if (_type == (int)PA_type::pa_tower)
		{
			m_creator.setType(CalibrateType::ct_patower);
		}
		else
		{
			m_creator.setType(CalibrateType::ct_paline);
		}
		m_creator.setValue(_start, _end, _step);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		}
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	void Calibrate::retractionTower(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_retract);
		m_creator.setValue(_start, _end, _step);
		if (creative_kernel::getEngineType() == creative_kernel::EngineType::ET_CURA)
		{
			job->setSceneCreator(&m_creator);
		}
		else
		{
			job->setSceneCreatorOrca(&m_creator);
		}
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

}