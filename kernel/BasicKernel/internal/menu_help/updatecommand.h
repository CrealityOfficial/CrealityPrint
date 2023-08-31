#ifndef UPDATECOMMAND_H
#define UPDATECOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class UpdateCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        UpdateCommand(QObject* parent = nullptr);
        virtual ~UpdateCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // UPDATECOMMAND_H
