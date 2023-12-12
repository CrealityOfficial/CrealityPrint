#include "cx3dexportjob.h"
#include "cx3dwriter.h"
#include "cx3dreader.h"
#include <QtCore/QFileInfo>
#include "cx3drenderex.h"
#include "data/fdmsupportgroup.h"
#include "cx3dautosaveproject.h"
#include "cx3dsubmenurecentproject.h"

#include "qtusercore/string/resourcesfinder.h"
#include "cxkernel/data/modelndataserial.h"

#include "interface/uiinterface.h"
#include "interface/projectinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/spaceinterface.h"

using namespace creative_kernel;
Cx3dExportJob::Cx3dExportJob(QString saveFilename,JobType jobtype,QObject* parent)
    :m_saveFilename(saveFilename)
    , m_jobType(jobtype)
    , m_buildScene(nullptr)
    , m_mainThread(nullptr)
    , m_closeWindow(false)
    , m_successObject(nullptr)
    , m_showProgress(true)
{
    m_mainThread = QThread::currentThread();
    m_buildScene = new Cx3dScene(this);
}

Cx3dExportJob::~Cx3dExportJob()
{
}

void Cx3dExportJob::AddModel2Scence()
{
}

QString Cx3dExportJob::getMessageText()
{
    return tr("No matching printer found, cannot import project file");
}

void Cx3dExportJob::failed()                        // invoke from main thread
{
    if ((m_jobType == JobType::WRITEJOB) && m_closeWindow)
        requestQmlCloseAction();

    creative_kernel::requestQmlDialog(this, "messageDlg");
}

void Cx3dExportJob::setMachineProfile()
{

}

bool Cx3dExportJob::showProgress()
{
    return m_showProgress;
}

QString Cx3dExportJob::name()
{
    if (m_jobType == JobType::WRITEJOB)
    {
        return "AutoSaveJob";
    }

    return "ReadJob";    
}

void Cx3dExportJob::successed(qtuser_core::Progressor* progressor)
{
    if (m_jobType == JobType::READJOB)
    {
        m_buildScene->setScene();
    }
    else
    {

    }

    

    projectUI()->setProjectPath(m_saveFilename);
    if (m_saveFilename.indexOf("tmpProject/default.cxprj")<0)
    {
        Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_saveFilename);
    }
    projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_saveFilename));
    projectUI()->updateProjectNameUI();

    if((m_jobType == JobType::WRITEJOB) && m_closeWindow)
        requestQmlCloseAction();

    if((m_jobType == JobType::WRITEJOB) && m_successObject)
        requestQmlDialog(m_successObject, "messageDlg");
}

void Cx3dExportJob::setCloseWindow(bool close)
{
    m_closeWindow = close;
}

void Cx3dExportJob::setCaches(const QList<creative_kernel::ModelN*>& models)
{
    m_caches = models;
}

void Cx3dExportJob::setExportSuccessObject(QObject* object)
{
    m_successObject = object;
}

void Cx3dExportJob::setShowProgress(bool show)
{
    m_showProgress = show;
}

void Cx3dExportJob::work(qtuser_core::Progressor* progressor)
{
    if(m_jobType == JobType::WRITEJOB)
    {
        if(!m_showProgress)
        {
            for (creative_kernel::ModelN* m : m_caches)
            {
                QFileInfo fileInfo(m->getSerialName());
                QString filename = fileInfo.fileName();
                filename = QString("%1/%2").arg(qtuser_core::getOrCreateAppDataLocation("tmpProject/models")).arg(filename);
                if (!QFileInfo(filename).exists())
                {
                    cxkernel::ModelNDataSerial serial;
                    serial.setData(m->modelNData());
                    serial.save(filename, nullptr);
                }
            }
        }
        Cx3dWriter writer(m_caches, m_saveFilename, progressor,nullptr,!m_showProgress);
    }
    else
    {
        Cx3dRenderEx reader(m_saveFilename, progressor);
        reader.build();
        m_buildScene->build(&reader, m_mainThread);
        //reader->deleteLater();
    }
}

void saveProject(const QString& projectName, bool showProgress, QObject* successObject, bool force)
{
    if (!force && !cxkernel::jobExecutorAvaillable())
        return;

    Cx3dExportJob* job = new Cx3dExportJob(projectName, Cx3dExportJob::JobType::WRITEJOB);
    job->setCaches(modelns());
    job->setShowProgress(showProgress);
    job->setExportSuccessObject(successObject);

    cxkernel::executeJob(job);
}

void loadProject(const QString& projectName)
{
    creative_kernel::projectUI()->clearSecen();

    Cx3dExportJob* job3 = new Cx3dExportJob(projectName, Cx3dExportJob::JobType::READJOB);
    cxkernel::executeJob(job3);
}

