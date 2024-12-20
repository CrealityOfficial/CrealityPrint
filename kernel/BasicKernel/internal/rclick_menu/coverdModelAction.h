#pragma once
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class CoverModelAction : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CoverModelAction(QObject* parent = nullptr);
        virtual ~CoverModelAction();

        Q_INVOKABLE void execute();
        virtual bool enabled();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    };
}