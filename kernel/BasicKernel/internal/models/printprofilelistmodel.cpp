#include "internal/models/printprofilelistmodel.h"

#include "internal/parameter/printmachine.h"
#include "internal/models/parameterdatamodel.h"

namespace creative_kernel {

  PrintProfileListModel::PrintProfileListModel(PrintMachine* machine, QObject* parent)
      : QAbstractListModel{ parent }, machine_{ machine } {
    for (auto index = 0; index < machine_->profilesCount(); ++index) {
      auto* const profile = machine_->profile(index);
      connect(profile->getDataModel(), &ParameterDataModel::settingsModifyedChanged,
              this, &PrintProfileListModel::onSettingsModifyedChanged,
              Qt::ConnectionType::UniqueConnection);
    }
  }

  auto PrintProfileListModel::get(int row) const -> QVariantMap {
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

  auto PrintProfileListModel::insertProfile(PrintProfile* profile, size_t index) -> void {
    beginInsertRows({}, index, index);
    connect(profile->getDataModel(), &ParameterDataModel::settingsModifyedChanged,
            this, &PrintProfileListModel::onSettingsModifyedChanged,
            Qt::ConnectionType::UniqueConnection);
    endInsertRows();
    profileInserted(index);
  }

  auto PrintProfileListModel::removeProfile(PrintProfile* profile, size_t index) -> void {
    beginRemoveRows({}, index, index);
    disconnect(profile->getDataModel(), &ParameterDataModel::settingsModifyedChanged,
               this, &PrintProfileListModel::onSettingsModifyedChanged);
    endRemoveRows();
    profileRemoved(index);
  }

  auto PrintProfileListModel::roleNames() const -> QHash<int, QByteArray> {
    static const decltype(roleNames()) ROLE_NAME_MAP{
      { static_cast<int>(Role::NAME), QByteArrayLiteral("name") },
      { static_cast<int>(Role::DEFAULT), QByteArrayLiteral("default") },
      { static_cast<int>(Role::MODIFYED), QByteArrayLiteral("modifyed") },
      { static_cast<int>(Role::OBJECT), QByteArrayLiteral("object") },
    };

    return ROLE_NAME_MAP;
  }

  auto PrintProfileListModel::rowCount(const QModelIndex& parent) const -> int {
    return machine_ ? machine_->profilesCount() : 0;
  }

  auto PrintProfileListModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (!machine_ || index.row() < 0 || index.row() >= rowCount()) {
      return { QVariant::Type::Invalid };
    }

    auto* const profile = machine_->profile(index.row());
    if (!profile) {
      return { QVariant::Type::Invalid };
    }

    switch (static_cast<Role>(role)) {
      case Role::NAME: {
        return QVariant::fromValue(profile->name());
      }
      case Role::DEFAULT: {
        return QVariant::fromValue(profile->isDefault());
      }
      case Role::MODIFYED: {
        const auto* const model = profile->getDataModel();
        return QVariant::fromValue(model && model->hasSettingModifyed());
      }
      case Role::OBJECT: {
        return QVariant::fromValue<QObject*>(profile);
      }
      default: {
        break;
      }
    }

    return { QVariant::Type::Invalid };
  }

  auto PrintProfileListModel::onSettingsModifyedChanged() -> void {
    dataChanged(createIndex(0, 0, nullptr),
                createIndex(rowCount(), 0, nullptr),
                { static_cast<int>(Role::MODIFYED) });
  }

}  // namespace creative_kernel
