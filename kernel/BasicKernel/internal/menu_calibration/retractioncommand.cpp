#include "retractioncommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    RetractionCommand::RetractionCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Retraction");
        m_actionNameWithout = "Retraction";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    RetractionCommand::~RetractionCommand()
    {
    }

    void RetractionCommand::execute()
    {
        requestQmlDialog(this, "RetractionObj");
    }

    QString RetractionCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString RetractionCommand::getCurrentVersion()
    {
        return version();
    }

    void RetractionCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void RetractionCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Retraction");
    }
}
