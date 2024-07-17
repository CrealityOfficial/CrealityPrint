#include "nozzleaction.h"

#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "setnozzlerange.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmachine.h"
namespace creative_kernel
{
    NozzleAction::NozzleAction(int nozzleIndex,QObject* parent, QString currentColor)
    {
        this->setParent(parent);
        m_NozzleIndex = nozzleIndex;

        //根据当双喷机型改变菜单内容
        PrintMachine* curMachine = ParameterManager::currentMachine_s();
        int extruderCount = curMachine->extruderCount();
        if (extruderCount == 1)
        {
            m_actionname = tr("Material ") + QString::number(m_NozzleIndex + 1);
        }
        else if (extruderCount == 2)
        {
            m_actionname = tr("Nozzle ") + QString::number(m_NozzleIndex + 1);
        }

        m_actionNameWithout = "Nozzle";
        m_bSubMenu = true;
        m_bCheckable = true;
        m_CurrentColor = currentColor;
        addUIVisualTracer(this,this);
    }

    NozzleAction::~NozzleAction()
    {

    }

    void NozzleAction::setExtruderObj(PrintExtruder* extruderObj)
    {
        assert(extruderObj);
        m_PrintExtruder = extruderObj;
    }

    QColor NozzleAction::nozzleColor()
    {
        return m_PrintExtruder->color();
    }

    void NozzleAction::setNozzleColor(QColor color)
    {

    }

    void NozzleAction::execute()
    {
        SetNozzleRange* nozzlerange = qobject_cast<SetNozzleRange*>(parent());
        setSelectionModelsNozzle(m_NozzleIndex);
        nozzlerange->updateNozzleCheckStatus(m_NozzleIndex, this->bChecked());
    }

    bool NozzleAction::enabled()
    {
        return true;
    }

    void NozzleAction::onThemeChanged(ThemeCategory category)
    {
    }

    void NozzleAction::onLanguageChanged(MultiLanguage language)
    {
        //根据当双喷机型改变菜单内容
        PrintMachine* curMachine = ParameterManager::currentMachine_s();
        int extruderCount = curMachine->extruderCount();
        if (extruderCount == 1)
        {
            m_actionname = tr("Material ") + QString::number(m_NozzleIndex + 1);
        }
        else if (extruderCount == 2)
        {
            m_actionname = tr("Nozzle ") + QString::number(m_NozzleIndex + 1);
        }
    }
}
