#ifndef CXKERNEL_ANONYMOUSJOB_1689304447078_H
#define CXKERNEL_ANONYMOUSJOB_1689304447078_H
#include "qtusercore/module/job.h"
#include "cxkernel/data/header.h"

namespace cxkernel
{
	class AnonymousJob : public qtuser_core::Job
	{
	public:
		AnonymousJob(anonymous_work_func wf, anonymous_func sf, QObject* parent = nullptr);
		virtual ~AnonymousJob();

	protected:
		QString name();
		QString description();
		void failed();                        // invoke from main thread
		void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
		void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	protected:
		anonymous_work_func work_func;
		anonymous_func success_func;
	};
}

#endif // CXKERNEL_ANONYMOUSJOB_1689304447078_H