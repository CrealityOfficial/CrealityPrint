#ifndef CXGCODE_DEFENUMSMODEL_1676525780906_H
#define CXGCODE_DEFENUMSMODEL_1676525780906_H
#include <QAbstractListModel>

namespace cxgcode
{
	class SettingItemDef;
	class DefEnumsModel : public QAbstractListModel {
		Q_OBJECT;
	public:
		DefEnumsModel(SettingItemDef* def, QObject* parent = nullptr);
		virtual ~DefEnumsModel();

	protected:
		int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;
	protected:
		SettingItemDef* m_def;

		QList<QString> m_keys;
		QList<QString> m_values;
	};
}

#endif // CXGCODE_DEFENUMSMODEL_1676525780906_H