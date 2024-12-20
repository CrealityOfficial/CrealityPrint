#include "updatecommand.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    UpdateCommand::UpdateCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Update");
        m_actionNameWithout = "Update";
        m_eParentMenu = eMenuType_Help;

        addUIVisualTracer(this,this);
    }

    UpdateCommand::~UpdateCommand()
    {
    }

    void UpdateCommand::execute()
    {
        requestQmlDialog(this, "updateObj");
    }

    void UpdateCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void UpdateCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Update");
    }
}
