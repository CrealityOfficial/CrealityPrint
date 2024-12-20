#ifndef QTUSER_3D_NOZZLEENTITY_H
#define QTUSER_3D_NOZZLEENTITY_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API NozzleEntity : public XEntity
	{
		Q_OBJECT
	public:
		NozzleEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~NozzleEntity();

		
	protected:
	
	};
}
#endif // QTUSER_3D_NOZZLEENTITY_H