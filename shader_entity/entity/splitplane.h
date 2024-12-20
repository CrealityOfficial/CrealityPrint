#ifndef _QTUSER_3D_SPLITPLANE_1592498725497_H
#define _QTUSER_3D_SPLITPLANE_1592498725497_H
#include "shader_entity_export.h"
#include "trimesh2/Box.h"
#include "entity/manipulateentity.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API SplitPlane : public qtuser_3d::ManipulateEntity
	{
	public:
		SplitPlane(Qt3DCore::QNode* parent = nullptr);
		virtual ~SplitPlane();

		void updatePlanePosition(const trimesh::box3& box);
	protected:
		virtual void onStateChanged(qtuser_3d::ControlState state);
	protected:
		trimesh::vec3 m_operateModleBoxSize;
	};
}
#endif // _QTUSER_3D_SPLITPLANE_1592498725497_H
