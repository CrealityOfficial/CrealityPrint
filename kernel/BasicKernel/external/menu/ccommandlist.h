#ifndef CCOMMANDLIST_H
#define CCOMMANDLIST_H

#include <QObject>
#include "basickernelexport.h"
#include "menu/actioncommand.h"

class ActionCommand;
class BASIC_KERNEL_API CCommandList : public QObject
{
    Q_OBJECT
public:
    explicit CCommandList(QObject *parent = nullptr);
    void addActionCommad(ActionCommand *pAction);
    void addActionCommads(QList< ActionCommand *>vecAction);
    //
    void addActionCommad(ActionCommand *pAction, int index);
    //
    void addActionCommad(ActionCommand *pAction,QString strInsetKey);
    void removeActionCommand(ActionCommand* command);
    int indexOfString(QString strInfo);
    QList<ActionCommand *> getActionCommandList();
    void startAddCommads();
     static CCommandList *getInstance();
private:
     QList<ActionCommand *> m_varCommandList;
     static CCommandList * m_pComInstance;
};

#endif // CCOMMANDLIST_H
