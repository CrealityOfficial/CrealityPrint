#ifndef BASICTREEMODEL_H
#define BASICTREEMODEL_H

#include <QObject>
#include <QtCore/QAbstractListModel>
#include "qtusercore/qtusercoreexport.h"

class BasicTreeCommand;

class QTUSER_CORE_API BasicTreeModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit BasicTreeModel(QObject* parent = nullptr);
    virtual ~BasicTreeModel();

    void addCommand(BasicTreeCommand* command);

    void removeCommand(BasicTreeCommand* command);
    void changeCommand(BasicTreeCommand* command);
    Q_INVOKABLE void removeAllCommand();

    void setSubLevel();
   // Q_INVOKABLE QVariant getCommands();
protected:
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    QList<BasicTreeCommand*> m_actionCommands;

    QHash<int, QByteArray> m_rolesName;
};

#endif // BASICTREEMODEL_H
