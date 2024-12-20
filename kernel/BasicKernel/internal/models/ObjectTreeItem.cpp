#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>
#include "ObjectTreeItem.h"
#include "data/modeln.h"
#include "internal/multi_printer/printer.h"
#include "external/interface/printerinterface.h"

namespace creative_kernel
{
    ObjectTreeItem::ObjectTreeItem(ObjType objType, ObjectTreeItem* parentItem)
        : m_ObjectType(objType),
        m_parentItem(parentItem)
    {
    }
    ObjectTreeItem::ObjectTreeItem(ObjType objType, const QVector<QVariant>& data, ObjectTreeItem* parentItem)
        :m_ObjectType(objType),
        m_itemData(data),
        m_parentItem(parentItem)
    {
    }

    ObjectTreeItem::~ObjectTreeItem()
    {
        //qDeleteAll(m_childItems);
        clear();
    }

    ObjectTreeItem::ObjType ObjectTreeItem::objectType()
    {
        return m_ObjectType;
    }

    void ObjectTreeItem::setObjectType(ObjType type)
    {
        m_ObjectType = type;
    }

    void ObjectTreeItem::setName(const QString& name)
    {
        //switch (m_ObjectType)
        //{
        //case creative_kernel::ObjectTreeItem::OT_PLATE:
        //    Printer* pt = m_itemData[0].value<Printer*>();
        //    pt->setName(name);
        //    break;
        //case creative_kernel::ObjectTreeItem::OT_GROUP:
        //    break;
        //case creative_kernel::ObjectTreeItem::OT_MODEL:
        //    break;
        //default:
        //    break;
        //}
    }

    QString ObjectTreeItem::name()
    {
        return QString();
    }

    void ObjectTreeItem::setChecked(bool checked)
    {
        if (checked == m_IsChecked)
            return;
        m_IsChecked = checked;
        emit checkedChanged();
    }

    bool ObjectTreeItem::checked()
    {
        return m_IsChecked;
    }

    void ObjectTreeItem::setColorIndex(int colorIndex)
    {
        updateWipeTowers();
    }

    void ObjectTreeItem::setParent(ObjectTreeItem* parent)
    {
        if (m_parentItem == parent)
            return;
        m_parentItem = parent;
    }

    void ObjectTreeItem::appendChild(ObjectTreeItem* child)
    {
        if (!child)
            return;
        m_childItems.append(child);
        child->setParent(this);
    }
    void ObjectTreeItem::prependChild(ObjectTreeItem* child)
    {
        if (!child)
            return;
        m_childItems.prepend(child);
        child->setParent(this);
    }
    void ObjectTreeItem::appendChildren(const QVector<ObjectTreeItem*>& children)
    {
        m_childItems.append(children);
    }
    void ObjectTreeItem::insert(int row, const QVector<ObjectTreeItem*>& children)
    {
        if (0 <= row && 0 < m_childItems.size())
        {
            for (int j = 0; j < children.size(); ++j)
            {
                m_childItems.insert(row + j, children[j]);
                children[j]->setParent(this);
            }
        }
    }
    void ObjectTreeItem::insert(int row, ObjectTreeItem* child)
    {
        if (!child)
            return;

        if (0 <= row && row <= m_childItems.size())
        {
            m_childItems.insert(row, child);
            child->setParent(this);
        }
    }
    void ObjectTreeItem::clear()
    {
        qDeleteAll(m_childItems);
        m_childItems.clear();
        m_itemData.clear();
    }

    void ObjectTreeItem::clearChildren()
    {
        qDeleteAll(m_childItems);
        m_childItems.clear();
    }

    void ObjectTreeItem::removeAtNoDel(int row)
    {
        if (0 <= row && row < m_childItems.size())
        {
            m_childItems.removeAt(row);
        }
    }

    void ObjectTreeItem::removeAt(int row)
    {
        if (0 <= row && row < m_childItems.size())
        {
            delete m_childItems.at(row);
            m_childItems.removeAt(row);
        }
    }

    void ObjectTreeItem::remove(ObjectTreeItem* child)
    {
        int index = 0;
        for (auto item : m_childItems)
        {
            if (item == child)
            {
                break;
            }
            index++;
        }

        removeAt(index);
    }

    void ObjectTreeItem::moveChild(int srcRow, int dstRow)
    {
        m_childItems.move(srcRow, dstRow);
    }

    ObjectTreeItem* ObjectTreeItem::child(int row)
    {
        if (row < 0 || row >= m_childItems.size())
            return nullptr;
        return m_childItems.at(row);
    }

    int ObjectTreeItem::childIndex(ObjectTreeItem* child)
    {
       return m_childItems.indexOf(child);
    }

    QVector<ObjectTreeItem*> ObjectTreeItem::children()
    {
        return m_childItems;
    }

    int ObjectTreeItem::childCount() const
    {
        return m_childItems.count();
    }
    int ObjectTreeItem::columnCount() const
    {
        return m_itemData.count();
    }

    Q_INVOKABLE QVariant ObjectTreeItem::data(int column) const
    {
        if (0 <= column && column < m_itemData.size())
        {
            return m_itemData.at(column);
        }
        return {};
    }
    void ObjectTreeItem::setDatas(const QVector<QVariant>& datas)
    {
        m_itemData = datas;
    }
    void ObjectTreeItem::setData(int index, const QVariant& data)
    {
        if (0 <= index && index < m_itemData.size())
        {
            m_itemData[index] = data;
        }
    }
    int ObjectTreeItem::row() const
    {
        if (m_parentItem)
            return m_parentItem->m_childItems.indexOf(const_cast<ObjectTreeItem*>(this));
        return 0;
    }

    ObjectTreeItem* ObjectTreeItem::parentItem() const
    {
        return m_parentItem;
    }

    QVariant ObjectTreeItem::itemData()
    {
        return data(0);
    }
}