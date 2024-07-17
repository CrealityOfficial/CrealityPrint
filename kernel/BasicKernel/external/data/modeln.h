#ifndef CREATIVE_KERNEL_MODELN_1592181954934_H
#define CREATIVE_KERNEL_MODELN_1592181954934_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "data/modelnrenderdata.h"

#include "cxkernel/data/nestdata.h"

#include <Qt3DCore/QEntity>
#include "qtuser3d/module/node3d.h"
#include "qtuser3d/math/ray.h"
#include "qtusercore/module/progressor.h"
#include <Qt3DRender/QGeometry>
#include <vector>

namespace us
{
	class USettings;
	class USetting;
}

namespace qtuser_3d
{
	class XEffect;
}

namespace creative_kernel
{
	class ModelNEntity;
	class BASIC_KERNEL_API ModelN : public qtuser_3d::Node3D
	{
		Q_OBJECT
    	Q_PROPERTY(bool flushIntoInfill READ flushIntoInfill WRITE setFlushIntoInfillEnabled NOTIFY flushIntoInfillChanged)
    	Q_PROPERTY(bool flushIntoObjects READ flushIntoObjects WRITE setFlushIntoObjectsEnabled NOTIFY flushIntoObjectsChanged)
    	Q_PROPERTY(bool flushIntoSupport READ flushIntoSupport WRITE setFlushIntoSupportEnabled NOTIFY flushIntoSupportChanged)
		Q_PROPERTY(bool supportEnabled READ supportEnabled WRITE setSupportEnabled NOTIFY supportEnabledChanged)
    	Q_PROPERTY(float supportAngle READ supportAngle WRITE setSupportAngle NOTIFY supportAngleChanged)
    	Q_PROPERTY(QString supportStructure READ supportStructure WRITE setSupportStructure NOTIFY supportStructureChanged)

	public:
		enum UnitType {
			UT_METRIC,//公制, 长度单位mm, 重量单位g
			UT_NOTMETRIC//非公制
		};

		enum RenderType {
			NoRender = 0xFF,
			ViewRender = 0x01,
			OffscreenRender = 0x10
		};

		ModelN(QObject* parent = nullptr);
		virtual ~ModelN();

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
		
		void setBoxState(int state);  // 0 hide,  1 show as select state
		void setVisibility(bool visibility);
		bool isVisible();
		Qt3DCore::QEntity* getModelEntity();
		void setModelEffect(qtuser_3d::XEffect* effect);
		
		//Variable Layer Height
		void useVariableLayerHeightEffect();
		void useDefaultModelEffect();

		void setNozzle(int nozzle);
		int nozzle();

		void enterSupportStatus();
		void leaveSupportStatus();
		void setSupportCos(float cos);

		qtuser_3d::Box3D boxWithSup();

		QVector3D localRotateAngle();
		QQuaternion rotateByStandardAngle(QVector3D axis, float angle, bool updateCurFlag = false);
		// redoFlag: false = undo， true = redo
		void updateDisplayRotation(bool redoFlag, int step = 1);
		void resetRotate();

		void SetModelViewClrState(qtuser_3d::ControlState statevalue, bool boxshow);

		trimesh::TriMesh* mesh();
		TriMeshPtr meshptr();
		TriMeshPtr globalMesh(bool needMergeColorMesh = true);
		TriMeshPtr localMesh(bool needMergeColorMesh = true);

		int getErrorEdges();
        int getErrorNormals();
		int getErrorHoles();
		int getErrorIntersects();

        void setErrorEdges(int value);
        void setErrorNormals(int value);
        void setErrorHoles(int value);
        void setErrorIntersects(int value);

		void setTexture();

		void setData(cxkernel::ModelNDataPtr data);
		cxkernel::ModelNDataPtr data();
		
		void setRenderData(ModelNRenderDataPtr renderData);
		ModelNRenderDataPtr renderData();

		qtuser_3d::Box3D calculateGlobalSpaceBox() override;
		qtuser_3d::Box3D calculateGlobalSpaceBoxNoScale() override;

		bool rayCheck(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D* normal);
		//global normal ,normal is not normalized , represent the average lenght of edges

		void convex(std::vector<trimesh::vec3>& datas, bool origin = false);
		QList<QVector3D> qConvex(bool toOrigin = false);

		trimesh::TriMesh* createGlobalMesh();
		std::vector<trimesh::vec3> outline_path(bool global = false, bool debug = false);
		std::vector<trimesh::vec3> concave_path();
		std::vector<trimesh::vec3> rsPath();
		const std::vector<trimesh::vec3>& sweepAreaPath();
		void updateSweepAreaPath();

		trimesh::quaternion nestRotation();
		void resetNestRotation();
		void dirtyNestData();
		cxkernel::NestDataPtr nestData();
		void copyNestData(ModelN* model);
		cxkernel::ModelNDataPtr modelNData();

