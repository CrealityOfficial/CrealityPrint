#include "cx3dimportercommand.h"
#include "cx3dexportjob.h"
#include "cx3dexporter.h"

#include "cxkernel/interface/jobsinterface.h"
#include "cx3dautosaveproject.h"
#include "kernel/projectinfoui.h"
#include "cx3dsubmenurecentproject.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"

namespace creative_kernel
{
    CX3DImporterCommand::CX3DImporterCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_shortcut = "Ctrl+O";
        m_actionname = tr("Open Project");
        m_actionNameWithout = "Open Project";
        m_icon = "qrc:/kernel/images/open.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this);

        disconnect(ProjectInfoUI::instance(), SIGNAL(acceptDialog()), this, SLOT(slotSaveLastPro()));
        connect(ProjectInfoUI::instance(), SIGNAL(acceptDialog()), this, SLOT(slotSaveLastPro()));

        disconnect(ProjectInfoUI::instance(), SIGNAL(cancelDialog()), this, SLOT(slotCancelSaveLastPro()));
        connect(ProjectInfoUI::instance(), SIGNAL(cancelDialog()), this, SLOT(slotCancelSaveLastPro()));
        cxkernel::registerOpenHandler(this);
    }

    CX3DImporterCommand::~CX3DImporterCommand()
    {

    }

    void CX3DImporterCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Open Project");
        cxkernel::openFile(this);
    }

    QString CX3DImporterCommand::filter()
    {
        QString _filter = "Project File (*.cxprj)";
        return _filter;
    }

    void CX3DImporterCommand::handle(const QString& fileName)
    {
        setNewPath(fileName);

        Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(fileName);
        QString strOldPath = ProjectInfoUI::instance()->getProjectPath();//Cx3dAutoSaveProject::instance()->getProjectPath();
        QString strPath = ProjectInfoUI::instance()->getProjectPath();
        if (!strOldPath.isEmpty() && strOldPath != fileName)
        {
            //        if(!strPath.isEmpty())
            //        {
            ProjectInfoUI::instance()->requestMenuDialog();
            //        }
        }
        else
        {
            ProjectInfoUI::instance()->clearSecen();
            qtuser_core::JobPtr job(new Cx3dExportJob(fileName, Cx3dExportJob::JobType::READJOB));
            cxkernel::executeJob(job);
            Cx3dAutoSaveProject::instance()->openProject(fileName);
        }
        getKernelUI()->switchPickMode();
    }

    void CX3DImporterCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void CX3DImporterCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Open Project") + "        " + m_shortcut;
    }

    void CX3DImporterCommand::setNewPath(QString pathName)
    {
        m_strNewPath = pathName;
    }

    void CX3DImporterCommand::slotSaveLastPro()
    {
        QString strOldPath = Cx3dAutoSaveProject::instance()->getProjectPath();

        Cx3dWriter* writer = new Cx3dWriter(strOldPath, nullptr);
        writer->deleteLater();

        ProjectInfoUI::instance()->clearSecen();
        qtuser_core::JobPtr job3(new Cx3dExportJob(m_strNewPath, Cx3dExportJob::JobType::READJOB));
        cxkernel::executeJob(job3);
        Cx3dAutoSaveProject::instance()->openProject(m_strNewPath);
    }

    void CX3DImporterCommand::slotCancelSaveLastPro()
    {
        qDebug() << "m_strNewPath =" << m_strNewPath;
        ProjectInfoUI::instance()->clearSecen();
        qtuser_core::JobPtr job3(new Cx3dExportJob(m_strNewPath, Cx3dExportJob::JobType::READJOB));
        cxkernel::executeJob(job3);
        Cx3dAutoSaveProject::instance()->openProject(m_strNewPath);
    }
}
