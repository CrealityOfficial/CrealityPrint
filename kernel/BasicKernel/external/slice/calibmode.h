#ifndef _CALIBRATEMODE_1603798647566_H
#define _CALIBRATEMODE_1603798647566_H
#include "crslice/crscene.h"
#include "crslice2/crscene.h"
#include <QtCore/QString>


namespace creative_kernel
{
	enum class CalibrateType
	{
		ct_temperature,
		ct_highflow,
		ct_lowflow,
		ct_paline,
		ct_patower,
		ct_maxvolumetric,
		ct_VFA,
		ct_retract,
		ct_retractspeed,
		ct_limitSpeed,
		ct_limitAcceleration,
		ct_speedTower,
		ct_accelerationTower,
		ct_arc2lerance,
		ct_accel2decel,
		ct_X_Y_Jerk,
		ct_fanSpeed
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
		void setValue(float _start, float _end, float _step, float bedTemp);
		void setHighValue(float _highStep);
		void setAccSpeed(float _accSpeed);
		void setHighFlow(float _highFlow);
		void setSpeed(float _addValue);
		crslice2::CrScene* generateScene();
	protected:
		crslice2::CrScene* createScene();
		void loadImage(crslice2::CrScene* scene);
		virtual void setConfig(crslice2::CrScene* scene);

		float m_start;
		float m_end;
		float m_step;
		float m_highStep;
		float m_highflow;
		int m_accSpeed;
		int m_speed;
		float m_bedTemp;
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

	class RetractionSpeed :public CalibMode
	{
	public:
		RetractionSpeed();
		~RetractionSpeed();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};


	class LimitSpeed :public CalibMode
	{
	public:
		LimitSpeed();
		~LimitSpeed();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

	class LimitAcceleration :public CalibMode
	{
	public:
		LimitAcceleration();
		~LimitAcceleration();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

	class SpeedTower :public CalibMode
	{
	public:
		SpeedTower();
		~SpeedTower();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};


	class AccelerationTower :public CalibMode
	{
	public:
		AccelerationTower();
		~AccelerationTower();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

	class Arc2lerance :public CalibMode
	{
	public:
		Arc2lerance();
		~Arc2lerance();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

	
	class Accel2decel :public CalibMode
	{
	public:
		Accel2decel();
		~Accel2decel();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

	class X_Y_Jerk :public CalibMode
	{
	public:
		X_Y_Jerk();
		~X_Y_Jerk();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

	class FanSpeed :public CalibMode
	{
	public:
		FanSpeed();
		~FanSpeed();
		void setConfig(crslice2::CrScene* scene);
	protected:
	private:
	};

}

#endif // _CALIBRATEMODE_1603798647566_H