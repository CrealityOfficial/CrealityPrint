#include "tutorialcommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    TutorialCommand::TutorialCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Tutorial");
        m_actionNameWithout = "Tutorial";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this,this);
    }

    TutorialCommand::~TutorialCommand()
    {
    }

    void TutorialCommand::execute()
    {
        openCalibrationTutorialWebsite();
    }

    QString TutorialCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString TutorialCommand::getCurrentVersion()
    {
        return version();
    }

    void TutorialCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void TutorialCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Tutorial");
    }
}
