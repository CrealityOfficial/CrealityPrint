#include "qtusercore/module/statenotifier.h"
#include <QtCore/QDebug>

StateReceiver::StateReceiver()
	:m_mask(0)
{

}

StateReceiver::~StateReceiver()
{

}

void StateReceiver::setMask(unsigned mask)
{
	m_mask = mask;
}

unsigned StateReceiver::mask()
{
	return m_mask;
}

void StateReceiver::updateState(unsigned mask)
{

}

StateNotifier::StateNotifier(QObject* parent)
	:QObject(parent)
{

}

StateNotifier::~StateNotifier()
{

}

void StateNotifier::notifyMask(unsigned mask)
{
	for (StateReceiver* receiver : m_receivers)
	{
		unsigned m = receiver->mask();
		if (mask & m)
			receiver->updateState(mask);
	}
}

void StateNotifier::add(StateReceiver* receiver)
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

void StateNotifier::remove(StateReceiver* receiver)
{
	if(receiver)
		m_receivers.removeOne(receiver);
}