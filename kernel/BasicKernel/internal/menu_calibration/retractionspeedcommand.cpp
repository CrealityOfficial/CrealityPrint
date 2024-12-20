#include "retractionspeedcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    RetractionSpeedCommand::RetractionSpeedCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("RetractionSpeed");
        m_actionNameWithout = "RetractionSpeed";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    RetractionSpeedCommand::~RetractionSpeedCommand()
    {
    }

    void RetractionSpeedCommand::execute()
    {
        requestQmlDialog(this, "RetractionSpeedObj");
    }

    QString RetractionSpeedCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString RetractionSpeedCommand::getCurrentVersion()
    {
        return version();
    }

    void RetractionSpeedCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void RetractionSpeedCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("RetractionSpeed");
    }
}
