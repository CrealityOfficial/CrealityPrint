#include "assembleaction.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"


namespace creative_kernel
{
    AssembleAction::AssembleAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Assemble");
        m_actionNameWithout = "Assemble";

        addUIVisualTracer(this,this);
    }

    AssembleAction::~AssembleAction()
    {
    }

    void AssembleAction::execute()
    {
        mergeSelectionsGroup();
    }

    bool AssembleAction::editMenuEnabled()
    {
        return  selectedGroups().size() > 1 &&selectedParts().size() == 0;
    }

    bool AssembleAction::enabled()
    {
        return selectedGroups().size() >= 2;
        // return true;
    }

    void AssembleAction::onThemeChanged(ThemeCategory category)
    {
    }

    void AssembleAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Assemble");
    }
}
