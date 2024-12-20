#include "arcfittingcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    ArcFittingCommand::ArcFittingCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("ArcFitting");
        m_actionNameWithout = "ArcFitting";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    ArcFittingCommand::~ArcFittingCommand()
    {
    }

    void ArcFittingCommand::execute()
    {
        requestQmlDialog(this, "ArcFittingObj");
    }

    QString ArcFittingCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString ArcFittingCommand::getCurrentVersion()
    {
        return version();
    }

    void ArcFittingCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void ArcFittingCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("ArcFitting");
    }
}
