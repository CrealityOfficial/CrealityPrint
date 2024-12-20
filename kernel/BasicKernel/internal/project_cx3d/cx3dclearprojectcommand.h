#ifndef CX3DCLEARPROJECTCOMMAND_H
#define CX3DCLEARPROJECTCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class Cx3dClearProjectCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        Cx3dClearProjectCommand(QObject* parent = nullptr);
        virtual ~Cx3dClearProjectCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    signals:
        void sigExecute();
    };
}

#endif // CX3DCLEARPROJECTCOMMAND_H
