#ifndef CREATIVE_KERNEL_INTERNAL_MODELGROUPINTERFACE_202403291431_H
#define CREATIVE_KERNEL_INTERNAL_MODELGROUPINTERFACE_202403291431_H
#include "data/header.h"
#include "data/undochange.h"
#include <QtCore/QList>

namespace creative_kernel
{
	class ModelGroup;

	void _mixUnionsModelGroup(const QList<NUnionChangedStruct>& structs, bool redo);
	void _setModelGroupRotation(ModelGroup* modelGroup, const QQuaternion& end, bool update = false);
	void _setModelGroupScale(ModelGroup* modelGroup, const QVector3D& end, bool update = false);
	void _setModelGroupPosition(ModelGroup* modelGroup, const QVector3D& end, bool update = false);
	void _setModelGroupColor(ModelGroup* modelGroup, const int& end);

}

#endif // CREATIVE_KERNEL_INTERNAL_MODELGROUPINTERFACE_202403291431_H