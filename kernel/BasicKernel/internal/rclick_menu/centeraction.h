#ifndef CENTERACTION_H
#define CENTERACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class CenterAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CenterAction(QObject* parent = nullptr);
        virtual ~CenterAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // CENTERACTION_H
