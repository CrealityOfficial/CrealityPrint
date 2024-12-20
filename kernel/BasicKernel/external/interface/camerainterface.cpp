#include "camerainterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "external/kernel/kernel.h"
#include "external/kernel/reuseablecache.h"

namespace creative_kernel
{
	qtuser_3d::CameraController* cameraController()
	{
		return getKernel()->cameraController();
	}

	qtuser_3d::ScreenCamera* visCamera()
	{
		return getKernel()->reuseableCache()->mainScreenCamera();
	}

	qtuser_3d::Ray visRay(const QPoint& point)
	{
		return getKernel()->reuseableCache()->mainScreenCamera()->screenRay(point);
	}

	float cameraScreenSpaceRatio(QVector3D position)
	{
		return visCamera()->screenSpaceRatio(position);
	}

	QVector3D cameraPosition()
	{
		return getKernel()->cameraController()->getViewPosition();
	}
}