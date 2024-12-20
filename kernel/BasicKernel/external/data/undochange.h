#ifndef _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
#define _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/rawdata.h"

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

	struct ColorChange
	{
		int start;
		int end;
	};

	struct NUnionChangedStruct
	{
		int64_t modelId;

		V3Change posChange;
		bool posActive;

		V3Change scaleChange;
		bool scaleActive;

		QChanged rotateChange;
		bool rotateActive;

		ColorChange colorChange;
		bool colorActive;

		int modelGroupId;

		NUnionChangedStruct()
		{
			posActive = false;
			scaleActive = false;
			rotateActive = false;
			colorActive = false;
		}
	};

	struct NodeChange
	{
		int64_t id;
		trimesh::xform start_xf;
		trimesh::xform end_xf;
	};

	struct ModelNPropertyMeshUndo
	{
		int64_t model_id;

		QString start_name;
		QString end_name;

		SharedDataID start_data_id;
		SharedDataID end_data_id;
	};

	struct LayoutChangeInfo
	{
		QList<int64_t> moveModelGroupIds;

		int printersCount;

		QList<QVector3D> endPoses;
		QList<QQuaternion> endQuaternions;
	};
	
	struct WipeTowerChange
	{
		int index;
		QVector3D start;
		QVector3D end;
	};
}
Q_DECLARE_METATYPE(creative_kernel::LayoutChangeInfo)
#endif // _creative_kernel_UNDOMATRIXCHANGE_1590115341614_H
