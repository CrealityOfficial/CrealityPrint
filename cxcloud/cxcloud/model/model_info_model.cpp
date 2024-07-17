#include "cxcloud/model/model_info_model.h"

#include "cxcloud/tool/function.h"

namespace cxcloud {

  auto ModelGroupCategoryListModel::get(int index) const -> QVariantMap {
    const auto& datas = rawData();
    if (datas.size() <= index) {
      return {};
    }

    const auto& data = datas[index];

    return {
      { QByteArrayLiteral("model_uid" ), data.uid },
      { QByteArrayLiteral("model_name"), data.name },
    };
  }

  auto ModelGroupCategoryListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto ModelGroupCategoryListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID ), QByteArrayLiteral("model_uid" ) },
      { static_cast<int>(DataRole::NAME), QByteArrayLiteral("model_name") },
    };

    return ROLE_NAMES;
  }





  auto ModelInfoListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }
      case DataRole::IMAGE: {
        return QVariant::fromValue(data.image);
      }
      case DataRole::SIZE: {
        return QVariant::fromValue(static_cast<int>(data.size));
      }
      case DataRole::SIZE_WITH_UNIT: {
        auto static const FORMAT = QStringLiteral("%1 %2");
        auto static const n2s = [](auto number) { return QString::number(number, 10, 2); };

        const auto size_b = static_cast<double>(data.size);
        auto value = size_b < 1_kb ? FORMAT.arg(n2s(size_b       )).arg(QStringLiteral("B" ))
                   : size_b < 1_mb ? FORMAT.arg(n2s(size_b / 1_kb)).arg(QStringLiteral("KB"))
                   : size_b < 1_gb ? FORMAT.arg(n2s(size_b / 1_mb)).arg(QStringLiteral("MB"))
                                   : FORMAT.arg(n2s(size_b / 1_gb)).arg(QStringLiteral("GB"));

        return QVariant::fromValue(std::move(value));
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto ModelInfoListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID           ), QByteArrayLiteral("model_uid"           ) },
      { static_cast<int>(DataRole::NAME          ), QByteArrayLiteral("model_name"          ) },
      { static_cast<int>(DataRole::IMAGE         ), QByteArrayLiteral("model_image"         ) },
      { static_cast<int>(DataRole::SIZE          ), QByteArrayLiteral("model_size"          ) },
      { static_cast<int>(DataRole::SIZE_WITH_UNIT), QByteArrayLiteral("model_size_with_unit") },
    };

    return ROLE_NAMES;
  }





  auto ModelGroupInfoListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }
      case DataRole::IMAGE: {
        return QVariant::fromValue(data.image);
      }
      case DataRole::LICENSE: {
        return QVariant::fromValue(data.license);
      }
      case DataRole::PRICE: {
        return QVariant::fromValue(static_cast<int>(data.price));
      }
      case DataRole::CREATION_TIME: {
        return QVariant::fromValue(static_cast<int>(data.creation_time));
      }
      case DataRole::COLLECTED: {
        return QVariant::fromValue(data.collected);
      }
      case DataRole::LIKED: {
        return QVariant::fromValue(data.liked);
      }
      case DataRole::AUTHOR_NAME: {
        return QVariant::fromValue(data.author_name);
      }
      case DataRole::AUTHOR_IMAGE: {
        return QVariant::fromValue(data.author_image);
      }
      case DataRole::MODEL_COUNT: {
        return QVariant::fromValue(static_cast<int>(data.model_count));
      }
      case DataRole::MODELS: {
        return QVariant::fromValue(data.models.get());
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto ModelGroupInfoListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID          ), QByteArrayLiteral("model_uid"          ) },
      { static_cast<int>(DataRole::NAME         ), QByteArrayLiteral("model_name"         ) },
      { static_cast<int>(DataRole::IMAGE        ), QByteArrayLiteral("model_image"        ) },
      { static_cast<int>(DataRole::LICENSE      ), QByteArrayLiteral("model_license"      ) },
      { static_cast<int>(DataRole::PRICE        ), QByteArrayLiteral("model_price"        ) },
      { static_cast<int>(DataRole::CREATION_TIME), QByteArrayLiteral("model_creation_time") },
      { static_cast<int>(DataRole::COLLECTED    ), QByteArrayLiteral("model_collected"    ) },
      { static_cast<int>(DataRole::AUTHOR_NAME  ), QByteArrayLiteral("model_author_name"  ) },
      { static_cast<int>(DataRole::AUTHOR_IMAGE ), QByteArrayLiteral("model_author_image" ) },
      { static_cast<int>(DataRole::MODEL_COUNT  ), QByteArrayLiteral("model_model_count"  ) },
      { static_cast<int>(DataRole::MODELS       ), QByteArrayLiteral("model_models"       ) },
    };

    return ROLE_NAMES;
  }





  ModelGroupInfoModel::ModelGroupInfoModel(const ModelGroupInfo& info, QObject* parent)
      : QObject{ parent }, info_{ info } {}

  ModelGroupInfoModel::ModelGroupInfoModel(ModelGroupInfo&& info, QObject* parent)
      : QObject{ parent }, info_{ std::move(info) } {}

  ModelGroupInfoModel::ModelGroupInfoModel(QObject* parent)
      : QObject{ parent } {}

  auto ModelGroupInfoModel::getUid() const -> QString {
    return info_.uid;
  }

  auto ModelGroupInfoModel::setUid(const QString& uid) -> void {
    if (info_.uid != uid) {
      info_.uid = uid;
      Q_EMIT uidChanged();
    }
  }

  auto ModelGroupInfoModel::getName() const -> QString {
    return info_.name;
  }

  auto ModelGroupInfoModel::setName(const QString& name) -> void {
    if (info_.name != name) {
      info_.name = name;
      Q_EMIT nameChanged();
    }
  }

  auto ModelGroupInfoModel::getImage() const -> QString {
    return info_.image;
  }

  auto ModelGroupInfoModel::setImage(const QString& image) -> void {
    if (info_.image != image) {
      info_.image = image;
      Q_EMIT imageChanged();
    }
  }

  auto ModelGroupInfoModel::getLicense() const -> QString {
    return info_.license;
  }

  auto ModelGroupInfoModel::setLicense(const QString& license) -> void {
    if (info_.license != license) {
      info_.license = license;
      Q_EMIT licenseChanged();
    }
  }

  auto ModelGroupInfoModel::getPrice() const -> int {
    return info_.price;
  }

  auto ModelGroupInfoModel::setPrice(int price) -> void {
    if (info_.price != price) {
      info_.price = price;
      Q_EMIT priceChanged();
    }
  }

  auto ModelGroupInfoModel::getCreationTime() const -> int {
    return info_.creation_time;
  }

  auto ModelGroupInfoModel::setCreationTime(int timestamp) -> void {
    if (info_.creation_time != timestamp) {
      info_.creation_time = timestamp;
      Q_EMIT creationTimeChanged();
    }
  }

  auto ModelGroupInfoModel::isCollected() const -> bool {
    return info_.collected;
  }

  auto ModelGroupInfoModel::setCollected(bool collected) -> void {
    if (info_.collected != collected) {
      info_.collected = collected;
      Q_EMIT collectedChanged();
    }
  }

  auto ModelGroupInfoModel::isLiked() const -> bool {
    return info_.liked;
  }

  auto ModelGroupInfoModel::setLiked(bool liked) -> void {
    if (info_.liked != liked) {
      info_.liked = liked;
      Q_EMIT likedChanged();
    }
  }

  auto ModelGroupInfoModel::getAuthorName() const -> QString {
    return info_.author_name;
  }

  auto ModelGroupInfoModel::setAuthorName(const QString& name) -> void {
    if (info_.author_name != name) {
      info_.author_name = name;
      Q_EMIT authorNameChanged();
    }
  }

  auto ModelGroupInfoModel::getAuthorImage() const -> QString {
    return info_.author_image;
  }

  auto ModelGroupInfoModel::setAuthorImage(const QString& image) -> void {
    if (info_.author_image != image) {
      info_.author_image = image;
      Q_EMIT authorImageChanged();
    }
  }

  auto ModelGroupInfoModel::getModelCount() const -> int {
    return info_.model_count;
  }

  auto ModelGroupInfoModel::setModelCount(int count) -> void {
    if (info_.model_count != count) {
      info_.model_count = count;
      Q_EMIT modelCountChanged();
    }
  }

  auto ModelGroupInfoModel::models() const -> std::weak_ptr<ModelInfoListModel> {
    return loadOrMakeModels();
  }

  auto ModelGroupInfoModel::getModels() const -> QAbstractListModel* {
    return loadOrMakeModels().get();
  }

  auto ModelGroupInfoModel::loadOrMakeModels() const -> std::shared_ptr<ModelInfoListModel> {
    if (!info_.models) {
      info_.models = std::make_shared<ModelInfoListModel>();
    }

    return info_.models;
  }

}  // namespace cxcloud
