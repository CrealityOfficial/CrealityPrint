#include "mainthreadjob.h"

namespace creative_kernel
{
	MainThreadJob::MainThreadJob(QObject* parent)
    {
      
    }

	MainThreadJob::~MainThreadJob()
    {

    }

	QString MainThreadJob::name()
	{
		return "MainThreadJob.";
	}

	QString MainThreadJob::description()
	{
		return "MainThreadJob.";
	}

	void MainThreadJob::failed() 
    {

    }

	void MainThreadJob::successed(qtuser_core::Progressor* progressor) 
    {
		emit sig_finished();
    }

	void MainThreadJob::work(qtuser_core::Progressor* progressor) 
    {
		emit sig_mainThreadWork();
    }

}