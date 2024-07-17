#include "calibratescenecreator.h"

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
		default:
			m_calibmode = nullptr;
			break;
	}
}

void CalibrateSceneCreator::setValue(float _start, float _end, float _step)
{
	if(m_calibmode)
		m_calibmode->setValue(_start, _end, _step);
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



