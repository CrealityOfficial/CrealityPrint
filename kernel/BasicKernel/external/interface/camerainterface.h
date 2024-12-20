#ifndef CREATIVE_KERNEL_CAMERAINTERFACE_1592732397175_H
#define CREATIVE_KERNEL_CAMERAINTERFACE_1592732397175_H
#include "basickernelexport.h"
#include "qtuser3d/math/ray.h"

namespace qtuser_3d
{
	class ScreenCamera;
	class CameraController;
}

namespace creative_kernel
{
	BASIC_KERNEL_API qtuser_3d::CameraController* cameraController();

	BASIC_KERNEL_API qtuser_3d::ScreenCamera* visCamera();
	BASIC_KERNEL_API qtuser_3d::Ray visRay(const QPoint& point);

	BASIC_KERNEL_API float cameraScreenSpaceRatio(QVector3D position);
	BASIC_KERNEL_API QVector3D cameraPosition();
}
#endif // CREATIVE_KERNEL_CAMERAINTERFACE_1592732397175_H