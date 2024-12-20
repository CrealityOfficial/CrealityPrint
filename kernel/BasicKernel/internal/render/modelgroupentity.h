#ifndef QTUSER_3D_MODEL_GROUP_ENTITY_202403271544_H
#define QTUSER_3D_MODEL_GROUP_ENTITY_202403271544_H
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"
#include "trimesh2/Vec.h"
#include <Qt3DRender/QTexture>

namespace qtuser_3d 
{
	class NozzleRegionEntity;
	class PureEntity;
}

namespace creative_kernel
{
	class ModelGroupEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		ModelGroupEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelGroupEntity();

		void updateConvex(const std::vector<trimesh::vec3>& lines);
		void setConvexPose(const QMatrix4x4& matrix);
	
		//print by object
		void setLinesPose(const QMatrix4x4& pose);
		void updateLines(const std::vector<trimesh::vec3>& lines);
		void setOuterLinesColor(const QVector4D& color);
		void setOuterLinesVisibility(bool visible);
		void setNozzleRegionVisibility(bool visible);

	protected:
		qtuser_3d::PureEntity* m_convexEntity = nullptr;

		qtuser_3d::NozzleRegionEntity* m_nozzleRegionEntity;
		qtuser_3d::PureEntity* m_lineEntity;
		bool m_nozzleRegionVisible{ false };
		bool m_lineEntityVisible{ false };
	};
}
#endif // QTUSER_3D_MODEL_GROUP_ENTITY_202403271544_H
