#ifndef _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
#define _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
#include "data/header.h"
#include "data/kernelenum.h"
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>
#include <QtGui/QMatrix4x4>
#include <QtCore/QList>
#include <QMetaType>

namespace creative_kernel
{
	struct V3Change
	{
		QVector3D start;
		QVector3D end;
	};

	struct QChanged
	{
		QQuaternion start;
		QQuaternion end;
	};

	struct NUnionChangedStruct
	{
		QString serialName;

		V3Change posChange;
		bool posActive;

		V3Change scaleChange;
		bool scaleActive;

		QChanged rotateChange;
		bool rotateActive;

		NUnionChangedStruct()
		{
			posActive = false;
			scaleActive = false;
			rotateActive = false;
		}
	};

	struct NMirrorStruct
	{
		QString serialName;
		MirrorOperation operation;
		QMatrix4x4 start;
		QMatrix4x4 end;
	};

	struct LayoutChangeInfo
	{
		QList<QString> moveModelsSerialNames;

		int prevPrintersCount;
		int nowPrintersCount;

		QList<QVector3D> startPoses;
		QList<QVector3D> endPoses;

		QList<QQuaternion> startQuaternions;
		QList<QQuaternion> endQuaternions;
		bool needPrinterLayout = true;
	};

}
Q_DECLARE_METATYPE(creative_kernel::LayoutChangeInfo)
#endif // _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
