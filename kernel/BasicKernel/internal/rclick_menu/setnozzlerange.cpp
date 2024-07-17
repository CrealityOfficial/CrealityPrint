#include "setnozzlerange.h"
#include "nozzleaction.h"

#include "interface/commandinterface.h"
#include "interface/modelinterface.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printextruder.h"
#include <QColor>
namespace creative_kernel
{
    SetNozzleRange::SetNozzleRange(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Set Nozzle Range");
        m_actionNameWithout = "Set Nozzle Range";
        m_bSubMenu = true;

        addUIVisualTracer(this,this);
    }

    SetNozzleRange::~SetNozzleRange()
    {
    }

    void SetNozzleRange::onThemeChanged(ThemeCategory category)
    {
    }

    void SetNozzleRange::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Set Nozzle Range");
    }

    QAbstractListModel* SetNozzleRange::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void SetNozzleRange::updateActionModel()
    {
        //根据当双喷机型改变菜单内容
        PrintMachine* curMachine = ParameterManager::currentMachine_s();
        int extruderCount = curMachine->extruderCount();
        if (extruderCount == 1)
        {
            m_actionname = tr("Set Material");
        }
        else if (extruderCount == 2)
        {
            m_actionname = tr("Set Nozzle Range");
        }

        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        m_actionModelList->removeAllCommand();
        int nozzle = getSelectionModelsNozzle();
        for (int index = 0; index < m_nNozzleCount; index++)
        {
            PrintExtruder* pe = qobject_cast<PrintExtruder*>(curMachine->extruderObject(index));
            NozzleAction* pAction = new NozzleAction(index,this);
            pAction->setExtruderObj(pe);

            m_actionModelList->addCommand(pAction);
            if(nozzle == index)
            {
                pAction->setChecked(true);
            }
        }
    }

    void SetNozzleRange::setNozzleCount(int nCount)
    {
        if (m_nNozzleCount != nCount)
        {
            m_nNozzleCount = nCount;
            updateActionModel();
        }
        else {
            int nozzle = getSelectionModelsNozzle();
            for (int i = 0; i < m_nNozzleCount; i++)
            {      
                ActionCommand* pCmd = m_actionModelList->getData(i);
                if (i == nozzle)
                {     
                    pCmd->setChecked(true);
                    m_actionModelList->changeCommand(pCmd);
                }
                else {
                    pCmd->setChecked(false);

                    m_actionModelList->changeCommand(pCmd);
                }
                
            }
        }
        emit enabledChanged();
    }

    void SetNozzleRange::setExtruderColors(QList<QColor> colors)
    {
        m_ExtruderColors = colors;
    }

    void SetNozzleRange::updateNozzleCheckStatus(int index, bool checked)
    {
        for (int i = 0; i < m_nNozzleCount; i++)
        {
            ActionCommand* pCmd = m_actionModelList->getData(i);
            if (i == index)
            {
                pCmd->setChecked(true);
                m_actionModelList->changeCommand(pCmd);
            }
            else {
                pCmd->setChecked(false);
                m_actionModelList->changeCommand(pCmd);
            }
        }

    }
    bool SetNozzleRange::enabled()
    {
        return getSelectionModelsNozzle() > -2;// true;
    }
}
