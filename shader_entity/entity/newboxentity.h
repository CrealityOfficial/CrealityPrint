#ifndef QTUSER_3D_NEWBOXENTITY_1616742128928_H
#define QTUSER_3D_NEWBOXENTITY_1616742128928_H

#include "shader_entity_export.h"
#include "entity/lineentity.h"
#include "qtuser3d/math/box3d.h"


namespace qtuser_3d
{
	class SHADER_ENTITY_API NewBoxEntity : public LineEntity
	{
		Q_OBJECT
	public:
		NewBoxEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~NewBoxEntity();

		void updateBox(Box3D& box);
		void updateLocal(Box3D& box, const QMatrix4x4& parentMatrix);
		void setColor(const QVector4D& color);

	protected:
	};
}
#endif // QTUSER_3D_NEWBOXENTITY_1616742128928_H
