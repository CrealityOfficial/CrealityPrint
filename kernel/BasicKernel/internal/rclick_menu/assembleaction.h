#ifndef ASSEMBLEACTION_H
#define ASSEMBLEACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class AssembleAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        AssembleAction(QObject* parent = nullptr);
        virtual ~AssembleAction();

        Q_INVOKABLE void execute();
        Q_INVOKABLE bool editMenuEnabled();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // ASSEMBLEACTION_H
