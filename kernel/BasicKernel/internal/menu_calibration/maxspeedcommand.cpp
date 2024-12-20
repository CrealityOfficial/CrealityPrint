#include "maxspeedcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    MaxSpeedCommand::MaxSpeedCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("MaxSpeed");
        m_actionNameWithout = "MaxSpeed";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    MaxSpeedCommand::~MaxSpeedCommand()
    {
    }

    void MaxSpeedCommand::execute()
    {
        requestQmlDialog(this, "MaxSpeedObj");
    }

    QString MaxSpeedCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString MaxSpeedCommand::getCurrentVersion()
    {
        return version();
    }

    void MaxSpeedCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void MaxSpeedCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("MaxSpeed");
    }
}
