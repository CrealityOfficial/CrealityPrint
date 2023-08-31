#ifndef RCLICKMENULIST_H
#define RCLICKMENULIST_H
#include "menu/actioncommand.h"

namespace creative_kernel
{
    class SetNozzleRange;
    class UploadModelAction;
    class RClickMenuList : public QObject
    {
        Q_OBJECT
    public:
        explicit RClickMenuList(QObject* parent = nullptr);
        virtual ~RClickMenuList();

        void addActionCommad(ActionCommand* pAction);
        void addActionCommad(ActionCommand* pAction, int index);
        void addActionCommad(ActionCommand* pAction, QString strInfo);
        void removeActionCommand(ActionCommand* command);

        void setNozzleCount(int nCount);
        Q_INVOKABLE QVariantList getCommandsList();
        Q_INVOKABLE QObject* getNozzleModel();
        Q_INVOKABLE QObject* getUploadModel();
    protected:
        int indexOfString(QString strInfo);
        void startAddCommads();
    private:
        QList<ActionCommand*> m_varCommandList;
        creative_kernel::SetNozzleRange* m_nozzleRange;
        creative_kernel::UploadModelAction* m_upAction;
    };
}

#endif // RClickMenuList_H
