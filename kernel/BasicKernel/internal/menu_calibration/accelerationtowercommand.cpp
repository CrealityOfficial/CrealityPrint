#include "accelerationtowercommand.h"
#include "kernel/kernelui.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    AccelerationTowerCommand::AccelerationTowerCommand(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("AccelerationTower");
        m_actionNameWithout = "AccelerationTower";
        m_eParentMenu = eMenuType_Calibration;

        addUIVisualTracer(this, this);
    }

    AccelerationTowerCommand::~AccelerationTowerCommand()
    {
    }

    void AccelerationTowerCommand::execute()
    {
        requestQmlDialog(this, "AccelerationTowerObj");
    }

    QString AccelerationTowerCommand::getWebsite()
    {
        return officialWebsite();
    }

    QString AccelerationTowerCommand::getCurrentVersion()
    {
        return version();
    }

    void AccelerationTowerCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void AccelerationTowerCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("AccelerationTower");
    }
}
