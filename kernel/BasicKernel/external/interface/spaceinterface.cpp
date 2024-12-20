#include "spaceinterface.h"
#include "kernel/kernel.h"
#include "data/modelspace.h"
#include "internal/utils/meshloadercenter.h"

#include "data/modeln.h"
#include "interface/reuseableinterface.h"

#include "interface/camerainterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "data/modelspaceundo.h"
#include "interface/printerinterface.h"

namespace creative_kernel
{
	ModelSpace* getModelSpace()
	{
		return getKernel()->modelSpace();
	}

	void traceSpace(SpaceTracer* tracer)
	{
		getModelSpace()->addSpaceTracer(tracer);
	}

	void unTraceSpace(SpaceTracer* tracer)
	{
		getModelSpace()->removeSpaceTracer(tracer);
	}

	QList<ModelGroup*> modelGroups()
	{
		ModelSpace* space = getKernel()->modelSpace();
		return space->modelGroups();
	}

	QList<ModelN*> modelns()
	{
		return getKernel()->modelSpace()->modelns();
	}

	std::vector<std::pair<std::string, std::string>> modelMetaData()
	{
		return getKernel()->modelMetaData();
	}

	void setModelMetaData(const std::vector<std::pair<std::string, std::string>>& data)
	{
		return getKernel()->setModelMetaData(data);
	}

	QString mainModelName()
	{
		return getKernel()->modelSpace()->mainModelName();
	}

	int modelNNum()
	{
		return getKernel()->modelSpace()->modelNNum();
	}

	bool haveModelN()
	{
		return getKernel()->modelSpace()->haveModelN();
	}

	ModelN* findModelBySerialName(const QString& serialName)
	{
		for (auto m : modelns())
		{
			if (m->getSerialName() == serialName)
				return m;
		}
		return NULL;
	}

	trimesh::dbox3 getBaseBoundingBox()
	{
		return getKernel()->modelSpace()->getBaseBoundingBox();
	}

	trimesh::dbox3 sceneBoundingBox()
	{
		return getKernel()->modelSpace()->sceneBoundingBox();
	}

	void setBaseBoundingBox(const trimesh::dbox3& box)
	{
		getKernel()->modelSpace()->setBaseBoundingBox(box);
	}

	void dirtyModelSpace()
	{
		getKernel()->modelSpace()->setSpaceDirty(true);
	}

	void clearModelSpaceDrity()
	{
		getKernel()->modelSpace()->setSpaceDirty(false);
	}

	bool modelSpaceDirty()
	{
		return getKernel()->modelSpace()->spaceDirty();
	}

	void clearProjectCache(bool clearModel)
	{
		if (clearModel)
		{
			resetPrinterNum();
			clearSpace();

			creative_kernel::resetIndicatorAngle();
		}	
		getKernel()->modelSpaceUndo()->clear();
	}
	
	ModelGroup* getModelGroupByObjectId(size_t objId)
	{
		return getModelSpace()->getModelGroupByObjectId(objId);
	}
	
	ModelN* getModelNByObjectId(size_t objId)
	{
		return getModelSpace()->getModelNByObjectId(objId);
	}

	ModelN* getModelNByFixedId(size_t fixedId)
	{
		return getModelSpace()->getModelNByFixedId(fixedId);
	}

	void setJoinedModelGroup(ModelGroup* group)
	{
		getKernel()->meshLoadCenter()->setJoinedModelGroup(group);
	}

	void setOpenedModelType(ModelNType type, ModelGroup* _model_group)
	{
		getKernel()->meshLoadCenter()->setOpenedModelType(type, _model_group);
	}

	void openMeshFile()
	{
		getKernel()->meshLoadCenter()->openMeshFile();
	}

	void openSpecifyMeshFile(const QString& file_name)
	{
		getKernel()->meshLoadCenter()->openSpecifyMeshFile(file_name);
	}

	void openGroupsMeshFile(const QList<QStringList>& group_files)
	{
		getKernel()->meshLoadCenter()->openGroupsMeshFile(group_files);
	}

	void createFromInput(const MeshCreateInput& create_input)
	{
		getKernel()->meshLoadCenter()->createFromInput(create_input);
	}

	void createFromInputs(const QList<MeshCreateInput>& create_inputs)
	{
		getKernel()->meshLoadCenter()->createFromInputs(create_inputs);
	}

	void modifyScene(const SceneCreateData& scene_data)
	{
		getModelSpace()->modifyScene(scene_data);
	}

	void clearSpace()
	{
		getModelSpace()->clearSpace();
	}

	void addModel(const ModelCreateData& model_data, ModelGroup* _model_group)
	{
		getModelSpace()->addModel(model_data, _model_group);
	}

