#ifndef EXPORTPRESETCOMMAND_H
#define EXPORTPRESETCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"
#include "qtusercore/module/cxhandlerbase.h"

namespace creative_kernel
{
    class ExportPresetCommand : public ActionCommand
        , public UIVisualTracer
        , public qtuser_core::CXHandleBase
    {
        Q_OBJECT
    public:
        ExportPresetCommand();
        virtual ~ExportPresetCommand();

        Q_INVOKABLE void execute();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
        QString filter() override;
        void handle(const QString& fileName) override;
    };
}

#endif // OPENFILECOMMAND_H
