#ifndef UPLOADMODELACTION_H
#define UPLOADMODELACTION_H
#include "menu/actioncommand.h"
#include "data/interface.h"
#include "menu/actioncommandmodel.h"

namespace creative_kernel
{
    class UploadModelSubAction;
    class UploadModelAction :public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        UploadModelAction(QObject* parent = nullptr);
        virtual ~UploadModelAction();

        Q_INVOKABLE void execute();
        void updateActionModel();
        QAbstractListModel* subMenuActionmodel();
        bool enabled() override;

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        ActionCommandModel* m_actionModelList = nullptr;
        UploadModelSubAction* m_uploadLoacalAction = nullptr;
        UploadModelSubAction* m_uploadSelectAction = nullptr;
    };
}

#endif // UPLOADMODELACTION_H
