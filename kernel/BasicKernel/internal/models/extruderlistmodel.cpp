#include "extruderlistmodel.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printextruder.h"

namespace creative_kernel
{
    enum {
        IndexRole = Qt::UserRole + 1,
        ColorRole,
        MaterialRole,
        ExtruderObj,
        MaterialIndex,
        MaterialBrand,
        CurMaterialObj
    };

    ExtruderListModel::ExtruderListModel(PrintMachine* machine, QObject* parent)
        : QAbstractListModel(parent)
        , m_machine(machine)
    {
        connect(this, &QAbstractItemModel::rowsInserted, this, &ExtruderListModel::countChanged);
        connect(this, &QAbstractItemModel::rowsRemoved, this, &ExtruderListModel::countChanged);
        connect(this, &QAbstractItemModel::modelReset, this, &ExtruderListModel::countChanged);
    }

    ExtruderListModel::~ExtruderListModel()
    {
    }

    void ExtruderListModel::refresh()
    {
        beginResetModel();
        endResetModel();
    }

    void ExtruderListModel::refreshItem(int index)
    {
        emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, 0));
    }

    Q_INVOKABLE void ExtruderListModel::elmInsertRow(int row)
    {
        int eRow = m_machine->extruderAllCount() - 1;
        beginInsertRows(QModelIndex(), eRow, eRow);
        if (row != -1) {
            eRow = row;
        }
        endInsertRows();
    }

    Q_INVOKABLE void ExtruderListModel::elmRemoveRow(int row)
    {
        beginResetModel();
        int eRow = m_machine->extruderAllCount() - 1;
        endResetModel();
    }

    QVariantList ExtruderListModel::colorList()
    {
        int count = m_machine->extruderAllCount();
        QVariantList colorList;
        for (int index = 0; index < count; ++index)
        {
            colorList << m_machine->extruderColor(index);
        }
        return colorList;
    }
    QVariantList ExtruderListModel::typeList()
    {
        int count = m_machine->extruderAllCount();
        QVariantList typeList;
        for (int index = 0; index < count; ++index)
        {
            PrintExtruder* pe = qobject_cast<PrintExtruder*>(m_machine->extruderObject(index));
            QString materialName = pe->materialName();
            PrintMaterial* pm = m_machine->materialWithName(materialName);
            if (pm)
            {
                typeList<<pm->type();
            }
        }
        return typeList;
    }
    QVariantMap ExtruderListModel::get(int index) const {
        QVariantMap result{};

        const auto role_names = roleNames();
        const auto model_index = createIndex(index, 0);
        for (auto iter = role_names.cbegin(); iter != role_names.cend(); ++iter) {
            result.insert(iter.value(), data(model_index, iter.key()));
        }

        return result;
    }

    int ExtruderListModel::getCount() const {
        return rowCount({});
    }

    void ExtruderListModel::setExtruder(int index, const QString& uid, const QString& type, const QString& color)
    {
        PrintExtruder* pe = qobject_cast<PrintExtruder*>(m_machine->extruderObject(index));
        if (pe)
        {
            pe->setColor(color);
        }
    }

    int ExtruderListModel::rowCount(const QModelIndex& parent) const
    {
        if (!m_machine)
            return 0;

        return m_machine->extruderAllCount();
    }

    QHash<int, QByteArray> ExtruderListModel::roleNames() const
    {
        QHash<int, QByteArray> roles;
        roles[IndexRole] = "extruderIndex";
        roles[ColorRole] = "extruderColor";
        roles[MaterialRole] = "extruderMaterial";
        roles[MaterialIndex] = "extruderMaterialIndex";
        roles[ExtruderObj] = "extruderObj";
        roles[MaterialBrand] = "materialBrand";
        roles[CurMaterialObj] = "curMaterialObj";
        return roles;
    }

    QVariant ExtruderListModel::data(const QModelIndex& index, int role) const
    {
        if(!m_machine)
            return QVariant(QVariant::Invalid);

        int i = index.row();
        if (i < 0 || i >= m_machine->extruderAllCount())
            return QVariant(QVariant::Invalid);

        QVariant res;
        switch (role)
        {
        case IndexRole:
            res = i+1;
            break;
        case ColorRole:
            res = m_machine->extruderColor(i);
            break;
        case MaterialRole:
            res = m_machine->extruderMaterialName(i);
            break;
        case MaterialIndex:
            res = m_machine->extruderMaterialIndex(i);
            break;
        case ExtruderObj:
            res = QVariant::fromValue(m_machine->extruderObject(i));
            break;
        case MaterialBrand:
        case CurMaterialObj:
            PrintExtruder* pe = qobject_cast<PrintExtruder*>(m_machine->extruderObject(i));
            QString materialName = pe->materialName();
            PrintMaterial* pm = m_machine->materialWithName(materialName);
            if (!pm)
            {
                pm = m_machine->materialAt(0);
            }
            res = QVariant::fromValue(pm);
            break;
        }

        return res;
    }
}
