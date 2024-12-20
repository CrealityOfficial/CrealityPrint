#ifndef CREATIVE_KERNEL_MODELGROUPINTERFACE_202403291418_H
#define CREATIVE_KERNEL_MODELGROUPINTERFACE_202403291418_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/undochange.h"
#include <QtCore/QList>
#include "cxkernel/data/modelndata.h"
#include "qtuser3d/trimesh2/conv.h"

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
	
	BASIC_KERNEL_API void mixUnionsModelGroup(const QList<NUnionChangedStruct>& structs, bool reversible = false);

	BASIC_KERNEL_API void moveModelGroup(ModelGroup* group, const QVector3D& start, const QVector3D& end, bool reversible = false);
	BASIC_KERNEL_API void moveModelGroups(const QList<ModelGroup*>& groups, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible = false);
	
	BASIC_KERNEL_API void rotateModelGroups(const QList<int>& groupIds, const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible = false);
	BASIC_KERNEL_API void mixTRModelGroups(const QList<int>& groupIds, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible = false);

	BASIC_KERNEL_API void scaleModelGroups(const QList<int>& groupIds, const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible = false);
	BASIC_KERNEL_API void mixTSModelGroups(const QList<int>& groupIds, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible = false);


	BASIC_KERNEL_API void setModelGroupsColor(const QList<ModelGroup*>& modelGroups, int colorIndex, bool reversible = false);
}

#endif // CREATIVE_KERNEL_MODELGROUPINTERFACE_202403291418_H
