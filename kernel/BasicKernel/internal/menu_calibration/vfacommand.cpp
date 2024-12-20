#include "vfacommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    VFACommand::VFACommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("VFA");
        m_actionNameWithout = "VFA";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    VFACommand::~VFACommand()
    {
    }

    void VFACommand::execute()
    {
        requestQmlDialog(this, "VFAObj");
    }

    QString VFACommand::getWebsite()
    {
        return officialWebsite();
    }

    QString VFACommand::getCurrentVersion()
    {
        return version();
    }

    void VFACommand::onThemeChanged(ThemeCategory category)
    {

    }

    void VFACommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("VFA");
    }
}
