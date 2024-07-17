#ifndef SELECT_PRINTER_ALL_ACTION_H
#define SELECT_PRINTER_ALL_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SelectPrinterAllAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SelectPrinterAllAction(QObject* parent = nullptr);
        virtual ~SelectPrinterAllAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // SELECT_PRINTER_ALL_ACTION_H
