#ifndef CX3DIMPORTERCOMMAND_H
#define CX3DIMPORTERCOMMAND_H
#include "qtusercore/module/cxhandlerbase.h"
#include "loadprojectcommand.h"
#include "data/interface.h"
#include "interface/projectinterface.h"

namespace creative_kernel
{
    class CX3DImporterCommand : public LoadProjectCommand
        , public qtuser_core::CXHandleBase
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CX3DImporterCommand(QObject* parent = nullptr);
        virtual ~CX3DImporterCommand();

        Q_INVOKABLE void execute();
        void openCxprjAnd3MF(const QString& projectPath, bool quiet=false);
    protected:
        QString filter() override;
        QString filterKey() override;
        void handle(const QString& fileName) override;
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
        void accept() override;
        void cancel() override;
    private:
        bool m_quiet = false;
        bool m_menuMode = true;
    };
}

#endif // CX3DIMPORTERCOMMAND_H
