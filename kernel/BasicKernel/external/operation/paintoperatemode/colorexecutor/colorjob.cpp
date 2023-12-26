#include "colorjob.h"
#include "colorexecutor.h"

ColorJob::ColorJob(ColorExecutor* executor)
    : m_executor(executor), qtuser_core::Job(executor)
{

}

bool ColorJob::showProgress()
{
    return false;
}

QString ColorJob::name() 
{
    return "colorjob";
}

QString ColorJob::description()
{
    return "colorjob";
}

void ColorJob::work(qtuser_core::Progressor* progressor) 
{
    while (true)
    {
        ColorWorker* worker = m_executor->takeWorker();
        if (worker == NULL)
            break;

        std::vector<int> data = worker->execute();
        worker->deleteLater();
        emit sig_oneFinished(data);
    }
}

void ColorJob::failed() 
{
    emit sig_allFinished();

}

void ColorJob::successed(qtuser_core::Progressor* progressor) 
{
    emit sig_allFinished();
}