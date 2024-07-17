#include "redoactioncmd.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/undointerface.h"

namespace creative_kernel
{
    RedoActionCmd::RedoActionCmd()
    {
        m_shortcut = "Ctrl+Y";      //不能有空格
        m_actionname = tr("Redo");
        m_actionNameWithout = "Redo";
        m_eParentMenu = eMenuType_Edit;
        m_icon = "qrc:/kernel/images/redo.png";

        addUIVisualTracer(this,this);
    }

    RedoActionCmd::~RedoActionCmd()
    {
    }

    void RedoActionCmd::execute()
    {
        cxkernel::redo();
    }

    void RedoActionCmd::onThemeChanged(ThemeCategory category)
    {

    }

    void RedoActionCmd::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Redo") + "      " + m_shortcut;
    }
}
