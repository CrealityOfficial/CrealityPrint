#include "job.h"

namespace qtuser_core
{
	Job::Job(QObject* parent)
		:QObject(parent)
	{
	}

	Job::~Job()
	{
	}

	QString Job::name()
	{
		return "";
	}

	QString Job::description()
	{
		return "";
	}

	void Job::failed()
	{

	}

	void Job::prepare()
	{

	}

	void Job::successed(qtuser_core::Progressor* progressor)
	{

	}

	void Job::work(Progressor* progressor)
	{
		(void)progressor;
	}
}
