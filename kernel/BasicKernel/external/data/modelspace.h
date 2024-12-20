#ifndef creative_kernel_MODEL_SPACE_H
#define creative_kernel_MODEL_SPACE_H
#include "basickernelexport.h"
#include "data/interface.h"
#include "data/rawdata.h"
#include "data/undochange.h"

#include <QtCore/QList>

#include "data/modeln.h"
#include "data/modelgroup.h"

#include "crslice2/cacheslice.h"

#include <atomic>

namespace creative_kernel
{
	BASIC_KERNEL_API ModelCreateData generateModelCreateData(ModelN* model);
	BASIC_KERNEL_API GroupCreateData generateGroupCreateData(ModelGroup* _model_group, bool generateModel = true);

	class Printer;
	class ModelSpaceUndo;
	class SharedDataManager;
	class ModelNSelector;
	class BASIC_KERNEL_API ModelSpace : public QObject
	{
		Q_OBJECT
			Q_PROPERTY(int modelNNum READ modelNNum NOTIFY modelNNumChanged)

			friend class OrcaCacheSlice;
	public:
		ModelSpace(QObject* parent = nullptr);
		~ModelSpace();

		void initialize();
		void uninitialize();

		void addSpaceTracer(SpaceTracer* tracer);
		void removeSpaceTracer(SpaceTracer* tracer);

		void setSpaceUndo(ModelSpaceUndo* space_undo);
		void setSharedDataManager(SharedDataManager* shared_data_manager);
		void setModelSelector(ModelNSelector* model_selector);

		trimesh::dbox3 sceneBoundingBox();

		trimesh::dbox3 getBaseBoundingBox();
		void setBaseBoundingBox(const trimesh::dbox3& box);

		QList<ModelN*> modelns();

		QString mainModelName();
		int modelNNum();
		bool haveModelN();

		QList<ModelGroup*> modelGroups();

		void setSpaceDirty(bool spaceDirty);
		bool spaceDirty();

		Q_INVOKABLE bool haveModelsOutPlatform();
		Q_INVOKABLE bool modelOutPlatform(ModelN* amodel);

		void mergeGroups(const QList<ModelGroup*>& groups);
		void splitGroup2Objects(ModelGroup* _model_group);
		void splitModel2Parts(ModelN* _model);
		void splitModel2Objects(ModelN* _model);
		void cloneGroups(const QList<ModelGroup*>& groups, int num);
		void cloneModels(const QList<ModelN*>& models, int num);
		void addPart2Group(ModelDataPtr data, ModelNType mtype, ModelGroup* _model_group);

		void uniformName(ModelN* item);

		ModelGroup* getModelGroupByObjectId(int objId);
		ModelN* getModelNByObjectId(int objId);
		ModelN* getModelNByFixedId(int fixedId);

		void beginNodeSnap(const QList<ModelGroup*>& groups, const QList<ModelN*>& models);
		void endNodeSnap();
		void runSnap(const QList<ModelGroup*>& groups, const QList<ModelN*>& models, std::function<void()> func, bool snap = true);


		void mirrorX(ModelGroup* _model_group, bool reversible);
		void mirrorY(ModelGroup* _model_group, bool reversible);
		void mirrorZ(ModelGroup* _model_group, bool reversible);
		void mirrorX(ModelN* _model, bool reversible);
		void mirrorY(ModelN* _model, bool reversible);
		void mirrorZ(ModelN* _model, bool reversible);

		void rotateXModels(double angle, bool reversible = true);
		void rotateYModels(double angle, bool reversible = true);
		void rotateZModels(double angle, bool reversible = true);
		void rotateModels(trimesh::dvec3 axis, double angle, bool reversible = true);

		void moveXModels(double distance, bool reversible = true);
		void moveYModels(double distance, bool reversible = true);

		void moveModel(ModelN* model, const QVector3D& start, const QVector3D& end, bool reversible = false);
		void moveModels(const QList<ModelN*>& models, const QList<QVector3D>& starts, const QList<QVector3D>& ends, bool reversible = false);

