#include "cxcloud/model/server_list_model.h"

namespace cxcloud {

  auto ServerListModel::get(int index) const -> QVariantMap {
    const auto& datas = rawData();
    if (datas.size() <= index) {
      return {};
    }

    const auto& data = datas[index];

    return {
      { QByteArrayLiteral("model_uid"  ), QVariant::fromValue(data.uid        ) },
      { QByteArrayLiteral("model_index"), QVariant::fromValue(data.uid.toInt()) },
      { QByteArrayLiteral("model_name" ), QVariant::fromValue(data.name       ) },
    };
  }

  auto ServerListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID:
        return QVariant::fromValue(data.uid);
      case DataRole::INDEX:
        return QVariant::fromValue(data.uid.toInt());
      case DataRole::NAME:
        return QVariant::fromValue(data.name);
      default:
        return { QVariant::Invalid };
    }
  }

  auto ServerListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID  ), QByteArrayLiteral("model_uid"  ) },
      { static_cast<int>(DataRole::INDEX), QByteArrayLiteral("model_index") },
      { static_cast<int>(DataRole::NAME ), QByteArrayLiteral("model_name" ) },
    };

    return ROLE_NAMES;
  }

}  // namespace cxcloud
