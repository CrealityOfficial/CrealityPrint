#ifndef SELECTALLACTION_H
#define SELECTALLACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SelectAllAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SelectAllAction(QObject* parent = nullptr);
        virtual ~SelectAllAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // SELECTALLACTION_H
