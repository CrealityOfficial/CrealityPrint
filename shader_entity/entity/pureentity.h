#ifndef CXKERNEL_PUREENTITY_1681992284923_H
#define CXKERNEL_PUREENTITY_1681992284923_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class PureXEffect;
	class SHADER_ENTITY_API PureEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		PureEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PureEntity();

		void setColor(const QVector4D& color);
	protected:
		PureXEffect* m_effect;
	};
}

#endif // CXKERNEL_PUREENTITY_1681992284923_H