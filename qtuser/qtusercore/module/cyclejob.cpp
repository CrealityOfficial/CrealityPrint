#include "cyclejob.h"
#include "progressortracer.h"

namespace qtuser_core
{
	Consumer::Consumer(QObject* parent)
		: QThread(parent)
		, m_semaphore(nullptr)
		, m_tracer(nullptr)
	{

	}

	Consumer::~Consumer()
	{

	}

	void Consumer::setInner(QSemaphore* semaphore, ccglobal::Tracer* tracer)
	{
		m_semaphore = semaphore;
		m_tracer = tracer;
	}

	Producer::Producer(QObject* parent)
		: QThread(parent)
		, m_semaphore(nullptr)
	{

	}

	Producer::~Producer()
	{
	}

	void Producer::setInner(QSemaphore* semaphore)
	{
		m_semaphore = semaphore;
	}

	CycleJob::CycleJob(QObject* parent)
		: Job(parent)
		, m_semaphore(2)
		, m_consumer(nullptr)
		, m_producer(nullptr)
	{

	}

	CycleJob::~CycleJob()
	{

	}

	void CycleJob::setProducer(Producer* producer)
	{
		m_producer = producer;
	}

	void CycleJob::setConsumer(Consumer* consumer)
	{
		m_consumer = consumer;
	}

	void CycleJob::work(qtuser_core::Progressor* progressor)
	{
		ProgressorTracer tracer(progressor);
		if (!m_consumer || !m_producer)
		{
			tracer.failed("no source no dest");
			tracer.progress(0.0f);
			return;
		}

		assert(!m_consumer->isRunning());
		assert(!m_producer->isRunning());

		m_producer->setInner(&m_semaphore);
		m_consumer->setInner(&m_semaphore, &tracer);
		beforeRun();

		m_semaphore.acquire(2);
		m_producer->start();
		m_consumer->start();

		m_consumer->wait();
		m_producer->requestInterruption();
		m_producer->wait();

		m_semaphore.acquire(2);
		m_semaphore.release(2);

		m_producer->setInner(nullptr);
		m_consumer->setInner(nullptr, nullptr);
	}

	void CycleJob::beforeRun()
	{

	}
}