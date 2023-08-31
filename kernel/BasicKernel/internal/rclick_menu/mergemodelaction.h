#ifndef MERGEMODELACTION_H
#define MERGEMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class MergeModelAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        MergeModelAction(QObject* parent = nullptr);
        virtual ~MergeModelAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // MERGEMODELACTION_H
