#include "selectallaction.h"

#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    SelectAllAction::SelectAllAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Select All Model");
        m_actionNameWithout = "Select All Model";
        setShortcut("Ctrl+A");

        addUIVisualTracer(this,this);
    }

    SelectAllAction::~SelectAllAction()
    {
    }

    void SelectAllAction::execute()
    {
        selectAllModelGroup();
    }

    bool SelectAllAction::enabled()
    {
        return creative_kernel::modelns().size() > 0;
    }

    void SelectAllAction::onThemeChanged(ThemeCategory category)
    {
    }

    void SelectAllAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Select All Model");
    }
}
