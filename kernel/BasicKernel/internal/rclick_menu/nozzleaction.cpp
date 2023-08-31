#include "nozzleaction.h"

#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "setnozzlerange.h"
namespace creative_kernel
{
    NozzleAction::NozzleAction(int nNumber,QObject* parent)
    {
        this->setParent(parent);
        m_nNumber = nNumber;
        m_actionname = tr("Nozzle ") + QString::number(m_nNumber + 1);
        m_actionNameWithout = "Nozzle";
        m_bSubMenu = true;

        addUIVisualTracer(this);
    }

    NozzleAction::~NozzleAction()
    {
    }

    void NozzleAction::execute()
    {
        SetNozzleRange* nozzlerange = qobject_cast<SetNozzleRange*>(parent());
        setSelectionModelsNozzle(m_nNumber);
        nozzlerange->updateNozzleCheckStatus(m_nNumber, this->bChecked());
        
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
        m_actionname = tr("Nozzle ") + QString::number(m_nNumber + 1);
    }
}
