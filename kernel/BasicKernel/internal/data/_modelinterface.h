#ifndef CREATIVE_KERNEL_INTERNAL_MODELINTERFACE_1592788083031_H
#define CREATIVE_KERNEL_INTERNAL_MODELINTERFACE_1592788083031_H
#include "data/header.h"
#include "data/undochange.h"
#include <QtCore/QList>

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;

	void _addModel(ModelN* model, bool update = false);
	void _modifySpace(const QList<ModelN*>& removes, const QList<ModelN*>& adds);
	void _setModelVisible(ModelN* model, bool visible);
	void _removeModel(ModelN* model, bool update = false);
	void _replaceModelsMesh(const QList<MeshChange>& changes);
	void _replaceModelMesh(const MeshChange& change, bool update = false);

	void _mixUnions(const QList<NUnionChangedStruct>& structs, bool redo);
	void _setModelRotation(ModelN* model, const QQuaternion& end, bool update = false);
	void _setModelScale(ModelN* model, const QVector3D& end, bool update = false);
	void _setModelPosition(ModelN* model, const QVector3D& end, bool update = false);

	void _requestUpdate();
	void _updateModel(ModelN* model);

	void _mirrorModels(const QList<NMirrorStruct>& changes);
	void _mirrorX(ModelN* model, bool update);
	void _mirrorY(ModelN* model, bool update);
	void _mirrorZ(ModelN* model, bool update);
	void _mirrorSet(ModelN* model, const QMatrix4x4& matrix, bool update);
}

#endif // CREATIVE_KERNEL_INTERNAL_MODELINTERFACE_1592788083031_H