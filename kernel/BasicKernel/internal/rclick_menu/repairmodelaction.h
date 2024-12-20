#ifndef REPAIRMODELACTION_H
#define REPAIRMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class RepairModelAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        RepairModelAction(QObject* parent = nullptr);
        virtual ~RepairModelAction();

        Q_INVOKABLE QString getMessageText();
        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private slots:
        void slotRepairSuccess();

    };
}

#endif // REPAIRMODELACTION_H
