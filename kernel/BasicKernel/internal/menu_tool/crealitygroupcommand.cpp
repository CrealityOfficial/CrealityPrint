#include "crealitygroupcommand.h"
#include "interface/uiinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    CrealityGroupCommand::CrealityGroupCommand()
    {
        m_actionname = tr("Model Library");
        m_actionNameWithout = "Model Library";
        m_eParentMenu = eMenuType_CrealityGroup;

        addUIVisualTracer(this,this);
    }

    CrealityGroupCommand::~CrealityGroupCommand()
    {

    }

    void CrealityGroupCommand::execute()
    {
        invokeModelLibraryDialogFunc("showModelLibraryDialog");
    }

    void CrealityGroupCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void CrealityGroupCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Model Library");
    }
}


