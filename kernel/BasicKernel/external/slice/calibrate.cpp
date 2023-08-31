#include "calibrate.h"
#include "job/calibratejob.h"

namespace creative_kernel
{
	Calibrate::Calibrate(creative_kernel::SliceFlow* _flow,QObject* parent)
	:m_flow(_flow)
	{

	}


	Calibrate::~Calibrate()
	{

	}

	void Calibrate::generate(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_temperature);
		m_creator.setValue(_start, _end, _step);
		job->setSceneCreator(&m_creator);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
		//getKernel()->sliceFlow()->calibrate(_statrt, _end, _step);
	}

	void Calibrate::lowflow()
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_lowflow);
		job->setSceneCreator(&m_creator);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	Q_INVOKABLE void Calibrate::highflow(int value)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_highflow);
		m_creator.setHighFlowValue(value);
		job->setSceneCreator(&m_creator);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	void Calibrate::maxFlowValume(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_maxvolumetric);
		m_creator.setValue(_start, _end, _step);
		job->setSceneCreator(&m_creator);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	void Calibrate::VFA(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_VFA);
		m_creator.setValue(_start, _end, _step);
		job->setSceneCreator(&m_creator);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

	Q_INVOKABLE void Calibrate::testType(int type)
	{
		m_creator.m_paType = (PA_type)type;
	}

	Q_INVOKABLE void Calibrate::paTower(float _start, float _end, float _step)
	{
		Calibratejob* job = new Calibratejob(m_flow);
		m_creator.setType(CalibrateType::ct_pressure);
		m_creator.setValue(_start, _end, _step);
		job->setSceneCreator(&m_creator);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}

}