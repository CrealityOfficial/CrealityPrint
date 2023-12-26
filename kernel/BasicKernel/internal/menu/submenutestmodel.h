#ifndef SUBMENUTESTMODEL_H
#define SUBMENUTESTMODEL_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API SubMenuTestModel : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuTestModel();
        virtual ~SubMenuTestModel();

        void initActionModel();
        QAbstractListModel* subMenuActionmodel();


        static SubMenuTestModel* getInstance();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        ActionCommandModel* m_actionModelList = nullptr;

        static SubMenuTestModel* m_instance;
    };
}

#endif // SUBMENUSTANDARDMODEL_H
