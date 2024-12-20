#ifndef _creative_kernel_QNODE3D_1590453972562_H
#define _creative_kernel_QNODE3D_1590453972562_H
#include "basickernelexport.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/math/box3d.h"
#include "trimesh2/Box.h"
#include "crslice2/matrix.h"
#include <QtGui/QMatrix4x4>

namespace creative_kernel
{
	class BASICKERNEL_API QNode3D : public qtuser_3d::Pickable
	{
		Q_OBJECT
	public:
		QNode3D(QObject* parent = nullptr);
		QNode3D(const QNode3D& node);

		virtual ~QNode3D();

		virtual trimesh::dbox3 localBoundingBox();
		virtual trimesh::dbox3 globalBoundingBox();

		void setLocalScale(const QVector3D& scale, bool update = true);
		void resetLocalScale(bool update = true);
		QVector3D localScale() const;

		QQuaternion localQuaternion() const;
		void setLocalQuaternion(const QQuaternion& q, bool update = true);
		void resetLocalQuaternion(bool update = true);

		void setLocalPosition(const QVector3D& position, bool update = true);
		void resetLocalPosition(bool update = true);
		QVector3D localPosition() const;

		virtual void onLocalScaleChanged(const trimesh::dvec3& newScale);
		virtual void onLocalPositionChanged(const trimesh::dvec3& newPosition);
		virtual void onLocalQuaternionChanged(const trimesh::dvec3& newQ);

		bool isFanZhuan() const;

		QVector3D mapGlobal2Local(QVector3D position);  // map global bottom center to position
		void buildLocalMatrix(const QMatrix4x4& gMatrix);

		void applyMatrix(const trimesh::xform& matrix);
		void setMatrix(const trimesh::xform& matrix);
		trimesh::xform getMatrix();

		void resetToIdentity();
		void setInitMatrix();
		void resetMatrix();
		void resetPosition();

		void snapMatrix();
		trimesh::xform getSnapMatrix();

		trimesh::dvec3 getNodeOffset();
		trimesh::dvec3 getNodeScalingFactor();
		void setNodeScalingFactor(const trimesh::dvec3& scaling_factor);

		//for rotate panel
		QVector3D localRotateAngle();
		QQuaternion rotateByStandardAngle(QVector3D axis, float angle, bool updateCurFlag = false);
		// redoFlag: false = undo£¬ true = redo
		void updateDisplayRotation(bool redoFlag, int step = 1);
		void resetRotate();

		void dirtyNode();

	protected:
		void _dirty();
		void dirtyAllChildren();

		qtuser_3d::Box3D convert(const trimesh::dbox3& box);
		virtual void mirror(const QMatrix4x4& matrix, bool apply = true);
		virtual trimesh::dbox3 calculateBoundingBox(const QMatrix4x4& m);

		trimesh::dbox3 _localBoundingBox();
		trimesh::dbox3 _globalBoundingBox();

		QMatrix4x4 _parent2Global();
		QMatrix4x4 _globalMatrix();
		QMatrix4x4 _localMatrix();
	protected:
		crslice2::Matrix m_matrix;
		trimesh::xform m_init_matrix;
		trimesh::xform m_snap_matrix;

		QVector3D m_localPosition;
		QVector3D m_localScale;
		QQuaternion m_localRotate;
		//QMatrix4x4 m_mirrorMatrix;

		trimesh::dbox3 m_globalSpaceBox;    // parent space box
		trimesh::dbox3 m_localBox;
		QMatrix4x4 m_local2Parent;

		bool m_localMatrixDirty;
		bool m_localBoxDirty;
		bool m_globalBoxDirty;

		int m_currentLocalDispalyAngle;
		QList<QVector3D> m_localAngleStack;
	};
}
#endif // _creative_kernel_QNODE3D_1590453972562_H
