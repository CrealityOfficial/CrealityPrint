#include "anonymousjob.h"
#include "qtusercore/module/progressortracer.h"

namespace cxkernel
{
	AnonymousJob::AnonymousJob(anonymous_work_func wf, anonymous_func sf, QObject* parent)
		:Job(parent)
		, work_func(wf)
		, success_func(sf)
	{

	}

	AnonymousJob::~AnonymousJob()
	{

	}

	void AnonymousJob::setFailedFunc(anonymous_func func)
	{
		failed_func = func;
	}

	QString AnonymousJob::name()
	{
		return QString("AnonymousJob");
	}

	QString AnonymousJob::description()
	{
		return QString("AnonymousJob");
	}

	void AnonymousJob::failed()
	{
		if (failed_func)
			failed_func();
	}

	void AnonymousJob::successed(qtuser_core::Progressor* progressor)
	{
		if (success_func)
			success_func();
	}

	void AnonymousJob::work(qtuser_core::Progressor* progressor)
	{
		qtuser_core::ProgressorTracer tracer(progressor);
		if (work_func)
			work_func(&tracer);
	}
}