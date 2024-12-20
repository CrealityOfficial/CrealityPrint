#ifndef CREATIVE_KERNEL_MODEL_COLLISION_MESSAGE_202402211714_H
#define CREATIVE_KERNEL_MODEL_COLLISION_MESSAGE_202402211714_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class ModelGroup;
	class ModelNCollisionMessage : public MessageObject
	{
		Q_OBJECT

	public:
		ModelNCollisionMessage(QList<ModelGroup*> groups, QObject* parent = NULL);
		~ModelNCollisionMessage();
		void setLevel(int level);
	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:
		QList<ModelGroup*> m_groups;
		int m_level{ 0 };
	};

};


#endif // !CREATIVE_KERNEL_MODEL_COLLISION_MESSAGE_202402211714_H