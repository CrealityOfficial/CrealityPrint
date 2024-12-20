#ifndef SIMPLIFIEDMODEL_H
#define SIMPLIFIEDMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SimplifiedModelAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        SimplifiedModelAction(QObject* parent = nullptr);
        virtual ~SimplifiedModelAction();

        Q_INVOKABLE void execute();
        bool enabled() override;

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // SIMPLIFIEDMODELACTION_H
