#ifndef CREATIVE_KERNEL_MODELGROUP_1592037009137_H
#define CREATIVE_KERNEL_MODELGROUP_1592037009137_H
#include "basickernelexport.h"
#include "data/qnode3d.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "trimesh2/quaternion.h"
#include "Qt3DCore/qentity.h"
#include "cxkernel/data//meshdata.h"
#include "data/rawdata.h"

#include "crslice2/cacheslice.h"

namespace Qt3DRender
{
	class QTexture2D;
}

namespace us
{
	class USettings;
}

// this class objects are only created by ModelSpace or ModelSpaceUndo
namespace creative_kernel
{
	class ModelN;
	class ModelSpace;
	class ModelGroupEntity;
	class BASIC_KERNEL_API ModelGroup : public QNode3D
	{
		Q_OBJECT
		Q_PROPERTY(QString groupName READ groupName WRITE setGroupName NOTIFY groupNameChanged)		
		Q_PROPERTY(bool isVisible READ isVisible WRITE setIsVisible NOTIFY isVisibleChanged)
		Q_PROPERTY(bool hasColors READ hasColors NOTIFY colorsDataChanged)
		Q_PROPERTY(bool hasSeamsData READ hasSeamsData NOTIFY seamsDataChanged)
		Q_PROPERTY(bool hasSupportsData READ hasSupportsData NOTIFY supportsDataChanged)
		Q_PROPERTY(bool adaptiveLayerData READ adaptiveLayerData NOTIFY adaptiveLayerDataChanged)
		Q_PROPERTY(int defaultColorIndex READ defaultColorIndex WRITE setDefaultColorIndex NOTIFY defaultColorIndexChanged)
		Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged)
		Q_PROPERTY(bool flushIntoInfill READ flushIntoInfill WRITE setFlushIntoInfillEnabled NOTIFY flushIntoInfillChanged)
		Q_PROPERTY(bool flushIntoObjects READ flushIntoObjects WRITE setFlushIntoObjectsEnabled NOTIFY flushIntoObjectsChanged)
		Q_PROPERTY(bool flushIntoSupport READ flushIntoSupport WRITE setFlushIntoSupportEnabled NOTIFY flushIntoSupportChanged)
		Q_PROPERTY(bool supportEnabled READ supportEnabled WRITE setSupportEnabled NOTIFY supportEnabledChanged)
		Q_PROPERTY(float supportAngle READ supportAngle WRITE setSupportAngle NOTIFY supportAngleChanged)
		Q_PROPERTY(QString supportStructure READ supportStructure WRITE setSupportStructure NOTIFY supportStructureChanged)

		friend class ModelSpace;
	public:
		ModelGroup(QObject* parent = nullptr);
		virtual ~ModelGroup();

		void setEntity(ModelGroupEntity* entity);
		ModelGroupEntity* entity();
		Qt3DCore::QEntity* qEntity();

		void addModel(ModelN* model);
		void addModels(const QList<ModelN*>& models);

		void removeModel(ModelN* model);
		void removeModels(const QList<ModelN*>& models);

		void setChildrenVisibility(bool visibility);

		void dirty();
		void resetDirty();
		void setDirty(bool dirty);
		bool isDirty();

		bool needInit();
		void setInitPosition(QVector3D QPosition);

		void setGroupName(const QString& groupName);
		QString groupName();

		void setVisible(bool bVisible);

		void setObjectId(int64_t objId);
		int64_t getObjectId();

		Q_INVOKABLE QList<QObject*> modelObjects(int type);
		QList<ModelN*> models(int type);
		QList<ModelN*> normalModels();
		QList<ModelN*> models();
		QList<ModelN*> selectedModels();
		QList<ModelN*> unselectedModels();

		/* edited status */
		bool isVisible();
		void setIsVisible(bool isVisible);

		bool hasColors();
		bool hasSeamsData();
		bool hasSupportsData();
		bool adaptiveLayerData();

		int defaultColorIndex();
		void setDefaultColorIndex(int index);
		void discardDefaultColor();

		ModelN* getModelNByObjectId(int id);
		ModelN* getModelNByFixedId(int id);
		
		bool flushIntoInfill();
		void setFlushIntoInfillEnabled(bool enabled);

		bool flushIntoObjects();
		void setFlushIntoObjectsEnabled(bool enabled);

		bool flushIntoSupport();
		void setFlushIntoSupportEnabled(bool enabled);

		bool supportEnabled();
		bool cSupportEnabled(); //cached support enabled
		void setSupportEnabled(bool enabled);

		float supportAngle();
		float cSupportAngle();	//cached support angle
		void setSupportAngle(float angle);

		QString supportStructure(); 
		QString cSupportStructure(); //cached support structure
		void setSupportStructure(QString structure);

