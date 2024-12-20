#ifndef CXKERNEL_AXISENTITY_1683287324896_H
#define CXKERNEL_AXISENTITY_1683287324896_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	enum class EAxis
	{
		ea_x,
		ea_y,
		ea_z
	};

	class SHADER_ENTITY_API KernelAxisEntity : public qtuser_3d::XEntity
	{
	public:
		KernelAxisEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~KernelAxisEntity();

		void setAxisEffect(qtuser_3d::XEffect* effect);
		void setAxisGeometry(Qt3DRender::QGeometry* geometry);
		void setAxisParameter(EAxis eaxis, const QString& name, const QVariant& value);
	protected:
		qtuser_3d::XEntity* m_entities[3];
	};
}

#endif // CXKERNEL_AXISENTITY_1683287324896_H