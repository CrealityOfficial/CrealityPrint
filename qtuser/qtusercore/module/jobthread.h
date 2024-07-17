#ifndef _QTUSER_CORE_JOBTHREAD_1590971578793_H
#define _QTUSER_CORE_JOBTHREAD_1590971578793_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/job.h"
#include "qtusercore/string/detaildescription.h"
#include <QtCore/QThread>

namespace qtuser_core
{
	class QTUSER_CORE_API JobThread: public QThread
		,public Progressor
	{
		Q_OBJECT
	public:
		JobThread(QObject* parent = nullptr);
		virtual ~JobThread();

		void setJob(JobPtr job);
		Job* job();

		void invokeFinished();
		DetailDescription* details();

		void setJobInterrupt();
		bool jobInterrupt();
	protected:
		void run() override;

		bool interrupt() override;
		void progress(float r) override;
		void failed(const QString& message) override;
		void message(const QString& msg) override;
		QString getFailReason() override;
	signals:
		void jobProgress(float r);
		void jobMessage(const QString& message);
	protected:
		JobPtr m_job;
		DetailDescription* m_details;

		bool m_jobFailed;
		bool m_jobInterrupt;
	};
}
#endif // _QTUSER_CORE_JOBTHREAD_1590971578793_H
