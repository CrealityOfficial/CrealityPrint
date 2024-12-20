#ifndef _QTUSER_3D_MANIPULATEENTITY_1590036879870_H
#define _QTUSER_3D_MANIPULATEENTITY_1590036879870_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/pxentity.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class XRenderPass;

	class SHADER_ENTITY_API ManipulateEntity : public PickXEntity
	{
	public:
		enum ManipulateEntityFlag
		{
			Light = 0x01,
			Alpha = 0x02,
			Pickable = 0x04,
			DepthTest = 0x08,
			Overlay = 0x10,
			PickDepthTest = 0x20
		};
		typedef int ManipulateEntityFlags;
		ManipulateEntity(Qt3DCore::QNode* parent = nullptr, ManipulateEntityFlags flags = Alpha & Pickable);
		virtual ~ManipulateEntity();

		void setColor(const QVector4D& color);
		void setTriggeredColor(const QVector4D& color);
		void setTriggerible(bool enable);
		void setPickable(bool enablePick);

		virtual void setVisualState(qtuser_3d::ControlState state) override;

	protected:
		QVector4D m_color;
		QVector4D m_triggeredColor;
		bool m_isTriggerible;
		XRenderPass* m_pickPass;
	};
}
#endif // _QTUSER_3D_MANIPULATEENTITY_1590036879870_H
