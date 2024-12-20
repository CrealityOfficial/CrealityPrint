#include "MeasureObjFactory.h"
#include "Point2PointMeasureObj.h"


AbstractMeasureObj* MeasureObjFactory::generateMeasureObj(int measure_type, QObject* object)
{
	switch (measure_type)
	{
	case 1:
		return new Point2PointMeasureObj(object);
	default:
		return nullptr;
	}
}

