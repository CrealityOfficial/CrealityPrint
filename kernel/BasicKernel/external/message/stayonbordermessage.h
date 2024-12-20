#ifndef CREATIVE_KERNEL_STAY_ON_BORDER_MESSAGE_202312131652_H
#define CREATIVE_KERNEL_STAY_ON_BORDER_MESSAGE_202312131652_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
    class ModelGroup;
	class StayOnBorderMessage : public MessageObject
	{
		Q_OBJECT

	public:
		StayOnBorderMessage(QList<ModelGroup*> groups, QObject* parent = NULL);
		~StayOnBorderMessage();

	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:
		QList<ModelGroup*> m_groups;

	};

};

#endif // CREATIVE_KERNEL_STAY_ON_BORDER_MESSAGE_202312131652_H