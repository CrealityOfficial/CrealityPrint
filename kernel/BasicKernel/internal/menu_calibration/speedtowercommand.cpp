#include "speedtowercommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    SpeedTowerCommand::SpeedTowerCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("SpeedTower");
        m_actionNameWithout = "SpeedTower";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    SpeedTowerCommand::~SpeedTowerCommand()
    {
    }

    void SpeedTowerCommand::execute()
    {
        requestQmlDialog(this, "SpeedTowerObj");
    }

    QString SpeedTowerCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString SpeedTowerCommand::getCurrentVersion()
    {
        return version();
    }

    void SpeedTowerCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void SpeedTowerCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("SpeedTower");
    }
}
