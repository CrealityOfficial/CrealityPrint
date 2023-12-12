#ifndef CREATIVE_KERNEL_MODELINTERFACE_1592788083031_H
#define CREATIVE_KERNEL_MODELINTERFACE_1592788083031_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/undochange.h"
#include <QtCore/QList>
#include "cxkernel/data/modelndata.h"

namespace qtuser_core
{
	class CXFileOpenAndSaveManager;
}

namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
	BASIC_KERNEL_API void openMeshFile();
	BASIC_KERNEL_API void appendResizeModel(ModelN* model);
	BASIC_KERNEL_API QList<ModelN*> getModelnsBySerialName(const QStringList& names);
	BASIC_KERNEL_API ModelN* getModelNBySerialName(const QString& name);
	BASIC_KERNEL_API void addModelLayout(ModelN* model);
	BASIC_KERNEL_API void addModel(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void modifySpace(const QList<ModelN*>& removes, const QList<ModelN*>& adds, bool reversible = false);
	BASIC_KERNEL_API void removeAllModel(bool reversible = false);
	BASIC_KERNEL_API void removeSelectionModels(bool reversible = false);

	BASIC_KERNEL_API QList<ModelN*> replaceModelsMesh(const QList<ModelN*>& models, const QList<cxkernel::ModelNDataPtr>& datas, bool reversible = false);

	BASIC_KERNEL_API void moveModel(ModelN* model, const QVector3D& start, const QVector3D& end, bool reversible = false);
	BASIC_KERNEL_API void moveModels(const QList<ModelN*>& models, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible = false);
	BASIC_KERNEL_API void bottomModel(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void bottomModels(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void bottomAllModels(bool reversible = false);
	BASIC_KERNEL_API void bottomSelectionModels(bool reversible = false);

	BASIC_KERNEL_API void alignModels(const QList<ModelN*>& models, const QVector3D& position, bool reversible = false);
	BASIC_KERNEL_API void alignAllModels(const QVector3D& position, bool reversible = false);
	BASIC_KERNEL_API void alignAllModels2BaseCenter(bool reversible = false);

	BASIC_KERNEL_API void mixTSModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QVector3D& sstart, const QVector3D& send, bool reversible = false);
	BASIC_KERNEL_API void mixTSModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible = false);

	BASIC_KERNEL_API void mixTRModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QQuaternion& rstart, const QQuaternion& rend, bool reversible = false);
	BASIC_KERNEL_API void mixTRModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible = false);

	BASIC_KERNEL_API void resetAllModels(bool reversible = false);

	BASIC_KERNEL_API void setModelRotation(ModelN* model, const QQuaternion& end, bool update = false);
	BASIC_KERNEL_API void setModelScale(ModelN* model, const QVector3D& end, bool update = false);
	BASIC_KERNEL_API void setModelPosition(ModelN* model, const QVector3D& end, bool update = false);
	BASIC_KERNEL_API void updateModel(ModelN* model);

	BASIC_KERNEL_API void rotateModelNormal2Bottom(ModelN* model, const QVector3D& normal);
	BASIC_KERNEL_API void rotateModelsNormal2Bottom(const QList<ModelN*>& model, const QVector3D& normal);

	BASIC_KERNEL_API void fillChangeStructs(QList<NUnionChangedStruct>& changes, bool start);

	BASIC_KERNEL_API void mixUnions(const QList<NUnionChangedStruct>& structs, bool reversible = false);

	BASIC_KERNEL_API void mirrorX(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void mirrorY(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void mirrorZ(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void mirrorReset(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void mirrorX(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void mirrorY(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void mirrorZ(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void mirrorReset(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void mirrorXSelections(bool reversible = false);
	BASIC_KERNEL_API void mirrorYSelections(bool reversible = false);
	BASIC_KERNEL_API void mirrorZSelections(bool reversible = false);
	BASIC_KERNEL_API void mirrorResetSelections(bool reversible = false);
	BASIC_KERNEL_API void mirrorModels(const QList<NMirrorStruct>& changes, bool reversible = false);
	BASIC_KERNEL_API QList<NMirrorStruct> generateMirrorChanges(const QList<ModelN*>& models, MirrorOperation operation);

	BASIC_KERNEL_API void setModelVisualMode(ModelVisualMode mode);

	BASIC_KERNEL_API void cloneModels(const QList<ModelN*>& models, int count, bool reversible = false);
	BASIC_KERNEL_API void cloneSelectionModels(int count, bool reversible = false);

	BASIC_KERNEL_API void setSelectionModelsNozzle(int nozzle);
	BASIC_KERNEL_API int getSelectionModelsNozzle();
	BASIC_KERNEL_API void setModelsNozzle(const QList<ModelN*>& models, int nozzle);

	BASIC_KERNEL_API QList<QVector3D> getModelHorizontalContour(ModelN* model);

	BASIC_KERNEL_API ModelN* createModelFromData(cxkernel::ModelNDataPtr data, ModelN* replaceModel = nullptr);
	BASIC_KERNEL_API void setMostRecentFile(QString filename);
}

#endif // CREATIVE_KERNEL_MODELINTERFACE_1592788083031_H
