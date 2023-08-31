#ifndef SUBMENUTHEMECOLOR_H
#define SUBMENUTHEMECOLOR_H

#include "menu/actioncommandmodel.h"
#include "menu/actioncommand.h"
#include "themecolorcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class SubMenuThemeColor : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        SubMenuThemeColor(QObject* parent = nullptr);
        virtual ~SubMenuThemeColor();

        QAbstractListModel* subMenuActionmodel();
        ActionCommandModel* getSubMenuList();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    private:
		ThemeColorCommand* darkTheme = nullptr;
		ThemeColorCommand* lightTheme = nullptr;
        ActionCommandModel* m_actionModelList = nullptr;
    };
}
#endif
