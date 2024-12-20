#ifndef USERFEEDBACKCOMMAND_H
#define USERFEEDBACKCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class UserFeedbackCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        UserFeedbackCommand(QObject* parent = nullptr);
        virtual ~UserFeedbackCommand();
        Q_INVOKABLE void execute();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}
#endif
