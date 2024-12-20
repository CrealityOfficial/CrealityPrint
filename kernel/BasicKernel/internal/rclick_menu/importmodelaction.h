#ifndef IMPORTMODELACTION_H
#define IMPORTMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ImportModelAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ImportModelAction(QObject *parent);
        virtual ~ImportModelAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // _IMPORTMODELACTION_H
