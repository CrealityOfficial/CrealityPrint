#include "printMaterialModel.h"
#include "printmachine.h"
#include "printextruder.h"
#include "parametercache.h"
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
		for (MaterialData* data : m_MaterialList)
		{
			if (!data->isChecked)
			{
				m_IsCheckedAll = false;
				return m_IsCheckedAll;
			}
		}
		m_IsCheckedAll = true;
		return m_IsCheckedAll;
	}

	void PrintMaterialModel::setIsCheckedAll(int checkedAll)
	{
		if (checkedAll == m_IsCheckedAll)
			return;

		m_IsCheckedAll = checkedAll;

		int index = 0;
		beginResetModel();
		for (MaterialData* data : m_MaterialList)
		{
			if (data->isVisible) {
				setChecked(checkedAll, index);
			}
			++index;
		}
		endResetModel();
	}

	Q_INVOKABLE bool PrintMaterialModel::setChecked(bool checked, int index)
	{
		if (m_MaterialList.count() <= index)
			return false;
		MaterialData* data = m_MaterialList.at(index);
		if (data->isChecked == checked)
			return false;;

		data->isChecked = checked;

		if (checked)
		{
			if (!m_SelectMaterials.contains(data->name)) 
			{
				m_SelectMaterials.append(data->name);
			}
		}
		else
		{
			if (m_SelectMaterials.contains(data->name))
			{
				m_SelectMaterials.removeOne(data->name);
			}
		}

		bool checkedAll = true;
		for (MaterialData* data : m_MaterialList)
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
		for (MaterialData* data : m_MaterialList)
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

		MaterialData* md = m_MaterialList.at(index.row());

		PrintMachine* pm = qobject_cast<PrintMachine*>(parent());
		QVector<PrintMachine::SyncExtruderInfo> seis;
		QList<PrintExtruder*> extruders;
		if (pm) {
			seis = pm->syncedExtrudersInfo();
			extruders = pm->extruders();
		}
		bool isEnabled = true;
		for (auto ex : seis)
		{
			if (md->uniqueName() == ex.materialName)
			{
				isEnabled = false;
			}
		}
		for(PrintExtruder* et:extruders)
		{
				if (md->uniqueName() == et->materialName())
				{
					isEnabled = false;
				}
		}

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
		case ME_ENABLED:
			return isEnabled;
			break;
		default:
			break;
		}
		return QVariant();
	}

	int PrintMaterialModel::checkedCount()
	{
		int res = 0;
		for (MaterialData* data : m_MaterialList)
		{
			if (data->isChecked && data->isVisible)
			{
				res++;
			}
		}
		return res;
	}

	void PrintMaterialModel::addMaterial(MaterialData* md)
	{
		if (!m_SupportMaterials.contains(md->uniqueName()))
		{
			return;
		}
		m_MaterialList.push_back(md);
		if (md->isChecked)
		{
			m_SelectMaterials.append(md->name);
			m_SelectMaterialsPre.append(md->name);
			m_SelectMaterialsDefault.append(md->name);
		}
	}

	void PrintMaterialModel::initMaterial()
	{
		return;
	}

	void PrintMaterialModel::setSupportMaterialNames(QStringList names)
	{
		m_SupportMaterials = names;
	}

	void PrintMaterialModel::filterType(QString type, bool checked)
	{
		int index = 0;
		for (auto material : m_MaterialList)
		{
			MaterialData* data = m_MaterialList.at(index);
			if (data->type == type)
			{
				data->isVisible = checked;
				QModelIndex begin = createIndex(index, 0);
				QModelIndex end = begin;
				//emit dataChanged(begin, end);
			}
			++index;
		}
	}

	void PrintMaterialModel::filterAllType(bool checked)
	{
		int index = 0;
		for (auto material : m_MaterialList)
		{
			MaterialData* data = m_MaterialList.at(index);
			data->isVisible = checked;
			++index;
		}
	}

	QStringList PrintMaterialModel::supportMaterialNames()
	{
		return m_SupportMaterials;
	}

	QList<MaterialData*> PrintMaterialModel::materialList()
	{
		return m_MaterialList;
	}

	void PrintMaterialModel::setSelectMaterials(QList<QString> materials)
	{
		m_SelectMaterials = materials;
		m_SelectMaterialsPre = materials;
		m_SelectMaterialsDefault = materials;
	}

	void PrintMaterialModel::resetModel()
	{
		PrintMachine* pm = qobject_cast<PrintMachine*>(parent());
		QVector<PrintMachine::SyncExtruderInfo> seis;
		QList<PrintExtruder*> extruders;
		QList<QString> topMaterials;
		if (pm) {
			seis = pm->syncedExtrudersInfo();
			extruders = pm->extruders();
			topMaterials = pm->topMaterial();
		}
		m_SelectMaterials = m_SelectMaterialsDefault;
		m_SelectMaterialsPre = m_SelectMaterialsDefault;
		for (auto data : m_MaterialList)
		{
			if (topMaterials.contains(data->id))
			{
				data->isChecked = true;
			}
			else
			{
				data->isChecked = false;
			}
			data->isVisible = true;
			for (auto ex : seis)
			{
				if (data->uniqueName() == ex.materialName)
				{
					data->isChecked = true;
				}
			}
			for(PrintExtruder* et:extruders)
			{
				if (data->uniqueName() == et->materialName())
				{
					data->isChecked = true;
				}
			}
		}

		writeCacheSelectMaterials(pm->uniqueName(), QStringList());

	}

	void PrintMaterialModel::refresh()
	{
		beginResetModel();
		endResetModel();
	}
	void PrintMaterialModel::onCancel()
	{
		m_SelectMaterials = m_SelectMaterialsPre;

		for (auto data : m_MaterialList)
		{
			QString material = data->name;
			if (m_SelectMaterials.contains(material))
			{
				data->isChecked = true;
			}
			else 
			{
				data->isChecked = false;
			}
		}
	}

	void PrintMaterialModel::onConfirm()
	{
		m_SelectMaterialsPre = m_SelectMaterials;
		PrintMachine* pm = qobject_cast<PrintMachine*>(parent());
		QStringList selectMaterials;
		for (auto data : m_MaterialList)
		{
			if (data->isChecked)
			{
				selectMaterials.append(data->id);
			}
			
		}
		writeCacheSelectMaterials(pm->uniqueName(), selectMaterials);
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
		roleHash[ME_ENABLED] = "meEnabled";
		return roleHash;
	}

	int PrintMaterialModel::rowCount(const QModelIndex& parent) const
	{
		return m_MaterialList.count();
	}
}