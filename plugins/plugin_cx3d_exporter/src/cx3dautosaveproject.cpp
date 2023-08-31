#include "cx3dautosaveproject.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
//#include "interface/jobsinterface.h"
#include "cx3dexportjob.h"
#include "cx3dexporter.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qtuserqml/property/qmlpropertysetter.h"
#include "kernel/projectinfoui.h"
#include "cx3dsubmenurecentproject.h"
#include "kernel/kernelui.h"
#include "interface/uiinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "kernel/kernel.h"
#include <thread>

using namespace creative_kernel;
using namespace qtuser_3d;
using namespace qtuser_qml;
// class creative_kernel::ModelSpaceUndo;
namespace creative_kernel
{
    class ModelSpaceUndo;
}
Cx3dAutoSaveProject::Cx3dAutoSaveProject(QObject* parent)
    :QObject(parent)
{
    QString tmpProject = qtuser_core::getOrCreateAppDataLocation("") + "/tmpProject";
    QDir tempDir;
    if (!tempDir.exists(tmpProject))
    {
        tempDir.mkpath(tmpProject);
    }
    m_strTempFilePath = qtuser_core::getOrCreateAppDataLocation("") + "/tmpProject" + "/default.cxprj";
    setSettingsData(m_strTempFilePath);
    QSettings setting;
    setting.beginGroup("AutoSave_Path");
    QString strFileName = setting.value("filePath","default").toString();
    setting.endGroup();
    if(!strFileName.contains("default"))
    {
        m_strTempFilePath = strFileName;
    }
    m_fSeconds = ProjectInfoUI::instance()->getMinutes();
    connect(ProjectInfoUI::instance(),SIGNAL(minutesChanged(float)),this,SLOT(slotMinutesChanged(float)));
}

Cx3dAutoSaveProject::~Cx3dAutoSaveProject()
{
    stop();
    if (tmp_timer)
    {
        tmp_timer->deleteLater();
    }
    
}

Cx3dAutoSaveProject *Cx3dAutoSaveProject::instance()
{
    static Cx3dAutoSaveProject m_saveManger;
    return  &m_saveManger;
}

void Cx3dAutoSaveProject::start()
{
    if (QFile::exists(m_strTempFilePath))
    {
        if (!tmp_timer)
        {
            tmp_timer = new QTimer(this);
            tmp_timer->start(500);
            connect(tmp_timer, SIGNAL(timeout()), this, SLOT(updateTmpTime()));
        }
    }
    else
    {
        startBackupTask();
    }
}

void Cx3dAutoSaveProject::startBackupTask()
{
    std::thread(&Cx3dAutoSaveProject::run, this).detach();
}

void Cx3dAutoSaveProject::stop()
{
    m_bExit = true;
}

void Cx3dAutoSaveProject::run()
{
    time_t last_backup_time = 0;
    while (!m_bExit)
    {
        if (getModelActiveTime() - last_backup_time > m_fSeconds)
        {
            Cx3dWriter writer(m_strTempFilePath, nullptr, nullptr, true);
            last_backup_time = time(nullptr);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Cx3dAutoSaveProject::reject()
{
    QFile file(m_strTempFilePath);
    if (file.exists())
    {
        file.remove();
    }
    startBackupTask();
}

void Cx3dAutoSaveProject::accept()
{
    qtuser_core::JobPtr job(new Cx3dExportJob(m_strTempFilePath, Cx3dExportJob::JobType::READJOB));
    cxkernel::executeJob(job);
    startBackupTask();
}

void Cx3dAutoSaveProject::openProject(QString strFilePath)
{
    Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(strFilePath);
    ProjectInfoUI::instance()->setProjectPath(strFilePath);
    ProjectInfoUI::instance()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(strFilePath));
    ProjectInfoUI::instance()->updateProjectNameUI();
}

void Cx3dAutoSaveProject::updateTmpTime()
{
    requestQmlDialog(this, "openDefaultCx3d");
    if (tmp_timer)
    {
        tmp_timer->stop();
    }
}

void Cx3dAutoSaveProject::slotMinutesChanged(float nMinute)
{
    m_fSeconds = nMinute;
    qDebug()<<"Cx3dAutoSaveProject::slotMinutesChanged nMinute =" <<m_fSeconds;
    if(nMinute == 0.0)return;
}

void Cx3dAutoSaveProject::setSettingsData(QString file)
{
    file = file.replace("file:///", "");
    QSettings setting;
    setting.beginGroup("AutoSave_Path");
    setting.setValue("filePath", file);
    setting.endGroup();
}

QString Cx3dAutoSaveProject::getProjectPath()
{
    return m_strTempFilePath;
}
