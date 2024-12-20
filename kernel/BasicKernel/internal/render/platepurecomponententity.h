#ifndef PLATE_PURE_COMPONENT_ENTITY_202311141735_H
#define PLATE_PURE_COMPONENT_ENTITY_202311141735_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/xentity.h"

namespace creative_kernel{

	class PlatePureComponentEntity : public qtuser_3d::XEntity
	{
	public:
		PlatePureComponentEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlatePureComponentEntity();

		void setColor(const QVector4D& color);

	protected:
		Qt3DRender::QParameter* m_colorParameter;
	};

}

#endif //PLATE_PURE_COMPONENT_ENTITY_202311141735_H