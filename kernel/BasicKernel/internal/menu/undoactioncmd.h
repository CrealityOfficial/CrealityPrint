#ifndef UNDOACTIONCMD_H
#define UNDOACTIONCMD_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class UndoActionCmd : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        UndoActionCmd();
        virtual ~UndoActionCmd();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // UNDOACTIONCMD_H
