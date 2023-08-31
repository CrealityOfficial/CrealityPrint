#ifndef _MANAGEPRINTE_H
#define _MANAGEPRINTE_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ManagePrinter : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ManagePrinter(QObject* parent = nullptr);
        virtual ~ManagePrinter();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // _MANAGEPRINTE_H
