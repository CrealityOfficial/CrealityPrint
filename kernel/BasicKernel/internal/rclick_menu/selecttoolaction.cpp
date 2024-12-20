#include "selecttoolaction.h"

#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

#include "kernel/kernelui.h"

namespace creative_kernel
{
    SelectToolAction::SelectToolAction(const QString& name, int toolId, QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr(name.toLatin1().data());
        m_actionNameWithout = name;
        m_toolId = toolId;

        addUIVisualTracer(this,this);
    }

    SelectToolAction::~SelectToolAction()
    {
    }

    void SelectToolAction::execute()
    {
        getKernelUI()->switchMode(m_toolId);
    }

    void SelectToolAction::onThemeChanged(ThemeCategory category)
    {
    }

    void SelectToolAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr(m_actionNameWithout.toLatin1().data());
    }

    bool SelectToolAction::enabled()
    {
        // return creative_kernel::modelns().size() > 0;
        return true;
    }
}

