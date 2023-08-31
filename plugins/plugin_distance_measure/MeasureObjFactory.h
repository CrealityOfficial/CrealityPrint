#ifndef _MEASURE_OBJ_FACTORY_H__
#define _MEASURE_OBJ_FACTORY_H__

#include "AbstractMeasureObj.h"
#include <QObject>

class MeasureObjFactory
{
public:
	static AbstractMeasureObj* generateMeasureObj(int measure_type, QObject* object);
};

 

#endif