		int supportFilament();
		int cSupportFilament(); //cached support filament
		void setSupportFilament(int index);

		int supportInterfaceFilament();
		int cSupportInterfaceFilament();
		void setSupportInterfaceFilament(int index);

		us::USettings* setting();

		bool checkSettingDirty();
		bool checkSettingDirty(const QString& key);

		bool hasParameter(const QString& key);
		void getUsedSetting(us::USettings* setting);
		void recordSetting();
		bool checkSupportSettingState();

		bool selected();
		void setSelected(bool selected);

		trimesh::dbox3 localBoundingBox();
		trimesh::dbox3 globalBoundingBox();
		qtuser_3d::Box3D globalBox();
		virtual qtuser_3d::Box3D globalSpaceBox();
		QMatrix4x4 globalMatrix();
		QMatrix4x4 localMatrix();

		std::vector<trimesh::vec3> concave_path();
		trimesh::quaternion nestRotation();
		QVector3D zeroLocation();
		bool isHigherThanBottom();
		void checkAndSetBottom();
		int normalPartCount();
		int firstNormalPartObjectId();

		//convex hull of the group when rotate or scale 
		std::vector<trimesh::vec3> rsPath();
		const std::vector<trimesh::vec3>& sweepAreaPath();
		void copyOutlinePath(const std::vector<trimesh::vec3>& copyPath);

		virtual void onLocalScaleChanged(const trimesh::dvec3& newScale);
		virtual void onLocalPositionChanged(const trimesh::dvec3& newPosition);
		virtual void onLocalQuaternionChanged(const trimesh::dvec3& newQ);

		void updateNode();

		cxkernel::MeshDataPtr meshDataWithHullFaces(bool global = true);

		void layerBottom();

		void setOutlinePathDirty();
		void setOutlinePathInitPos(const trimesh::dvec3& pos);
		trimesh::dvec3 getOutlinePathInitPos();
		void resetOutlineState();

		//print by object
		void updateSweepAreaPath();
		void setOuterLinesColor(const QVector4D& color);
		void setOuterLinesVisibility(bool visible);
		void setNozzleRegionVisibility(bool visible);

		//use for variable layers
		void setLayerHeightProfile(const std::vector<double>& layerHeightProfile);
		const std::vector<double>& layerHeightProfile();
		void setLayersTexture(const QImage& image);

	protected:
		std::vector<trimesh::vec3> caculateOutlinePath();
		void showOutlinePath();
		void setChildrenPartStateDirty();
		void updateAllPartsIndex();

	private slots:
		void onModelNTypeChanged();
		void onModelNDirtyChanged();

	private slots:
		void onGlobalSettingsChanged(QObject* parameter_base, const QString& key);
		void onSettingsChanged(const QString& key,us::USetting* setting);
		void updateSetting();
		
	signals:
		void groupNameChanged();
		void isVisibleChanged();
		void colorsDataChanged();
		void seamsDataChanged();
		void supportsDataChanged();
		void adaptiveLayerDataChanged();
		void defaultColorIndexChanged();
		void selectedChanged();
		void modelNTypeChanged(QObject* model);
		void dirtyChanged();
		void formModified();
		void settingsChanged();
		void flushIntoInfillChanged();
		void flushIntoObjectsChanged();
		void flushIntoSupportChanged();
		void supportEnabledChanged();
		void supportAngleChanged();
		void supportStructureChanged();
		void supportSupportFilamentChanged();
		void supportInterfaceFilamentChanged();

	protected:
		us::USettings* m_lastUsedSetting;
		us::USettings* m_setting;

		QString m_groupName;
		int64_t m_objectid;

		bool m_isDirty { false };

		int m_extruderIndex { -1 };

		QList<ModelN*> m_componentModels;
		bool m_selected{ false };

		ModelGroupEntity* m_modelGroupEntity;

		bool m_outlinePathIsDirty{ true }; // outline path is dirty
		std::vector<trimesh::vec3> m_rsPath;  // the convex hull
		bool m_sweepPathDirty{ true };
		std::vector<trimesh::vec3> m_sweepPath;

		trimesh::dvec3 m_outlinePathInitPos;  // the outlinePath position when the group outlinePath need to recaculate
		bool m_outlineIsCopy{ false };

		trimesh::dvec3 m_collishPathInitPos;

		crslice2::CrSliceObject* m_crslice_object = nullptr;

		std::vector<double> m_layerHeightProfile;
		Qt3DRender::QTexture2D* m_layerHeightTexture = nullptr;

		bool m_supportEnabled;
		float m_supportAngle;
		QString m_supportStructure; 
		int m_supportFilament;
		int m_supportInterfaceFilament;

	};
}
#endif // CREATIVE_KERNEL_MODELGROUP_1592037009137_H