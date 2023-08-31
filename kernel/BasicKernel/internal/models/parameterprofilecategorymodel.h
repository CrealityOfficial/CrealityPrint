#ifndef CREATIVE_KERNEL_PARAMETERPROFILECATEGORYMODEL_1676525780906_H
#define CREATIVE_KERNEL_PARAMETERPROFILECATEGORYMODEL_1676525780906_H
#include <QtCore/QAbstractListModel>
#include "us/usettings.h"

namespace creative_kernel
{
	class ParameterProfileCategoryModel : public QAbstractListModel {
		Q_OBJECT;
	public:
		ParameterProfileCategoryModel(us::USettings* settings, QObject* parent = nullptr, const bool& isSingleExtruder = false);
		virtual ~ParameterProfileCategoryModel();
		Q_INVOKABLE QString getFirtCategory();
	protected:
		int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;
	protected:
		us::USettings* m_settings;
		QList<us::SettingGroupDef*> m_defs;
	};
}

#endif // CREATIVE_KERNEL_PARAMETERPROFILECATEGORYMODEL_1676525780906_H