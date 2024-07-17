#ifndef MERGE_MODEL_TO_PRINTER_ACTION_H
#define MERGE_MODEL_TO_PRINTER_ACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class MergeModelToPrinterAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        MergeModelToPrinterAction(QObject* parent = nullptr);
        virtual ~MergeModelToPrinterAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // MERGE_MODEL_TO_PRINTER_ACTION_H
