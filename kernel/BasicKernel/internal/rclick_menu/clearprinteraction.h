#ifndef CLEAR_PRINTER_ACTION_H
#define CLEAR_PRINTER_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ClearPrinterAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ClearPrinterAction(QObject* parent = nullptr);
        virtual ~ClearPrinterAction();

        Q_INVOKABLE void execute();

        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    };
}

#endif // CLEAR_PRINTER_ACTION_H
