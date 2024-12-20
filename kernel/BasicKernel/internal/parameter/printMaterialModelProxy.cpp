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
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		if (!sModel)
			return;
		QList<MaterialData*> materialList = sModel->materialList();
		if (checked)
		{
			if (!m_MaterialFilterTypes.contains(mType))
			{
				m_MaterialFilterTypes.append(mType);
			}
		}
		else {
			if (m_MaterialFilterTypes.contains(mType))
			{
				m_MaterialFilterTypes.removeOne(mType);
			}
		}

		invalidateFilter();
	}

	void PrintMaterialModelProxy::checkedAllTypes(bool checked)
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		if (!sModel)
			return;

		if (checked)
		{
			m_MaterialFilterTypes = m_MaterialFilterTypesDefault;
		}
		else
		{
			m_MaterialFilterTypes.clear();
		}
		invalidateFilter();
	}

	void PrintMaterialModelProxy::refreshState()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		// QList<MaterialData*> materialList = sModel->materialList();
		sModel->filterType("Type", true);
		emit checkStateChanged();
		emit visibleMaterialsChanged();
		invalidateFilter();
	}

	void PrintMaterialModelProxy::setSelectMaterialTypes(QStringList selectMaterials)
	{
		m_MaterialFilterTypesDefault = selectMaterials;
		m_MaterialFilterTypesPrevious = selectMaterials;
		m_MaterialFilterTypes = selectMaterials;
	}

	int PrintMaterialModelProxy::visibleMaterials()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		if (!sModel)
			return false;

		QList<MaterialData*> materialsList = sModel->materialList();

		int visibleCount = 0;
		for (MaterialData* data : materialsList)
		{
			if (m_MaterialFilterTypes.contains(data->type))
			{
				++visibleCount;
			}
		}

		return visibleCount;
	}

	Q_INVOKABLE void PrintMaterialModelProxy::filterMeterilasByIsUser(bool isUser)
	{
		m_IsCurrentUser = isUser;
		invalidateFilter();
	}

	void PrintMaterialModelProxy::confirm()
	{
		m_MaterialFilterTypesPrevious = m_MaterialFilterTypes;

		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		sModel->onConfirm();
	}

	void PrintMaterialModelProxy::resetModel()
	{
		m_MaterialFilterTypes = m_MaterialFilterTypesDefault;
		m_MaterialFilterTypesPrevious = m_MaterialFilterTypesDefault;

		invalidateFilter();

		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		sModel->resetModel();
		//emit isCheckedAllChanged();
	}

	void PrintMaterialModelProxy::cancelModel()
	{
		m_MaterialFilterTypes = m_MaterialFilterTypesPrevious;

		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		sModel->onCancel();

		emit checkStateChanged();
	}

	bool PrintMaterialModelProxy::isVisible(const QString& materialType)
	{
		bool isContains = m_MaterialFilterTypes.contains(materialType);
		return isContains;
	}
	
	int PrintMaterialModelProxy::checkState()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		if (!sModel)
			return false;

		QList<MaterialData*> materialsList = sModel->materialList();

		int checkedCount = 0;
		int visibleCount = 0;
		for (MaterialData* data : materialsList)
		{
			if (m_MaterialFilterTypes.contains(data->type))
			{
				++visibleCount;
				if (data->isChecked)
				{
					++checkedCount;
				}
			}
		}

		int res = checkedCount == visibleCount ?
			Qt::CheckState::Checked : (checkedCount == 0 ? Qt::CheckState::Unchecked : Qt::CheckState::PartiallyChecked);

		m_CheckState = res;
		return res;
	}

	void PrintMaterialModelProxy::setCheckState(int checkedAll)
	{
		if (checkedAll == m_CheckState)
			return;

		m_CheckState = checkedAll;
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		sModel->setIsCheckedAll(checkedAll);

		emit checkStateChanged();
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

	void PrintMaterialModelProxy::setChecked(bool checked, int index)
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		bool checkedAll = sModel->setChecked(checked, index);

		emit checkStateChanged();
	}

	void PrintMaterialModelProxy::refreshModel()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		sModel->refresh();
	}

	int PrintMaterialModelProxy::selectMaterialsCount()
	{
		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		int checkedCount = sModel->checkedCount();
		return checkedCount;
	}

	bool PrintMaterialModelProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
	{
		QModelIndex stateIndex = sourceModel()->index(sourceRow, 0, sourceParent);
		QString meName = sourceModel()->data(stateIndex, PrintMaterialModel::ME_NAME).toString();

		PrintMaterialModel* sModel = qobject_cast<PrintMaterialModel*>(sourceModel());
		QList<MaterialData*> materialList = sModel->materialList();

		bool res = false;
		MaterialData* materialData = materialList.at(sourceRow);

		if (m_MaterialFilterTypes.contains(materialData->type) /*&& materialData->isVisible*/)
		{
			res = true;
		}

		return res;
	}
}
