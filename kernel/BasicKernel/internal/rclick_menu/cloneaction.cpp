#include "cloneaction.h"
#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    CloneAction::CloneAction(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Clone");
        m_actionNameWithout = "Clone";

        addUIVisualTracer(this,this);
    }

    CloneAction::~CloneAction()
    {
    }

    void CloneAction::onThemeChanged(ThemeCategory category)
    {
    }

    void CloneAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Clone");
    }

    bool CloneAction::isSelect()
    {
        QList<ModelN*> selections = selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void CloneAction::clone(int numb)
    {
        cloneSelections(numb);
    }

    void CloneAction::execute()
    {
        requestQmlDialog(this, "cloneNumObj");
    }

    bool CloneAction::enabled()
    {
        return isSelect();
    }
}
