#include "internal/models/printmachinelistmodel.h"

#include <set>

#include "internal/models/parameterdatamodel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/parameterbase.h"


namespace creative_kernel {

  namespace {
    auto CreateMachineDataModelSet(PrintMachine* machine) -> std::set<ParameterDataModel*> {
      if (!machine) {
        return {};
      }

      std::set<ParameterDataModel*> set;

      set.emplace(machine->getDataModel());
      for (size_t index = 0; index < machine->extruders().size(); ++index) {
        auto data_model = machine->getExtruderDataModel(index);
        if (data_model) {
          set.emplace(data_model);
        }
      }

      return set;
    }
  }

  PrintMachineListModel::PrintMachineListModel(ParameterManager* manager, QObject* parent)
      : QAbstractListModel{ parent }, manager_{ manager } {
    for (auto index = 0; index < manager_->machinesCount(); ++index) {
      for (auto& data_model : CreateMachineDataModelSet(manager_->machine(index))) {
        connect(data_model, &ParameterDataModel::settingsModifyedChanged,
                this, &PrintMachineListModel::onSettingsModifyedChanged,
                Qt::ConnectionType::UniqueConnection);
      }
    }
  }

  auto PrintMachineListModel::get(int row) const -> QVariantMap {
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

  auto PrintMachineListModel::getCount() const -> int {
    return rowCount({});
  }

  auto PrintMachineListModel::insertMachine(PrintMachine* machine, size_t index) -> void {
    beginInsertRows({}, index, index);
    for (auto& data_model : CreateMachineDataModelSet(machine)) {
      connect(data_model, &ParameterDataModel::settingsModifyedChanged,
              this, &PrintMachineListModel::onSettingsModifyedChanged,
              Qt::ConnectionType::UniqueConnection);
    }
    endInsertRows();
    countChanged();
  }

  auto PrintMachineListModel::removeMachine(PrintMachine* machine, size_t index) -> void {
    beginRemoveRows({}, index, index);
    for (auto& data_model : CreateMachineDataModelSet(machine)) {
      disconnect(data_model, &ParameterDataModel::settingsModifyedChanged,
                  this, &PrintMachineListModel::onSettingsModifyedChanged);
    }
    endRemoveRows();
    countChanged();
  }

  auto PrintMachineListModel::roleNames() const -> QHash<int, QByteArray> {
    static const decltype(roleNames()) ROLE_NAME_MAP{
      { static_cast<int>(Role::NAME), QByteArrayLiteral("name") },
      { static_cast<int>(Role::BASENAME), QByteArrayLiteral("baseName") },
      { static_cast<int>(Role::DEFAULT), QByteArrayLiteral("default") },
      { static_cast<int>(Role::MODIFYED), QByteArrayLiteral("modifyed") },
      { static_cast<int>(Role::OBJECT), QByteArrayLiteral("object") },
    };

    return ROLE_NAME_MAP;
  }

  auto PrintMachineListModel::rowCount(const QModelIndex& parent) const -> int {
    return manager_ ? manager_->machinesCount() : 0;
  }

  auto PrintMachineListModel::data(const QModelIndex& model_index, int role) const -> QVariant {
    if (!manager_ || model_index.row() < 0 || model_index.row() >= rowCount()) {
      return { QVariant::Type::Invalid };
    }

    auto* const machine = manager_->machine(model_index.row());
    if (!machine) {
      return { QVariant::Type::Invalid };
    }

    switch (static_cast<Role>(role)) {
      case Role::NAME: {
        return QVariant::fromValue(machine->name());
      }
      case Role::BASENAME: {
          return QVariant::fromValue(machine->baseName());
      }
      case Role::DEFAULT: {
        return QVariant::fromValue(machine->isDefault());
      }
      case Role::MODIFYED: {
        for (auto& data_model : CreateMachineDataModelSet(machine)) {
          if (data_model->hasSettingModifyed()) {
            return QVariant::fromValue(true);
          }
        }

        return QVariant::fromValue(false);
      }
      case Role::OBJECT: {
        return QVariant::fromValue<QObject*>(machine);
      }
      default: {
        break;
      }
    }

    return { QVariant::Type::Invalid };
  }

  auto PrintMachineListModel::onSettingsModifyedChanged() -> void {
    dataChanged(createIndex(0, 0, nullptr),
                createIndex(rowCount(), 0, nullptr),
                { static_cast<int>(Role::MODIFYED) });
  }

}  // namespace creative_kernel
