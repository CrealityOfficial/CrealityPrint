#include "node3d.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/util.h"

namespace qtuser_3d
{
	Node3D::Node3D(QObject* parent)
		:Pickable(parent)
		, m_localMatrixDirty(true)
		, m_parentMatrixDirty(true)
		, mirrorZ_count(0)
	{
		m_localScale = QVector3D(1.0f, 1.0f, 1.0f);
	}

	Node3D::Node3D(const Node3D& node)
		: mirrorZ_count(0)
		, m_localCenter()
		, m_localPosition(node.m_localPosition)
		, m_localScale(node.m_localScale)
		, m_localRotate(node.m_localRotate)
		, m_globalSpaceBox()
		, m_localBox()
		, m_local2Parent()
		, m_parent2Global()
		, m_localMatrixDirty(true)
		, m_parentMatrixDirty(true)
		, m_globalMatrix(node.m_globalMatrix)
		, m_objectMatrix()
		, m_mirrorMatrix()
	{
	}

	Node3D::~Node3D()
	{
	}

	void Node3D::parentGlobalChanged(const QMatrix4x4& parent2Global)
	{
		m_parent2Global = parent2Global;
		m_parentMatrixDirty = true;

		updateMatrix();
	}

	void Node3D::notifyGlobalChanged(const QMatrix4x4& globalMatrix)
	{
		updateGlobalSpaceBox();
		onGlobalMatrixChanged(globalMatrix);

		QList<Node3D*> node3ds = findChildren<Node3D*>(QString(), Qt::FindDirectChildrenOnly);
		for (Node3D* node : node3ds)
		{
			node->parentGlobalChanged(m_globalMatrix);
		}
	}

	void Node3D::onGlobalMatrixChanged(const QMatrix4x4& globalMatrix)
	{

	}

	void Node3D::onLocalMatrixChanged(const QMatrix4x4& localMatrix)
	{

	}

	QVector3D Node3D::center() const
	{
		return m_localCenter;
	}

	void Node3D::setCenter(const QVector3D& center, bool update)
	{
		m_localCenter = center;
		m_localMatrixDirty = true;

		if (update) updateMatrix();
	}

	void Node3D::setLocalScale(const QVector3D& scale, bool update)
	{
		m_localScale = scale;
		checkQVector3D(m_localScale, QVector3D(-100, -100, -100), QVector3D(1000, 1000, 1000));
		m_localMatrixDirty = true;

		if (update) updateMatrix();
	}

	void Node3D::resetLocalScale(bool update)
	{
		setLocalScale(QVector3D(1.0f, 1.0f, 1.0f), update);
	}

	QVector3D Node3D::localScale() const
	{
		return m_localScale;
	}

	QQuaternion Node3D::localQuaternion() const
	{
		return m_localRotate;
	}

	void Node3D::setLocalQuaternion(const QQuaternion& q, bool update)
	{
		m_localRotate = q;
		m_localMatrixDirty = true;

		if (update) updateMatrix();
	}

	void Node3D::resetLocalQuaternion(bool update)
	{
		setLocalQuaternion(QQuaternion(), update);
	}

	void Node3D::setLocalPosition(const QVector3D& position, bool update)
	{
		m_localPosition = position;
		m_localMatrixDirty = true;

		if (update) updateMatrix();
	}

	void Node3D::resetLocalPosition(bool update)
	{
		setLocalPosition(QVector3D(0.0f, 0.0f, 0.0f), update);
	}

	QVector3D Node3D::localPosition() const
	{
		return m_localPosition;
	}

	void Node3D::updateMatrix()
	{
		if (!m_localMatrixDirty && !m_parentMatrixDirty)
			return;

		m_local2Parent.setToIdentity();
		m_local2Parent.translate(m_localPosition + m_localCenter);
		QMatrix4x4 t = m_local2Parent;

		t.rotate(m_localRotate);
		t.scale(m_localScale);
		t.translate(-m_localCenter);

		m_local2Parent *= m_mirrorMatrix;
		
		m_local2Parent.rotate(m_localRotate);
		m_local2Parent.scale(m_localScale);
		m_local2Parent.translate(-m_localCenter);

		onLocalMatrixChanged(m_local2Parent);

		m_globalMatrix = m_parent2Global * m_local2Parent;
		notifyGlobalChanged(m_globalMatrix);

		m_objectMatrix = m_parent2Global  * t;

		m_localMatrixDirty = false;
		m_parentMatrixDirty = false;
	}

	qtuser_3d::Box3D Node3D::localBox() const
	{
		return m_localBox;
	}

