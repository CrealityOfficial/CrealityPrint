#include "primitiveshapecache.h"
#include "qtuser3d/geometry/boxcreatehelper.h"
#include "qtuser3d/geometry/pointscreatehelper.h"
#include "qtuser3d/geometry/arrowcreatehelper.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"

namespace qtuser_3d
{
	Qt3DRender::QGeometry* PrimitiveShapeCache::create(const QString& name)
	{
		if (name == "box")
		{
			return BoxCreateHelper::create();  //盒子
		}
		else if (name == "box_nobottom")
		{
			return BoxCreateHelper::createNoBottom();  //没底座的盒子
		}
		else if (name == "point")
		{
			return  PointsCreateHelper::create();
		}
		else if (name == "arrow")
		{	// 箭头
			return BasicShapeCreateHelper::createInstructions(0.010, 1.0, 0.065, 0.20);//ArrowCreateHelper::create();
		}
		else if (name == "cylinder")
		{
			//圆柱
			return BasicShapeCreateHelper::createCylinder();
		}
		else if (name == "pen")
		{
			return BasicShapeCreateHelper::createPen();
		}
		else if (name == "scaleindicator")
		{
			return BasicShapeCreateHelper::createScaleIndicator(0.010, 1.0, 15, 0.20);
		}

		return nullptr;
	}

	Qt3DRender::QGeometry* createLinesPrimitive(const QString& name)
	{
		if (name == "box")
			return BoxCreateHelper::create();
		else if(name == "box_nobottom")
			return BoxCreateHelper::createNoBottom();
		else
			return nullptr;
	}

	Qt3DRender::QGeometry* createPointsPrimitive(const QString& name)
	{
		return nullptr;
	}

	Qt3DRender::QGeometry* createTrianglesPrimitive(const QString& name)
	{
		if (name == "cylinder")
			return BasicShapeCreateHelper::createCylinder();
		else if (name == "arrow")
			return BasicShapeCreateHelper::createInstructions(0.02f, 0.8f, 0.08f, 0.3f);
		else if (name == "scaleindicator")
			return BasicShapeCreateHelper::createScaleIndicator(0.02f, 0.8f, 36.0f, 0.15f);
		else
			return nullptr;
	}

	Qt3DRender::QGeometry* createTrianglesPrimitiveDLP(const QString& name)
	{
		if (name == "cylinder")
			return BasicShapeCreateHelper::createCylinder();
		else if (name == "arrow")
			return BasicShapeCreateHelper::createInstructionsDLP(0.02f, 0.8f, 0.08f, 0.3f);
		else if (name == "scaleindicator")
			return BasicShapeCreateHelper::createScaleIndicatorDLP(0.02f, 0.8f, 36.0f, 0.15f);
		else
			return nullptr;
	}
}