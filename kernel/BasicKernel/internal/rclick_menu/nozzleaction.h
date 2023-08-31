#ifndef NOZZLEACTION_H
#define NOZZLEACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class NozzleAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        NozzleAction(int nNumber,QObject* parent);
        virtual ~NozzleAction();

        Q_INVOKABLE void execute();
        bool enabled() override;
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        int m_nNumber = 1;
    };
}

#endif // NOZZLEACTION_H
