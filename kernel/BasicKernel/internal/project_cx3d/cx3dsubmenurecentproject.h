#ifndef CX3DSUBMENURECENTPROJECT_H
#define CX3DSUBMENURECENTPROJECT_H
#include "menu/actioncommand.h"
#include "menu/actioncommandmodel.h"
#include "data/interface.h"

#include <QtCore/QSettings>

#define recentFileCount "Creality3DrecentProject/numOfRecentFiles"
#define recentFileListId "Creality3DrecentProject/fileList"
#define Max_RecentFile_Size 10

namespace creative_kernel
{
    class Cx3dSubmenuRecentProject : public ActionCommand
        , public UIVisualTracer
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractListModel* subMenuActionmodel READ subMenuActionmodel CONSTANT)
    public:
        explicit Cx3dSubmenuRecentProject(QObject *parent);
        virtual ~Cx3dSubmenuRecentProject();

        QAbstractListModel* subMenuActionmodel();

        void setNumOfRecentFiles(int n);
        void setMostRecentFile(const QString fileName);
        static Cx3dSubmenuRecentProject* getInstance();
        static Cx3dSubmenuRecentProject* createInstance(QObject* parent);
    protected:
        void initActionModel();
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

        void updateRecentFiles(QSettings& settings);
    public slots:
        void slotClear();
    private:
        ActionCommandModel* m_actionModelList = nullptr;
        static Cx3dSubmenuRecentProject* m_instance;
    };
}
#endif // CX3DSUBMENURECENTPROJECT_H
