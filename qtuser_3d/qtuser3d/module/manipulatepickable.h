#ifndef QTUSER_3D_MANIPULATEPICKABLE_1595553785446_H
#define QTUSER_3D_MANIPULATEPICKABLE_1595553785446_H
#include "qtuser3d/module/simplepickable.h"

namespace qtuser_3d
{
	class QTUSER_3D_API ManipulatePickable : public SimplePickable
	{
		Q_OBJECT
	public:
		ManipulatePickable(QObject* parent = nullptr);
		virtual ~ManipulatePickable();

		void setStateFactor(ControlState sf[3]);

		void setShowEntity(PickXEntity* entity);


	protected:
		void onStateChanged(ControlState state) override;
		void pickableEntityChanged();

	protected:

		ControlState m_stateFactor[3] = { ControlState::hover, ControlState::none, ControlState::none };
		PickXEntity* m_showEntity;
	};
}
#endif // QTUSER_3D_MANIPULATEPICKABLE_1595553785446_H