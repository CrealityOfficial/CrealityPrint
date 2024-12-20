#ifndef ACTIONCOMMANDMODEL_H
#define ACTIONCOMMANDMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include "basickernelexport.h"

class ActionCommand;
class BASIC_KERNEL_API ActionCommandModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ActionCommandModel(QObject* parent = nullptr);
    virtual ~ActionCommandModel();

    void addCommand(ActionCommand* command);
    void addCommandFront(ActionCommand* command);

    void removeCommandFromQString(QString strName);
    void removeCommand(ActionCommand* command);
    void changeCommand(ActionCommand* command);
    Q_INVOKABLE void removeAllCommand();
    void removeCommandButLastIndex();
    ActionCommand* getData(int index);

    QList<ActionCommand*> actionCommands();
protected:
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    QList<ActionCommand*> m_actionCommands;
    QHash<int, QByteArray> m_rolesName;
};
#endif // ACTIONCOMMANDMODEL_H
