#ifndef QTUSER_QML_SOURCEPROVIDER_1615883776969_H
#define QTUSER_QML_SOURCEPROVIDER_1615883776969_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/statenotifier.h"
#include "qtusercore/module/translatenotifier.h"

namespace qtuser_qml
{
	class QTUSER_CORE_API SourceProvider : public QObject
		, public StateReceiver
		, public TranslatorReceiver
	{
		Q_OBJECT
	public:
		SourceProvider(QObject* parent = nullptr);
		virtual ~SourceProvider();

		virtual QString source() = 0;
		virtual void onStartFlow() = 0;
		virtual void onEndFlow() = 0;
	};
}

#endif // QTUSER_QML_SOURCEPROVIDER_1615883776969_H