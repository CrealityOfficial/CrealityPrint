#ifndef CREATIVE_KERNEL_MESSAGEBUMP_1592732397175_H
#define CREATIVE_KERNEL_MESSAGEBUMP_1592732397175_H
#include "basickernelexport.h"
#include "message/messageobject.h"
#include <QtCore/QObject>

namespace creative_kernel
{
	class MessageNotify : public QObject
	{
		Q_OBJECT
	public:
		MessageNotify(QObject* parent = nullptr);
		virtual ~MessageNotify();

		void invokeBriefMessage(MessageObject* object);  //don't modify object anymore
	protected:

	};
}
#endif // CREATIVE_KERNEL_MESSAGEBUMP_1592732397175_H