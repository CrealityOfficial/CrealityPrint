#include "submenuThemeColor.h"
#include "themecolorcommand.h"
#include "interface/commandinterface.h"


namespace creative_kernel
{
    SubMenuThemeColor::SubMenuThemeColor(QObject* parent)
        : ActionCommand(parent)
    {
        m_actionname = tr("Theme color change");
        m_actionNameWithout = "Theme color change";
        m_eParentMenu = eMenuType_Tool;
        m_bSubMenu = true;
        m_bCheckable = true;

        m_actionModelList = new ActionCommandModel(this);
        m_actionModelList->removeAllCommand();

        darkTheme = new ThemeColorCommand(ThemeCategory::tc_dark, this);
        m_actionModelList->addCommand(darkTheme);
        lightTheme = new ThemeColorCommand(ThemeCategory::tc_light, this);
        m_actionModelList->addCommand(lightTheme);

		addUIVisualTracer(this,this);
    }

    SubMenuThemeColor::~SubMenuThemeColor()
    {
    }

    QAbstractListModel* SubMenuThemeColor::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    ActionCommandModel* SubMenuThemeColor::getSubMenuList()
    {
        return m_actionModelList;
    }

	void SubMenuThemeColor::onThemeChanged(ThemeCategory theme)
	{
		darkTheme->setChecked(theme == ThemeCategory::tc_dark);
		lightTheme->setChecked(theme == ThemeCategory::tc_light);
		m_actionModelList->layoutChanged();
	}

    void SubMenuThemeColor::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Theme color change");
    }
}
