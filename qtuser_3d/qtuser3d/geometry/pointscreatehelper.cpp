#include "pointscreatehelper.h"
#include "qtuser3d/geometry/bufferhelper.h"

namespace qtuser_3d
{
	PointsCreateHelper::PointsCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}
	
	PointsCreateHelper::~PointsCreateHelper()
	{
	}

	Qt3DRender::QGeometry* PointsCreateHelper::create(Qt3DCore::QNode* parent)
	{
		QVector<QVector3D> points;
		points.push_back(QVector3D(0.0f, 0.0f, 0.0f));
		return create(points, parent);
	}

	Qt3DRender::QGeometry* PointsCreateHelper::create(const QVector<QVector3D>& points, Qt3DCore::QNode* parent)
	{
		if (points.size() == 0) return nullptr;

		return create((float*)&points.at(0), (int)points.size(), parent);
	}

	Qt3DRender::QGeometry* PointsCreateHelper::create(float* points, int count, Qt3DCore::QNode* parent)
	{
		if (count == 0 || !points) return nullptr;

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)points, AttribueSlot::Position, count);
		return GeometryCreateHelper::create(parent, positionAttribute);
	}
}