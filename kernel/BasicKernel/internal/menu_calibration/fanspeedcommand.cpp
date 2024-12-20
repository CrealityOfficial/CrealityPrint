#include "fanspeedcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    FanSpeedCommand::FanSpeedCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("FanSpeed");
        m_actionNameWithout = "FanSpeed";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    FanSpeedCommand::~FanSpeedCommand()
    {
    }

    void FanSpeedCommand::execute()
    {
        requestQmlDialog(this, "FanSpeedObj");
    }

    QString FanSpeedCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString FanSpeedCommand::getCurrentVersion()
    {
        return version();
    }

    void FanSpeedCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void FanSpeedCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("FanSpeed");
    }
}
