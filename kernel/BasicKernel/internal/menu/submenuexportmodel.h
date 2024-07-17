#ifndef SUBMENUEXPORTMODEL_H
#define SUBMENUEXPORTMODEL_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API SubMenuExportModel : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
            Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuExportModel();
        virtual ~SubMenuExportModel();

        void initActionModel();
        QAbstractListModel* subMenuActionmodel();


        static SubMenuExportModel* getInstance();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        ActionCommandModel* m_actionModelList = nullptr;

        static SubMenuExportModel* m_instance;
    };
}

#endif // SUBMENUIMPORTMODEL_H
