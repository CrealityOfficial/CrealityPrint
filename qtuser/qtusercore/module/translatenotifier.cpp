#include "qtusercore/module/translatenotifier.h"
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

TranslatorReceiver::TranslatorReceiver()
{

}

TranslatorReceiver::~TranslatorReceiver()
{

}

void TranslatorReceiver::updateTrans()
{

}

TranslatorNotifier::TranslatorNotifier(QObject* parent)
	:QObject(parent)
	, m_currentTranslator(nullptr)
{

}

TranslatorNotifier::~TranslatorNotifier()
{

}

void TranslatorNotifier::install(const QString& file)
{
	QTranslator* translator = nullptr;
	QMap<QString, QTranslator*>::iterator it = m_translators.find(file);
	if (it != m_translators.end())
		translator = it.value();
	else
	{
		translator = new QTranslator(this);
		if (!translator->load(file))
		{
			qDebug() << "load Translate File Error. " << file;
			delete translator;
			translator = nullptr;
		}

		if (translator)
			m_translators.insert(file, translator);
	}

	if (translator)
	{
		if (m_currentTranslator)
			QCoreApplication::removeTranslator(m_currentTranslator);

		QCoreApplication::installTranslator(translator);
		m_currentTranslator = translator;

		for (TranslatorReceiver* receiver : m_receivers)
			receiver->updateTrans();
	}
}

void TranslatorNotifier::add(TranslatorReceiver* receiver)
{
	if (receiver)
	{
		if (m_receivers.contains(receiver))
		{
			qDebug() << "reciver already in the notifier.";
		}
		else
		{
			m_receivers.append(receiver);
		}
	}
}

void TranslatorNotifier::remove(TranslatorReceiver* receiver)
{
	if(receiver)
		m_receivers.removeOne(receiver);
}