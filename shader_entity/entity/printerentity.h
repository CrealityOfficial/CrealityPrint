#ifndef _PRINTERENTITY_USE_1588651416007_H
#define _PRINTERENTITY_USE_1588651416007_H
#include "shader_entity_export.h"
#include "entity/pureentity.h"
#include "qtuser3d/geometry/attribute.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API PrinterEntity : public Qt3DCore::QEntity
	{
	public:
		PrinterEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterEntity();

		void updateSize(const trimesh::box& box);
		trimesh::box3 boundingBox();
	protected:
		trimesh::box3 m_bounding;
		qtuser_3d::PureEntity* m_boxEntity;
		//qtuser_3d::PrinterGrid* m_printerGrid;
	};
}

#endif // _PRINTERENTITY_USE_1588651416007_H
