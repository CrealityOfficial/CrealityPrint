#include "simplifiedModel.h"

#include "interface/selectorinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    SimplifiedModelAction::SimplifiedModelAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Simplified Model");
        m_actionNameWithout = "Simplified Model";

        addUIVisualTracer(this,this);
    }

    SimplifiedModelAction::~SimplifiedModelAction()
    {
    }

    void SimplifiedModelAction::execute()
    {
        requestQmlDialog("SimplifiedModel");
    }

    bool SimplifiedModelAction::enabled()
    {
        return selectionms().size() > 0;
    }

    void SimplifiedModelAction::onThemeChanged(ThemeCategory category)
    {

    }

    void SimplifiedModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Simplified Model");
    }
}