	void addModels(const QList<ModelCreateData>& model_datas, ModelGroup* _model_group)
	{
		getModelSpace()->addModels(model_datas, _model_group);
	}

	void deleteModelGroup(ModelGroup* _model_group)
	{
		getModelSpace()->deleteModelGroup(_model_group);
	}

	void deleteModelGroups(const QList<ModelGroup*>& _model_groups)
	{
		getModelSpace()->deleteModelGroups(_model_groups);
	}

	void deleteModel(ModelN* model)
	{
		getModelSpace()->deleteModel(model);
	}

	void deleteModels(const QList<ModelN*>& models)
	{
		getModelSpace()->deleteModels(models);
	}

	void beginNodeSnap(const QList<ModelGroup*>& groups, const QList<ModelN*>& models)
	{
		getModelSpace()->beginNodeSnap(groups, models);
	}

	void endNodeSnap()
	{
		getModelSpace()->endNodeSnap();
	}

	void mirrorX(ModelGroup* _model_group, bool reversible)
	{
		getModelSpace()->mirrorX(_model_group, reversible);
	}

	void mirrorY(ModelGroup* _model_group, bool reversible)
	{
		getModelSpace()->mirrorY(_model_group, reversible);
	}

	void mirrorZ(ModelGroup* _model_group, bool reversible)
	{
		getModelSpace()->mirrorZ(_model_group, reversible);
	}

	void mirrorX(ModelN* _model, bool reversible)
	{
		getModelSpace()->mirrorX(_model, reversible);
	}

	void mirrorY(ModelN* _model, bool reversible)
	{
		getModelSpace()->mirrorY(_model, reversible);
	}

	void mirrorZ(ModelN* _model, bool reversible)
	{
		getModelSpace()->mirrorZ(_model, reversible);
	}

	void rotateXModels(double angle, bool reversible)
	{
		getModelSpace()->rotateXModels(angle, reversible);
	}

	void rotateYModels(double angle, bool reversible)
	{
		getModelSpace()->rotateYModels(angle, reversible);
	}

	void rotateZModels(double angle, bool reversible)
	{
		getModelSpace()->rotateZModels(angle, reversible);
	}

	void moveXModels(double distance, bool reversible)
	{
		getModelSpace()->moveXModels(distance, reversible);
	}

	void moveYModels(double distance, bool reversible)
	{
		getModelSpace()->moveYModels(distance, reversible);
	}

	void moveModel(ModelN* model, const QVector3D& start, const QVector3D& end, bool reversible)
	{
		getModelSpace()->moveModel(model, start, end, reversible);
	}

	void moveModels(const QList<ModelN*>& models, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible)
	{
		getModelSpace()->moveModels(models, starts, ends, reversible);
	}

	void resetAllModelsAndGroups(bool reversible)
	{
		getModelSpace()->resetAllModelsAndGroups(reversible);
	}

	void mergePosition(bool reversible)
	{
		getModelSpace()->mergePosition(reversible);
	}

	void mergePosition(const QList<ModelGroup*>& modelGroups, const trimesh::dbox3& base, bool reversible)
	{
		getModelSpace()->mergePosition(modelGroups, base, reversible);
	}

