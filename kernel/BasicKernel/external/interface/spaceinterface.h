#ifndef CREATIVE_KERNEL_SPACEINTERFACE_1592880998363_H
#define CREATIVE_KERNEL_SPACEINTERFACE_1592880998363_H
#include "basickernelexport.h"
#include <Qt3DCore/QEntity>
#include "data/modelspace.h"
#include "data/modelgroup.h"

namespace creative_kernel
{
	BASIC_KERNEL_API ModelSpace* getModelSpace();
	BASIC_KERNEL_API void traceSpace(SpaceTracer* tracer);
	BASIC_KERNEL_API void unTraceSpace(SpaceTracer* tracer);

	BASIC_KERNEL_API QList<ModelGroup*> modelGroups();
	BASIC_KERNEL_API QList<ModelN*> modelns();

    BASIC_KERNEL_API std::vector<std::pair<std::string, std::string>> modelMetaData();
    BASIC_KERNEL_API void setModelMetaData(const std::vector<std::pair<std::string, std::string>>& data);

	BASIC_KERNEL_API QString mainModelName();
	BASIC_KERNEL_API int modelNNum();
	BASIC_KERNEL_API bool haveModelN();
	BASIC_KERNEL_API ModelN* findModelBySerialName(const QString& serialName);

	BASIC_KERNEL_API trimesh::dbox3 getBaseBoundingBox();
	BASIC_KERNEL_API void setBaseBoundingBox(const trimesh::dbox3& box);

	BASIC_KERNEL_API trimesh::dbox3 sceneBoundingBox();      // changed when scene modified , trait it again

	BASIC_KERNEL_API void dirtyModelSpace();
	BASIC_KERNEL_API void clearModelSpaceDrity();
	BASIC_KERNEL_API bool modelSpaceDirty();

	BASIC_KERNEL_API void clearProjectCache(bool clearModel = true);

	BASIC_KERNEL_API ModelGroup* getModelGroupByObjectId(size_t objId);
	BASIC_KERNEL_API ModelN* getModelNByObjectId(size_t objId);
	BASIC_KERNEL_API ModelN* getModelNByFixedId(size_t fixedId);

	BASIC_KERNEL_API void setJoinedModelGroup(ModelGroup* group);
	BASIC_KERNEL_API void setOpenedModelType(ModelNType type, ModelGroup* _model_group = nullptr);  //just for once open
	BASIC_KERNEL_API void openMeshFile();
	BASIC_KERNEL_API void openSpecifyMeshFile(const QString& file_name);
	BASIC_KERNEL_API void openGroupsMeshFile(const QList<QStringList>& group_files);
	BASIC_KERNEL_API void createFromInput(const MeshCreateInput& create_input);
	BASIC_KERNEL_API void createFromInputs(const QList<MeshCreateInput>& create_inputs);

	BASIC_KERNEL_API void modifyScene(const SceneCreateData& scene_data);
	BASIC_KERNEL_API void clearSpace();
	BASIC_KERNEL_API void addModel(const ModelCreateData& model_data, ModelGroup* _model_group = nullptr);
	BASIC_KERNEL_API void addModels(const QList<ModelCreateData>& model_datas, ModelGroup* _model_group = nullptr);
	BASIC_KERNEL_API void deleteModelGroup(ModelGroup* _model_group);
	BASIC_KERNEL_API void deleteModelGroups(const QList<ModelGroup*>& _model_groups);
	BASIC_KERNEL_API void deleteModel(ModelN* model);
	BASIC_KERNEL_API void deleteModels(const QList<ModelN*>& models);

	BASIC_KERNEL_API void beginNodeSnap(const QList<ModelGroup*>& groups, const QList<ModelN*>& models);
	BASIC_KERNEL_API void endNodeSnap();

	BASIC_KERNEL_API void mirrorX(ModelGroup* _model_group, bool reversible = false);
	BASIC_KERNEL_API void mirrorY(ModelGroup* _model_group, bool reversible = false);
	BASIC_KERNEL_API void mirrorZ(ModelGroup* _model_group, bool reversible = false);

	BASIC_KERNEL_API void mirrorX(const QList<ModelGroup*>& _model_groups, bool reversible = false);
	BASIC_KERNEL_API void mirrorY(const QList<ModelGroup*>& _model_groups, bool reversible = false);
	BASIC_KERNEL_API void mirrorZ(const QList<ModelGroup*>& _model_groups, bool reversible = false);


	BASIC_KERNEL_API void mirrorX(ModelN* _model, bool reversible = false);
	BASIC_KERNEL_API void mirrorY(ModelN* _model, bool reversible = false);
	BASIC_KERNEL_API void mirrorZ(ModelN* _model, bool reversible = false);

