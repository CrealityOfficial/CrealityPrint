#include "submenulanguage.h"
#include "languagecommand.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    SubMenuLanguage::SubMenuLanguage()
    {
        m_actionname = tr("Language");
        m_actionNameWithout = "Language";
        m_eParentMenu = eMenuType_Tool;
        m_bSubMenu = true;
        m_bCheckable = true;

        addUIVisualTracer(this);
        updateActionModel();
    }

    SubMenuLanguage::~SubMenuLanguage()
    {
    }

    QAbstractListModel* SubMenuLanguage::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    ActionCommandModel* SubMenuLanguage::getSubMenuList()
    {
        return m_actionModelList;
    }

    void SubMenuLanguage::updateActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        m_actionModelList->removeAllCommand();
        LanguageCommand* command0 = new LanguageCommand(MultiLanguage::eLanguage_EN_TS, this);
        m_actionModelList->addCommand(command0);
        LanguageCommand* command1 = new LanguageCommand(MultiLanguage::eLanguage_ZHCN_TS, this);
        m_actionModelList->addCommand(command1);
        LanguageCommand* command2 = new LanguageCommand(MultiLanguage::eLanguage_ZHTW_TS, this);
        m_actionModelList->addCommand(command2);
        LanguageCommand* command3 = new LanguageCommand(MultiLanguage::eLanguage_KO_TS, this);
        m_actionModelList->addCommand(command3);
    }

    void SubMenuLanguage::onThemeChanged(ThemeCategory category)
    {

    }

    void SubMenuLanguage::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Language");
    }
}
