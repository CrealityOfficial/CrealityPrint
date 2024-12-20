#ifndef CREATIVE_KERNEL_MODELN_1592181954934_H
#define CREATIVE_KERNEL_MODELN_1592181954934_H

#include <set>

#include "data/header.h"
#include "data/kernelenum.h"
#include "data/qnode3d.h"

#include "cxkernel/data/nestdata.h"
#include "cxkernel/data/meshdata.h"

#include "qtuser3d/math/ray.h"
#include "qtusercore/module/progressor.h"
#include <Qt3DRender/QGeometry>
#include <vector>
#include "shareddatamanager/shareddataid.h"
#include "cxkernel/data/meshdata.h"
#include "shareddatamanager/renderdata.h"
#include "shareddatamanager/shareddatatracer.h"

#include "crslice2/cacheslice.h"

class FontMeshWrapper;

namespace us
{
	class USettings;
	class USetting;
}

namespace qtuser_3d
{
	class XEffect;
};

namespace creative_kernel
{
	class ModelNEntity;
	class ModelGroup;
	class ModelSpace;
	class BASIC_KERNEL_API ModelN : public QNode3D, public SharedDataTracer
	{
		Q_OBJECT
		Q_PROPERTY(bool flushIntoInfill READ flushIntoInfill WRITE setFlushIntoInfillEnabled NOTIFY flushIntoInfillChanged)
		Q_PROPERTY(bool flushIntoObjects READ flushIntoObjects WRITE setFlushIntoObjectsEnabled NOTIFY flushIntoObjectsChanged)
		Q_PROPERTY(bool flushIntoSupport READ flushIntoSupport WRITE setFlushIntoSupportEnabled NOTIFY flushIntoSupportChanged)
		Q_PROPERTY(bool supportEnabled READ supportEnabled WRITE setSupportEnabled NOTIFY supportEnabledChanged)
		Q_PROPERTY(float supportAngle READ supportAngle WRITE setSupportAngle NOTIFY supportAngleChanged)
		Q_PROPERTY(QString supportStructure READ supportStructure WRITE setSupportStructure NOTIFY supportStructureChanged)
		Q_PROPERTY(bool hasColors READ hasColors NOTIFY colorsDataChanged)
		Q_PROPERTY(bool hasSeamsData READ hasSeamsData NOTIFY seamsDataChanged)
		Q_PROPERTY(bool hasSupportsData READ hasSupportsData NOTIFY supportsDataChanged)
		Q_PROPERTY(bool adaptiveLayerData READ adaptiveLayerData NOTIFY adaptiveLayerDataChanged)
		Q_PROPERTY(bool isRelief READ isRelief CONSTANT)
		Q_PROPERTY(int modelNType READ modelNTypeInt NOTIFY modelNTypeChanged)

		//ui
		Q_PROPERTY(bool isVisible READ isVisible WRITE setVisibility NOTIFY isVisibleChanged)
		Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
		Q_PROPERTY(int defaultColorIndex READ defaultColorIndex WRITE setDefaultColorIndex NOTIFY defaultColorIndexChanged)

	public:
		friend class ModelGroup;
		friend class ModelSpace;
		ModelN(QObject* parent = nullptr);
		virtual ~ModelN();

		void setEntity(ModelNEntity* entity);
		ModelNEntity* entity();
		Qt3DCore::QEntity* qEntity();

        bool needInit();
		QVector3D& GetInitPosition();
		void SetInitPosition(QVector3D QPosition);

		void rotateNormal2Bottom(const QVector3D& normal, QVector3D& position, QQuaternion& rotate);

		us::USettings* setting();
		void setsetting(us::USettings* modelsetting);
		bool hasParameter(const QString& key);
		void getUsedSetting(us::USettings* setting);
		void recordSetting();
		bool checkSettingDirty();
		bool checkSettingDirty(const QString& key);

		void setState(int state);
		int getState();

		void setVisibility(bool visibility);
		bool isVisible();

		//Variable Layer Height
		void useVariableLayerHeightEffect();
		void useDefaultModelEffect();

		void setNozzle(int nozzle);
		int nozzle();

		void SetModelViewClrState(qtuser_3d::ControlState statevalue, bool boxshow);

