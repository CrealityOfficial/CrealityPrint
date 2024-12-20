#ifndef SUBMENULANGUAGE_H
#define SUBMENULANGUAGE_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SubMenuLanguage : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        SubMenuLanguage();
        ~SubMenuLanguage();

        void updateActionModel();
        QAbstractListModel* subMenuActionmodel();
        ActionCommandModel* getSubMenuList();

        static SubMenuLanguage* getInstance()
        {
            static SubMenuLanguage inst;
            return &inst;
        }
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        ActionCommandModel* m_actionModelList = nullptr;
    };
}

#endif // SUBMENULANGUAGE_H
