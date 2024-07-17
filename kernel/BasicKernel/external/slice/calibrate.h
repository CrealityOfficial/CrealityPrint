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

		Q_INVOKABLE void temp(float _start, float _end, float _step);//温度
		Q_INVOKABLE void lowflow();//流量粗调
		Q_INVOKABLE void highflow(int value);//流量细调
		Q_INVOKABLE void maxFlowValume(float _start, float _end, float _step);//最大体积流量
		Q_INVOKABLE void setVFA(float _start, float _end, float _step);//VFA
		Q_INVOKABLE void setPA(float _start, float _end, float _step, int _type);//pa  paLine :paTower :paPattern
		Q_INVOKABLE void retractionTower(float _start, float _end, float _step);//retraction
	protected:
		creative_kernel::SliceFlow* m_flow;
		CalibrateSceneCreator m_creator;


	};
}
#endif // _CALIBRATE_1603798647566_H