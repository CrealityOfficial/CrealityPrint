#ifndef QTUSER_3D_SIMPLEPICKABLE_1595204740110_H
#define QTUSER_3D_SIMPLEPICKABLE_1595204740110_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/module/pickable.h"

namespace qtuser_3d
{
	class PickXEntity;
	class QTUSER_3D_API SimplePickable : public Pickable
	{
		Q_OBJECT
	public:
		SimplePickable(QObject* parent = nullptr);
		virtual ~SimplePickable();

		void setPickableEntity(PickXEntity* entity);
	protected:
		void onStateChanged(ControlState state) override;
		void faceBaseChanged(int faceBase) override;

		virtual void pickableEntityChanged();
	protected:
		PickXEntity* m_pickableEntity;
	};
}
#endif // QTUSER_3D_SIMPLEPICKABLE_1595204740110_H