#ifndef _CREATIVE_KERNEL_JOBEXECUTOR_1590379536512_H
#define _CREATIVE_KERNEL_JOBEXECUTOR_1590379536512_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/jobthread.h"
#include <QtCore/QList>

namespace qtuser_core
{
	class JobTracer
	{
	public:
		virtual ~JobTracer() {}
		virtual void jobExecutorStart() = 0;
		virtual void jobExecutorStop() = 0;
	};

	class QTUSER_CORE_API JobExecutor: public QObject
	{
		Q_OBJECT
		Q_PROPERTY(bool isRunning READ isRunning NOTIFY  isRunningChanged)
		Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
	public:
		JobExecutor(QObject* parent = nullptr);
		virtual ~JobExecutor();

		Q_INVOKABLE void stop();

        Q_INVOKABLE QString getDescription();
		bool isRunning();
		bool visible();
		bool execute(const QList<JobPtr>& jobs, bool front = false);
		bool execute(JobPtr job, bool front = false);
		bool execute(Job* job, bool front = false);
		void addJobTracer(JobTracer* tracer);
		void removeJobTracer(JobTracer* tracer);
	public slots:
		void onJobFinished();
	signals:
		void jobsStart();
		void jobsEnd();
		void jobStart(QObject* details);
		void jobEnd(QObject* details);
		void jobProgress(float r);
		void jobMessage(const QString& msg);
		void isRunningChanged();
		void visibleChanged();
	protected:
		void startJob();
	protected:
		QList<JobPtr> m_exsitJobs;
		JobPtr m_runningJob;

		JobThread* m_runThread;

		bool m_running;
		QList<JobTracer*> m_tracers;

        QString m_jobDescription = "";
	};
}
#endif // _CREATIVE_KERNEL_JOBEXECUTOR_1590379536512_H
