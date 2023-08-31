#ifndef OPENFILECOMMAND_H
#define OPENFILECOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class OpenFileCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        OpenFileCommand();
        virtual ~OpenFileCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // OPENFILECOMMAND_H