	qtuser_3d::Box3D Node3D::globalSpaceBox() const
	{
		return m_globalSpaceBox;
	}

	Box3D Node3D::globalSpaceBoxNoScale() const
	{
		return m_globalSpaceBoxNoScale;
	}

	qtuser_3d::Box3D Node3D::calculateGlobalSpaceBox()
	{
		return qtuser_3d::Box3D();
	}

	Box3D Node3D::calculateGlobalSpaceBoxNoScale()
	{
		return Box3D();
	}

	void Node3D::updateGlobalSpaceBox()
	{
		m_globalSpaceBox = calculateGlobalSpaceBox();
		m_globalSpaceBoxNoScale = calculateGlobalSpaceBoxNoScale();
	}

	QMatrix4x4 Node3D::parent2Global() const
	{
		return m_parent2Global;
	}

	QMatrix4x4 Node3D::globalMatrix() const
	{
		return m_globalMatrix;
	}

	QMatrix4x4 Node3D::localMatrix() const
	{
		return m_local2Parent;
	}

	QMatrix4x4 Node3D::objectMatrix() const
	{
		return m_objectMatrix;
	}

	bool Node3D::isFanZhuan() const
	{
		bool fanzhuan = false;
		for (int i = 0; i < 3; i++)
		{
			if (m_mirrorMatrix(i, i) < -0.9)
			{
				fanzhuan = !fanzhuan;
			}
		}
		return fanzhuan;
	}

	void Node3D::setParent2Global(const QMatrix4x4& matrix)
	{
		if (m_parent2Global != matrix)
		{
			m_parent2Global = matrix;
			m_parentMatrixDirty = true;

			updateMatrix();
		}
	}

	void Node3D::mirrorX()
	{
		QMatrix4x4 m;
		m(0, 0) = -1;
		mirror(m, true);
	}

	void Node3D::mirrorY()
	{
		QMatrix4x4 m;
		m(1, 1) = -1;
		mirror(m, true);
	}

	void Node3D::_mirrorZ()
	{
		QMatrix4x4 m;
		m(2, 2) = -1;
		mirror(m, true);
	}

	void Node3D::_mirrorSet(const QMatrix4x4& m)
	{
		mirror(m, false);
	}

	void Node3D::mirrorZ()  /////ZZ
	{
		QMatrix4x4 m;
		if (mirrorZ_count % 2 == 0)
		{
			liftZUp(m_globalSpaceBox.max.z());
			m(2, 2) = -1;
		}

		else
		{
			liftZUp(0.0);  //reverse
			m(2, 2) = -1;
		}
		mirror(m, true);
		mirrorZ_count += 1;
	}

	void Node3D::mirrorSet(const QMatrix4x4& m)
	{
		if (mirrorZ_count > 0)
		{
			QMatrix4x4 m;
			if (mirrorZ_count % 2 != 0)
			{
				liftZUp(0.0);
				m(2, 2) = -1;
			}

			else
			{
				liftZUp(m_globalSpaceBox.max.z());  //reverse
				m(2, 2) = -1;
			}
			mirror(m, true);
			mirrorZ_count -= 1;
			return;
		}
		mirror(m, false);
	}

	QMatrix4x4 Node3D::mirrorMatrix()
	{
		return m_mirrorMatrix;
	}

	void Node3D::mirror(const QMatrix4x4& matrix, bool apply)
	{
		if (apply)
		{
			m_mirrorMatrix = matrix * m_mirrorMatrix;
		}
		else
		{
			m_mirrorMatrix = matrix;
		}

		m_localMatrixDirty = true;
		updateMatrix();
	}

	QVector3D Node3D::mapGlobal2Local(QVector3D position)
	{
		qtuser_3d::Box3D box = globalSpaceBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		center.setZ(center.z() - size.z() / 2.0f);

		QVector3D offset = position - center;
		QVector3D localPos = localPosition();
		return localPos + offset;
	}

	QVector3D Node3D::mapGlobal2LocalAD(QVector3D position)
	{
		qtuser_3d::Box3D box = globalSpaceBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		//center.setZ(center.z() - size.z() / 2.0f);

		QVector3D offset = position - center;
		QVector3D localPos = localPosition();
		return localPos + offset;
	}

	void Node3D::liftZUp(float z)
	{
		qtuser_3d::Box3D box = globalSpaceBox();
		float z0 = box.min.z();

		QVector3D offset = QVector3D(0.0f, 0.0f, 0.0f);
		offset.setZ(z - z0);

		QVector3D localPos = localPosition();
		QVector3D newLocalPos = localPos + offset;
		setLocalPosition(newLocalPos);
	}
}
