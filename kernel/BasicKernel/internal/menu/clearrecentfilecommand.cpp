#include "clearrecentfilecommand.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    ClearRecentFileCommand::ClearRecentFileCommand()
    {
        m_actionname = tr("Clear history");
        m_eParentMenu = eMenuType_File;
        m_bSeparator = true;

        addUIVisualTracer(this,this);
    }

    ClearRecentFileCommand::~ClearRecentFileCommand()
    {
    }

    void ClearRecentFileCommand::execute()
    {
        emit sigExecute();
    }

    void ClearRecentFileCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void ClearRecentFileCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Clear history");
    }
}

