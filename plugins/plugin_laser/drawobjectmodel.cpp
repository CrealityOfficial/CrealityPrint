#include "drawobjectmodel.h"

DrawObjectModel::DrawObjectModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DrawObjectModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;
    return m_drawItemList.size();

}

QVariant DrawObjectModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    DrawObject *dobj = m_drawItemList.at(index.row());
    switch (role)
    {
            case NameRole:
                return dobj->name();
            case TypeRole:
                return dobj->type();
            default:
                break;
    }
    return QVariant(0);
}

bool DrawObjectModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();

    Q_EMIT sigModelCountChanged(getModelCount());
    return true;
}

bool DrawObjectModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= 0)
    {
        DrawObject* drawObj = m_drawItemList.at(row);
        delete  drawObj;
        beginRemoveRows(parent, row, row + count - 1);
        m_drawItemList.removeAt(row);
        endRemoveRows();

        Q_EMIT sigModelCountChanged(getModelCount());
    }
    
    return true;
}
QHash<int, QByteArray> DrawObjectModel::roleNames() const {
      QHash<int, QByteArray> roles;
      roles[NameRole] = "name";
      roles[TypeRole] = "type";
      return roles;
  }
void DrawObjectModel::addDrawObject(DrawObject* obj)
{
    if (obj)
        {
            m_drawItemList.push_back(obj);
            int index = m_drawItemList.size()-1;
            beginInsertRows(QModelIndex(), index, index);
            endInsertRows();

            Q_EMIT sigModelCountChanged(getModelCount());
        }
}
DrawObject* DrawObjectModel::getData(int index)
{

    if(index<m_drawItemList.length()&&index>=0)
    {
        return m_drawItemList.at(index);

    }
    return nullptr;
}
int DrawObjectModel::getIndex(QObject *obj)
{
    for(int i=0;i<m_drawItemList.length();i++)
    {
        DrawObject*o = m_drawItemList.at(i);
        if(o->qmlObject()==obj)
        {
            return i;
        }
    }
    return -1;
}
DrawObject* DrawObjectModel::getDataByQmlObj(QObject *obj)
{
    int index = getIndex(obj);
    if(index>=0)
    {
        return m_drawItemList.at(index);
    }else{
        return nullptr;
    }

}

int DrawObjectModel::getModelCount() const {
  return m_drawItemList.size();
}
