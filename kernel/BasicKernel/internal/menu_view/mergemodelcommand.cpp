#include "mergemodelcommand.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    MergeModelCommand::MergeModelCommand(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Merge Model Locations");
        m_actionNameWithout = "Unit As One";
        m_eParentMenu = eMenuType_View;

        addUIVisualTracer(this);
    }

    MergeModelCommand::~MergeModelCommand()
    {
    }

    void MergeModelCommand::execute()
    {
        alignAllModels2BaseCenter(true);
    }

    void MergeModelCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void MergeModelCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Merge Model Locations");
    }
}