	void mixTSModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QVector3D& sstart, const QVector3D& send, bool reversible)
	{
		getModelSpace()->mixTSModel(model, sstart, tend, sstart, send, reversible);
	}

	void mixTSModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible)
	{
		getModelSpace()->mixTSModels(models, tStarts, tEnds, sStarts, sEnds, reversible);
	}

	void mixTRModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
		const QQuaternion& rstart, const QQuaternion& rend, bool reversible)
	{
		getModelSpace()->mixTRModel(model, tstart, tend, rstart, rend, reversible);
	}

	void mixTRModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
		const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible)
	{
		getModelSpace()->mixTRModels(models, tStarts, tEnds, rStarts, rEnds, reversible);
	}

	void mixUnions(const QList<NUnionChangedStruct>& changes, bool reversible)
	{
		getModelSpace()->mixUnions(changes, reversible);
	}

	void setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		getModelSpace()->setModelPosition(model, end, update);
	}

	void setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		getModelSpace()->setModelRotation(model, end, update);
	}

	void setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		getModelSpace()->setModelScale(model, end, update);
	}

	void updateModel(ModelN* model)
	{
		getModelSpace()->updateModel(model);
	}

	void fillChangeStructs(const QList<ModelN*>& models, QList<NUnionChangedStruct>& changes, bool start)
	{
		getModelSpace()->fillChangeStructs(models, changes, start);
	}

	void fillChangeStructs(const QList<ModelGroup*>& groups, QList<NUnionChangedStruct>& changes, bool start)
	{
		getModelSpace()->fillChangeStructs(groups, changes, start);
	}

	void bottomModel(ModelN* model, bool reversible)
	{
		getModelSpace()->bottomModel(model, reversible);
	}

	void bottomModels(const QList<ModelN*>& models, bool reversible)
	{
		getModelSpace()->bottomModels(models, reversible);
	}

	void bottomModelGroup(ModelGroup* group, bool reversible)
	{
		getModelSpace()->bottomModelGroup(group, reversible);
	}

	void bottomModelGroups(const QList<ModelGroup*>& groups, bool reversible)
	{
		getModelSpace()->bottomModelGroups(groups, reversible);
	}

	void setModelGroupsMaxFaceBottom(const QList<ModelGroup*>& modelGroups)
	{
		getModelSpace()->setModelGroupsMaxFaceBottom(modelGroups);
	}

	void setModelNsMaxFaceBottom(const QList<ModelN*>& models)
	{
		getModelSpace()->setModelNsMaxFaceBottom(models);
	}

	void setModelGroupsFaceBottom(const QList<ModelGroup*>& modelGroups, const trimesh::dvec3& normal)
	{
		getModelSpace()->setModelGroupsFaceBottom(modelGroups, normal);
	}

	void setModelNsFaceBottom(const QList<ModelN*>& models, const trimesh::dvec3& normal)
	{
		getModelSpace()->setModelNsFaceBottom(models, normal);
	}

	void _setModelRotation(ModelN* model, const QQuaternion& end, bool update)
	{
		getModelSpace()->_setModelRotation(model, end, update);
	}

	void _setModelScale(ModelN* model, const QVector3D& end, bool update)
	{
		getModelSpace()->_setModelScale(model, end, update);
	}

	void _setModelPosition(ModelN* model, const QVector3D& end, bool update)
	{
		getModelSpace()->_setModelPosition(model, end, update);
	}

	void updateSpaceNodeRender(ModelGroup* group, bool capture)
	{
		getModelSpace()->updateSpaceNodeRender(group, capture);
	}

	void updateSpaceNodeRender(const QList<ModelGroup*>& groups, bool capture)
	{
		getModelSpace()->updateSpaceNodeRender(groups, capture);
	}

	void updateSpaceNodeRender(ModelN* model, bool capture)
	{
		getModelSpace()->updateSpaceNodeRender(model, capture);
	}

	void updateSpaceNodeRender(const QList<ModelN*>& models, bool capture)
	{
		getModelSpace()->updateSpaceNodeRender(models, capture);
	}

	void notifySpaceChange(bool modelChanged)
	{
		getModelSpace()->notifySpaceChange(modelChanged);
	}

	void replaceModelNMesh(const ModelNPropertyMeshChange& change, bool reversible)
	{
		getModelSpace()->replaceModelNMesh(change, reversible);
	}

	void replaceModelNMeshes(const QList<ModelNPropertyMeshChange>& changes, bool reversible)
	{
		getModelSpace()->replaceModelNMeshes(changes, reversible);
	}
	
	void replaceModelsProperty(const QList<ModelNPropertyMeshUndo>& changes, bool reversible)
	{
		getModelSpace()->replaceModelsProperty(changes, reversible);
	}

	void addPart2Group(ModelDataPtr data, ModelNType mtype, ModelGroup* _model_group)
	{
		getModelSpace()->addPart2Group(data, mtype, _model_group);
	}

	void mirrorX(const QList<ModelN*>& models, bool reversible)
	{
		for (auto& m : models)
		{
			mirrorX(m, reversible);
		}
	}

	void mirrorY(const QList<ModelN*>& models, bool reversible)
	{
		for (auto& m : models)
		{
			mirrorY(m, reversible);
		}
	}

	void mirrorZ(const QList<ModelN*>& models, bool reversible)
	{
		for (auto& m : models)
		{
			mirrorZ(m, reversible);
		}
	}

	void mirrorX(const QList<ModelGroup*>& _model_groups, bool reversible)
	{
		for (auto g : _model_groups)
		{
			mirrorX(g, reversible);
		}
	}
	
	void mirrorY(const QList<ModelGroup*>& _model_groups, bool reversible)
	{
		for (auto g : _model_groups)
		{
			mirrorY(g, reversible);
		}
	}

	void mirrorZ(const QList<ModelGroup*>& _model_groups, bool reversible)
	{
		for (auto g : _model_groups)
		{
			mirrorZ(g, reversible);
		}
	}
}