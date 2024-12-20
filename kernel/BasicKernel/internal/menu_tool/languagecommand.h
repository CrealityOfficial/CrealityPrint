#ifndef LANGUAGECOMMAND_H
#define LANGUAGECOMMAND_H
#include "data/kernelenum.h"
#include "data/interface.h"
#include "menu/actioncommand.h"

namespace creative_kernel
{
    class LanguageCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        LanguageCommand(MultiLanguage language, QObject* parent = nullptr);
        virtual ~LanguageCommand();

        Q_INVOKABLE void execute();
    protected:
        void updateChecked();
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        MultiLanguage m_language;
    };
}

#endif // LANGUAGECOMMAND_H
