#include "cx3dimportercommand.h"

#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cx3dexportjob.h"
#include "kernel/kernel.h"

#include "internal/project_cx3d/cx3dmanager.h"
#include <QCoreApplication>
namespace creative_kernel
{
    CX3DImporterCommand::CX3DImporterCommand(QObject* parent)
        : LoadProjectCommand(parent)
    {
        // m_shortcut = "Ctrl+O";
        m_actionname = tr("Open Project");
        m_actionNameWithout = "Open Project";
        m_icon = "qrc:/kernel/images/open.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
        cxkernel::registerOpenHandler(this);
    }

    CX3DImporterCommand::~CX3DImporterCommand()
    {

    }
    void CX3DImporterCommand::accept() 
    {
        QTimer::singleShot(1000,[this](){
                QString savePath = creative_kernel::projectUI()->getProjectPath();
                savePath = saveAsDialogPath();
                if(savePath.isEmpty())
                {
                    return;
                }
                getKernel()->cx3dManager()->setQuiet(m_quiet);
                saveProject(savePath);
                if (!m_menuMode)
                {
                    openProject(m_willOpenProject);
                }
                else
                {
                    cxkernel::openFile(this, QCoreApplication::translate("MenuCmdObj", "Open Project"), true);
                }
            });      
    }

    void CX3DImporterCommand::cancel()
    {
        if (!m_menuMode)
        {
            openProject(m_willOpenProject);
        }
        else
        {
            cxkernel::openFile(this, QCoreApplication::translate("MenuCmdObj", "Open Project"), true);
        }
    }

    void CX3DImporterCommand::execute()
    {
        m_menuMode = true;
        m_quiet = false;
        sensorAnlyticsTrace("File Menu", "Open Project");
        if (creative_kernel::projectUI()->spaceDirty())
        {
            creative_kernel::projectUI()->requestMenuDialog(this);
        }else{
            cxkernel::openFile(this, QCoreApplication::translate("MenuCmdObj", "Open Project"),true);
        }        
    }

    QString CX3DImporterCommand::filter()
    {
        QString _filter = "Project File (*.cxprj *.3mf) ";
        return _filter;
    }

    QString CX3DImporterCommand::filterKey()
    {
        return "Open Project";
    }

    void CX3DImporterCommand::handle(const QString& fileName)
    {
        openProject(fileName);
        getKernelUI()->switchPickMode();
    }

    void CX3DImporterCommand::openCxprjAnd3MF(const QString& projectPath, bool quiet)
    {
        m_willOpenProject = projectPath;
        m_quiet = quiet;
        m_menuMode = false;
        QString strOldPath = creative_kernel::projectUI()->getProjectPath();
        if (creative_kernel::projectUI()->spaceDirty() && !strOldPath.isEmpty() && strOldPath != m_willOpenProject && !quiet)
        {
            creative_kernel::projectUI()->requestMenuDialog(this);
        }
        else
        {
            openProject(projectPath);
        }
    }

    void CX3DImporterCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void CX3DImporterCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Open Project") + "        " + m_shortcut;
    }
}
