#ifndef CREATIVE_KERNEL_WIPE_TOWER_COLLISION_MESSAGE_202403121900_H
#define CREATIVE_KERNEL_WIPE_TOWER_COLLISION_MESSAGE_202403121900_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class WipeTowerEntity;

	class WipeTowerCollisionMessage : public MessageObject
	{
		Q_OBJECT

	public:
		WipeTowerCollisionMessage(WipeTowerEntity *wt, QObject* parent = NULL);
		~WipeTowerCollisionMessage();
	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:
		WipeTowerEntity* m_wipeTower;
	};

};


#endif // !CREATIVE_KERNEL_WIPE_TOWER_COLLISION_MESSAGE_202403121900_H