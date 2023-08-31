#ifndef CREALITYGOURPCOMMAND_H
#define CREALITYGOURPCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class CrealityGroupCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CrealityGroupCommand();
        virtual ~CrealityGroupCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // CREALITYGOURPCOMMAND_H
