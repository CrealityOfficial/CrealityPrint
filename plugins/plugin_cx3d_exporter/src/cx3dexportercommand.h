#ifndef CX3DEXPORTERCOMMAND_H
#define CX3DEXPORTERCOMMAND_H
#include "menu/actioncommand.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "data/interface.h"

namespace creative_kernel
{
    class CX3DExporterCommand : public ActionCommand
        , public qtuser_core::CXHandleBase
        , public UIVisualTracer
    {
        Q_OBJECT
    public:
        CX3DExporterCommand(QObject* parent = nullptr);
        virtual ~CX3DExporterCommand();

        Q_INVOKABLE void execute();
        Q_INVOKABLE QUrl historyPath();
        Q_INVOKABLE QString getMessageText();
        Q_INVOKABLE void accept();
    private:
        QString m_strMessageText;
    protected:
        QString filter() override;
        void handle(const QString& fileName) override;
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private slots:
        void slotOpenLocalFolder();
    };
}

#endif // CX3DEXPORTERCOMMAND_H
