#include "qtusercore/plugin/qmlentries.h"

enum
{
	tc_item = Qt::UserRole + 1
};

QmlEntries::QmlEntries(QObject* parent)
    :QAbstractListModel(parent)
{
    m_rolesName[tc_item] = "item";
}

QmlEntries::~QmlEntries()
{
}

void QmlEntries::add(QmlEntry* entry)
{
    if (entry)
    {
        const QModelIndex& index = QModelIndex();

		int insertIndex = 0;
		for (QmlEntry* e : m_entries)
		{
            int _index = m_entries.indexOf(entry);
            if (_index >= 0 && _index < m_entries.size())
                return;

			if (e->order() >= entry->order())
				break;
		
			++insertIndex;
		}

        beginInsertRows(index, insertIndex, insertIndex);
		m_entries.insert(insertIndex, entry);
        endInsertRows();

        if(!entry->parent())
			entry->setParent(this);
    }
}

void QmlEntries::append(QmlEntry* entry)
{
    if (entry)
    {
        const QModelIndex& index = QModelIndex();

        int size = m_entries.size();
        beginInsertRows(index, size, size);
        m_entries.append(entry);
        endInsertRows();

        if (!entry->parent())
            entry->setParent(this);
    }
}

void QmlEntries::remove(QmlEntry* entry)
{
    if (entry)
    {
        int index = m_entries.indexOf(entry);
        if (index >= 0 && index < m_entries.size())
        {
			beginRemoveRows(QModelIndex(), index, index);
			m_entries.removeAt(index);
			endRemoveRows();
        }
    }
}

void QmlEntries::removeFirstOne()
{
    int size = m_entries.size();
    if (size > 1)
    {
        QmlEntry* first = m_entries.front();
        beginRemoveRows(QModelIndex(), 1, 1);
        if (m_entries.at(1)->parent() == this)
            delete m_entries.at(1);      
        m_entries.removeAt(1);
        endRemoveRows();
    }
}

void QmlEntries::clearButFirst()
{
    int size = m_entries.size();
    if (size > 1)
    {
        QmlEntry* first = m_entries.front();
        beginRemoveRows(QModelIndex(), 1, size - 1);
        for (int i = 1; i < size; ++i)
            if (m_entries.at(i)->parent() == this)
                delete m_entries.at(i);
        m_entries.clear();
        m_entries.append(first);
        endRemoveRows();
    }
}

QObject* QmlEntries::rawData(int index)
{
    if (m_entries.size() > index && index >= 0)
        return m_entries.at(index);
    return nullptr;
 }

int QmlEntries::rowCount(const QModelIndex& parent) const
{
    return m_entries.size();
}

int QmlEntries::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QVariant QmlEntries::data(const QModelIndex& index, int role) const
{
    if (index.row() >= 0 && index.row() < m_entries.size())
    {
        QmlEntry* entry = m_entries.at(index.row());
        return QVariant::fromValue(entry);
    }
    return QVariant(0);
}

bool QmlEntries::setData(const QModelIndex& index, const QVariant& value, int role)
{
    return false;
}

QHash<int, QByteArray> QmlEntries::roleNames() const
{
    return m_rolesName;
}


