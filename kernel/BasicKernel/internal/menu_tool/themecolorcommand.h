#ifndef THEMECOLORCOMMAND_H
#define THEMECOLORCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ThemeColorCommand : public ActionCommand, public UIVisualTracer
    {
        Q_OBJECT
    public:
        ThemeColorCommand(ThemeCategory theme, QObject* parent = nullptr);
        virtual ~ThemeColorCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        ThemeCategory m_theme = ThemeCategory::tc_dark;
    };
}
#endif
