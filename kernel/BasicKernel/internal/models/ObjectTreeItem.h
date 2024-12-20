#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>
namespace creative_kernel
{
    class ObjectTreeItem : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QVariant itemData READ itemData NOTIFY itemDataChanged);
        Q_PROPERTY(int objectType READ objectType NOTIFY objectTypeChanged);
        Q_PROPERTY(bool checked READ checked NOTIFY checkedChanged);

    public:
        enum ObjType 
        {
            OT_ROOT = -1,
            OT_PLATE = 0,
            OT_GROUP,
            OT_MODEL,
            OT_INVALID
        };

        explicit ObjectTreeItem(ObjType objType, ObjectTreeItem* parentItem = nullptr);
        explicit ObjectTreeItem(ObjType objType, const QVector<QVariant>& data, ObjectTreeItem* parentItem = nullptr);
        virtual ~ObjectTreeItem();

        void setParent(ObjectTreeItem* parent);
        void appendChild(ObjectTreeItem* child);
        void prependChild(ObjectTreeItem* child);
        void appendChildren(const QVector<ObjectTreeItem*>& children);
        void insert(int row, const QVector<ObjectTreeItem*>& children);
        void insert(int row, ObjectTreeItem* child);
        void clear();
        void clearChildren();
        void removeAtNoDel(int row);
        void removeAt(int row);
        void remove(ObjectTreeItem* child);
        void moveChild(int srcRow, int dstRow);
        ObjectTreeItem* child(int row);
        int childIndex(ObjectTreeItem* child);
        QVector<ObjectTreeItem*> children();
        int childCount() const;
        int columnCount() const;

        QVariant data(int column) const;
        void setDatas(const QVector<QVariant>& datas);
        void setData(int index, const QVariant& data);
        int row() const;

        ObjectTreeItem* parentItem() const;

        QVariant itemData();
        ObjType objectType();
        void setObjectType(ObjType type);

        void setName(const QString& name);
        QString name();

        void setChecked(bool checked);
        bool checked();

        Q_INVOKABLE void setColorIndex(int colorIndex);
    signals:
        void itemDataChanged();
        void objectTypeChanged();
        void checkedChanged();

    private:
        bool m_IsChecked = false;
        ObjType m_ObjectType;
        QVector<ObjectTreeItem*> m_childItems;
        QVector<QVariant> m_itemData;
        ObjectTreeItem* m_parentItem = nullptr;
    };
}