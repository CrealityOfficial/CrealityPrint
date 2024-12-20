#ifndef CLONEACTION_H
#define CLONEACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class CloneAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CloneAction(QObject* parent = nullptr);
        virtual ~CloneAction();
        Q_INVOKABLE bool isSelect();
        Q_INVOKABLE void clone(int numb);
        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}
#endif // CLONEACTION_H
