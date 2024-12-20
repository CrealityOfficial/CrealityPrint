#include "internal/models/printmateriallistmodel.h"

#include "internal/parameter/printmachine.h"
#include "internal/models/parameterdatamodel.h"

namespace creative_kernel {

  PrintMaterialListModel::PrintMaterialListModel(PrintMachine* machine, QObject* parent)
      : QAbstractListModel{ parent }, machine_{ machine } {
    for (auto index = 0; index < machine_->materialCount(); ++index) {
      auto* const material = machine_->material(index);
      connect(material->getDataModel(), &ParameterDataModel::settingsModifyedChanged,
              this, &PrintMaterialListModel::onSettingsModifyedChanged,
              Qt::ConnectionType::UniqueConnection);
    }
  }

  auto PrintMaterialListModel::get(int row) const -> QVariantMap {
    QVariantMap name_value_map = {};

    const auto index = createIndex(std::max(0, std::min(row, rowCount())), 0);
    const auto role_name_map = roleNames();
    for (auto iter = role_name_map.cbegin(); iter != role_name_map.cend(); ++iter) {
      const auto& role = iter.key();
      const auto& name = iter.value();
      name_value_map.insert(name, data(index, role));
    }

    return name_value_map;
  }

  auto PrintMaterialListModel::insertMaterial(PrintMaterial* material, size_t index) -> void {
    beginInsertRows({}, index, index);
    connect(material->getDataModel(), &ParameterDataModel::settingsModifyedChanged,
            this, &PrintMaterialListModel::onSettingsModifyedChanged,
            Qt::ConnectionType::UniqueConnection);
    endInsertRows();
  }

  auto PrintMaterialListModel::removeMaterial(PrintMaterial* material, size_t index) -> void {
    beginRemoveRows({}, index, index);
    disconnect(material->getDataModel(), &ParameterDataModel::settingsModifyedChanged,
               this, &PrintMaterialListModel::onSettingsModifyedChanged);
    endRemoveRows();
  }

  void PrintMaterialListModel::refresh()
  {
      beginResetModel();
      endResetModel();
  }

  auto PrintMaterialListModel::roleNames() const -> QHash<int, QByteArray> {
    static const decltype(roleNames()) ROLE_NAME_MAP{
      { static_cast<int>(Role::NAME), QByteArrayLiteral("name") },
      { static_cast<int>(Role::DEFAULT), QByteArrayLiteral("default") },
      { static_cast<int>(Role::DEFAULTORSYNC), QByteArrayLiteral("defaultOrSync") },
      { static_cast<int>(Role::COLOR), QByteArrayLiteral("materialColor") },
      { static_cast<int>(Role::COLORINDEX), QByteArrayLiteral("materialColorIndex") },
      { static_cast<int>(Role::SYNCPBOXINDEX), QByteArrayLiteral("syncBoxIndex") },
      { static_cast<int>(Role::MODIFYED), QByteArrayLiteral("modifyed") },
      { static_cast<int>(Role::OBJECT), QByteArrayLiteral("object") },
    };

    return ROLE_NAME_MAP;
  }

  auto PrintMaterialListModel::rowCount(const QModelIndex& parent) const -> int {
     if (!machine_)
         return 0;

     if (machine_->isSyncedExtruder())
     {
         QVector<PrintMachine::SyncExtruderInfo>& infos = machine_->syncedExtrudersInfo();
         return machine_->materialCount() + infos.count();
     }
     else 
     {
         return machine_->materialCount();
     }
  } 

  auto PrintMaterialListModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (!machine_ || index.row() < 0 || index.row() >= rowCount()) {
      return { QVariant::Type::Invalid };
    }

    int row = index.row();   

    QVariant name;
    QVariant isModify;
    QVariant isDefault;
    QVariant defaultOrSync;
    QVariant color;
    QVariant colorIndex ;
    QVariant obj;
    QVariant reserve;

    generateData(machine_->isSyncedExtruder(), row, name, isModify, isDefault, defaultOrSync, color, colorIndex, obj, reserve);
    switch (static_cast<Role>(role)) {
    case Role::NAME: {
        return name;
    }
    case Role::DEFAULT: {
        return isDefault;
    }
    case Role::DEFAULTORSYNC: {
        return defaultOrSync;
    }
    case Role::MODIFYED: {
        return isModify;
    }
    case Role::COLOR: {
        return color;
    }
    case Role::COLORINDEX: {
        return colorIndex;
    }  
    case Role::SYNCPBOXINDEX: {
        return reserve;
    }
    case Role::OBJECT: {
        return obj;
    }
    default: {
        break;
    }
    }

    return { QVariant::Type::Invalid };
  }

  void PrintMaterialListModel::generateData(bool isSynced, int row, QVariant& name, QVariant& isModify, QVariant& isDefault,
      QVariant& defaultOrSync, QVariant& color, QVariant& colorIndex, QVariant& obj, QVariant& reserve) const
  {
      QVector<PrintMachine::SyncExtruderInfo>& infos = machine_->syncedExtrudersInfo();
      if (isSynced && row < infos.count())
      {
            PrintMachine::SyncExtruderInfo curExtruderInfo = infos.at(row);
            QString temp = curExtruderInfo.materialName;
            name = temp.remove("-1.75");
            isDefault = true;
            defaultOrSync = QString("2");
            color = curExtruderInfo.color;
            colorIndex = row + 1;
            reserve = QVariant::fromValue(curExtruderInfo.boxIndex);
            
            auto* const material = machine_->materialWithName(curExtruderInfo.materialName);
            if (!material) {
                return;
            }

            const auto* const model = material->getDataModel();
            isModify = QVariant::fromValue(model && model->hasSettingModifyed());
            obj = QVariant::fromValue(material);
      }
      else {
          row = row - infos.count();
          auto* const material = machine_->material(row);
          if (!material) {
              return;
          }
          name = material->name();
          const auto* const model = material->getDataModel();
          isModify = QVariant::fromValue(model && model->hasSettingModifyed());
          isDefault = !material->isUserDef();
          defaultOrSync = QString(isDefault.toBool() ? "0" : "1");
          color = QString();
          colorIndex = -1;
          obj = QVariant::fromValue(material);
      }
  }

  auto PrintMaterialListModel::onSettingsModifyedChanged() -> void {
    dataChanged(createIndex(0, 0, nullptr),
                createIndex(rowCount(), 0, nullptr),
                { static_cast<int>(Role::MODIFYED) });
  }

}  // namespace creative_kernel
