#ifndef SPLITPARTSACTION_H
#define SPLITPARTSACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SplitPartsAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SplitPartsAction(QObject* parent = nullptr);
        virtual ~SplitPartsAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // SPLITPARTSACTION_H
