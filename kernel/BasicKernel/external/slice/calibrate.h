#ifndef _CALIBRATE_1603798647566_H
#define _CALIBRATE_1603798647566_H
#include "data/interface.h"
#include "calibratescenecreator.h"

namespace creative_kernel
{
	class SliceFlow;
	class Calibrate : public QObject
	{
		Q_OBJECT
	public:
		Calibrate(creative_kernel::SliceFlow* _flow,QObject* parent = nullptr);
		virtual ~Calibrate();

		Q_INVOKABLE void temp(float _start, float _end, float _step,float bedTemp);//温度
		Q_INVOKABLE void lowflow();//流量粗调
		Q_INVOKABLE void highflow(int value);//流量细调
		Q_INVOKABLE void maxFlowValume(float _start, float _end, float _step,float bedTemp);//最大体积流量
		Q_INVOKABLE void setVFA(float _start, float _end, float _step, float bedTemp);//VFA
		Q_INVOKABLE void setPA(float _start, float _end, float _step, int _type, float bedTemp);//pa  paLine :paTower :paPattern
		Q_INVOKABLE void retractionTower(float _start, float _end, float _step, float bedTemp);//retraction

		//crality add
		Q_INVOKABLE void retractionTower_speed(float _start, float _end, float _step, float bedTemp);//回抽速度
		Q_INVOKABLE void limitSpeed(float _start, float _end, float _step, float _accSpeed, float bedTemp);// 极限速度
		Q_INVOKABLE void limitAcceleration(float _start, float _end, float _step, float _speed,float bedTemp);// 极限加速度
		Q_INVOKABLE void speedTower(float _start, float _end, float _step, float bedTemp);// 速度塔
		Q_INVOKABLE void accelerationTower(float _start, float _end, float _step, float bedTemp);// 加速度塔
		Q_INVOKABLE void arc2lerance(float _start, float _end, float _step);// 圆弧拟合公差
		Q_INVOKABLE void accel2decel(float _start, float _end, float _step, float _hight_step, float bedTemp);// 制动加速度
		Q_INVOKABLE void x_y_Jerk(float _start, float _end, float _step, float bedTemp);// 抖动速度塔
		Q_INVOKABLE void fanSpeed(float _start, float _end, float _step, float bedTemp);// 冷却风扇速度
	protected:
		creative_kernel::SliceFlow* m_flow;
		CalibrateSceneCreator m_creator;


	};
}
#endif // _CALIBRATE_1603798647566_H