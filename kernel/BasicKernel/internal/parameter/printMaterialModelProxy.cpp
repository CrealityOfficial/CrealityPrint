#include "printMaterialModelProxy.h"
#include "printMaterialModel.h"
#include "data/header.h"

namespace creative_kernel
{
	PrintMaterialModelProxy::PrintMaterialModelProxy(QObject* parent) :
		QSortFilterProxyModel(parent)
	{

	}

	PrintMaterialModelProxy::~PrintMaterialModelProxy()
	{

	}

	Q_INVOKABLE void PrintMaterialModelProxy::filterMaterialType(const QString& mType, bool checked)
	{
		if (checked)
		{
			PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
			QList<MaterialMeta*> materialList = sModel->materialList();
			for (MaterialMeta* data : materialList)
			{
				if (data->type == mType)
				{
					data->isVisible = true;
				}
			}
		}
		else {
			PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
			QList<MaterialMeta*> materialList = sModel->materialList();
			for (MaterialMeta* data : materialList)
			{
				if (data->type == mType)
				{
					data->isChecked = false;
					data->isVisible = false;
				}
			}
		}

		invalidateFilter();
	}

	void PrintMaterialModelProxy::setSelectMaterialTypes(QStringList selectMaterials)
	{
		m_MaterialFilterTypes = selectMaterials;
	}

	Q_INVOKABLE void PrintMaterialModelProxy::filterMeterilasByIsUser(bool isUser)
	{
		m_IsCurrentUser = isUser;
		invalidateFilter();
	}

	bool PrintMaterialModelProxy::isVisible(const QString& materialType)
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		QList<MaterialMeta*> materialList = sModel->materialList();

		bool isVisible = false;
		for (MaterialMeta* data : materialList)
		{
			if (data->type == materialType)
			{
				if (data->isVisible)
				{
					isVisible = true;
					break;
				}
			}
		}

		return isVisible;
	}
	
	bool PrintMaterialModelProxy::isCheckedAll()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		return sModel->isCheckedAll();
	}

	void PrintMaterialModelProxy::setIsCheckedAll(bool checkedAll)
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		sModel->setIsCheckedAll(checkedAll);
	}

	QStringList PrintMaterialModelProxy::userMaterialsList()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		QStringList mlist = sModel->userMaterialList();
		return mlist;
	}

	void PrintMaterialModelProxy::setUserMaterialsList(QStringList materials)
	{
	}

	Q_INVOKABLE void PrintMaterialModelProxy::setChecked(bool checked, int index)
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		bool checkedAll = sModel->setChecked(checked, index);
		m_IsCheckedAll = checkedAll;
		emit isCheckedAllChanged();
	}

	bool PrintMaterialModelProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
	{
		QModelIndex stateIndex = sourceModel()->index(sourceRow, 0, sourceParent);
		QString meName = sourceModel()->data(stateIndex, PrintMaterialModel::ME_NAME).toString();

		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		QList<MaterialMeta*> materialList = sModel->materialList();

		bool res = false;
		MaterialMeta* materialData = materialList.at(sourceRow);

		if (m_MaterialFilterTypes.contains(materialData->type) && materialData->isVisible)
		{
			res = true;
		}

		return res;
	}
}
