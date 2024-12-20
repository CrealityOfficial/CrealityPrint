#include "cx3dclearprojectcommand.h"
#include "interface/commandinterface.h"
#include "buildinfo.h"
#include <QCoreApplication>

namespace creative_kernel
{
    Cx3dClearProjectCommand::Cx3dClearProjectCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Clear History");
        m_actionNameWithout = "Clear history";
        m_eParentMenu = eMenuType_File;
        m_bSeparator = true;

        addUIVisualTracer(this,this);
    }

    Cx3dClearProjectCommand::~Cx3dClearProjectCommand()
    {
    }

    void Cx3dClearProjectCommand::execute()
    {
        emit sigExecute();
    }

    void Cx3dClearProjectCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void Cx3dClearProjectCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate(TRANSLATE_CONTEXT, "Clear History");
    }
}