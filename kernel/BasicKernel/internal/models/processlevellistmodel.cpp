#include "internal/models/processlevellistmodel.h"

#include <qtusercore/util/settings.h>

namespace creative_kernel {

  namespace {

    enum class ListOrder : unsigned int {
      // Custom = 0,
      // Basic,
      // Advanced,
      // Expert,

      Basic = 0,
      Expert,
    };

  }  // namespace

  ProcessLevelListModel::ProcessLevelListModel(QObject* parent)
      : SuperType{ DataContainer{
          // {
          //   QStringLiteral("custom"),
          //   QStringLiteral("Custom"),
          //   99,
          // },
          {
            QStringLiteral("basic"),
            QStringLiteral("Basic"),
            2,
          },
          // {
          //   QStringLiteral("advanced"),
          //   QStringLiteral("Advanced"),
          //   1,
          // },
          {
            QStringLiteral("expert"),
            QStringLiteral("Expert"),
            0,
          },
        }, parent } {}

  auto ProcessLevelListModel::get(int index) const -> QVariantMap {
    const auto& datas = rawData();
    if (datas.size() <= index) {
      return {};
    }

    const auto& data = datas[index];

    return {
      { QByteArrayLiteral("uid" ), data.uid },
      { QByteArrayLiteral("name"), data.name },
      { QByteArrayLiteral("level"), data.level },
    };
  }

  auto ProcessLevelListModel::getCurrentIndex() const -> int {
    qtuser_core::VersionSettings setting;
    setting.beginGroup(QStringLiteral("ui_config"));
    auto succssed = false;
    auto index = setting.value(QStringLiteral("current_process_level_index")).toInt(&succssed);
    return succssed ? index : getDefaultIndex();
  }

  auto ProcessLevelListModel::setCurrentIndex(int index) -> void {
    if (getCurrentIndex() != index) {
      qtuser_core::VersionSettings setting;
      setting.beginGroup(QStringLiteral("ui_config"));
      setting.setValue(QStringLiteral("current_process_level_index"), QVariant::fromValue(index));
      currentIndexChanged();
    }
  }

  auto ProcessLevelListModel::getDefaultIndex() const -> int {
    return static_cast<int>(ListOrder::Basic);
  }

  auto ProcessLevelListModel::getCustomIndex() const -> int {
    // return static_cast<int>(ListOrder::Custom);
    return -1;
  }

  auto ProcessLevelListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    const auto& data = datas[data_index];
    switch (static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }
      case DataRole::LEVEL: {
        return QVariant::fromValue(data.level);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto ProcessLevelListModel::roleNames() const -> QHash<int, QByteArray> {
    static const QHash<int, QByteArray> ROLE_NAMES {
      { static_cast<int>(DataRole::UID), QByteArrayLiteral("uid") },
      { static_cast<int>(DataRole::NAME), QByteArrayLiteral("name") },
      { static_cast<int>(DataRole::LEVEL), QByteArrayLiteral("level") },
    };

    return ROLE_NAMES;
  }

}  // namespace creative_kernel
