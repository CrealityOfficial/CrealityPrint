#include "messagenotify.h"
#include "interface/uiinterface.h"
#include <QtCore/QTimer>

namespace creative_kernel
{
	MessageNotify::MessageNotify(QObject* parent)
		:QObject(parent)
	{

	}

	MessageNotify::~MessageNotify()
	{

	}

	void MessageNotify::invokeBriefMessage(MessageObject* object)
	{
		if (!object)
			return;

		QMetaObject::invokeMethod(this, [this, object]() {
			requestQmlMessage(object);
			int code = object->code();
			QTimer::singleShot(3000, [code]() {
				destroyQmlMessage(code);
				});
			}, Qt::ConnectionType::QueuedConnection);
	}
}