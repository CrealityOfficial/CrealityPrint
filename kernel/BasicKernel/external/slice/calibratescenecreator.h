#ifndef _CALIBRATESCENECREATOR_1684374260520_H
#define _CALIBRATESCENECREATOR_1684374260520_H
#include "crslice/crscene.h"

enum class CalibrateType
{
	ct_temperature,
	ct_highflow,
	ct_lowflow,
	ct_pressure,
	ct_maxvolumetric,
	ct_VFA
};

enum class PA_type
{
	ct_tower,
	ct_dash
};

class CalibrateSceneCreator : public crslice::SceneCreator
{
public:
	CalibrateSceneCreator();
	~CalibrateSceneCreator();

	void setType(CalibrateType type);
	void setValue(float _start,float _end,float _step);
	void setFlowHigh(bool high);
	void setHighFlowValue(int value) {m_highFlowValue = value;}
	PA_type m_paType;
protected:
	void reset();
	crslice::CrScene* create(ccglobal::Tracer* tracer) override;
protected:
	CalibrateType m_type;

	bool m_flowHigh;
	int m_highFlowValue;
	float m_baseX;
	float m_baseY;
	float m_gap;


	float m_start;
	float m_end;
	float m_step;
};


#endif // _CALIBRATESCENECREATOR_1684374260520_H