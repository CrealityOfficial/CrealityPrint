#include "languagecommand.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    LanguageCommand::LanguageCommand(MultiLanguage language, QObject* parent)
        : ActionCommand(parent)
        , m_language(language)
    {
        m_eParentMenu = eMenuType_Tool;

        addUIVisualTracer(this,this);

        onLanguageChanged(m_language);
    }

    void LanguageCommand::updateChecked()
    {
        MultiLanguage language = currentLanguage();
        setChecked(m_language == language);
    }

    LanguageCommand::~LanguageCommand()
    {
    }

    void LanguageCommand::execute()
    {
        changeLanguage(m_language);
    }

    void LanguageCommand::onThemeChanged(ThemeCategory category)
    {

    }

    void LanguageCommand::onLanguageChanged(MultiLanguage language)
    {
        updateChecked();
        setName(languageName(m_language));
    }
}

