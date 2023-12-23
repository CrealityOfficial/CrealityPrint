#include "cxcloud/model/base_model.h"

namespace cxcloud {

QVariantMap ServerListModel::get(int index) const {
  if (rawData().size() <= index) {
    return {};
  }

  const auto& data = rawData().at(index);
  return {
    { QByteArrayLiteral("model_uid"  ), QVariant::fromValue(data.uid        ) },
    { QByteArrayLiteral("model_index"), QVariant::fromValue(data.uid.toInt()) },
    { QByteArrayLiteral("model_name" ), QVariant::fromValue(data.name       ) },
  };
}

QVariant ServerListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  QVariant result;
  const auto& data = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(data.uid);
      break;
    case DataRole::INDEX:
      result = QVariant::fromValue(data.uid.toInt());
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(data.name);
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> ServerListModel::roleNames() const {
  static const QHash<int, QByteArray> ROLE_NAMES{
    { static_cast<int>(DataRole::UID  ), QByteArrayLiteral("model_uid"  ) },
    { static_cast<int>(DataRole::INDEX), QByteArrayLiteral("model_index") },
    { static_cast<int>(DataRole::NAME ), QByteArrayLiteral("model_name" ) },
  };

  return ROLE_NAMES;
}

}  // namespace cxcloud
