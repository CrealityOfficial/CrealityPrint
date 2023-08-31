#ifndef PrintMaterialModel_H
#define PrintMaterialModel_H

#include <QObject>
#include <QAbstractListModel>
#include "data/header.h"

namespace creative_kernel
{

	class PrintMaterialModel : public QAbstractListModel
	{
		Q_OBJECT

	public:
		enum MaterialListEnum {
			ME_NAME,
			ME_CHECKED,
			ME_ISUSER,
			ME_ITEM
		};
		explicit PrintMaterialModel(QObject* parent = nullptr);
		PrintMaterialModel(const PrintMaterialModel& printMaterialModel){}
		virtual ~PrintMaterialModel();

		void addMaterial(MaterialMeta* md);
		void initMaterial();
		void setSupportMaterialNames(QStringList names);
		QStringList supportMaterialNames();
		QList<MaterialMeta*> materialList();

		bool isCheckedAll();
		void setIsCheckedAll(bool checkedAll);
		bool setChecked(bool checked, int index);

		QStringList userMaterialList();
	protected:
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		virtual QHash<int, QByteArray> roleNames() const override;
		int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	signals:
		void isCheckedAllChanged();

	private:
		bool m_IsCheckedAll;
		QStringList m_SupportMaterials;;
		QList<MaterialMeta*> m_MaterialList;
	};
}
Q_DECLARE_METATYPE(creative_kernel::PrintMaterialModel)
#endif //PrintMaterialModel_H
