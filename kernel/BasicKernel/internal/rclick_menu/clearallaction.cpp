#include "clearallaction.h"
#include "kernel/kernelui.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    ClearAllAction::ClearAllAction(QObject* parent):ActionCommand(parent)
    {
        m_actionname = tr("Clear All");
        m_actionNameWithout = "Clear All";
        m_strMessageText = tr("Do you Want to Clear All Model?");
        m_source = "qrc:/UI/photo/menuImg/delete_all_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/delete_all_p.svg";

        addUIVisualTracer(this,this);
    }

    ClearAllAction::~ClearAllAction()
    {
    }

    void ClearAllAction::execute()
    {
        requestQmlDialog(this, "messageDlg");
    }

    QString ClearAllAction::getMessageText()
    {
        return m_strMessageText;
    }

    void ClearAllAction::onThemeChanged(ThemeCategory category)
    {
    }

    void ClearAllAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Clear All");
        m_strMessageText = tr("Do you Want to Clear All Model?");
    }

    void ClearAllAction::accept()
    {
        clearSpace();
        getKernelUI()->switchPickMode();//by TCJ
    }

    bool ClearAllAction::enabled()
    {
        return modelns().size() > 0;
    }
}