		void adaptBox(const qtuser_3d::Box3D& box);
		void adaptSmallBox(const qtuser_3d::Box3D& box);//适配以m为单位创建的模型
		QVector3D zeroLocation();

		trimesh::point getMinYZ();
		float localZ();

		int primitiveNum() override;

		//设置单位类型
		void setUnitType(UnitType type);
		UnitType unitType();
		void setSerialName(const QString& serialName);
		QString getSerialName();
		void setLocalData(const trimesh::vec3& position, const QQuaternion& q, const trimesh::vec3& scale);
		std::vector<trimesh::vec3> getoutline_ObjectExclude();

		/* 涂色相关 */
		void setPose(const QMatrix4x4& pose);
		QMatrix4x4 pose();

		std::string getColors2Facets(int index);
		void setColors2Facets(const std::vector<std::string>& data);
		std::vector<std::string> getColors2Facets();
		std::vector<std::string> getSeam2Facets();
		std::vector<std::string> getSupport2Facets();
		bool hasColors();
		bool hasSeamsData();
		bool hasSupportsData();

		void setFacet2Facets(const std::vector<int>& facet2Facets);
		int getFacet2Facets(int faceId);

		void setDefaultColorIndex(int colorIndex);
		int defaultColorIndex() const;

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

		//print by object
		void setOuterLinesColor(const QVector4D& color);
		void setOuterLinesVisibility(bool visible);
		void setNozzleRegionVisibility(bool visible);

		bool isContainsMaterial(int materialIndex);

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
		std::vector<double> layerHeightProfile();

		/* 渲染方式 */
		void setRenderType(int type);
		void setLayersColor(const QVariantList& layersColor);
		void setOffscreenRenderOffset(const QVector3D& offset);

		void setBoxVisibility(bool visibility);

		void setObjectId(int objId);
		int getObjectId();
		void setGroupId(int groupId);
		int getGroupId();

		cxkernel::ModelNDataPtr getScaledUniformUnCheckData();

	signals:
		void flushIntoInfillChanged();
		void flushIntoObjectsChanged();
		void flushIntoSupportChanged();
		void supportEnabledChanged();
		void supportAngleChanged();
		void supportStructureChanged();

	signals:
		void prepareCheckMaterial();
		void dirtyChanged();
		void colorsDataChanged();
		void seamsDataChanged();
		void supportsDataChanged();
		void settingsChanged();
		void defaultColorIndexChanged();
		void adaptiveLayerDataChanged();
	private slots:
		void onGlobalSettingsChanged(QObject* parameter_base, const QString& key);
		void onSettingsChanged(const QString& key,us::USetting* setting);
		void updateSetting();

	private:
		// std::vector<std::string> m_colors2Facets; //序列化数据
		// std::vector<int> m_facet2Facets;//原始面与细分数据映射表
		int m_colorIndex{0};
		
	protected:
		void onGlobalMatrixChanged(const QMatrix4x4& globalMatrix) override;
		void onStateChanged(qtuser_3d::ControlState state) override;
		void meshChanged();
		void faceBaseChanged(int faceBase) override;

	protected:
		void onLocalPositionChanged(const QVector3D& newPosition) override;
		void onLocalScaleChanged(const QVector3D& newScale) override;
		void onLocalQuaternionChanged(const QQuaternion& newQ) override;

	protected:
		cxkernel::ModelNDataPtr m_data;
		ModelNRenderDataPtr m_renderData;

		ModelNEntity* m_entity;
        bool m_needInit { true };
		QVector3D m_initPosition;
		us::USettings* m_setting { NULL };
		us::USettings* m_lastUsedSetting { NULL };

		qtuser_3d::XEffect *m_modelOffscreenEffect;
		qtuser_3d::XEffect *m_modelCombindEffect;

		int m_currentLocalDispalyAngle;
		QList<QVector3D> m_localAngleStack;

		bool m_visibility;
		cxkernel::NestDataPtr m_nestData;
		
		UnitType m_ut = UT_METRIC;
		//show error
        int m_errorEdges;      //缺陷边
        int m_errorNormals;    //缺陷的法线
		int m_errorHoles;      //孔洞个数
		int m_errorIntersects; //非流面

		double m_nestRotation;
		QString m_serialName;
		bool m_isDirty { false };

		bool m_rIsDirty{ true }; //旋转数据脏
		bool m_sIsDirty{ true }; //缩放数据脏
		std::vector<trimesh::vec3> m_rsPath;  //旋转缩放的凸包轮廓
		bool m_sweepPathDirty{ true };
		std::vector<trimesh::vec3> m_sweepPath;

		std::vector<double> m_layerHeightProfile;

		int m_objectId;
		int m_groupId;

		int m_renderType{ -1 };

	};

	BASIC_KERNEL_API Qt3DRender::QGeometry* createGeometryFromMesh(trimesh::TriMesh* mesh);
}
#endif // CREATIVE_KERNEL_MODELN_1592181954934_H
