#include "jobsinterface.h"
#include "cxkernel/kernel/cxkernel.h"

namespace cxkernel
{
	bool jobExecutorAvaillable()
	{
		return !cxKernel->jobExecutor()->isRunning();
	}

	bool executeJobs(const QList<qtuser_core::JobPtr>& jobs, bool front)
	{
		return cxKernel->jobExecutor()->execute(jobs, front);
	}

	bool executeJob(qtuser_core::JobPtr job, bool front)
	{
		return cxKernel->jobExecutor()->execute(job, front);
	}

	bool executeJob(qtuser_core::Job* job, bool front)
	{
		return cxKernel->jobExecutor()->execute(job, front);
	}

	void addJobTracer(qtuser_core::JobTracer* tracer)
	{
		cxKernel->jobExecutor()->addJobTracer(tracer);
	}

	void removeJobTracer(qtuser_core::JobTracer* tracer)
	{
		cxKernel->jobExecutor()->removeJobTracer(tracer);
	}
}