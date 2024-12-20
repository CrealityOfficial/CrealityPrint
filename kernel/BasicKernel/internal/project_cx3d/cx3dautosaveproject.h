#ifndef CX3DAUTOSAVEPROJECT_H
#define CX3DAUTOSAVEPROJECT_H
#include "data/interface.h"
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include "blockingqueue.h"
#include "autosavejob.h"
static BlockingQueue<std::shared_ptr<AutoSaveJob>> g_autosave_queue;
class SaveTaskJob : public QThread
{
public:
    SaveTaskJob(QObject* parent) : QThread(parent)
    {

    };
    void run() override
    {
        m_bRunning = true;
        while (m_bRunning) {
            if(!g_autosave_queue.isEmpty())
            {
                auto value = g_autosave_queue.take();
                value.get()->Save();
            }
            QThread::msleep(10);

            //qDebug() << "Consumer thread: " << QThread::currentThreadId() << ", value: " ;
        }
    }
    void stop()
    {
        g_autosave_queue.cleanQueue();
        while(this->isRunning())
        {
            m_bRunning = false;
            QThread::msleep(10);
        }
    }
private:
    bool m_bRunning = false;
};

class  Cx3dAutoSaveProject : public QObject
{
    Q_OBJECT
public:
    Cx3dAutoSaveProject(QObject* parent = nullptr);
    virtual ~Cx3dAutoSaveProject();

    Q_INVOKABLE void startAutoSave();
    Q_INVOKABLE void stopAutoSaveJob();
    Q_INVOKABLE void startAutoSaveJob();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();
    Q_INVOKABLE void triggerAutoSave(QList<creative_kernel::ModelGroup*>  modelgroups,AutoSaveJob::SaveType type);
public slots:
    void updateTmpTime();

private:
    SaveTaskJob *m_saveTaskJob;
};

#endif // CX3DAUTOSAVEPROJECT_H
