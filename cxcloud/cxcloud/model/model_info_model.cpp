#include "cxcloud/model/model_info_model.h"

#include "cxcloud/tool/function.h"

namespace cxcloud {

  auto ModelGroupCategoryListModel::get(int index) const -> QVariantMap {
    const auto& datas = rawData();
    if (datas.size() <= static_cast<size_t>(index)) {
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
    const auto data_index = static_cast<size_t>(index.row());
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
    const auto data_index = static_cast<size_t>(index.row());
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
      case DataRole::FORMAT: {
        return QVariant::fromValue(data.format);
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
      case DataRole::BROKEN: {
        return QVariant::fromValue(data.broken);
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
      { static_cast<int>(DataRole::FORMAT        ), QByteArrayLiteral("model_format"        ) },
      { static_cast<int>(DataRole::IMAGE         ), QByteArrayLiteral("model_image"         ) },
      { static_cast<int>(DataRole::SIZE          ), QByteArrayLiteral("model_size"          ) },
      { static_cast<int>(DataRole::SIZE_WITH_UNIT), QByteArrayLiteral("model_size_with_unit") },
      { static_cast<int>(DataRole::BROKEN        ), QByteArrayLiteral("model_broken"        ) },
    };

    return ROLE_NAMES;
  }





  const std::map<ModelGroupInfo::Status, QString> ModelGroupInfo::STATUS_STRING_MAP{
    { ModelGroupInfo::Status::REVIEW_UNFINISHED, "review_unfinished" },
    { ModelGroupInfo::Status::REVIEW_PASSED    , "review_passed"     },
    { ModelGroupInfo::Status::REVIEW_FAILED    , "review_failed"     },
    { ModelGroupInfo::Status::REMOVED          , "removed"           },
    { ModelGroupInfo::Status::DELETED          , "deleted"           },
  };

  const std::map<QString, ModelGroupInfo::Status> ModelGroupInfo::STRING_STATUS_MAP{
    { "review_unfinished", ModelGroupInfo::Status::REVIEW_UNFINISHED },
    { "review_passed"    , ModelGroupInfo::Status::REVIEW_PASSED     },
    { "review_failed"    , ModelGroupInfo::Status::REVIEW_FAILED     },
    { "removed"          , ModelGroupInfo::Status::REMOVED           },
    { "deleted"          , ModelGroupInfo::Status::DELETED           },
  };

  auto ModelGroupInfoListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = static_cast<size_t>(index.row());
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
      case DataRole::STATUS: {
        return QVariant::fromValue(ModelGroupInfo::STATUS_STRING_MAP.at(data.status));
      }
      case DataRole::COLLECTED: {
        return QVariant::fromValue(data.collected);
      }
      case DataRole::LIKED: {
        return QVariant::fromValue(data.liked);
      }
      case DataRole::PURCHASED: {
        return QVariant::fromValue(data.purchased);
      }
      case DataRole::RESTRICTED: {
        return QVariant::fromValue(data.restricted);
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
      case DataRole::MODEL_BROKEN_COUNT: {
        return QVariant::fromValue(static_cast<int>(data.model_broken_count));
      }
      case DataRole::MODELS: {
        return QVariant::fromValue(data.models.get());
      }
      case DataRole::PROJECT_COUNT: {
        return QVariant::fromValue(static_cast<int>(data.project_count));
      }
      case DataRole::PROJECTS: {
        return QVariant::fromValue(data.projects.get());
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
      { static_cast<int>(DataRole::STATUS       ), QByteArrayLiteral("model_status"       ) },
      { static_cast<int>(DataRole::COLLECTED    ), QByteArrayLiteral("model_collected"    ) },
      { static_cast<int>(DataRole::LIKED        ), QByteArrayLiteral("model_liked"        ) },
      { static_cast<int>(DataRole::PURCHASED    ), QByteArrayLiteral("model_purchased"    ) },
      { static_cast<int>(DataRole::RESTRICTED   ), QByteArrayLiteral("model_restricted"   ) },
      { static_cast<int>(DataRole::AUTHOR_NAME  ), QByteArrayLiteral("model_author_name"  ) },
      { static_cast<int>(DataRole::AUTHOR_IMAGE ), QByteArrayLiteral("model_author_image" ) },
      { static_cast<int>(DataRole::MODEL_COUNT  ), QByteArrayLiteral("model_model_count"  ) },
      { static_cast<int>(DataRole::MODEL_BROKEN_COUNT),
        QByteArrayLiteral("model_model_broken_count") },
      { static_cast<int>(DataRole::MODELS       ), QByteArrayLiteral("model_models"       ) },
      { static_cast<int>(DataRole::PROJECT_COUNT), QByteArrayLiteral("model_project_count") },
      { static_cast<int>(DataRole::PROJECTS     ), QByteArrayLiteral("model_projects"     ) },
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
      uidChanged();
    }
  }

  auto ModelGroupInfoModel::getName() const -> QString {
    return info_.name;
  }

  auto ModelGroupInfoModel::setName(const QString& name) -> void {
    if (info_.name != name) {
      info_.name = name;
      nameChanged();
    }
  }

  auto ModelGroupInfoModel::getImage() const -> QString {
    return info_.image;
  }

  auto ModelGroupInfoModel::setImage(const QString& image) -> void {
    if (info_.image != image) {
      info_.image = image;
      imageChanged();
    }
  }

  auto ModelGroupInfoModel::getLicense() const -> QString {
    return info_.license;
  }

  auto ModelGroupInfoModel::setLicense(const QString& license) -> void {
    if (info_.license != license) {
      info_.license = license;
      licenseChanged();
    }
  }

  auto ModelGroupInfoModel::getPrice() const -> int {
    return info_.price;
  }

  auto ModelGroupInfoModel::setPrice(int price) -> void {
    if (info_.price != static_cast<size_t>(price)) {
      info_.price = price;
      priceChanged();
    }
  }

  auto ModelGroupInfoModel::getCreationTime() const -> int {
    return info_.creation_time;
  }

  auto ModelGroupInfoModel::setCreationTime(int timestamp) -> void {
    if (info_.creation_time != static_cast<size_t>(timestamp)) {
      info_.creation_time = timestamp;
      creationTimeChanged();
    }
  }

  auto ModelGroupInfoModel::getStatus() const -> ModelGroupInfo::Status {
    return info_.status;
  }

  auto ModelGroupInfoModel::setStatus(ModelGroupInfo::Status status) -> void {
    if (info_.status != status) {
      info_.status = status;
      statusChanged();
    }
  }

  auto ModelGroupInfoModel::getStatusString() const -> QString {
    return ModelGroupInfo::STATUS_STRING_MAP.at(getStatus());
  }

  auto ModelGroupInfoModel::isCollected() const -> bool {
    return info_.collected;
  }

  auto ModelGroupInfoModel::setCollected(bool collected) -> void {
    if (info_.collected != collected) {
      info_.collected = collected;
      collectedChanged();
    }
  }

  auto ModelGroupInfoModel::isLiked() const -> bool {
    return info_.liked;
  }

  auto ModelGroupInfoModel::setLiked(bool liked) -> void {
    if (info_.liked != liked) {
      info_.liked = liked;
      likedChanged();
    }
  }

  auto ModelGroupInfoModel::isPurchased() const -> bool {
    return info_.purchased;
  }

  auto ModelGroupInfoModel::setPurchased(bool purchased) -> void {
    if (info_.purchased != purchased) {
      info_.purchased = purchased;
      purchasedChanged();
    }
  }

  auto ModelGroupInfoModel::isRestricted() const -> bool {
    return info_.restricted;
  }

  auto ModelGroupInfoModel::setRestricted(bool restricted) -> void {
    if (info_.restricted != restricted) {
      info_.restricted = restricted;
      restrictedChanged();
    }
  }

  auto ModelGroupInfoModel::getAuthorName() const -> QString {
    return info_.author_name;
  }

  auto ModelGroupInfoModel::setAuthorName(const QString& name) -> void {
    if (info_.author_name != name) {
      info_.author_name = name;
      authorNameChanged();
    }
  }

  auto ModelGroupInfoModel::getAuthorImage() const -> QString {
    return info_.author_image;
  }

  auto ModelGroupInfoModel::setAuthorImage(const QString& image) -> void {
    if (info_.author_image != image) {
      info_.author_image = image;
      authorImageChanged();
    }
  }

  auto ModelGroupInfoModel::getModelCount() const -> int {
    return info_.model_count;
  }

  auto ModelGroupInfoModel::setModelCount(int count) -> void {
    if (info_.model_count != static_cast<size_t>(count)) {
      info_.model_count = count;
      modelCountChanged();
    }
  }

  auto ModelGroupInfoModel::getModelBrokenCount() const -> int {
    return info_.model_broken_count;
  }

  auto ModelGroupInfoModel::setModelBrokenCount(int count) -> void {
    if (info_.model_broken_count != static_cast<size_t>(count)) {
      info_.model_broken_count = count;
      modelBrokenCountChanged();
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

  auto ModelGroupInfoModel::getProjectCount() const -> int {
    return info_.project_count;
  }

  auto ModelGroupInfoModel::setProjectCount(int count) -> void {
    if (info_.project_count != static_cast<size_t>(count)) {
      info_.project_count = count;
      projectCountChanged();
    }
  }

  auto ModelGroupInfoModel::projects() const -> std::weak_ptr<ProjectListModel> {
    return loadOrMakeProjects();
  }

  auto ModelGroupInfoModel::getProjects() const -> QAbstractListModel* {
    return loadOrMakeProjects().get();
  }

  auto ModelGroupInfoModel::loadOrMakeProjects() const -> std::shared_ptr<ProjectListModel> {
    if (!info_.projects) {
      info_.projects = std::make_shared<ProjectListModel>();
    }

    return info_.projects;
  }

}  // namespace cxcloud
