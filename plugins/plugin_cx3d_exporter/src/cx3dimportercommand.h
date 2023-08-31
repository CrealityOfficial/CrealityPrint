#ifndef CX3DIMPORTERCOMMAND_H
#define CX3DIMPORTERCOMMAND_H
#include "qtusercore/module/cxhandlerbase.h"
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class CX3DImporterCommand : public ActionCommand
        , public qtuser_core::CXHandleBase
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CX3DImporterCommand(QObject* parent = nullptr);
        virtual ~CX3DImporterCommand();

        Q_INVOKABLE void execute();
        void setNewPath(QString pathName);
    protected:
        QString filter() override;
        void handle(const QString& fileName) override;
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    public slots:
        void slotSaveLastPro();
        void slotCancelSaveLastPro();
    private:
        QString m_strNewPath;
    };
}

#endif // CX3DIMPORTERCOMMAND_H
