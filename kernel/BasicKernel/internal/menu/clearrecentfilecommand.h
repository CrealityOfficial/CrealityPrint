#ifndef CLEARRECENTFILECOMMAND_H
#define CLEARRECENTFILECOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ClearRecentFileCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ClearRecentFileCommand();
        virtual ~ClearRecentFileCommand();
        Q_INVOKABLE void execute();
    signals:
        void sigExecute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // CLEARRECENTFILECOMMAND_H
