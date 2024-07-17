#ifndef _CALIBRATEMODE_1603798647566_H
#define _CALIBRATEMODE_1603798647566_H
#include "crslice/crscene.h"
#include "crslice2/crscene.h"
#include <QtCore/QString>


enum class CalibrateType
{
	ct_temperature,
	ct_highflow,
	ct_lowflow,
	ct_paline,
	ct_patower,
	ct_maxvolumetric,
	ct_VFA,
	ct_retract
};

enum class PA_type
{
	pa_tower,
	pa_line,
	pa_pattern
};

class CalibMode
{
public:
	CalibMode();
	~CalibMode();
	void setType(CalibrateType type);
	void setValue(float _start, float _end, float _step);
	crslice2::CrScene* generateScene();
protected:
	crslice2::CrScene* createScene();
	void loadImage(crslice2::CrScene* scene);
	virtual void setConfig(crslice2::CrScene* scene);

	float m_start;
	float m_end;
	float m_step;
	CalibrateType m_type;
	QString m_calibPNGName;
private:
};

class Temp :public CalibMode
{
public:
	Temp();
	~Temp();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};

class VFA :public CalibMode
{
public:
	VFA();
	~VFA();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};


class LowFlow :public CalibMode
{
public:
	LowFlow();
	~LowFlow();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};

class HighFlow :public CalibMode
{
public:
	HighFlow();
	~HighFlow();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};

class MaxFlowValume :public CalibMode
{
public:
	MaxFlowValume();
	~MaxFlowValume();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};


class RetractionTower :public CalibMode
{
public:
	RetractionTower();
	~RetractionTower();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};

class PaLine :public CalibMode
{
public:
	PaLine();
	~PaLine();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};


class PaTower :public CalibMode
{
public:
	PaTower();
	~PaTower();
	void setConfig(crslice2::CrScene* scene);
protected:
private:
};



#endif // _CALIBRATEMODE_1603798647566_H