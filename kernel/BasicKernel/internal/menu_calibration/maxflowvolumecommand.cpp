#include "maxflowvolumecommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    MaxFlowVolumeCommand::MaxFlowVolumeCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("MaxFlowVolume");
        m_actionNameWithout = "MaxFlowVolume";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    MaxFlowVolumeCommand::~MaxFlowVolumeCommand()
    {
    }

    void MaxFlowVolumeCommand::execute()
    {
        requestQmlDialog(this, "MaxFlowVolumeObj");
    }

    QString MaxFlowVolumeCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString MaxFlowVolumeCommand::getCurrentVersion()
    {
        return version();
    }

    void MaxFlowVolumeCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void MaxFlowVolumeCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("MaxFlowVolume");
    }
}
