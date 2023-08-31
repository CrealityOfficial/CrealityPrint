#include "parameterprofilecategorymodel.h"

namespace creative_kernel
{
    enum {
        NameRole = Qt::UserRole + 1,
        NameRole1,
        NameRole2,
        NameRole3,
        NameRole4
    };

	ParameterProfileCategoryModel::ParameterProfileCategoryModel(us::USettings* settings, QObject* parent, const bool& isSingleExtruder)
        : QAbstractListModel(parent)
        , m_settings(settings)
	{
        //m_defs = m_settings->def()->profileCategoryGroup(isSingleExtruder);
        QList<us::SettingGroupDef*> defs = m_settings->def()->profileCategoryGroup(isSingleExtruder);
        Q_FOREACH(us::SettingGroupDef * group, defs)
        {
            for (QHash<QString, us::USetting*>::const_iterator it = m_settings->settings().constBegin();
                it != m_settings->settings().constEnd(); ++it)
            {
                us::SettingItemDef* def = it.value()->def();
                if (def->category == group->key)
                {
                    m_defs.append(group);
                    break;
                }
            }
        }
	}

	ParameterProfileCategoryModel::~ParameterProfileCategoryModel()
	{

	}

	int ParameterProfileCategoryModel::rowCount(const QModelIndex& parent) const
	{
		return m_defs.count();
	}
    QString ParameterProfileCategoryModel::getFirtCategory()
    {
        if (m_defs.size() > 0)
        {
            return m_defs.at(0)->key;
        }
        else {
            return "";
        }
       
    }
	QVariant ParameterProfileCategoryModel::data(const QModelIndex& index, int role) const
	{
        int i = index.row();
        if (i < 0 || i >= rowCount())
            return QVariant(QVariant::Invalid);

        us::SettingGroupDef* def = m_defs.at(i);
        if (role == NameRole)
            return QVariant(def->key);
        if (role == NameRole1)
            return QVariant(def->label);

        return QVariant(QVariant::Invalid);
	}

	QHash<int, QByteArray> ParameterProfileCategoryModel::roleNames() const
	{
        QHash<int, QByteArray> roles;
        roles[NameRole] = "key";
        roles[NameRole1] = "label";
        roles[NameRole2] = "NameRole2";
        roles[NameRole3] = "NameRole3";
        roles[NameRole4] = "NameRole4";

        return roles;
	}
}