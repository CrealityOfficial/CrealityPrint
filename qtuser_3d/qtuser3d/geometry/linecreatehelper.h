#ifndef QTUSER_3D_LINECREATEHELPER_1594893373349_H
#define QTUSER_3D_LINECREATEHELPER_1594893373349_H
#include "qtuser3d/geometry/geometrycreatehelper.h"

namespace qtuser_3d
{
	class QTUSER_3D_API LineCreateHelper : public GeometryCreateHelper
	{
		Q_OBJECT
	public:
		LineCreateHelper(QObject* parent = nullptr);
		virtual ~LineCreateHelper();

		static Qt3DRender::QGeometry* create(int num, float* position, float* color, Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_LINECREATEHELPER_1594893373349_H