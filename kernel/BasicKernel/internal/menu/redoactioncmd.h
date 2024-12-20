#ifndef REDOACTIONCMD_H
#define REDOACTIONCMD_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class RedoActionCmd : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        RedoActionCmd();
        virtual ~RedoActionCmd();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // REDOACTIONCMD_H
