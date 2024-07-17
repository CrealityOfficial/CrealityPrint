#ifndef _CALIBRATESCENECREATOR_1684374260520_H
#define _CALIBRATESCENECREATOR_1684374260520_H
#include "crslice/crscene.h"
#include "crslice2/crscene.h"
#include <QtCore/QString>
#include "calibmode.h"
#include <memory>

class CalibrateSceneCreator : public crslice::SceneCreator,public crslice2::SceneCreator
{
public:
	CalibrateSceneCreator();
	~CalibrateSceneCreator();
	void setType(CalibrateType _type);
	void setValue(float _start,float _end,float _step);
protected:
	crslice::CrScene* create(ccglobal::Tracer* tracer) override;
	crslice2::CrScene* createOrca(ccglobal::Tracer* tracer) override;
protected:
	std::unique_ptr<CalibMode> m_calibmode;
};


#endif // _CALIBRATESCENECREATOR_1684374260520_H