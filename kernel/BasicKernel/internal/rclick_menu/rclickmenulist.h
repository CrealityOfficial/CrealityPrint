#ifndef RCLICKMENULIST_H
#define RCLICKMENULIST_H
#include "menu/actioncommand.h"
#include <QMap>

namespace creative_kernel
{
    class SubMenuStandardPart;
    class SubMenuRecentFiles;
    class SubMenuTestModel;
    class SetNozzleRange;
    class UploadModelAction;
    class RClickMenuList : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QVariantList noModelsMenuList READ noModelsMenuList)
        Q_PROPERTY(QVariantList singleModelMenuList READ singleModelMenuList)
        Q_PROPERTY(QVariantList multiModelsMenuList READ multiModelsMenuList)

        Q_PROPERTY(QObject* recentFilesModel READ recentFilesModel)
        Q_PROPERTY(QObject* testModelsModel READ testModelsModel)
        Q_PROPERTY(QObject* nozzleModel READ nozzleModel)
        Q_PROPERTY(QObject* uploadModel READ uploadModel)

        Q_PROPERTY(int menuType READ menuType)

    public:
        explicit RClickMenuList(QObject* parent = nullptr);
        virtual ~RClickMenuList();

        // void addActionCommad(ActionCommand* pAction);
        // void addActionCommad(ActionCommand* pAction, int index);
        // void addActionCommad(ActionCommand* pAction, QString strInfo);
        // void removeActionCommand(ActionCommand* command);
        void setNozzleCount(int nCount);

        Q_INVOKABLE void updateState();

        QVariantList noModelsMenuList();
        QVariantList singleModelMenuList();
        QVariantList multiModelsMenuList();
        QObject* recentFilesModel();
        QObject* testModelsModel();
        QObject* nozzleModel();
        QObject* uploadModel();

        void setMenuType(int type);
        int menuType();

        Q_INVOKABLE QObject* getActionObject(const QString& actionName);
        Q_INVOKABLE QObject* getSubMenuObject(const QString& menuName);

    protected:
        // int indexOfString(QString strInfo);
        void startAddCommads();

    private:
        QList<ActionCommand*> m_noModelMenuList;
        QList<ActionCommand*> m_singleModelMenuList;
        QList<ActionCommand*> m_multiModelsMenuList;

        QMap<QString, ActionCommand*> m_commandsMap;
        
        SubMenuStandardPart* m_addPart;
        SubMenuStandardPart* m_addNegativePart;
        SubMenuStandardPart* m_addModifier;
        SubMenuStandardPart* m_addSupportBlocker;
        SubMenuStandardPart* m_addSupportEnforcer;
        
        SubMenuRecentFiles* m_recentFiles { NULL };
        SubMenuTestModel* m_testModels { NULL };
        UploadModelAction* m_upAction;
        QList<QColor> m_ExtruderColors;

        int m_menuType { 0 };
    };
}

#endif // RClickMenuList_H
