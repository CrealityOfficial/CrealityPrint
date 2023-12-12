#ifndef QTUSER_CORE_CYCLEJOB_1655528919841_H
#define QTUSER_CORE_CYCLEJOB_1655528919841_H
#include "qtusercore/module/job.h"
#include <QtCore/QSemaphore>

namespace ccglobal
{
	class Tracer;
}

namespace qtuser_core
{
	class QTUSER_CORE_API Consumer : public QThread
	{
		friend class CycleJob;
	public:
		Consumer(QObject* parent = nullptr);
		virtual ~Consumer();

	protected:
		void setInner(QSemaphore* semaphore, ccglobal::Tracer* tracer);
	protected:
		QSemaphore* m_semaphore;
		ccglobal::Tracer* m_tracer;
	};

	class QTUSER_CORE_API Producer : public QThread
	{
		friend class CycleJob;
	public:
		Producer(QObject* parent = nullptr);
		virtual ~Producer();

	protected:
		void setInner(QSemaphore* semaphore);
	protected:
		QSemaphore* m_semaphore;
	};

	class QTUSER_CORE_API CycleJob : public Job
	{
		Q_OBJECT
	public:
		CycleJob(QObject* parent = nullptr);
		virtual ~CycleJob();

		void setProducer(Producer* producer);
		void setConsumer(Consumer* consumer);
		void work(qtuser_core::Progressor* progressor) override;
	protected:
		virtual void beforeRun();
	protected:
		QSemaphore m_semaphore;

		Producer* m_producer;
		Consumer* m_consumer;
	};
}

#endif // QTUSER_CORE_CYCLEJOB_1655528919841_H