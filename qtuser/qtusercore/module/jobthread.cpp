#include "jobthread.h"
#include <QtCore/QDebug>

namespace qtuser_core
{
	JobThread::JobThread(QObject* parent)
		:QThread(parent)
		, m_job(nullptr)
		, m_jobFailed(false)
		, m_jobInterrupt(false)
	{
		m_details = new DetailDescription(this);
	}

	JobThread::~JobThread()
	{
	}

	void JobThread::setJob(JobPtr job)
	{
		if (isRunning())
		{
			qDebug() << "Job thread is running.";
			return;
		}

		m_details->clear();
		m_job = job;
		m_jobFailed = false;
		m_jobInterrupt = false;
		if (m_job)
		{
			m_details->set("name", m_job->name());
			m_details->set("description", m_job->description());
		}
	}

	Job* JobThread::job()
	{
		return m_job.data();
	}

	void JobThread::run()
	{
		if (m_job)
		{
			m_job->work(this);
		}
	}

	void JobThread::invokeFinished()
	{
		if (!m_job)
		{
			m_details->set("status", "failed");
			m_details->set("reason", "null job");
			return;
		}

		if (m_jobFailed)
		{
			m_job->failed();
			return;
		}

		if (m_jobInterrupt)
		{
			m_job->failed();
			return;
		}

		m_details->set("status", "success");

		m_job->successed(this);
	}

	DetailDescription* JobThread::details()
	{
		return m_details;
	}

	bool JobThread::interrupt()
	{
		return isInterruptionRequested();
	}

	void JobThread::progress(float r)
	{
		emit jobProgress(r);
	}

	void JobThread::failed(const QString& message)
	{
		m_jobFailed = true;
		m_details->set("status", "failed");
		m_details->set("reason", message);
	}

	void JobThread::message(const QString& msg)
	{
		emit jobMessage(msg);
	}

	QString JobThread::getFailReason()
	{
		return m_details->get("reason");
	}

	void JobThread::setJobInterrupt()
	{
		m_jobInterrupt = true;
		m_details->set("status", "failed");
		m_details->set("reason", "user interrupt.");
	}

	bool JobThread::jobInterrupt()
	{
		return m_jobInterrupt;
	}
}
