#include "defenumsmodel.h"
#include "cxgcode/us/settingitemdef.h"

namespace cxgcode
{
    enum {
        NameRole = Qt::UserRole + 1,
        NameRole1,
        NameRole2,
        NameRole3,
        NameRole4
    };

    DefEnumsModel::DefEnumsModel(SettingItemDef* def, QObject* parent)
        : QAbstractListModel(parent)
        , m_def(def)
	{
		if (m_def->type == "optional_extruder" || m_def->type == "extruder")
		{
			m_keys << "-1" << "0" << "1";
			m_values << "Not overridden" << "Extruder 1" << "Extruder 2";
		}
		else if (def->type == "enum") {
			for (QMap<QString, QString>::const_iterator it = def->options.constBegin(); it != def->options.constEnd(); ++it)
			{
				m_keys.append(it.key());
				m_values.append(it.value());
			}
		}
	}

    DefEnumsModel::~DefEnumsModel()
	{

	}

	int DefEnumsModel::rowCount(const QModelIndex& parent) const
	{
		if (m_def->type == "optional_extruder"
			|| m_def->type == "extruder")
			return 3;
		return m_def->options.count();
	}

	QVariant DefEnumsModel::data(const QModelIndex& index, int role) const
	{
        int i = index.row();
        if (i < 0 || i >= rowCount())
            return QVariant(QVariant::Invalid);

		if (role == NameRole)
			return QVariant(m_keys.at(i));
		if (role == NameRole1)
			return QVariant(m_values.at(i));
		if (role == NameRole2)
			return QVariant("");

        return QVariant(QVariant::Invalid);
	}

	QHash<int, QByteArray> DefEnumsModel::roleNames() const
	{
        QHash<int, QByteArray> roles;
        roles[NameRole] = "key";
        roles[NameRole1] = "modelData";
        roles[NameRole2] = "shapeImg";
        roles[NameRole3] = "NameRole3";
        roles[NameRole4] = "NameRole4";

        return roles;
	}
}