#ifndef SUBMENUIMPORTMODEL_H
#define SUBMENUIMPORTMODEL_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API SubMenuImportModel : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
            Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuImportModel();
        virtual ~SubMenuImportModel();

        Q_INVOKABLE QVariant getOpt(const QString& optName);

        void initActionModel();
        QAbstractListModel* subMenuActionmodel();


        static SubMenuImportModel* getInstance();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        ActionCommandModel* m_actionModelList = nullptr;

        static SubMenuImportModel* m_instance;
    };
}

#endif // SUBMENUIMPORTMODEL_H
