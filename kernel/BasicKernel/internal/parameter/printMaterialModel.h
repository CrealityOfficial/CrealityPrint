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
			ME_ITEM,
			ME_ENABLED
		};
		explicit PrintMaterialModel(QObject* parent = nullptr);
		PrintMaterialModel(const PrintMaterialModel& printMaterialModel){}
		virtual ~PrintMaterialModel();

		int checkedCount();
		void addMaterial(MaterialData* md);
		void initMaterial();
		void setSupportMaterialNames(QStringList names);
		void filterType(QString type, bool checked);
		void filterAllType(bool checked);
		QStringList supportMaterialNames();
		QList<MaterialData*> materialList();

		void setSelectMaterials(QList<QString> materials);
		void resetModel();
		void refresh();

		void onCancel();
		void onConfirm();

		bool isCheckedAll();
		void setIsCheckedAll(int checkedAll);
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
		int m_IsCheckedAll;
		QStringList m_SupportMaterials;;
		QList<MaterialData*> m_MaterialList;

		QList<QString> m_SelectMaterials;
		QList<QString> m_SelectMaterialsPre;
		QList<QString> m_SelectMaterialsDefault;
	};
}
Q_DECLARE_METATYPE(creative_kernel::PrintMaterialModel)
#endif //PrintMaterialModel_H
