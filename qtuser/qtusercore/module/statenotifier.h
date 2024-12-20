#ifndef QML_STATE_NOTIFIER_H
#define QML_STATE_NOTIFIER_H
#include "qtusercore/qtusercoreexport.h"

class QTUSER_CORE_API StateReceiver
{
public:
	StateReceiver();
	virtual ~StateReceiver();

	void setMask(unsigned mask);
	unsigned mask();

	virtual void updateState(unsigned mask);
protected:
	unsigned m_mask;
};

class QTUSER_CORE_API StateNotifier : public QObject
{
	Q_OBJECT
public:
	StateNotifier(QObject* parent = nullptr);
	virtual ~StateNotifier();

	void notifyMask(unsigned mask);

	void add(StateReceiver* receiver);
	void remove(StateReceiver* receiver);
protected:
	QList<StateReceiver*> m_receivers;
};
#endif // QML_STATE_NOTIFIER_H