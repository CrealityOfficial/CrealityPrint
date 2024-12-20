#ifndef USECOURSECOMMAND_H
#define USECOURSECOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class UseCourseCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        UseCourseCommand(QObject* parent = nullptr);
        virtual ~UseCourseCommand();
        Q_INVOKABLE void execute();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // USECOURSECOMMAND_H
