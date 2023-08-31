#ifndef PRINTMATERIALMODELPROXY_H
#define PRINTMATERIALMODELPROXY_H

#include <QObject>
#include <QSortFilterProxyModel>
namespace creative_kernel
{
	class PrintMaterialModelProxy : public QSortFilterProxyModel
	{
		Q_OBJECT
			Q_PROPERTY(bool isCheckedAll READ isCheckedAll WRITE setIsCheckedAll NOTIFY isCheckedAllChanged)
			Q_PROPERTY(QStringList userMaterialsList READ userMaterialsList WRITE setUserMaterialsList NOTIFY isUserMaterialsListChanged)

	public:
		PrintMaterialModelProxy(QObject* parent = nullptr);
		~PrintMaterialModelProxy();

		Q_INVOKABLE void filterMaterialType(const QString& mType, bool checked);
		void setSelectMaterialTypes(QStringList selectMaterials);

		//根据用户还是默认耗材过滤
		Q_INVOKABLE void filterMeterilasByIsUser(bool isUser);

		Q_INVOKABLE bool isVisible(const QString& materialType);
		Q_INVOKABLE bool isCheckedAll();
		Q_INVOKABLE void setIsCheckedAll(bool checkedAll);

		QStringList userMaterialsList();
		void setUserMaterialsList(QStringList materials);

		Q_INVOKABLE void setChecked(bool checked, int index);

	signals:
		void isCheckedAllChanged();
		void isUserMaterialsListChanged();

	protected:
		bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

	private:
		bool m_IsCurrentUser = false;
		bool m_IsCheckedAll;
		QStringList m_MaterialFilterTypes;
	};
}

#endif // !PRINTMATERIALMODELPROXY_H
