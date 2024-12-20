#ifndef ONESIDEASBOTTOMFACE_H
#define ONESIDEASBOTTOMFACE_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class OneSideAsBottomFace : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        OneSideAsBottomFace(QObject* parent = nullptr);
        virtual ~OneSideAsBottomFace();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // ONESIDEASBOTTOMFACE_H
