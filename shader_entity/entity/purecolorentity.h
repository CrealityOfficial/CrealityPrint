#ifndef _QTUSER_3D_PURECOLORENTITY_1591765873312_H
#define _QTUSER_3D_PURECOLORENTITY_1591765873312_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API PureColorEntity: public XEntity
	{
	public:
		PureColorEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PureColorEntity();

		void setColor(QVector4D color);
	protected:
		Qt3DRender::QParameter* m_colorParameter;
	};
}
#endif // _QTUSER_3D_PURECOLORENTITY_1591765873312_H
