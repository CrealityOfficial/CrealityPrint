#include "splitplane.h"
#include <Qt3DRender/QBlendEquationArguments>

#include "qtuser3d/trimesh2/renderprimitive.h"
#include "effect/purexeffect.h"

namespace qtuser_3d
{
	SplitPlane::SplitPlane(Qt3DCore::QNode* parent)
		: qtuser_3d::ManipulateEntity(parent, ManipulateEntity::Alpha | ManipulateEntity::Pickable | ManipulateEntity::DepthTest | ManipulateEntity::PickDepthTest)
	{
		setObjectName("Splitop.splitPlaneEntity");
		setColor(QVector4D(74.0f / 255.0f, 147.0f / 255.0f, 217.0f / 255.0f, 0.5f));
		setTriggeredColor(QVector4D(1.0f, 0.79f, 0.0f, 0.3f));
		setTriggerible(true);
	}

	SplitPlane::~SplitPlane()
	{
	}

	void SplitPlane::onStateChanged(qtuser_3d::ControlState state)
	{
		if (m_isTriggerible)
		{
			if ((int)state)
				setParameter("color", m_triggeredColor);
			else
				setParameter("color", m_color);
		}
	}

	void SplitPlane::updatePlanePosition(const trimesh::box3& box)
	{
		if (m_operateModleBoxSize == box.size())  //防止在更新 splitPlane时, 频繁地调用 createCubeTriangles->createIndicesGeometry->CreateVertexAttribute, 由此而造成 QByteArray频繁的申请内存而崩溃
			return;

		m_operateModleBoxSize = box.size();

		trimesh::vec3 size = box.size();
		float length = trimesh::length(size);
		float radius = length * 0.5f;

		trimesh::vec3 min(-radius, -radius, -0.1f);
		trimesh::vec3 max(radius, radius, 0.1f);
		trimesh::box3 thinbox(min, max);
		setGeometry(qtuser_3d::createCubeTriangles(thinbox));
	}
}
