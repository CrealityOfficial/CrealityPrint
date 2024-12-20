#include "maxaccelerationcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    MaxAccelerationCommand::MaxAccelerationCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("MaxAcceleration");
        m_actionNameWithout = "MaxAcceleration";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    MaxAccelerationCommand::~MaxAccelerationCommand()
    {
    }

    void MaxAccelerationCommand::execute()
    {
        requestQmlDialog(this, "MaxAccelerationObj");
    }

    QString MaxAccelerationCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString MaxAccelerationCommand::getCurrentVersion()
    {
        return version();
    }

    void MaxAccelerationCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void MaxAccelerationCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("MaxAcceleration");
    }
}
