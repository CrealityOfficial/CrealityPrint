#include "deletemodelaction.h"

#include <QtCore/QSettings>

#include "interface/uiinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "data/modeln.h"

namespace creative_kernel
{
    DeleteModelAction::DeleteModelAction(QObject* parent)
        :ActionCommand(parent)
    {
        m_actionname = tr("Delete Model");
        m_actionNameWithout = "Delete Model";
        m_strMessageText = tr("Do you Want to Delete SelectModel?");
        m_source = "qrc:/UI/photo/menuImg/delete_n.svg";
        m_icon = "qrc:/UI/photo/menuImg/delete_p.svg";
        setShortcut("Delete");
        addUIVisualTracer(this,this);
    }

    DeleteModelAction::~DeleteModelAction()
    {
    }

    void DeleteModelAction::execute()
    {
        if(enabled())
        {
            if (isPopPage())
                requestMenuDialog();
            else
            {
                accept();
            }
        }
    }

    void DeleteModelAction::onThemeChanged(ThemeCategory category)
    {

    }

    void DeleteModelAction::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Delete Model");
        m_strMessageText = tr("Do you Want to Delete SelectModel?");
    }

    void DeleteModelAction::requestMenuDialog()
    {
        requestQmlDialog(this, "deletemessageDlg");
    }

    QString DeleteModelAction::getMessageText()
    {
        return m_strMessageText;
    }

    void DeleteModelAction::writeReg(bool flag)
    {
        QSettings settings;
        QString path = "UIparam/isPop";
        settings.setValue(path, QVariant(flag));
    }

    bool DeleteModelAction::isPopPage()
    {
        QSettings setting;
        setting.beginGroup("UIparam");
        bool flag = setting.value("isPop", true).toBool();
        setting.endGroup();

        return flag;
    }

    void DeleteModelAction::accept()
    {
        removeSelections();
    }

    bool DeleteModelAction::enabled()
    {
        QList<creative_kernel::ModelN*> selections = creative_kernel::selectionms();
        if (selections.size() > 0)
        {
            return true;
        }
        return false;
    }
}