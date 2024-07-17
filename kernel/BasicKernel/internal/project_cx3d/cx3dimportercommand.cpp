#include "cx3dimportercommand.h"

#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include <QCoreApplication>
namespace creative_kernel
{
    CX3DImporterCommand::CX3DImporterCommand(QObject* parent)
        : LoadProjectCommand(parent)
    {
        m_shortcut = "Ctrl+O";
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

    void CX3DImporterCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Open Project");
        cxkernel::openFile(this, QCoreApplication::translate("MenuCmdObj", "Open Project"),true);
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
    void CX3DImporterCommand::openCxprjAnd3MF(const QString& projectPath)
    {
        openProject(projectPath);
    }
    void CX3DImporterCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void CX3DImporterCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Open Project") + "        " + m_shortcut;
    }
}
