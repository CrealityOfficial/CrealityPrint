#include "qnode3d.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/util.h"
#include "qtuser3d/trimesh2/conv.h"
#include "data/spaceutils.h"

namespace creative_kernel
{
	QNode3D::QNode3D(QObject* parent)
		:Pickable(parent)
		, m_localBoxDirty(true)
		, m_globalBoxDirty(true)
		, m_localMatrixDirty(true)
		, m_localAngleStack({ QVector3D() })
		, m_currentLocalDispalyAngle(0)
	{
		m_localScale = QVector3D(1.0f, 1.0f, 1.0f);
	}

	QNode3D::QNode3D(const QNode3D& node)
		: m_localPosition(node.m_localPosition)
		, m_localScale(node.m_localScale)
		, m_localRotate(node.m_localRotate)
		, m_localBoxDirty(true)
		, m_globalBoxDirty(true)
		, m_localMatrixDirty(true)
		, m_localAngleStack(node.m_localAngleStack)
		, m_currentLocalDispalyAngle(node.m_currentLocalDispalyAngle)
	{
	}

	QNode3D::~QNode3D()
	{
	}

	trimesh::dbox3 QNode3D::localBoundingBox()
	{
		return _localBoundingBox();
	}

	trimesh::dbox3 QNode3D::globalBoundingBox()
	{
		return _globalBoundingBox();
	}

	void QNode3D::setLocalScale(const QVector3D& scale, bool update)
	{
	}

	void QNode3D::resetLocalScale(bool update)
	{
		setLocalScale(QVector3D(1.0f, 1.0f, 1.0f), update);
	}

	QVector3D QNode3D::localScale() const
	{
		return m_localScale;
	}

	QQuaternion QNode3D::localQuaternion() const
	{
		return m_localRotate;
	}

	void QNode3D::setLocalQuaternion(const QQuaternion& q, bool update)
	{
	}

	void QNode3D::resetLocalQuaternion(bool update)
	{
		setLocalQuaternion(QQuaternion(), update);
	}

	void QNode3D::setLocalPosition(const QVector3D& position, bool update)
	{
	}

	void QNode3D::resetLocalPosition(bool update)
	{
		setLocalPosition(QVector3D(0.0f, 0.0f, 0.0f), update);
	}

	QVector3D QNode3D::localPosition() const
	{
		return m_localPosition;
	}

	void QNode3D::onLocalScaleChanged(const trimesh::dvec3& newScale)
	{
	}

	void QNode3D::onLocalPositionChanged(const trimesh::dvec3& newPosition)
	{
	}

	void QNode3D::onLocalQuaternionChanged(const trimesh::dvec3& newQ)
	{
	}

	bool QNode3D::isFanZhuan() const
	{
		trimesh::xform theMatrix = m_matrix.get_matrix();
		bool fanzhuan = false;
		for (int i = 0; i < 3; i++)
		{
			if (theMatrix(i, i) < -0.9)
			{
				fanzhuan = !fanzhuan;
			}
		}
		return fanzhuan;
	}

	//QMatrix4x4 QNode3D::mirrorMatrix()
	//{
	//	return m_mirrorMatrix;
	//}

	qtuser_3d::Box3D QNode3D::convert(const trimesh::dbox3& b)
	{
		qtuser_3d::Box3D bb;
		bb += QVector3D(b.max.x, b.max.y, b.max.z);
		bb += QVector3D(b.min.x, b.min.y, b.min.z);
		return bb;
	}

	void QNode3D::mirror(const QMatrix4x4& matrix, bool apply)
	{
		//if (apply)
		//{
		//	m_mirrorMatrix = matrix * m_mirrorMatrix;
		//}
		//else
		//{
		//	m_mirrorMatrix = matrix;
		//}

		//m_localMatrixDirty = true;
	}

	trimesh::dbox3 QNode3D::calculateBoundingBox(const QMatrix4x4& m)
	{
		return trimesh::dbox3();
	}

	QMatrix4x4 QNode3D::_parent2Global()
	{
		QMatrix4x4 m;
		m.setToIdentity();
		QNode3D* node = qobject_cast<QNode3D*>(parent());
		if (node)
			m = node->_globalMatrix();
		return m;
	}

	QMatrix4x4 QNode3D::_globalMatrix()
	{
		return _parent2Global() * _localMatrix();
	}

	QMatrix4x4 QNode3D::_localMatrix()
	{
		return qtuser_3d::xform2QMatrix(trimesh::fxform(getMatrix()));
	}

	trimesh::dbox3 QNode3D::_localBoundingBox()
	{
		if (m_localBoxDirty)
		{
			m_localBox = calculateBoundingBox(_localMatrix());
			m_localBoxDirty = false;
		}
		return m_localBox;
	}

	trimesh::dbox3 QNode3D::_globalBoundingBox()
	{
		if (m_globalBoxDirty)
		{
			m_globalSpaceBox = calculateBoundingBox(_globalMatrix());
			m_globalBoxDirty = false;
		}
		return m_globalSpaceBox;
	}

	QVector3D QNode3D::mapGlobal2Local(QVector3D position)
	{
		qtuser_3d::Box3D box = convert(_globalBoundingBox());
		QVector3D size = box.size();
		QVector3D center = box.center();
		center.setZ(center.z() - size.z() / 2.0f);

		QVector3D offset = position - center;
		QVector3D localPos = localPosition();
		return localPos + offset;
	}

