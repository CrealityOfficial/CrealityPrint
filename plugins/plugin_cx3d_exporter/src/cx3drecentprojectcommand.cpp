
#include <QUrl>
#include <QFileInfo>
#include "cx3drecentprojectcommand.h"
#include "kernel/translator.h"
#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "cx3dexportjob.h"
#include "cx3dsubmenurecentproject.h"
#include "cx3dautosaveproject.h"
#include "kernel/projectinfoui.h"
#include "interface/uiinterface.h"

using namespace creative_kernel;
Cx3dRecentProjectCommand::Cx3dRecentProjectCommand(QObject* parent):ActionCommand(parent)
{
    m_actionname = "";
    m_eParentMenu = eMenuType_File;
    m_projectUI = new ProjectInfoUI(this);
    disconnect(m_projectUI, SIGNAL(acceptDialog()), this, SLOT(slotSaveLastPro()));
    connect(m_projectUI, SIGNAL(acceptDialog()), this, SLOT(slotSaveLastPro()));
    disconnect(m_projectUI,SIGNAL(cancelDialog()),this,SLOT(slotCancelSaveLastPro()));
    connect(m_projectUI,SIGNAL(cancelDialog()),this,SLOT(slotCancelSaveLastPro()));
}
Cx3dRecentProjectCommand::~Cx3dRecentProjectCommand()
{
    m_projectUI->deleteLater();
    m_projectUI = nullptr;
}
void Cx3dRecentProjectCommand:: execute()
{
    m_actionname = m_actionname.replace("file:///", "");//去掉前缀

    QFileInfo fileInfo(m_actionname);
    if (!fileInfo.exists())
    {
        requestQmlDialog(this, "openRecentlyFileDlg");
        return;
    }

    
    QString strOldPath = ProjectInfoUI::instance()->getProjectPath();//Cx3dAutoSaveProject::instance()->getProjectPath();
    if (!strOldPath.isEmpty() && strOldPath != m_actionname)
    {
        m_projectUI->requestMenuDialog();
    }
    else
    {
        m_projectUI->clearSecen();
        qtuser_core::JobPtr job(new Cx3dExportJob(m_actionname, Cx3dExportJob::JobType::READJOB));
        cxkernel::executeJob(job);
        Cx3dAutoSaveProject::instance()->openProject(m_actionname);
    }
}

void Cx3dRecentProjectCommand::slotSaveLastPro()
{
    qDebug()<<"m_strNewPath =" <<m_actionname;
    QString strOldPath = Cx3dAutoSaveProject::instance()->getProjectPath();

    Cx3dWriter *writer = new Cx3dWriter(strOldPath,nullptr);
    writer->deleteLater();

    ProjectInfoUI::instance()->clearSecen();
    qtuser_core::JobPtr job3(new Cx3dExportJob(m_actionname, Cx3dExportJob::JobType::READJOB));
    cxkernel::executeJob(job3);
    Cx3dAutoSaveProject::instance()->openProject(m_actionname);
    QSettings settings;
    Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_actionname);
}

void Cx3dRecentProjectCommand::slotCancelSaveLastPro()
{
    qDebug()<<"m_strNewPath =" <<m_actionname;
    ProjectInfoUI::instance()->clearSecen();
    qtuser_core::JobPtr job3(new Cx3dExportJob(m_actionname, Cx3dExportJob::JobType::READJOB));
    cxkernel::executeJob(job3);
    Cx3dAutoSaveProject::instance()->openProject(m_actionname);
    QSettings settings;
    Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_actionname);
}
