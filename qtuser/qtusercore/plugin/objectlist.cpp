#include "objectlist.h"

namespace qtuser_qml
{
    enum
    {
        tc_item = Qt::UserRole + 1,
    };

    ObjectList::ObjectList(QObject* parent)
        :QAbstractListModel(parent)
    {
        m_rolesName[tc_item] = "item";
    }

    ObjectList::~ObjectList()
    {
    }

    void ObjectList::setItems(QList<QObject*> items)
    {
        if (m_items.size() > 0)
        {
            beginRemoveRows(QModelIndex(), 0, m_items.size() - 1);
            m_items.clear();
            endRemoveRows();
        }

        if (items.size() > 0)
        {
            beginInsertRows(QModelIndex(), 0, items.size() - 1);
            m_items = items;
            endInsertRows();
        }
    }

    int ObjectList::rowCount(const QModelIndex& parent) const
    {
        return m_items.size();
    }

    int ObjectList::columnCount(const QModelIndex& parent) const
    {
        return 1;
    }

    QVariant ObjectList::data(const QModelIndex& index, int role) const
    {
        if (index.row() >= 0 && index.row() < m_items.size())
        {
            QObject* item = m_items.at(index.row());
            switch (role)
            {
            case tc_item:
                return QVariant::fromValue(item);
            default:
                break;
            }
        }
        return QVariant(0);
    }

    bool ObjectList::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        return false;
    }

    QHash<int, QByteArray> ObjectList::roleNames() const
    {
        return m_rolesName;
    }
}