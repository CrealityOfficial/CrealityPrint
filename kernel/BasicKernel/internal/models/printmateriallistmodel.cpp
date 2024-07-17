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

  auto PrintMaterialListModel::roleNames() const -> QHash<int, QByteArray> {
    static const decltype(roleNames()) ROLE_NAME_MAP{
      { static_cast<int>(Role::NAME), QByteArrayLiteral("name") },
      { static_cast<int>(Role::DEFAULT), QByteArrayLiteral("default") },
      { static_cast<int>(Role::MODIFYED), QByteArrayLiteral("modifyed") },
      { static_cast<int>(Role::OBJECT), QByteArrayLiteral("object") },
    };

    return ROLE_NAME_MAP;
  }

  auto PrintMaterialListModel::rowCount(const QModelIndex& parent) const -> int {
    return machine_ ? machine_->materialCount() : 0;
  }

  auto PrintMaterialListModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (!machine_ || index.row() < 0 || index.row() >= rowCount()) {
      return { QVariant::Type::Invalid };
    }

    auto* const material = machine_->material(index.row());
    if (!material) {
      return { QVariant::Type::Invalid };
    }

    switch (static_cast<Role>(role)) {
      case Role::NAME: {
        return QVariant::fromValue(material->name());
      }
      case Role::DEFAULT: {
        return QVariant::fromValue(!material->isUserDef());
      }
      case Role::MODIFYED: {
        const auto* const model = material->getDataModel();
        return QVariant::fromValue(model && model->hasSettingModifyed());
      }
      case Role::OBJECT: {
        return QVariant::fromValue<QObject*>(material);
      }
      default: {
        break;
      }
    }

    return { QVariant::Type::Invalid };
  }

  auto PrintMaterialListModel::onSettingsModifyedChanged() -> void {
    dataChanged(createIndex(0, 0, nullptr),
                createIndex(rowCount(), 0, nullptr),
                { static_cast<int>(Role::MODIFYED) });
  }

}  // namespace creative_kernel
