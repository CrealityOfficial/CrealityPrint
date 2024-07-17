#include "aboutuscommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    AboutUsCommand::AboutUsCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("About Us");
        m_actionNameWithout = "About Us";
        m_eParentMenu = eMenuType_Help;

        addUIVisualTracer(this,this);
    }

    AboutUsCommand::~AboutUsCommand()
    {
    }

    void AboutUsCommand::execute()
    {
        requestQmlDialog(this, "aboutUsObj");
    }

    void AboutUsCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void AboutUsCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("About Us");
    }
}
