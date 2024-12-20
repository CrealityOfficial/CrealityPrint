#include "flowfinetuningcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    FlowFineTuningCommand::FlowFineTuningCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("FlowFineTuning");
        m_actionNameWithout = "FlowFineTuning";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    FlowFineTuningCommand::~FlowFineTuningCommand()
    {
    }

    void FlowFineTuningCommand::execute()
    {
        requestQmlDialog(this, "FlowFineTuningObj");
    }

    QString FlowFineTuningCommand::getWebsite()
    {
        return officialWebsite();
    }

    void FlowFineTuningCommand::getTuningTutorial()
    {
         openCalibrationTutorialWebsite();
    }

    QString FlowFineTuningCommand::getCurrentVersion()
    {
        return version();
    }

    void FlowFineTuningCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void FlowFineTuningCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("FlowFineTuning");
    }
}
