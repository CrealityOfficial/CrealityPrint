#ifndef SUBMENURECENTFILES_H
#define SUBMENURECENTFILES_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"
#include <QtCore/QSettings>

#define recentFileCount "Creality3DrecentFiles/numOfRecentFiles"
#define recentFileListId "Creality3DrecentFiles/fileList"
#define Max_RecentFile_Size 10

namespace creative_kernel
{
    class BASIC_KERNEL_API SubMenuRecentFiles : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit SubMenuRecentFiles();
        virtual ~SubMenuRecentFiles();

        void initActionModel();
        QAbstractListModel* subMenuActionmodel();

        void setNumOfRecentFiles(int n);
        void updateRecentFiles(QSettings& settings);
        void setMostRecentFile(const QString fileName);


        static SubMenuRecentFiles* getInstance();
    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;
    public slots:
        void slotClear();
    private:
        ActionCommandModel* m_actionModelList = nullptr;

        static SubMenuRecentFiles* m_instance;
    };
}

#endif // SUBMENURECENTFILES_H
