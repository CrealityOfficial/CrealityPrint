#include "cx3dautosaveproject.h"
#include "cx3dexportjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/projectinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/visualsceneinterface.h"
#include "unittest/unittestflow.h"
#include <QFileInfo>
#include <QDir>
using namespace creative_kernel;

Cx3dAutoSaveProject::Cx3dAutoSaveProject(QObject* parent)
    : QObject(parent)
{    
    m_saveTaskJob = new SaveTaskJob(this);
}

Cx3dAutoSaveProject::~Cx3dAutoSaveProject()
{
    m_saveTaskJob->stop();
}
void Cx3dAutoSaveProject::stopAutoSaveJob()
{
    m_saveTaskJob->stop();
}
void Cx3dAutoSaveProject::startAutoSaveJob()
{
    m_saveTaskJob->start();
}
void Cx3dAutoSaveProject::startAutoSave()
{
    m_saveTaskJob->start();
    if (QFile::exists(projectUI()->getAutoProjectPath()))
    {
        QStringList params = qApp->arguments();
        if (params.size() == 2)
        {
            auto param = params[1];
            if (param.startsWith("crealityprintlink"))
            {
                return;
            }
        }
        requestQmlDialog(this, "openDefaultCx3d");
    }
    else
    {
        QString newPrejctPath = ":/cx3d/cx3d_exporter/.cxprj";
        QFileInfo file(newPrejctPath);
        if (file.exists())
        {
            qDebug() << "CX3DManager::createProject() file exists";
            creative_kernel::projectUI()->setProjectPath(newPrejctPath);
            creative_kernel::projectUI()->setProjectName("Untitled.cxprj");
            loadProject(newPrejctPath);
        }
    }
}

void Cx3dAutoSaveProject::reject()
{
    QDir tempDir(projectUI()->getAutoProjectPath());
    if(tempDir.exists())
    {
        tempDir.removeRecursively();
        tempDir.mkpath(tempDir.absolutePath());
        tempDir.mkpath(tempDir.absolutePath() + "/3D/Objects");
		tempDir.mkpath(tempDir.absolutePath() + "/Metadatas");
        tempDir.mkdir(tempDir.absolutePath() + "/3D/Objects/RecentSerializeModel/");
    } 
    projectUI()->setSettingsData("defalut");
    QString newPrejctPath = ":/cx3d/cx3d_exporter/.cxprj";
    QFileInfo file(newPrejctPath);
    if (file.exists())
    {
            loadProject(newPrejctPath,true);
            creative_kernel::projectUI()->setProjectPath(newPrejctPath);
            creative_kernel::projectUI()->setProjectName("Untitled.cxprj");
    }
    
}

void Cx3dAutoSaveProject::triggerAutoSave(QList<creative_kernel::ModelGroup*>  modelgroups,AutoSaveJob::SaveType type)
{
    std::shared_ptr<AutoSaveJob> autosavejob = std::make_shared<AutoSaveJob>(projectUI()->getAutoProjectPath(),modelgroups,type);
    if(m_saveTaskJob->isRunning())
        g_autosave_queue.put(autosavejob);
    //saveProject(projectUI()->getAutoProjectPath(), false, NULL, true,false,false);
}

void Cx3dAutoSaveProject::accept()
{
    projectUI()->setInitProjectDirty(true);
    loadProject(projectUI()->getAutoProjectPath(),false);
}

void Cx3dAutoSaveProject::updateTmpTime()
{
    saveProject(projectUI()->getAutoProjectPath(), false, NULL, false, false, false);
}
