#ifndef PICK_BOTTOM_ACTION_H
#define PICK_BOTTOM_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class PickBottomAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        PickBottomAction(QObject* parent = nullptr);
        virtual ~PickBottomAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    };
}

#endif // PICK_BOTTOM_ACTION_H
