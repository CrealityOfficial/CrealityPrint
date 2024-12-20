#include "mergemodelcommand.h"
#include "interface/commandinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"

namespace creative_kernel
{
    MergeModelCommand::MergeModelCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Merge Model Locations");
        m_actionNameWithout = "Unit As One";
        m_eParentMenu = eMenuType_View;

        addUIVisualTracer(this,this);
    }

    MergeModelCommand::~MergeModelCommand()
    {
    }

    void MergeModelCommand::execute()
    {
        mergePosition();
    }

    bool MergeModelCommand::enabled()
    {
        return selectedParts().size() == 0;
    }

    void MergeModelCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void MergeModelCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Merge Model Locations");
    }
}

