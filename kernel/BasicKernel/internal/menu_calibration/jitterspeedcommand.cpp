#include "jitterspeedcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    JitterSpeedCommand::JitterSpeedCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("JitterSpeed");
        m_actionNameWithout = "JitterSpeed";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    JitterSpeedCommand::~JitterSpeedCommand()
    {
    }

    void JitterSpeedCommand::execute()
    {
        requestQmlDialog(this, "JitterSpeedObj");
    }

    QString JitterSpeedCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString JitterSpeedCommand::getCurrentVersion()
    {
        return version();
    }

    void JitterSpeedCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void JitterSpeedCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("JitterSpeed");
    }
}
