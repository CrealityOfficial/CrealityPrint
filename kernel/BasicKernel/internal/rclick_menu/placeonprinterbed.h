#ifndef PLACEONPRINTERBED_H
#define PLACEONPRINTERBED_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class PlaceOnPrinterBed : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        PlaceOnPrinterBed(QObject* parent = nullptr);
        virtual ~PlaceOnPrinterBed();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // PLACEONPRINTERBED_H
