#ifndef CREATIVE_KERNEL_MODELN_1592181954934_H
#define CREATIVE_KERNEL_MODELN_1592181954934_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "cxkernel/data/modelndata.h"
#include "qcxutil/util/nestdata.h"

#include <Qt3DCore/QEntity>
#include "qtuser3d/module/node3d.h"
#include "qtuser3d/math/ray.h"
#include "qtusercore/module/progressor.h"
#include <vector>

namespace us
{
	class USettings;
}

namespace creative_kernel
{
	class ModelNEntity;
	class FDMSupportGroup;
	class BASIC_KERNEL_API ModelN : public qtuser_3d::Node3D
	{
		Q_OBJECT
	public:
		enum UnitType {
			UT_METRIC,//公制, 长度单位mm, 重量单位g
			UT_NOTMETRIC//非公制
		};

		ModelN(QObject* parent = nullptr);
		virtual ~ModelN();

        bool needInit();
		QVector3D& GetInitPosition();
		void SetInitPosition(QVector3D QPosition);

		void rotateNormal2Bottom(const QVector3D& normal, QVector3D& position, QQuaternion& rotate);

		us::USettings* setting();
		void setsetting(us::USettings* modelsetting);

		void setState(float state);
		float getState();
		void setErrorState(bool error);
		void setBoxState(int state);  // 0 hide,  1 show as select state
		void setVisibility(bool visibility);
		bool isVisible();
		Qt3DCore::QEntity* getModelEntity();

		// 自定义颜色，当 state 值大于 5 时生效
		void setCustomColor(QColor color);
		QColor getCustomColor();

		void setNozzle(int nozzle);
		int nozzle();

        void buildFDMSupport();
		void setVisualMode(ModelVisualMode mode);

		void enterSupportStatus();
		void leaveSupportStatus();
		void setSupportCos(float cos);

		//void setNeedCheckScope(int checkscope);

		qtuser_3d::Box3D boxWithSup();

		bool hasFDMSupport();
		void setFDMSup(FDMSupportGroup* fdmSup);

        FDMSupportGroup* fdmSupport();

		void setFDMSuooprt(bool detectAdd);
		bool getFDMSuooprt();

		QVector3D localRotateAngle();
		QQuaternion rotateByStandardAngle(QVector3D axis, float angle, bool updateCurFlag = false);
		// redoFlag: false = undo， true = redo
		void updateDisplayRotation(bool redoFlag, int step = 1);
		void resetRotate();

		void SetModelViewClrState(qtuser_3d::ControlState statevalue, bool boxshow);

		trimesh::TriMesh* mesh();
		TriMeshPtr meshptr();
		TriMeshPtr globalMesh();

		int getErrorEdges();
        int getErrorNormals();
		int getErrorHoles();
		int getErrorIntersects();

        void setErrorEdges(int value);
        void setErrorNormals(int value);
        void setErrorHoles(int value);
        void setErrorIntersects(int value);

		void needDetectError();

		void setTexture();

		void setData(cxkernel::ModelNDataPtr data);
		cxkernel::ModelNDataPtr data();

		// edit mesh vertices, return the matrix of scale and rotate
		QMatrix4x4 embedScaleNRot();
		void embedMatrix(QMatrix4x4 mat);

		qtuser_3d::Box3D calculateGlobalSpaceBox() override;
		qtuser_3d::Box3D calculateGlobalSpaceBoxNoScale() override;

		bool rayCheck(int primitiveID, const qtuser_3d::Ray& ray, QVector3D& collide, QVector3D* normal);
		//global normal ,normal is not normalized , represent the average lenght of edges

		void convex(std::vector<trimesh::vec3>& datas);
		QList<QVector3D> qConvex(bool toOrigin = false);

		trimesh::TriMesh* createGlobalMesh();
		std::vector<trimesh::vec3> outline_path(bool global = false, bool debug = false);
		std::vector<trimesh::vec3> concave_path();
		trimesh::quaternion nestRotation();
		void resetNestRotation();
		void dirtyNestData();
		qcxutil::NestDataPtr nestData();
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
	protected:
		void onGlobalMatrixChanged(const QMatrix4x4& globalMatrix) override;
		void onStateChanged(qtuser_3d::ControlState state) override;
		void meshChanged();
		void faceBaseChanged(int faceBase) override;

		void setSupportsVisibility(bool visibility);
		void mirror(const QMatrix4x4& matrix, bool apply = true) override;
	protected:
		cxkernel::ModelNDataPtr m_data;
		ModelNEntity* m_entity;

        bool m_needInit { true };
		QVector3D m_initPosition;
		us::USettings* m_setting;

		FDMSupportGroup* m_fdmSupportGroup;

		int m_currentLocalDispalyAngle;
		QList<QVector3D> m_localAngleStack;

		bool m_visibility;
		bool m_detectAdd; //for erase support;
		qcxutil::NestDataPtr m_nestData;
		
		UnitType m_ut = UT_METRIC;
		//show error
        int m_errorEdges;      //缺陷边
        int m_errorNormals;    //缺陷的法线
		int m_errorHoles;      //孔洞个数
		int m_errorIntersects; //非流面

		double m_nestRotation;
	};
}
#endif // CREATIVE_KERNEL_MODELN_1592181954934_H
