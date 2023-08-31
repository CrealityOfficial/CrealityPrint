#include "openfilecommand.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"

namespace creative_kernel
{
    OpenFileCommand::OpenFileCommand()
        :ActionCommand()
    {
        m_shortcut = "Ctrl+I";
        m_actionname = tr("Open File");
        m_actionNameWithout = "Open File";
        m_icon = "qrc:/kernel/images/open.png";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this);
    }

    OpenFileCommand::~OpenFileCommand()
    {
    }

    void OpenFileCommand::execute()
    {
        cxkernel::openFile();
    }

    void OpenFileCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void OpenFileCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Open File") + "        " + m_shortcut;
    }
}
