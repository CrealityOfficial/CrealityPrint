#include "colorexecutor.h"
#include "colorjob.h"
#include <QDebug>
#include "cxkernel/interface/jobsinterface.h"
#include "spread/meshspread.h"

#define AVAILABLE_THREAD 10

ColorExecutor::ColorExecutor(QObject* parent) :
    QObject(parent)
{
    m_operateTimer.setSingleShot(true);
}

void ColorExecutor::setColorIndex(int colorIndex)
{
    m_colorIndex = colorIndex;
}

trimesh::TriMesh* ColorExecutor::getTrimesh()
{
    return NULL;
}

void ColorExecutor::triangleColor(spread::MeshSpreadWrapper *wrapper)
{
    ColorWorker worker(wrapper, m_colorIndex, spread::CursorType::TRIANGLE);
    auto dirtyChunks = worker.execute();
    emit sig_needUpdate(wrapper, dirtyChunks);
}

void ColorExecutor::circleColor(spread::MeshSpreadWrapper *wrapper, const spread::SceneData& data, bool isFirstCircle)
{
    ColorWorker worker(wrapper, m_colorIndex, spread::CursorType::CIRCLE);
    worker.setCircleParameter(data, isFirstCircle);
    auto dirtyChunks = worker.execute();
    emit sig_needUpdate(wrapper, dirtyChunks);
}

void ColorExecutor::sphereColor(spread::MeshSpreadWrapper *wrapper, const spread::SceneData& data, float sphereSize, bool isFirstCircle)
{
    ColorWorker worker(wrapper, m_colorIndex, spread::CursorType::SPHERE);
    worker.setSphereParameter(data, sphereSize, isFirstCircle);
    auto dirtyChunks = worker.execute();
    emit sig_needUpdate(wrapper, dirtyChunks);
}
	
void ColorExecutor::fillColor(spread::MeshSpreadWrapper *wrapper)
{
    ColorWorker worker(wrapper, m_colorIndex, spread::CursorType::FILL);
    auto dirtyChunks = worker.execute();
    emit sig_needUpdate(wrapper, dirtyChunks);
}

void ColorExecutor::heightRangeColor(spread::MeshSpreadWrapper *wrapper, const spread::SceneData& data, float height)
{
    ColorWorker worker(wrapper, m_colorIndex, spread::CursorType::HEIGHT_RANGE);
    worker.setHeightRangeParameter(data, height);
    auto dirtyChunks = worker.execute();
    emit sig_needUpdate(wrapper, dirtyChunks);
}

void ColorExecutor::fillGapColor(spread::MeshSpreadWrapper *wrapper)
{
    ColorWorker worker(wrapper, m_colorIndex, spread::CursorType::GAP_FILL);
    auto dirtyChunks = worker.execute();
    emit sig_needUpdate(wrapper, dirtyChunks);
}

void ColorExecutor::appendWorker(ColorWorker* worker)
{
    lockWork();
    m_workers << worker;
    unLockWork();
}

ColorWorker* ColorExecutor::takeWorker()
{
    ColorWorker* worker = NULL;

    lockWork();
    if (m_workers.count() <= 0)
        worker = NULL;
    else 
        worker = m_workers.takeFirst();
    unLockWork();

    return worker;
}

void ColorExecutor::clearWorkers()
{
    lockWork();
    qDeleteAll(m_workers);
    m_workers.clear();
    unLockWork();
}

void ColorExecutor::lockWork()
{
    m_workerMutex.lock();
}

void ColorExecutor::unLockWork()
{
    m_workerMutex.unlock();
}

bool ColorExecutor::isOperateValid()
{
    if (m_operateTimer.isActive())
        return false;
    
    if (m_workers.count() >= AVAILABLE_THREAD)
        return false;
    
    //m_operateTimer.start(40);
    return true;
}

void ColorExecutor::activeJob()
{
    if (m_job != NULL)
        return;

    m_job = new ColorJob(this);
    connect(m_job, &ColorJob::sig_allFinished, this, [=] ()
    {
        m_job = NULL;
    });

    connect(m_job, &ColorJob::sig_oneFinished, this, [=] (std::vector<int> dirtyChunks)
    {
        if (m_workers.count() > 3)  
            return;     // 剩余任务过多时不依次显示

        emit sig_needUpdate(NULL, dirtyChunks);

    }, Qt::QueuedConnection);

	cxkernel::executeJob(qtuser_core::JobPtr(m_job));
}