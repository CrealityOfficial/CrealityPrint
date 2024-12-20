#include "slowaccelerationcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    SlowAccelerationCommand::SlowAccelerationCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("SlowAcceleration");
        m_actionNameWithout = "SlowAcceleration";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    SlowAccelerationCommand::~SlowAccelerationCommand()
    {
    }

    void SlowAccelerationCommand::execute()
    {
        requestQmlDialog(this, "SlowAccelerationObj");
    }

    QString SlowAccelerationCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString SlowAccelerationCommand::getCurrentVersion()
    {
        return version();
    }

    void SlowAccelerationCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void SlowAccelerationCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("SlowAcceleration");
    }
}
