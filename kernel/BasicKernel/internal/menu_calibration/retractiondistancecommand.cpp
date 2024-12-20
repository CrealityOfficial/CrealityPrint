#include "retractiondistancecommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    RetractionDistanceCommand::RetractionDistanceCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("RetractionDistance");
        m_actionNameWithout = "RetractionDistance";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    RetractionDistanceCommand::~RetractionDistanceCommand()
    {
    }

    void RetractionDistanceCommand::execute()
    {
        requestQmlDialog(this, "RetractionDistanceObj");
    }

    QString RetractionDistanceCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString RetractionDistanceCommand::getCurrentVersion()
    {
        return version();
    }

    void RetractionDistanceCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void RetractionDistanceCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("RetractionDistance");
    }
}
