#include "cxcloud/model/model_info_model.h"

#include "cxcloud/tool/function.h"

namespace cxcloud {

QVariantMap ModelGroupCategoryListModel::get(int index) const {
  if (rawData().size() <= index) {
    return {};
  }

  const auto& info = rawData().at(index);
  return {
    { QByteArrayLiteral("model_uid" ), info.uid },
    { QByteArrayLiteral("model_name"), info.name },
  };
}

QVariant ModelGroupCategoryListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  QVariant result;
  const auto& data = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(data.uid);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(data.name);
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> ModelGroupCategoryListModel::roleNames() const {
  static const QHash<int, QByteArray> ROLE_NAMES{
    { static_cast<int>(DataRole::UID ), QByteArrayLiteral("model_uid" ) },
    { static_cast<int>(DataRole::NAME), QByteArrayLiteral("model_name") },
  };


  return ROLE_NAMES;
}

ModelInfoListModel::ModelInfoListModel(QObject* parent)
    : DataListModel<ModelInfo>(parent) {}

QVariant ModelInfoListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  QVariant result;
  const auto& data = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(data.uid);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(data.name);
      break;
    case DataRole::IMAGE:
      result = QVariant::fromValue(data.image);
      break;
    case DataRole::SIZE:
      result = QVariant::fromValue(static_cast<int>(data.size));
      break;
    case DataRole::SIZE_WITH_UNIT: {
      auto static const FORMAT = QStringLiteral("%1 %2");
      auto static const n2s = [](auto number) { return QString::number(number, 10, 2); };

      auto const size_b = static_cast<double>(data.size);
      auto value = size_b < 1_kb ? FORMAT.arg(n2s(size_b       )).arg(QStringLiteral("B" ))
                 : size_b < 1_mb ? FORMAT.arg(n2s(size_b / 1_kb)).arg(QStringLiteral("KB"))
                 : size_b < 1_gb ? FORMAT.arg(n2s(size_b / 1_mb)).arg(QStringLiteral("MB"))
                                 : FORMAT.arg(n2s(size_b / 1_gb)).arg(QStringLiteral("GB"));

      result = QVariant::fromValue(std::move(value));
      break;
    }
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> ModelInfoListModel::roleNames() const {
  static const QHash<int, QByteArray> ROLE_NAMES{
    { static_cast<int>(DataRole::UID           ), QByteArrayLiteral("model_uid"           ) },
    { static_cast<int>(DataRole::NAME          ), QByteArrayLiteral("model_name"          ) },
    { static_cast<int>(DataRole::IMAGE         ), QByteArrayLiteral("model_image"         ) },
    { static_cast<int>(DataRole::SIZE          ), QByteArrayLiteral("model_size"          ) },
    { static_cast<int>(DataRole::SIZE_WITH_UNIT), QByteArrayLiteral("model_size_with_unit") },
  };

  return ROLE_NAMES;
}

QVariant ModelGroupInfoListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  QVariant result;
  const auto& data = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(data.uid);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(data.name);
      break;
    case DataRole::IMAGE:
      result = QVariant::fromValue(data.image);
      break;
    case DataRole::LICENSE:
      result = QVariant::fromValue(data.license);
      break;
    case DataRole::PRICE:
      result = QVariant::fromValue(static_cast<int>(data.price));
      break;
    case DataRole::CREATION_TIME:
      result = QVariant::fromValue(static_cast<int>(data.creation_time));
      break;
    case DataRole::COLLECTED:
      result = QVariant::fromValue(data.collected);
      break;
    case DataRole::LIKED:
        result = QVariant::fromValue(data.liked);
        break;
    case DataRole::AUTHOR_NAME:
      result = QVariant::fromValue(data.author_name);
      break;
    case DataRole::AUTHOR_IMAGE:
      result = QVariant::fromValue(data.author_image);
      break;
    case DataRole::MODEL_COUNT:
      result = QVariant::fromValue(static_cast<int>(data.model_count));
      break;
    case DataRole::MODELS:
      result = QVariant::fromValue(data.models.get());
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> ModelGroupInfoListModel::roleNames() const {
  static const QHash<int, QByteArray> ROLE_NAMES{
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
    : QObject(parent), info_(info) {}

ModelGroupInfoModel::ModelGroupInfoModel(ModelGroupInfo&& info, QObject* parent)
    : QObject(parent), info_(std::move(info)) {}

ModelGroupInfoModel::ModelGroupInfoModel(QObject* parent)
    : QObject(parent), info_({}) {}

QString ModelGroupInfoModel::getUid() const {
  return info_.uid;
}

void ModelGroupInfoModel::setUid(const QString& uid) {
  info_.uid = uid;
  Q_EMIT uidChanged();
}

QString ModelGroupInfoModel::getName() const {
  return info_.name;
}

void ModelGroupInfoModel::setName(const QString& name) {
  info_.name = name;
  Q_EMIT nameChanged();
}

QString ModelGroupInfoModel::getImage() const {
  return info_.image;
}

void ModelGroupInfoModel::setImage(const QString& image) {
  info_.image = image;
  Q_EMIT imageChanged();
}

QString ModelGroupInfoModel::getLicense() const {
  return info_.license;
}

void ModelGroupInfoModel::setLicense(const QString& license) {
  info_.license = license;
  Q_EMIT licenseChanged();
}

int ModelGroupInfoModel::getPrice() const {
  return info_.price;
}

void ModelGroupInfoModel::setPrice(int price) {
  info_.price = price;
  Q_EMIT priceChanged();
}

int ModelGroupInfoModel::getCreationTime() const {
  return info_.creation_time;
}

void ModelGroupInfoModel::setCreationTime(int timestamp) {
  info_.creation_time = timestamp;
  Q_EMIT creationTimeChanged();
}

bool ModelGroupInfoModel::isCollected() const {
  return info_.collected;
}

void ModelGroupInfoModel::setCollected(bool collected) {
  info_.collected = collected;
  Q_EMIT collectedChanged();
}

bool ModelGroupInfoModel::isLiked() const {
    return info_.liked;
}

void ModelGroupInfoModel::setLiked(bool liked) {
    info_.liked = liked;
    Q_EMIT likedChanged();
}

QString ModelGroupInfoModel::getAuthorName() const {
  return info_.author_name;
}

void ModelGroupInfoModel::setAuthorName(const QString& name) {
  info_.author_name = name;
  Q_EMIT authorNameChanged();
}

QString ModelGroupInfoModel::getAuthorImage() const {
  return info_.author_image;
}

void ModelGroupInfoModel::setAuthorImage(const QString& image) {
  info_.author_image = image;
  Q_EMIT authorImageChanged();
}

int ModelGroupInfoModel::getModelCount() const {
  return info_.model_count;
}

void ModelGroupInfoModel::setModelCount(int count) {
  info_.model_count = count;
  Q_EMIT modelCountChanged();
}

QAbstractListModel* ModelGroupInfoModel::getModels() const {
  return loadOrMakeModels().get();
}

std::weak_ptr<ModelInfoListModel> ModelGroupInfoModel::models() const {
  return loadOrMakeModels();
}

std::shared_ptr<ModelInfoListModel> ModelGroupInfoModel::loadOrMakeModels() const {
  if (!info_.models) {
    info_.models = std::make_shared<ModelInfoListModel>();
  }

  return info_.models;
}

}  // namespace cxcloud
