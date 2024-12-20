#ifndef SPLITOBJECTSACTION_H
#define SPLITOBJECTSACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SplitObjectsAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SplitObjectsAction(QObject* parent = nullptr);
        virtual ~SplitObjectsAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // SPLITOBJECTSACTION_H
