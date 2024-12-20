#ifndef CREATIVE_KERNEL_HIGHER_THAN_BOTTOM_MESSAGE_202405241610_H
#define CREATIVE_KERNEL_HIGHER_THAN_BOTTOM_MESSAGE_202405241610_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class HigherThanBottomMessage : public MessageObject
	{
		Q_OBJECT

	public:
		HigherThanBottomMessage(int m_groupId, QObject* parent = NULL);
		~HigherThanBottomMessage();

	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:
		int m_groupId;

	};

};

#endif // CREATIVE_KERNEL_STAY_ON_BORDER_MESSAGE_202312131652_H