		trimesh::TriMesh* mesh();
		TriMeshPtr meshptr();
		TriMeshPtr globalMesh(bool needMergeColorMesh = true);
		TriMeshPtr localMesh(bool needMergeColorMesh = true);

		void setModelNType(ModelNType type);
		ModelNType modelNType();
		int modelNTypeInt();

		bool needRepair();

		void setSharedData(SharedDataID id);
		cxkernel::MeshDataPtr data();

		virtual void beginUpdateRender();
		virtual void updateRender();
		virtual void endUpdateRender();

		virtual void onDiscardMaterial(int materialIndex);

		void setRenderData();
		RenderDataPtr renderData();

		QString fileName() const;
		void setFileName(const QString& fileName);

		QString name() const;
		void setName(const QString& name);

		/* relief */
		bool isRelief() const;
		QString reliefText() const;

		FontMeshWrapper* fontMesh();
		void setFontMesh(FontMeshWrapper* fontMesh);
		void setFontMesh(std::string fontMeshStr);

		std::string fontMeshString(bool isClone = false);

		QString description() const;
		void setDescription(const QString& description);

		trimesh::dbox3 localBoundingBox();
		trimesh::dbox3 globalBoundingBox();
		qtuser_3d::Box3D globalSpaceBox();
		QMatrix4x4 globalMatrix();
		QMatrix4x4 localMatrix();
		trimesh::xform globalDMatrix();  // global matrix of double type
		QMatrix4x4 modelNormalMatrix();

		trimesh::dvec3 componentOffset();
		virtual void onLocalScaleChanged(const trimesh::dvec3& newScale);
		virtual void onLocalPositionChanged(const trimesh::dvec3& newPosition);
		virtual void onLocalQuaternionChanged(const trimesh::dvec3& newQ);

		bool rayCheck(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D* normal, bool accurate = false);
		//global normal ,normal is not normalized , represent the average lenght of edges
		bool rayCheckEx(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D& outNormal);

		void convex(std::vector<trimesh::vec3>& datas, bool origin = false);
		QList<QVector3D> qConvex(bool toOrigin = false);

		TriMeshPtr createGlobalMesh();
		TriMeshPtr createGlobalMeshWithNormal();
		std::vector<trimesh::vec3> outline_path(bool global = false, bool debug = false);
		std::vector<trimesh::vec3> concave_path();

		trimesh::quaternion nestRotation();
		void resetNestRotation();
		void dirtyNestData();
		cxkernel::NestDataPtr nestData();
		void copyNestData(ModelN* model);
		cxkernel::MeshDataPtr modelNData();

		QVector3D zeroLocation();

		trimesh::point getMinYZ();
		float localZ();

		int primitiveNum() override;

		void setSerialName(const QString& serialName);
		QString getSerialName();
		std::vector<trimesh::vec3> getoutline_ObjectExclude();

		/* 涂色相关 */
		void setPose(const QMatrix4x4& pose);

		std::string getColors2Facets(int index);
		void setColors2Facets(const std::vector<std::string>& data);
		std::vector<std::string> getColors2Facets();
		std::vector<std::string> getSeam2Facets();
		std::vector<std::string> getSupport2Facets();
		bool hasColors();
		bool hasSeamsData();
		bool hasSupportsData();
		bool adaptiveLayerData();

		void setFacet2Facets(const std::vector<int>& facet2Facets);
		int getFacet2Facets(int faceId);

		void setDefaultColorIndex(int colorIndex);
		int defaultColorIndex();

		void updateRenderByMeshSpreadData(const std::vector<std::string>& data, bool updateCapture = true);
		void updateRender(bool updateCapture = true);

		//paint seam
		void setSeam2Facets(const std::vector<std::string>& data);

		//paint support
		void setSupport2Facets(const std::vector<std::string>& data);

		/* 脏状态 */
		bool isDirty();
		void dirty();
		void resetDirty();

		void needRegenerate();

		bool isContainsMaterial(int materialIndex);
		std::set<int> colorIndexes();

		/* 冲刷参数 */
		bool flushIntoInfill();
		void setFlushIntoInfillEnabled(bool enabled);

    	bool flushIntoObjects();
		void setFlushIntoObjectsEnabled(bool enabled);

    	bool flushIntoSupport();
		void setFlushIntoSupportEnabled(bool enabled);