	BASIC_KERNEL_API void mirrorX(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void mirrorY(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void mirrorZ(const QList<ModelN*>& models, bool reversible = false);


	BASIC_KERNEL_API void rotateXModels(double angle, bool reversible = true);
	BASIC_KERNEL_API void rotateYModels(double angle, bool reversible = true);
	BASIC_KERNEL_API void rotateZModels(double angle, bool reversible = true);
	BASIC_KERNEL_API void moveXModels(double distance, bool reversible = true);
	BASIC_KERNEL_API void moveYModels(double distance, bool reversible = true);

	BASIC_KERNEL_API void moveModel(ModelN* model, const QVector3D& start, const QVector3D& end, bool reversible = false);
	BASIC_KERNEL_API void moveModels(const QList<ModelN*>& models, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible = false);
	BASIC_KERNEL_API void mixTSModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QVector3D& sstart, const QVector3D& send, bool reversible = false);
	BASIC_KERNEL_API void mixTSModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible = false);
	BASIC_KERNEL_API void mixTRModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QQuaternion& rstart, const QQuaternion& rend, bool reversible = false);
	BASIC_KERNEL_API void mixTRModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible = false);
	BASIC_KERNEL_API void setModelRotation(ModelN* model, const QQuaternion& end, bool update = false);
	BASIC_KERNEL_API void setModelScale(ModelN* model, const QVector3D& end, bool update = false);
	BASIC_KERNEL_API void setModelPosition(ModelN* model, const QVector3D& end, bool update = false);
	BASIC_KERNEL_API void updateModel(ModelN* model);

	BASIC_KERNEL_API void fillChangeStructs(const QList<ModelN*>& models, QList<NUnionChangedStruct>& changes, bool start);
	BASIC_KERNEL_API void fillChangeStructs(const QList<ModelGroup*>& groups, QList<NUnionChangedStruct>& changes, bool start);
	BASIC_KERNEL_API void mixUnions(const QList<NUnionChangedStruct>& structs, bool reversible = false);

	BASIC_KERNEL_API void resetAllModelsAndGroups(bool reversible = false);
	/*
	* reset the model position to the original position saved in stl file; each modelGroup allow only one model part inside
	*/
	BASIC_KERNEL_API void mergePosition(bool reversible = true);
	BASIC_KERNEL_API void mergePosition(const QList<ModelGroup*>& modelGroups, const trimesh::dbox3& base, bool reversible = true);

	BASIC_KERNEL_API void bottomModel(ModelN* model, bool reversible = false);
	BASIC_KERNEL_API void bottomModels(const QList<ModelN*>& models, bool reversible = false);
	BASIC_KERNEL_API void bottomModelGroup(ModelGroup* group, bool reversible = false);
	BASIC_KERNEL_API void bottomModelGroups(const QList<ModelGroup*>& groups, bool reversible = false);

	BASIC_KERNEL_API void setModelGroupsMaxFaceBottom(const QList<ModelGroup*>& modelGroups);
	BASIC_KERNEL_API void setModelNsMaxFaceBottom(const QList<ModelN*>& models);
	BASIC_KERNEL_API void setModelGroupsFaceBottom(const QList<ModelGroup*>& modelGroups, const trimesh::dvec3& normal);
	BASIC_KERNEL_API void setModelNsFaceBottom(const QList<ModelN*>& models, const trimesh::dvec3& normal);

	BASIC_KERNEL_API void _setModelRotation(ModelN* model, const QQuaternion& end, bool update = false);
	BASIC_KERNEL_API void _setModelScale(ModelN* model, const QVector3D& end, bool update = false);
	BASIC_KERNEL_API void _setModelPosition(ModelN* model, const QVector3D& end, bool update = false);

	void updateSpaceNodeRender(ModelGroup* group, bool capture = false);
	void updateSpaceNodeRender(const QList<ModelGroup*>& groups, bool capture = false);
	void updateSpaceNodeRender(ModelN* model, bool capture = false);
	void updateSpaceNodeRender(const QList<ModelN*>& models, bool capture = false);
	BASIC_KERNEL_API void notifySpaceChange(bool modelChanged = false);

	BASIC_KERNEL_API void replaceModelNMesh(const ModelNPropertyMeshChange& change, bool reversible = false);
	BASIC_KERNEL_API void replaceModelNMeshes(const QList<ModelNPropertyMeshChange>& changes, bool reversible = false);
	BASIC_KERNEL_API void replaceModelsProperty(const QList<ModelNPropertyMeshUndo>& changes, bool reversible = false);

	BASIC_KERNEL_API void addPart2Group(ModelDataPtr data, ModelNType mtype, ModelGroup* _model_group);
}
#endif // CREATIVE_KERNEL_SPACEINTERFACE_1592880998363_H