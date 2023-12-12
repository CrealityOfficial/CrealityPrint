#ifndef QTUSER_3D_POINTSCREATEHELPER_1594872813497_H
#define QTUSER_3D_POINTSCREATEHELPER_1594872813497_H
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include <QtGui/QVector3D>

namespace qtuser_3d
{
	class QTUSER_3D_API PointsCreateHelper : public GeometryCreateHelper
	{
		Q_OBJECT
	public:
		PointsCreateHelper(QObject* parent = nullptr);
		virtual ~PointsCreateHelper();

		static Qt3DRender::QGeometry* create(Qt3DCore::QNode* parent = nullptr);
		static Qt3DRender::QGeometry* create(const QVector<QVector3D>& points, Qt3DCore::QNode* parent = nullptr);
		static Qt3DRender::QGeometry* create(float* points, int count, Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_POINTSCREATEHELPER_1594872813497_H