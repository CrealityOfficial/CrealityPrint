#include "cxcloud/model/printer_model.h"

namespace cxcloud {

  auto PrinterListModel::get(int index) const -> QVariantMap {
    const auto& datas = rawData();
    if (datas.size() <= index) {
      return {};
    }

    const auto& data = datas[index];

    return {
      { QByteArrayLiteral("model_uid"           ), QVariant::fromValue(data.uid           ) },
      { QByteArrayLiteral("model_ip"            ), QVariant::fromValue(data.ip            ) },
      { QByteArrayLiteral("model_image"         ), QVariant::fromValue(data.image         ) },
      { QByteArrayLiteral("model_nickname"      ), QVariant::fromValue(data.nickname      ) },
      { QByteArrayLiteral("model_device_id"     ), QVariant::fromValue(data.device_id     ) },
      { QByteArrayLiteral("model_device_name"   ), QVariant::fromValue(data.device_name   ) },
      { QByteArrayLiteral("model_online"        ), QVariant::fromValue(data.online        ) },
      { QByteArrayLiteral("model_connected"     ), QVariant::fromValue(data.connected     ) },
      { QByteArrayLiteral("model_tf_card_exisit"), QVariant::fromValue(data.tf_card_exisit) },
    };
  }

  auto PrinterListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::IP: {
        return QVariant::fromValue(data.ip);
      }
      case DataRole::IMAGE: {
        return QVariant::fromValue(data.image);
      }
      case DataRole::NICKNAME: {
        return QVariant::fromValue(data.nickname);
      }
      case DataRole::DEVICE_ID: {
        return QVariant::fromValue(data.device_id);
      }
      case DataRole::DEVICE_NAME: {
        return QVariant::fromValue(data.device_name);
      }
      case DataRole::ONLINE: {
        return QVariant::fromValue(data.online);
      }
      case DataRole::CONNECTED: {
        return QVariant::fromValue(data.connected);
      }
      case DataRole::TF_CARD_EXISIT: {
        return QVariant::fromValue(data.tf_card_exisit);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto PrinterListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID           ), QByteArrayLiteral("model_uid"           ) },
      { static_cast<int>(DataRole::IP            ), QByteArrayLiteral("model_ip"            ) },
      { static_cast<int>(DataRole::IMAGE         ), QByteArrayLiteral("model_image"         ) },
      { static_cast<int>(DataRole::NICKNAME      ), QByteArrayLiteral("model_nickname"      ) },
      { static_cast<int>(DataRole::DEVICE_ID     ), QByteArrayLiteral("model_device_id"     ) },
      { static_cast<int>(DataRole::DEVICE_NAME   ), QByteArrayLiteral("model_device_name"   ) },
      { static_cast<int>(DataRole::ONLINE        ), QByteArrayLiteral("model_online"        ) },
      { static_cast<int>(DataRole::CONNECTED     ), QByteArrayLiteral("model_connected"     ) },
      { static_cast<int>(DataRole::TF_CARD_EXISIT), QByteArrayLiteral("model_tf_card_exisit") },
    };

    return ROLE_NAMES;
  }

}  // namespace cxcloud
