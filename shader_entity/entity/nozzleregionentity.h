#ifndef QTUSER_3D_NOZZLE_REGION_ENTITY_202401191418_H
#define QTUSER_3D_NOZZLE_REGION_ENTITY_202401191418_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class XRenderPass;
	class SHADER_ENTITY_API NozzleRegionEntity : public XEntity
	{
		Q_OBJECT
	public:
		NozzleRegionEntity(Qt3DCore::QNode* parent = nullptr);
		~NozzleRegionEntity();


	protected:

	};

}

#endif //QTUSER_3D_NOZZLE_REGION_ENTITY_202401191418_H