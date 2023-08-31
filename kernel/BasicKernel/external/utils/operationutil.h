#ifndef creative_kernel_OPERATION_UTIL_H
#define creative_kernel_OPERATION_UTIL_H
#include "basickernelexport.h"
#include "trimesh2/Box.h"
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/math/ray.h"


enum class TMode
{
	null,
	x,
	y,
	z,
	xy,
	yz,
	zx,
	xyz,
	sp
};

namespace creative_kernel
{
	class ModelN;
}


extern float VALUE_MAX;


/*
 * 旋转和缩放
 */
BASIC_KERNEL_API void getTSProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m);

/*
 * 旋转
 */
BASIC_KERNEL_API void getRProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m);

/*
 * op_type:
 *		= 0  getTSProperPlane
 *		= 1  getRProperPlane
 */
BASIC_KERNEL_API QVector3D operationProcessCoord(const QPoint& point, creative_kernel::ModelN* model, int op_type, TMode m);


#endif // creative_kernel_OPERATION_UTIL_H