#ifndef LOCK_PLATE_ACTION_H
#define LOCK_PLATE_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class LockPlateAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        LockPlateAction(QObject* parent = nullptr);
        virtual ~LockPlateAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    };
}

#endif // LOCK_PLATE_ACTION_H
