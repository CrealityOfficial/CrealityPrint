#ifndef _VIEWSHOWCOMMAND_H
#define _VIEWSHOWCOMMAND_H
#include "menu/actioncommand.h"
#include "data/kernelenum.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ViewShowCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        ViewShowCommand(EViewShow eType, QObject* parent = nullptr);
        virtual ~ViewShowCommand();

        Q_INVOKABLE void execute();
        Q_INVOKABLE void updateCheck();
        bool enabled();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        int m_nShowType = 0;
    };
}

#endif // _VIEWSHOWCOMMAND_H
