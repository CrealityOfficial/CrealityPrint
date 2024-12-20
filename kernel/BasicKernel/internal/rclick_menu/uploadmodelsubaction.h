#ifndef UPLOADMODELSUBACTION_H
#define UPLOADMODELSUBACTION_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "data/interface.h"

namespace creative_kernel
{
    class UploadModelSubAction :public ActionCommand
        , public UIVisualTracer
        , public qtuser_core::CXHandleBase
    {
        Q_OBJECT
    public:
        enum UpType {
            UPLOADSELECT,
            UPLOADLOCAL
        };

        UploadModelSubAction(UpType type, const QString actionName, QObject* parent = nullptr);
        virtual ~UploadModelSubAction();

        Q_INVOKABLE void execute();
        bool enabled() override;

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
        QString filter() override;     //format "XXXX (*.sufix1 *.sufix2 *.sufix3)"
        void handle(const QStringList& fileNames) override;
    private:
        ActionCommandModel* m_actionModelList = nullptr;
        UpType m_Type = UPLOADSELECT;
    };
}

#endif // UPLOADMODELSUBACTION_H
