#ifndef QTUSER_3D_ARROWCREATEHELPER_1595046356033_H
#define QTUSER_3D_ARROWCREATEHELPER_1595046356033_H
#include "qtuser3d/geometry/geometrycreatehelper.h"

namespace qtuser_3d
{
	class QTUSER_3D_API ArrowCreateHelper : public GeometryCreateHelper
	{
		Q_OBJECT
	public:
		ArrowCreateHelper(QObject* parent = nullptr);
		virtual ~ArrowCreateHelper();

		static Qt3DRender::QGeometry* create(Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_ARROWCREATEHELPER_1595046356033_H