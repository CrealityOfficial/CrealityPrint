#include "setnozzlerange.h"
#include "nozzleaction.h"

#include "interface/commandinterface.h"
#include "interface/modelinterface.h"
namespace creative_kernel
{
    SetNozzleRange::SetNozzleRange(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Set Nozzle Range");
        m_actionNameWithout = "Set Nozzle Range";
        m_bSubMenu = true;

        addUIVisualTracer(this);
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
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        m_actionModelList->removeAllCommand();
        int nozzle = getSelectionModelsNozzle();
        for (int i = 0; i < m_nNozzleCount; i++)
        {
            NozzleAction* pAction = new NozzleAction(i,this);
            m_actionModelList->addCommand(pAction);
            if(nozzle == i)
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
