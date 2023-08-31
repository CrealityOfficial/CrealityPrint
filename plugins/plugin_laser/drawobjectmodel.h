#ifndef DRAWOBJECTMODEL_H
#define DRAWOBJECTMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include "drawobject.h"
class DrawObjectModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit DrawObjectModel(QObject *parent = nullptr);
    enum TitleRoles {
              NameRole = Qt::UserRole + 1,
              TypeRole
          };
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;
    void addDrawObject(DrawObject* obj);

    int objSize() { return m_drawItemList.size(); }
public slots:
    DrawObject* getData(int index);
    DrawObject* getDataByQmlObj(QObject *obj);
    int getIndex(QObject *obj);

public:
  Q_PROPERTY(int modelCount READ getModelCount NOTIFY sigModelCountChanged);
  Q_INVOKABLE int getModelCount() const;
  Q_SIGNAL void sigModelCountChanged(int model_count);

private:
    QList<DrawObject*> m_drawItemList;
};

#endif // DRAWOBJECTMODEL_H
