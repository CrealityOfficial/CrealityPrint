#ifndef _CAPTURE_SETUP_DATA_H
#define _CAPTURE_SETUP_DATA_H
#include <QtCore/QObject>
#include <Qt3DRender/QGeometry>
#include <QMatrix4x4>
#include "qtuser3d/math/box3d.h"


namespace qtuser_3d
{

	struct CaptureSetupData
	{
		// capture entity geometry
		Qt3DRender::QGeometry* geometry = nullptr;

		//capture entity pos
		QMatrix4x4 entityPos;

		QString captureImageName;

		//pickerCamera view vector
		QVector3D viewCenter;

		//pickerCamera up vector
		QVector3D upVector;

		//pickerCamera eye vector
		QVector3D eyePosition;

		//pickerCamera projection matrix
		QMatrix4x4 projectionMatrix;
	};

}

#endif // _CAPTURE_SETUP_DATA_H