#include "gcodeextrudertablemodel.h"

namespace cxgcode {

ModelItem::ModelItem(MaterialConsume* model, MaterialConsume* flush,
    MaterialConsume* filamentTower, MaterialConsume* total, QObject* parent)
    : QObject(parent) 
{
    m_Model = model;
    m_Flush = flush;
    m_FilamentTower = filamentTower;
    m_Total = total;
}

ModelItem::~ModelItem() {}

MaterialConsume* ModelItem::getDataByType(ModelItem::ConsumeType type)
{
    switch (type)
    {
    case cxgcode::ModelItem::CT_Color:
        return modelMaterial();
        break;
    case cxgcode::ModelItem::CT_Model:
        return modelMaterial();
        break;
    case cxgcode::ModelItem::CT_Flush:
        return flushMaterial();
        break;
    case cxgcode::ModelItem::CT_FilamentTower:
        return filamentTower();
        break;
    case cxgcode::ModelItem::CT_Total:
        return totalMaterial();
        break;
    default:
        break;
    }

    return nullptr;
}

MaterialConsume* ModelItem::modelMaterial() const
{
    return m_Model;
}

MaterialConsume* ModelItem::flushMaterial() const
{
    return m_Flush;
}

MaterialConsume* ModelItem::filamentTower() const
{
    return m_FilamentTower;
}

MaterialConsume* ModelItem::totalMaterial() const
{
    return m_Total;
}

GcodeExtruderTableModel::GcodeExtruderTableModel(QObject* parent)
: QAbstractTableModel(parent)
{

}

void GcodeExtruderTableModel::setColorList(const QList<GcodeExtruderData>& data_list)
{
    assert(data_list.count() > 0);
    beginResetModel();
    m_ColorsData = data_list;
    endResetModel();
}

void GcodeExtruderTableModel::setData(QList<ModelItem*> dataList)
{
    m_Materials = dataList;
}

void GcodeExtruderTableModel::setData(std::vector<std::pair<int, double>>& model, 
    std::vector<std::pair<int, double>>& flush, std::vector<std::pair<int, double>>& filamentTower)
{

    beginResetModel();

    m_IsSingleColor = false;
    if (!flush.size() && !filamentTower.size())
    {
        //单色模型
        m_IsSingleColor = true; 
    }

    if (m_Materials.size() > 0)
        m_Materials.clear();

    ModelItem* item = nullptr;
    MaterialConsume* mc_model = nullptr;
    MaterialConsume* mc_flush = nullptr;
    MaterialConsume* mc_filamentTower = nullptr;
    MaterialConsume* mc_total = nullptr;

    for (int index = 0; index < model.size(); ++index)
    {
        if (!(index % 2))
        {
            mc_model = new MaterialConsume(item);
            mc_flush = new MaterialConsume(item);
            mc_filamentTower = new MaterialConsume(item);
            mc_total = new MaterialConsume(item);
            
        }

        if (!(index % 2))
        {
            mc_model->materialIndex = model[index].first;
            mc_flush->materialIndex = model[index].first;
            mc_filamentTower->materialIndex = model[index].first;
            mc_total->materialIndex = model[index].first;

            if (!m_ColorsData.isEmpty())
            {
				mc_model->materialColor = m_ColorsData.at(mc_filamentTower->materialIndex).color;
				mc_flush->materialColor = m_ColorsData.at(mc_filamentTower->materialIndex).color;
				mc_filamentTower->materialColor = m_ColorsData.at(mc_filamentTower->materialIndex).color;
				mc_total->materialColor = m_ColorsData.at(mc_filamentTower->materialIndex).color;
            }

            mc_model->length = model[index].second;
            if (flush.size() > index)
                mc_flush->length = m_IsSingleColor ? 0 : flush[index].second;
            if(filamentTower.size() > index)
                mc_filamentTower->length = m_IsSingleColor ? 0 : filamentTower[index].second;
            mc_total->length = mc_model->length + mc_flush->length + mc_filamentTower->length;
        }
        else {
            mc_model->weight = model[index].second;
            if (flush.size() > index)
                mc_flush->weight = m_IsSingleColor ? 0 : flush[index].second;
            if (filamentTower.size() > index)
                mc_filamentTower->weight = m_IsSingleColor ? 0 : filamentTower[index].second;
            mc_total->weight = mc_model->weight + mc_flush->weight + mc_filamentTower->weight;
        }

        if (!(index % 2))
        {
            item = new ModelItem(mc_model, mc_flush, mc_filamentTower, mc_total, this);
            m_Materials.append(item);
        }
    }

    endResetModel();
    emit isSingleColorChanged();
}

bool GcodeExtruderTableModel::isSingleColor()
{
    return m_IsSingleColor;
}

void GcodeExtruderTableModel::setIsSingleColor(bool isSingle)
{
    m_IsSingleColor = isSingle;
}

int GcodeExtruderTableModel::getFirstItemMaterialIndex()
{
    int index = 0;
    if (m_Materials.size() > 0)
    {
        ModelItem* item = m_Materials.at(0);
        index = item->getMaterialIndex();
    }
    return index;
}



int GcodeExtruderTableModel::rowCount(const QModelIndex& parent) const 
{
  return m_Materials.count();
}

int GcodeExtruderTableModel::columnCount(const QModelIndex & parent) const
{
    if (m_IsSingleColor)
        return 1;
    else
        return 3;
}

QVariant GcodeExtruderTableModel::data(const QModelIndex& index, int role) const 
{
  QVariant result{ QVariant::Type::Invalid };

  if (index.row() < 0 || index.row() >= rowCount()) {
    return result;
  }

  if (m_ColorsData.size() <= index.row())
  {
      return result;
  }

  ModelItem* itemData = m_Materials.at(index.row());

  switch (role) 
  {
      case ModelItem::CT_Color:
      case ModelItem::CT_ColorIndex:
      {
          MaterialConsume* mc = itemData->getDataByType(ModelItem::CT_Model);
          return QVariant::fromValue(mc);
      }
      case ModelItem::CT_Model:
      {
          MaterialConsume* mc = itemData->getDataByType(ModelItem::CT_Model);
          return QVariant::fromValue(mc);
      }
      case ModelItem::CT_Flush:
      {
          MaterialConsume* mc = itemData->getDataByType(ModelItem::CT_Flush);
          return QVariant::fromValue(mc);
      }
      case ModelItem::CT_FilamentTower:
      {
          MaterialConsume* mc = itemData->getDataByType(ModelItem::CT_FilamentTower);
          return QVariant::fromValue(mc);
      }
      case ModelItem::CT_Total:
      {
          MaterialConsume* mc = itemData->getDataByType(ModelItem::CT_Total);
          return QVariant::fromValue(mc);
      }
      
      default:
          break;
  }

  return QVariant();
}

QHash<int, QByteArray> GcodeExtruderTableModel::roleNames() const {
    QHash<int, QByteArray> hash =  { {ModelItem::CT_Model, "ctModel"},
        {ModelItem::CT_Color, "ctColor"},
        {ModelItem::CT_Flush, "ctFlush"},
        {ModelItem::CT_FilamentTower, "ctFilamentTower"},
        {ModelItem::CT_Total, "ctTotal"}, };

    return hash;
}

QVariant GcodeExtruderTableModel::headerData(int section, Qt::Orientation orientation, int role) const 
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        return QString("Column %1").arg(section);
    }
    else {
        return QString("Row %1").arg(section);
    }
}
}; // namespace cxgcode
