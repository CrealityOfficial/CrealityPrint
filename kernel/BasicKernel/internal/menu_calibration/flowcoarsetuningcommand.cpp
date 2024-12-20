#include "flowcoarsetuningcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    FlowCoarseTuningCommand::FlowCoarseTuningCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("FlowCoarseTuning");
        m_actionNameWithout = "FlowCoarseTuning";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    FlowCoarseTuningCommand::~FlowCoarseTuningCommand()
    {
    }

    void FlowCoarseTuningCommand::execute()
    {
        requestQmlDialog(this, "FlowCoarseTuningObj");
    }

    QString FlowCoarseTuningCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString FlowCoarseTuningCommand::getCurrentVersion()
    {
        return version();
    }

    void FlowCoarseTuningCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void FlowCoarseTuningCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("FlowCoarseTuning");
    }
}
