#ifndef SUBMENUVIEWSHOW_H
#define SUBMENUVIEWSHOW_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"

#define recentFileCount "Creality3DrecentFiles/numOfRecentFiles"
#define recentFileListId "Creality3DrecentFiles/fileList"

namespace creative_kernel
{
    class SubMenuViewShow : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuViewShow();
        virtual ~SubMenuViewShow();

        QAbstractListModel* subMenuActionmodel();

        virtual bool enabled()override;
    protected:
        void initActionModel();
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    private:
        ActionCommandModel* m_actionModelList = nullptr;
    };
}

#endif // SUBMENUVIEWSHOW_H