		void mixTSModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
			const QVector3D& sstart, const QVector3D& send, bool reversible = false);
		void mixTSModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
			const QList<QVector3D>& sStarts, const QList<QVector3D>& sEnds, bool reversible = false);
		void mixTRModel(ModelN* model, const QVector3D& tstart, const QVector3D& tend,
			const QQuaternion& rstart, const QQuaternion& rend, bool reversible = false);
		void mixTRModels(const QList<ModelN*>& models, const QList<QVector3D>& tStarts, const QList<QVector3D>& tEnds,
			const QList<QQuaternion>& rStarts, const QList<QQuaternion>& rEnds, bool reversible = false);
		void setModelRotation(ModelN* model, const QQuaternion& end, bool update = false);
		void setModelScale(ModelN* model, const QVector3D& end, bool update = false);
		void setModelPosition(ModelN* model, const QVector3D& end, bool update = false);
		void updateModel(ModelN* model);
		void fillChangeStructs(const QList<ModelN*>& models, QList<NUnionChangedStruct>& changes, bool start);
		void fillChangeStructs(const QList<ModelGroup*>& groups, QList<NUnionChangedStruct>& changes, bool start);
		void mixUnions(const QList<NUnionChangedStruct>& structs, bool reversible = false);
		void _mixUnions(const QList<NUnionChangedStruct>& structs, bool redo);

		void resetAllModelsAndGroups(bool reversible = true);
		void mergePosition(bool reversible = true);
		void mergePosition(const QList<ModelGroup*>& modelGroups, const trimesh::dbox3& base, bool reversible);

		void bottomModel(ModelN* model, bool reversible = true);
		void bottomModels(const QList<ModelN*>& models, bool reversible = true);
		void bottomModelGroup(ModelGroup* group, bool reversible = true);
		void bottomModelGroups(const QList<ModelGroup*>& groups, bool reversible = true);

		void setModelGroupsMaxFaceBottom(const QList<ModelGroup*>& modelGroups);
		void setModelNsMaxFaceBottom(const QList<ModelN*>& models);
		void setModelGroupsFaceBottom(const QList<ModelGroup*>& modelGroups, const trimesh::dvec3& normal);
		void setModelNsFaceBottom(const QList<ModelN*>& models, const trimesh::dvec3& normal);
		void setModelNFaceBottom(ModelN* model, const trimesh::dvec3& normal);
		void setModelGroupFaceBottom(ModelGroup* modelGroup, const trimesh::dvec3& normal);

		void _setModelRotation(ModelN* model, const QQuaternion& end, bool update = false);
		void _setModelScale(ModelN* model, const QVector3D& end, bool update = false);
		void _setModelPosition(ModelN* model, const QVector3D& end, bool update = false);

		void updateSpaceNodeRender(ModelGroup* group, bool capture = false);
		void updateSpaceNodeRender(const QList<ModelGroup*>& groups, bool capture = false);
		void updateSpaceNodeRender(ModelN* model, bool capture = false);
		void updateSpaceNodeRender(const QList<ModelN*>& models, bool capture = false);

		void renameModelGroup(ModelGroup* _model_group, const QString& name);
		void renameModelN(ModelN* _model, const QString& name);
		void replaceModelNMesh(const ModelNPropertyMeshChange& change, bool reversible = false);
		void replaceModelNMeshes(const QList<ModelNPropertyMeshChange>& changes, bool reversible = false);
		void replaceModelsProperty(const QList<ModelNPropertyMeshUndo>& changes, bool reversible = false);

		void modifyScene(const SceneCreateData& scene_data);
		void clearSpace();
		void addModel(const ModelCreateData& model_data, ModelGroup* _model_group = nullptr);
		void addModels(const QList<ModelCreateData>& model_datas, ModelGroup* _model_group = nullptr);
		void deleteModelGroup(ModelGroup* _model_group);
		void deleteModelGroups(const QList<ModelGroup*>& _model_groups);
		void deleteModel(ModelN* model);
		void deleteModels(const QList<ModelN*>& models);
		void deleteUnion(const QList<ModelGroup*>& _model_groups, const QList<ModelN*>& models);

		void printSpace();

		void notifySpaceChange(bool modelChanged = false);

		void checkModelGroupSupportGenerator(ModelGroup* group = NULL);

		void update_crslice2_matrix(const trimesh::xform& offset_xf);
		void update_crslice2_parameter();
	private:
		void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds);
		void onModelTypeChanged(ModelN* model);
		
	protected:
		friend class SpaceSerialCommand;
		friend class ModelNPropertyMeshCommand;
		void addModelGroup_ur(ModelGroup* _model_group);
		void addModelGroups_ur(const QList<ModelGroup*>& _model_groups);
		void removeModelGroup_ur(ModelGroup* _model_group);
		void removeModelGroups_ur(const QList<ModelGroup*>& _model_groups);
		void addModel_ur(ModelN* model, ModelGroup* _model_group);
		void addModels_ur(const QList<ModelN*>& models, ModelGroup* _model_group);
		void removeModel_ur(ModelN* model, ModelGroup* _model_group);
		void removeModels_ur(const QList<ModelN*>& models, ModelGroup* _model_group);

		ModelN* createModel(QObject* parent, int64_t fixedId = -1, int64_t id = -1);
		ModelGroup* createModelGroup(QObject* parent, int64_t id = -1);
		int64_t generateUniqueId();
		int64_t generateFixedId();

	public slots:
		void onSelectedPrinterChanged(Printer* printer);
	
	private slots:
		void onModelNTypeChanged(QObject* model);
		void onSupportEnabledChanged();
		void onSelectedPrinterModelsChanged();

	signals:
		void signalVisualChanged(bool capture);
		void modelNNumChanged();
		void sigAddModel();
		void sigRemoveModel();

	private:
		ModelSpaceUndo* m_space_undo;
		SharedDataManager* m_shared_data_manager;
		ModelNSelector* m_model_selector;
		Printer* m_selectedPrinter {NULL};

		trimesh::dbox3 m_baseBoundingBox;
		QList<SpaceTracer*> m_spaceTracers;

		bool m_spaceDirty;

		QList<ModelGroup*> m_sceneModelGroups;
		std::atomic<int64_t> m_uniqueID;
		std::atomic<int64_t> m_uniqueFixedID;

		QList<NodeChange> m_nodes_snap;

		crslice2::CrSliceModel m_crslice_model;
	};
}
#endif // creative_kernel_MODEL_SPACE_H