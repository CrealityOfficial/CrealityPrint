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

		Q_INVOKABLE void temp(float _start, float _end, float _step,float bedTemp);//�¶�
		Q_INVOKABLE void lowflow();//�����ֵ�
		Q_INVOKABLE void highflow(int value);//����ϸ��
		Q_INVOKABLE void maxFlowValume(float _start, float _end, float _step,float bedTemp);//����������
		Q_INVOKABLE void setVFA(float _start, float _end, float _step, float bedTemp);//VFA
		Q_INVOKABLE void setPA(float _start, float _end, float _step, int _type, float bedTemp);//pa  paLine :paTower :paPattern
		Q_INVOKABLE void retractionTower(float _start, float _end, float _step, float bedTemp);//retraction

		//crality add
		Q_INVOKABLE void retractionTower_speed(float _start, float _end, float _step, float bedTemp);//�س��ٶ�
		Q_INVOKABLE void limitSpeed(float _start, float _end, float _step, float _accSpeed, float bedTemp);// �����ٶ�
		Q_INVOKABLE void limitAcceleration(float _start, float _end, float _step, float _speed,float bedTemp);// ���޼��ٶ�
		Q_INVOKABLE void speedTower(float _start, float _end, float _step, float bedTemp);// �ٶ���
		Q_INVOKABLE void accelerationTower(float _start, float _end, float _step, float bedTemp);// ���ٶ���
		Q_INVOKABLE void arc2lerance(float _start, float _end, float _step);// Բ����Ϲ���
		Q_INVOKABLE void accel2decel(float _start, float _end, float _step, float _hight_step, float bedTemp);// �ƶ����ٶ�
		Q_INVOKABLE void x_y_Jerk(float _start, float _end, float _step, float bedTemp);// �����ٶ���
		Q_INVOKABLE void fanSpeed(float _start, float _end, float _step, float bedTemp);// ��ȴ�����ٶ�
	protected:
		creative_kernel::SliceFlow* m_flow;
		CalibrateSceneCreator m_creator;


	};
}
#endif // _CALIBRATE_1603798647566_H