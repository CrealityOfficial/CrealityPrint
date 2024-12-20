#ifndef _MERGEMODELCOMMAND_H
#define _MERGEMODELCOMMAND_H
#include "menu/actioncommand.h"
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class MergeModelCommand : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        MergeModelCommand(QObject* parent = nullptr);
        virtual ~MergeModelCommand();
        Q_INVOKABLE void execute();
        Q_INVOKABLE bool enabled();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}

#endif // _MERGEMODELCOMMAND_H
