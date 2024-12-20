#ifndef PRINTMATERIALMODELPROXY_H
#define PRINTMATERIALMODELPROXY_H

#include <QObject>
#include <QSortFilterProxyModel>
namespace creative_kernel
{
	class PrintMaterialModelProxy : public QSortFilterProxyModel
	{
		Q_OBJECT
			Q_PROPERTY(int checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged)
			Q_PROPERTY(int selectMaterialsCount READ selectMaterialsCount NOTIFY selectMaterialsCountChanged)
			Q_PROPERTY(int visibleMaterials READ visibleMaterials NOTIFY visibleMaterialsChanged)
			Q_PROPERTY(QStringList userMaterialsList READ userMaterialsList WRITE setUserMaterialsList NOTIFY isUserMaterialsListChanged)

	public:
		PrintMaterialModelProxy(QObject* parent = nullptr);
		~PrintMaterialModelProxy();

		Q_INVOKABLE void filterMaterialType(const QString& mType, bool checked);
		Q_INVOKABLE void checkedAllTypes(bool checked);
		void setSelectMaterialTypes(QStringList selectMaterials);

		//根据用户还是默认耗材过滤
		Q_INVOKABLE void filterMeterilasByIsUser(bool isUser);

		Q_INVOKABLE void confirm();
		Q_INVOKABLE void resetModel();
		Q_INVOKABLE void cancelModel();
		Q_INVOKABLE void refreshState();
		Q_INVOKABLE bool isVisible(const QString& materialType);
		Q_INVOKABLE void setChecked(bool checked, int index);
		Q_INVOKABLE void refreshModel();

		int visibleMaterials();

		int checkState();
		void setCheckState(int checkedAll);

		int selectMaterialsCount();

		QStringList userMaterialsList();
		void setUserMaterialsList(QStringList materials);

	signals:
		void checkStateChanged();
		void isUserMaterialsListChanged();
		void selectMaterialsCountChanged();
		void visibleMaterialsChanged();

	protected:
		bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

	private:
		bool m_IsCurrentUser = false;
		int m_CheckState;
		QStringList m_MaterialFilterTypes;
		QStringList m_MaterialFilterTypesDefault;
		QStringList m_MaterialFilterTypesPrevious;
	};
}

#endif // !PRINTMATERIALMODELPROXY_H
