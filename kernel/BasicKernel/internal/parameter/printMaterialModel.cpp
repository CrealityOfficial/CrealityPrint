#include "printMaterialModel.h"

namespace creative_kernel
{
	PrintMaterialModel::PrintMaterialModel(QObject* parent) :
		QAbstractListModel(parent)
	{

	}

	PrintMaterialModel::~PrintMaterialModel()
	{
	}

	Q_INVOKABLE bool PrintMaterialModel::isCheckedAll()
	{
		bool res = true;
		for (MaterialMeta* data : m_MaterialList)
		{
			if (!data->isChecked)
			{
				m_IsCheckedAll = false;
			}
		}

		return m_IsCheckedAll;
	}

	void PrintMaterialModel::setIsCheckedAll(bool checkedAll)
	{
		m_IsCheckedAll = checkedAll;

		beginResetModel();
		for (MaterialMeta* data : m_MaterialList)
		{
			data->isChecked = checkedAll;
		}
		endResetModel();
	}

	Q_INVOKABLE bool PrintMaterialModel::setChecked(bool checked, int index)
	{
		MaterialMeta* data = m_MaterialList.at(index);
		data->isChecked = checked;

		bool checkedAll = true;
		for (MaterialMeta* data : m_MaterialList)
		{
			if (!data->isChecked)
			{
				checkedAll = false;
			}
		}
		 
		m_IsCheckedAll = checkedAll;
		return m_IsCheckedAll;
	}

	QStringList PrintMaterialModel::userMaterialList()
	{
		QStringList materialsList;
		for (MaterialMeta* data : m_MaterialList)
		{
			if (data->isUserDef)
			{
				materialsList.push_back(data->name);
			}
		}

		return materialsList;
	}

	QVariant PrintMaterialModel::data(const QModelIndex& index, int role) const
	{
		int i = index.row();
		if (i < 0 || i >= m_MaterialList.count())
			return QVariant(QVariant::Invalid);

		MaterialMeta* md = m_MaterialList.at(index.row());
		switch (role)
		{
		case ME_NAME:
			return md->name;
			break;
		case ME_CHECKED:
			return md->isChecked;
			break;
		case ME_ISUSER:
			return md->isUserDef;
			break;

		default:
			break;
		}
		return QVariant();
	}

	void PrintMaterialModel::addMaterial(MaterialMeta* md)
	{
		QString materialName = md->uniqueName();
		if (!m_SupportMaterials.contains(materialName))
		{
			return;
		}
		m_MaterialList.push_back(md);
	}

	void PrintMaterialModel::initMaterial()
	{
		return;
	}

	void PrintMaterialModel::setSupportMaterialNames(QStringList names)
	{
		m_SupportMaterials = names;
	}

	QStringList PrintMaterialModel::supportMaterialNames()
	{
		return m_SupportMaterials;
	}

	QList<MaterialMeta*> PrintMaterialModel::materialList()
	{
		return m_MaterialList;
	}

	bool PrintMaterialModel::setData(const QModelIndex& index, const QVariant& value, int role)
	{
		return false;
	}

	QHash<int, QByteArray> PrintMaterialModel::roleNames() const
	{
		QHash<int, QByteArray> roleHash;
		roleHash[ME_NAME] = "meName";
		roleHash[ME_CHECKED] = "meChecked";
		roleHash[ME_ISUSER] = "meIsUser";
		roleHash[ME_ITEM] = "meItem";
		return roleHash;
	}

	int PrintMaterialModel::rowCount(const QModelIndex& parent) const
	{
		return m_MaterialList.count();
	}
}