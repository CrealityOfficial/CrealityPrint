#ifndef _LOGVIEW_COMMAND_H
#define _LOGVIEW_COMMAND_H
#include "data/interface.h"
#include "menu/actioncommand.h"

namespace creative_kernel
{
    class LogViewCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        LogViewCommand(QObject* parent = nullptr);
        virtual ~LogViewCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // _LOGVIEW_COMMAND_H
