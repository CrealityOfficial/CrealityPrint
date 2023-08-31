#include "cx3dexportjob.h"
#include "cx3dwriter.h"
#include "cx3dreader.h"
#include "trimesh2/TriMesh.h"
#include "data/modelspace.h"
#include "utils/modelpositioninitializer.h"
#include <QtCore/QFileInfo>
#include "cx3drenderex.h"
#include "data/fdmsupportgroup.h"
#include "interface/uiinterface.h"

using namespace creative_kernel;
Cx3dExportJob::Cx3dExportJob(QString saveFilename,JobType jobtype,QObject* parent)
    :m_saveFilename(saveFilename)
    ,m_jobType(jobtype)
    , m_buildScene(nullptr)
    , m_mainThread(nullptr)
{
    m_mainThread = QThread::currentThread();
}

Cx3dExportJob::~Cx3dExportJob()
{
    if (m_buildScene)
    {
        delete m_buildScene;
        m_buildScene = nullptr;
    }
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
    creative_kernel::requestQmlDialog(this, "messageDlg");
}

void Cx3dExportJob::setMachineProfile()
{

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
}

void Cx3dExportJob::work(qtuser_core::Progressor* progressor)
{
    if(m_jobType == JobType::WRITEJOB)
    {
        Cx3dWriter *writer = new Cx3dWriter(m_saveFilename,progressor);
        writer->deleteLater();

    }
    else
    {
        Cx3dRenderEx* reader = new Cx3dRenderEx(m_saveFilename, progressor);
        reader->build();
        m_buildScene = new Cx3dScene();
        m_buildScene->build(reader,m_mainThread);
        //reader->deleteLater();
    }
}

