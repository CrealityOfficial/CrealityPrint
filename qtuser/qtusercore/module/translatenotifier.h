#ifndef TRANSLATE_STATE_NOTIFIER_H
#define TRANSLATE_STATE_NOTIFIER_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QTranslator>
#include <QtCore/QMap>

class QTUSER_CORE_API TranslatorReceiver
{
public:
	TranslatorReceiver();
	virtual ~TranslatorReceiver();

	virtual void updateTrans();
protected:
};

class QTUSER_CORE_API TranslatorNotifier : public QObject
{
	Q_OBJECT
public:
	TranslatorNotifier(QObject* parent = nullptr);
	virtual ~TranslatorNotifier();

	void install(const QString& file);

	void add(TranslatorReceiver* receiver);
	void remove(TranslatorReceiver* receiver);
protected:
	QList<TranslatorReceiver*> m_receivers;

	QMap<QString, QTranslator*> m_translators;
	QTranslator* m_currentTranslator;
};
#endif // TRANSLATE_STATE_NOTIFIER_H