		/* 支撑参数 */
		bool supportEnabled();
		void setSupportEnabled(bool enabled);

    	float supportAngle();
		void setSupportAngle(float angle);

    	QString supportStructure();
		void setSupportStructure(const QString& structure);

		void setLayerHeightProfile(const std::vector<double>& layerHeightProfile);
		const std::vector<double>& layerHeightProfile();

		/* 渲染方式 */
		void updateEntityRender();
		void setOffscreenRenderEnabled(bool enabled, bool applyNow = true);
		void setEntityType(int type, bool applyNow = true);
		void setVisible(bool visible, bool applyNow = true);
		void setLayersColor(const QVariantList& layersColor);
		void setOffscreenRenderOffset(const QVector3D& offset);
		void useCustomEffect(qtuser_3d::XEffect* effect);

		/* palette */
		void updatePalette();

		// ==== modelGroup logic ====
		void setObjectId(int64_t objId);
		int64_t getObjectId();
		void setFixedId(int64_t fixedId);
		int64_t getFixedId();
		void onAttachModelGroup(ModelGroup* mg);
		ModelGroup* getModelGroup();
		Q_INVOKABLE QObject* getModelGroupObject();
		Q_INVOKABLE int getModelGroupSize();
		void setCacheGlobalBoxDirty();
		void setPartIndex(int partIndex);
		int getPartIndex();

		bool getFanZhuanState();

		void updateNode();

	signals:
		void flushIntoInfillChanged();
		void flushIntoObjectsChanged();
		void flushIntoSupportChanged();
		void supportEnabledChanged();
		void supportAngleChanged();
		void supportStructureChanged();
		void isVisibleChanged();
		void nameChanged();

	signals:
		void prepareCheckMaterial();
		void dirtyChanged();
		void colorsDataChanged();
		void seamsDataChanged();
		void supportsDataChanged();
		void settingsChanged();
		void defaultColorIndexChanged();
		void adaptiveLayerDataChanged();
		void modelNTypeChanged();
		void formModified(); 	// 模型形态改变

	private slots:
		void onGlobalSettingsChanged(QObject* parameter_base, const QString& key);
		void onSettingsChanged(const QString& key,us::USetting* setting);
		void updateSetting();

	private:
		// std::vector<std::string> m_colors2Facets; //序列化数据
		// std::vector<int> m_facet2Facets;//原始面与细分数据映射表
		int m_colorIndex{0};

	protected:
		trimesh::dbox3 calculateBoundingBox(const QMatrix4x4& m) override;
		void onStateChanged(qtuser_3d::ControlState state) override;
		void meshChanged();
		void faceBaseChanged(int faceBase) override;

		void setParentGroupStateDirty();

	protected:
		QString m_fileName;
		QString m_name;
		QString m_description;
		int m_primitiveNum { 0 };

		bool m_needRepair { false };

		/* relief */
		FontMeshWrapper* m_fontMesh{ NULL };

		ModelNEntity* m_entity;

        bool m_needInit { true };
		QVector3D m_initPosition;
		us::USettings* m_setting { NULL };
		us::USettings* m_lastUsedSetting { NULL };

		bool m_visibility;
		cxkernel::NestDataPtr m_nestData;

		double m_nestRotation;
		QString m_serialName;
		bool m_isDirty { false };

		std::vector<double> m_layerHeightProfile;

		int64_t m_objectId;
		int64_t m_fixedId;
		ModelGroup* m_modelGroup;
		int m_partIndex{ -1 }; // relate to the index where this model part is in the modelGroup

		int m_renderType{ -1 };

		bool m_renderFlagCache;
		trimesh::dbox3 m_cacheGlobalBox;
		trimesh::dvec3 m_cacheGlobalBoxInitPos;
		bool m_cacheGlobalBoxIsDirty{ true };

		crslice2::CrSliceVolume* m_crslice_volume = nullptr;
		bool m_need_regenerate_id = true;
	};

	BASIC_KERNEL_API Qt3DRender::QGeometry* createGeometryFromMesh(trimesh::TriMesh* mesh);

	struct ModelNPropertyMeshChange
	{
		ModelN* model = nullptr;
		cxkernel::MeshDataPtr mesh;
		QString name;
	};
}
#endif // CREATIVE_KERNEL_MODELN_1592181954934_H
