#ifndef ABOUTUSCOMMAND_H
#define ABOUTUSCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class AboutUsCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        AboutUsCommand(QObject* parent = nullptr);
        virtual ~AboutUsCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // ABOUTUSCOMMAND_H
