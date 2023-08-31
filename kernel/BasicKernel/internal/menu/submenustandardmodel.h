#ifndef SUBMENUSTANDARDMODEL_H
#define SUBMENUSTANDARDMODEL_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

namespace creative_kernel
{
    class BASIC_KERNEL_API SubMenuStandardModel : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuStandardModel();
        virtual ~SubMenuStandardModel();

        void initActionModel();
        QAbstractListModel* subMenuActionmodel();


        static SubMenuStandardModel* getInstance();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
        ActionCommandModel* m_actionModelList = nullptr;

        static SubMenuStandardModel* m_instance;
    };
}

#endif // SUBMENUSTANDARDMODEL_H
