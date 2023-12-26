#ifndef QTUSER_3D_INDICATOR_PICKABLE_H
#define QTUSER_3D_INDICATOR_PICKABLE_H

#include "shader_entity_export.h"
#include "qtuser3d/module/pickable.h"
#include "worldindicatorentity.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API IndicatorPickable : public qtuser_3d::Pickable
	{
		Q_OBJECT
	public:
		IndicatorPickable(QObject* parent = nullptr);
		virtual ~IndicatorPickable();

		void setPickableEntity(WorldIndicatorEntity * entity);

		int primitiveNum() override;
		
		bool isGroup() override;

		bool hoverPrimitive(int primitiveID) override;
		void unHoverPrimitive() override;

		void click(int primitiveID) override;

	protected:
		void onStateChanged(qtuser_3d::ControlState state) override;
		void faceBaseChanged(int faceBase) override;
		
	protected:
		WorldIndicatorEntity* m_pickableEntity;
	};
}
#endif // QTUSER_3D_INDICATOR_PICKABLE_H