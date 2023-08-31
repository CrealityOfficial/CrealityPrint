#ifndef _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
#define _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
#include "data/header.h"
#include "data/kernelenum.h"
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>
#include <QtGui/QMatrix4x4>
#include <QtCore/QList>

namespace creative_kernel
{
	class ModelN;

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
		ModelN* model;

		V3Change posChange;
		bool posActive;

		V3Change scaleChange;
		bool scaleActive;

		QChanged rotateChange;
		bool rotateActive;

		NUnionChangedStruct()
			: model(nullptr)
		{
			posActive = false;
			scaleActive = false;
			rotateActive = false;
		}
	};

	struct NMirrorStruct
	{
		ModelN* model;
		MirrorOperation operation;
		QMatrix4x4 start;
		QMatrix4x4 end;
	};

	struct MeshChange
	{
		ModelN* model;
		TriMeshPtr start;
		TriMeshPtr end;
		QString startName;
		QString endName;

		MeshChange()
			: model(nullptr)
		{
		}
	};
}

#endif // _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
