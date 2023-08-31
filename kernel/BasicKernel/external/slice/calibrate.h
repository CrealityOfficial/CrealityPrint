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

		Q_INVOKABLE void generate(float _start, float _end, float _step);
		Q_INVOKABLE void lowflow();
		Q_INVOKABLE void highflow(int value);
		Q_INVOKABLE void maxFlowValume(float _start, float _end, float _step);
		Q_INVOKABLE void VFA(float _start, float _end, float _step);
		Q_INVOKABLE void testType(int type);
		Q_INVOKABLE void paTower(float _start, float _end, float _step);
	protected:
		creative_kernel::SliceFlow* m_flow;
		CalibrateSceneCreator m_creator;


	};
}
#endif // _CALIBRATE_1603798647566_H