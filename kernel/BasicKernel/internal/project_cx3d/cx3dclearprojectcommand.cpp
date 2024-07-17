#include "cx3dclearprojectcommand.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    Cx3dClearProjectCommand::Cx3dClearProjectCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Clear");
        m_actionNameWithout = "Clear";
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
        m_actionname = tr("Clear");
    }
}