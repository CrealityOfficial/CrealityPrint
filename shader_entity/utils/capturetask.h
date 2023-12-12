#ifndef _CAPTURETASK_1596608263929_H
#define _CAPTURETASK_1596608263929_H
#include "shader_entity_export.h"
#include <QtCore/QObject>
#include <Qt3DRender/QGeometry>
#include <functional>
#include <QImage>
#include "qtuser3d/math/box3d.h"

typedef std::function<void(QString index, QImage& image)> captureCallbackFunc;

namespace qtuser_3d
{

	class  SHADER_ENTITY_API CaptureTask
	{
	public:
		CaptureTask();
		~CaptureTask();

		int m_state;
		int m_layer;
		qtuser_3d::Box3D m_box;

		int resolution_x;
		int resolution_y;

		captureCallbackFunc m_func;
	};

}

#endif // _CAPTURETASK_1596608263929_H