	void QNode3D::buildLocalMatrix(const QMatrix4x4& gMatrix)
	{
		QVector3D trans = gMatrix.column(3).toVector3D();
		setLocalPosition(trans, false);

		QVector3D scale;
		scale.setX(QVector3D(gMatrix(0, 0), gMatrix(1, 0), gMatrix(2, 0)).length());
		scale.setY(QVector3D(gMatrix(0, 1), gMatrix(1, 1), gMatrix(2, 1)).length());
		scale.setZ(QVector3D(gMatrix(0, 2), gMatrix(1, 2), gMatrix(2, 2)).length());
		setLocalScale(scale, false);

		QMatrix4x4 m;
		m.scale(scale);
		m = m.inverted() * gMatrix;
		QMatrix3x3 rotationMatrix = m.normalMatrix();
		QQuaternion qrot = QQuaternion::fromRotationMatrix(rotationMatrix);
		setLocalQuaternion(qrot, true);
	}

	void QNode3D::applyMatrix(const trimesh::xform& matrix)
	{
		trimesh::xform m = getMatrix();
		setMatrix(matrix * m);
	}

	void QNode3D::setMatrix(const trimesh::xform& matrix)
	{
		trimesh::dvec3 oldOffset = m_matrix.get_offset();
		trimesh::dvec3 oldScale = m_matrix.get_scaling_factor();
		trimesh::dvec3 oldRotate = m_matrix.get_rotation();

		m_matrix.set_matrix(matrix);

		trimesh::dvec3 newOffset = m_matrix.get_offset();
		trimesh::dvec3 newRotation = m_matrix.get_rotation();
		trimesh::dvec3 newScale = m_matrix.get_scaling_factor();

		if (!fuzzyCompareDvec3(oldOffset, newOffset))
		{
			onLocalPositionChanged(newOffset);
		}

		if (!fuzzyCompareDvec3(oldRotate, newRotation))
		{
			onLocalQuaternionChanged(newRotation);
		}

		if (!fuzzyCompareDvec3(oldScale, newScale))
		{
			onLocalScaleChanged(newScale);
		}

		_dirty();
	}

	trimesh::xform QNode3D::getMatrix()
	{
		m_localMatrixDirty = false;
		return m_matrix.get_matrix();
	}

	void QNode3D::resetToIdentity()
	{
		setMatrix(trimesh::xform::identity());
	}

	void QNode3D::setInitMatrix()
	{
		m_init_matrix = m_matrix.get_matrix();
	}

	void QNode3D::resetMatrix()
	{
		setMatrix(m_init_matrix);
	}

	void QNode3D::resetPosition()
	{
		m_matrix.set_offset(trimesh::dvec3(m_init_matrix(3, 0), m_init_matrix(3, 1), m_init_matrix(3, 2)));
		_dirty();
	}

	void QNode3D::snapMatrix()
	{
		m_snap_matrix = m_matrix.get_matrix();
	}

	trimesh::xform QNode3D::getSnapMatrix()
	{
		return m_snap_matrix;
	}

	trimesh::dvec3 QNode3D::getNodeOffset()
	{
		return m_matrix.get_offset();
	}

	trimesh::dvec3 QNode3D::getNodeScalingFactor()
	{
		return m_matrix.get_scaling_factor();
	}

	void QNode3D::setNodeScalingFactor(const trimesh::dvec3& scaling_factor)
	{
		trimesh::dvec3 oldScale = m_matrix.get_scaling_factor();
		
		m_matrix.set_scaling_factor(scaling_factor);

		if (!fuzzyCompareDvec3(oldScale, scaling_factor))
		{
			onLocalScaleChanged(scaling_factor);
		}

		_dirty();
	}

	void QNode3D::_dirty()
	{
		m_localMatrixDirty = true;
		m_globalBoxDirty = true;
		m_localBoxDirty = true;

		dirtyAllChildren();
	}

	void QNode3D::dirtyAllChildren()
	{
		QList<QNode3D*> node3ds = findChildren<QNode3D*>(QString(), Qt::FindDirectChildrenOnly);
		for (QNode3D* node : node3ds)
		{
			node->m_globalBoxDirty = true;
		}
	}

	QVector3D QNode3D::localRotateAngle()
	{
		return m_localAngleStack[m_currentLocalDispalyAngle];
	}

	QQuaternion QNode3D::rotateByStandardAngle(QVector3D axis, float angle, bool updateCurFlag)
	{
		if (!updateCurFlag)
		{
			QVector3D currentLDR = m_localAngleStack[m_currentLocalDispalyAngle];
			QVector3D v = axis * angle;
			currentLDR += v;

			m_localAngleStack.erase(m_localAngleStack.begin() + m_currentLocalDispalyAngle + 1, m_localAngleStack.end());
			m_localAngleStack.push_back(currentLDR);
			m_currentLocalDispalyAngle++;
		}
		else if (m_currentLocalDispalyAngle > 0)
		{
			m_localAngleStack[m_currentLocalDispalyAngle] = m_localAngleStack[m_currentLocalDispalyAngle - 1] + axis * angle;
		}

		return m_localRotate;
	}

	void QNode3D::updateDisplayRotation(bool redoFlag, int step)
	{
		if (redoFlag)
			m_currentLocalDispalyAngle += step;
		else
			m_currentLocalDispalyAngle -= step;

		m_currentLocalDispalyAngle = std::min(m_localAngleStack.size() - 1, std::max(0, m_currentLocalDispalyAngle));
	}

	void QNode3D::resetRotate()
	{
		QVector3D currentLDR = QVector3D();

		m_localAngleStack.erase(m_localAngleStack.begin() + m_currentLocalDispalyAngle + 1, m_localAngleStack.end());
		m_localAngleStack.push_back(currentLDR);
		m_currentLocalDispalyAngle++;
	}

	void QNode3D::dirtyNode()
	{
		_dirty();
	}
}
