#include "crealitygroupcommand.h"
#include "interface/uiinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    CrealityGroupCommand::CrealityGroupCommand()
    {
        m_actionname = tr("Models");
        m_actionNameWithout = "Models";
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
        m_actionname = tr("Models");
    }
}


