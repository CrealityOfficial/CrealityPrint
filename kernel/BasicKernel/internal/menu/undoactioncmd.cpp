#include "undoactioncmd.h"
#include "cxkernel/interface/undointerface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    UndoActionCmd::UndoActionCmd()
    {
        m_shortcut = "Ctrl+Z";      //不能有空格
        m_actionname = tr("Undo");
        m_actionNameWithout = "Undo";
        m_icon = "qrc:/kernel/images/undo.png";
        m_eParentMenu = eMenuType_Edit;

        addUIVisualTracer(this,this);
    }

    UndoActionCmd::~UndoActionCmd()
    {
    }

    void UndoActionCmd::execute()
    {
        cxkernel::undo();
    }

    void UndoActionCmd::onThemeChanged(ThemeCategory category)
    {

    }

    void UndoActionCmd::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Undo") + "      " + m_shortcut;
    }
}
