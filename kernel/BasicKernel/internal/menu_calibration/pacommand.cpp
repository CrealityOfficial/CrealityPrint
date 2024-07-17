#include "pacommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    PACommand::PACommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("PA");
        m_actionNameWithout = "PA";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    PACommand::~PACommand()
    {
    }

    void PACommand::execute()
    {
        requestQmlDialog(this, "PAObj");
    }

    QString PACommand::getWebsite()
    {
        return officialWebsite();
    }

    QString PACommand::getCurrentVersion()
    {
        return version();
    }

    void PACommand::onThemeChanged(ThemeCategory category)
    {

    }

    void PACommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("PA");
    }
}
