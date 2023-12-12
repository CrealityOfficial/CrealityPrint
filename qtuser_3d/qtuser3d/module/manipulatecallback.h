#ifndef MANIPULATECALLBACK_1595654189097_H
#define MANIPULATECALLBACK_1595654189097_H
#include <QtGui/QQuaternion>

namespace qtuser_3d
{
	class RotateCallback
	{
	public:
		virtual ~RotateCallback() {}
		virtual bool shouldStartRotate() { return true; }
		virtual void onStartRotate() = 0;
		virtual void onRotate(QQuaternion q) = 0;
		virtual void onEndRotate(QQuaternion q) = 0;
		virtual void setRotateAngle(QVector3D axis, float angle) = 0;

	};
}
#endif // MANIPULATECALLBACK_1595654189097_H