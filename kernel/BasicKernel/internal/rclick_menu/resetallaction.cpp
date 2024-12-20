#include "resetallaction.h"

#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    ResetAllAction::ResetAllAction(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Reset All Model");
        m_actionNameWithout = "Reset All Model";
        m_strMessageText = tr("Do you Want to Reset All Model?");

        addUIVisualTracer(this,this);
    }

    ResetAllAction::~ResetAllAction()
    {
    }

    void ResetAllAction::execute()
    {
        requestMenuDialog();
    }

    void ResetAllAction::onThemeChanged(ThemeCategory category)
    {
    }

    void ResetAllAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Reset All Model");
        m_strMessageText = tr("Do you Want to Reset All Model?");
    }

    void ResetAllAction::requestMenuDialog()
    {
        requestQmlDialog(this, "messageDlg");
    }

    QString ResetAllAction::getMessageText()
    {
        return m_strMessageText;
    }

    void ResetAllAction::accept()
    {
        resetAllModelsAndGroups();
    }

    bool ResetAllAction::enabled()
    {
        return creative_kernel::modelns().size() > 0;
    }
}

