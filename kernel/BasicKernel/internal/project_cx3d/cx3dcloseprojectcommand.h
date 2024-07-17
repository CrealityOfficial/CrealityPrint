#ifndef CX3DCLOSEPROJECTCOMMAND_H
#define CX3DCLOSEPROJECTCOMMAND_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "data/interface.h"

#include <QtCore/QUrl>

namespace creative_kernel
{
    class Cx3dCloseProjectCommand : public ActionCommand
        , public qtuser_core::CXHandleBase
        , public UIVisualTracer
        , public CloseHook
    {
        Q_OBJECT
    public:
        Cx3dCloseProjectCommand(QObject* parent = nullptr);
        virtual ~Cx3dCloseProjectCommand();
        
        Q_INVOKABLE void execute();
        Q_INVOKABLE void saveFile(bool bExit = false);
        Q_INVOKABLE void noSaveFile();
        Q_INVOKABLE QUrl historyPath();
        Q_INVOKABLE void delDefaultProject();
        Q_INVOKABLE void clearScreen();
    protected:
        QString filter() override;
        void handle(const QString& fileName) override;
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

        void onWindowClose() override;

        void saveProPath();
        void requestMenuDialog();
    };
}

#endif // CX3DCLOSEPROJECTCOMMAND_H
