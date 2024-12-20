#include "calibratescenecreator.h"


namespace creative_kernel
{
	CalibrateSceneCreator::CalibrateSceneCreator()
	{

	}

	CalibrateSceneCreator::~CalibrateSceneCreator()
	{

	}

	void CalibrateSceneCreator::setType(CalibrateType _type)
	{
		switch (_type)
		{
		case CalibrateType::ct_temperature:
		{
			m_calibmode.reset(new Temp());
			break;
		}
		case CalibrateType::ct_highflow:
		{
			m_calibmode.reset(new HighFlow());
			break;
		}
		case CalibrateType::ct_lowflow:
		{
			m_calibmode.reset(new LowFlow());
			break;
		}
		case CalibrateType::ct_paline:
		{
			m_calibmode.reset(new PaLine());
			break;
		}
		case CalibrateType::ct_patower:
		{
			m_calibmode.reset(new PaTower());
			break;
		}
		case CalibrateType::ct_maxvolumetric:
		{
			m_calibmode.reset(new MaxFlowValume());
			break;
		}
		case CalibrateType::ct_VFA:
		{
			m_calibmode.reset(new VFA());
			break;
		}
		case CalibrateType::ct_retract:
		{
			m_calibmode.reset(new RetractionTower());
			break;
		}
		case CalibrateType::ct_retractspeed:
		{
			m_calibmode.reset(new RetractionSpeed());
			break;
		}
		case CalibrateType::ct_limitSpeed:
		{
			m_calibmode.reset(new LimitSpeed());
			break;
		}
		case CalibrateType::ct_limitAcceleration:
		{
			m_calibmode.reset(new LimitAcceleration());
			break;
		}
		case CalibrateType::ct_speedTower:
		{
			m_calibmode.reset(new SpeedTower());
			break;
		}
		case CalibrateType::ct_accelerationTower:
		{
			m_calibmode.reset(new AccelerationTower());
			break;
		}
		case CalibrateType::ct_arc2lerance:
		{
			m_calibmode.reset(new Arc2lerance());
			break;
		}
		case CalibrateType::ct_accel2decel:
		{
			m_calibmode.reset(new Accel2decel());
			break;
		}
		case CalibrateType::ct_X_Y_Jerk:
		{
			m_calibmode.reset(new X_Y_Jerk());
			break;
		}
		case CalibrateType::ct_fanSpeed:
		{
			m_calibmode.reset(new FanSpeed());
			break;
		}
		default:
			m_calibmode = nullptr;
			break;
		}
	}

	void CalibrateSceneCreator::setValue(float _start, float _end, float _step, float bedTemp)
	{
		if (m_calibmode)
			m_calibmode->setValue(_start, _end, _step,bedTemp);
	}


	void CalibrateSceneCreator::setHighStep(float _highStep)
	{
		if (m_calibmode)
			m_calibmode->setHighValue(_highStep);
	}

	void CalibrateSceneCreator::setHighFlowValue(float _value)
	{
		if (m_calibmode)
			m_calibmode->setHighFlow(_value);
	}

	void CalibrateSceneCreator::setAccSpeed(float _accSpeed)
	{
		if (m_calibmode)
			m_calibmode->setAccSpeed(_accSpeed);
	}


	void CalibrateSceneCreator::setSpeed(float _speed)
	{
		if (m_calibmode)
			m_calibmode->setSpeed(_speed);
	}

	crslice::CrScene* CalibrateSceneCreator::create(ccglobal::Tracer* tracer)
	{
		crslice::CrScene* scene = new crslice::CrScene();
		return scene;
	}


	crslice2::CrScene* CalibrateSceneCreator::createOrca(ccglobal::Tracer* tracer)
	{
		if (m_calibmode)
			return m_calibmode->generateScene();

		return nullptr;
	}
}




