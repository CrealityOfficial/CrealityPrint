#include "themecolorcommand.h"
#include "interface/commandinterface.h"
#include "interface/appsettinginterface.h"

#include <QtCore/QDebug>

namespace creative_kernel
{
    ThemeColorCommand::ThemeColorCommand(ThemeCategory theme, QObject* parent)
        : ActionCommand(parent)
        , m_theme(theme)
    {
        m_eParentMenu = eMenuType_Tool;
        addUIVisualTracer(this,this);

        onLanguageChanged(MultiLanguage::eLanguage_EN_TS);
    }

    ThemeColorCommand::~ThemeColorCommand()
    {

    }

    void ThemeColorCommand::execute()
    {
        changeTheme(m_theme);
    }

    void ThemeColorCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void ThemeColorCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = themeName(m_theme);
    }
}


