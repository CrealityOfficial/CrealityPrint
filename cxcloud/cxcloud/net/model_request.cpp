#include "cxcloud/net/model_request.h"

#include <QDate>
#include <QVariant>

namespace cxcloud {

// ---------- model library service [begin] ----------

LoadModelGroupCategoryListRequest::LoadModelGroupCategoryListRequest(QObject* parent)
    : LoadCategoryListRequest(Type::MODEL_V3, parent) {}

bool SyncHttpResponseDataToModel(const LoadModelGroupCategoryListRequest& request,
                                 ModelGroupCategoryListModel& model) {
  const auto json = request.getResponseJson();
  if (json.empty()) { return false; }

  const auto result = json[QStringLiteral("result")].toObject();
  if (result.empty()) { return false; }

  const auto list = result[QStringLiteral("list")].toArray();
  if (list.empty()) { return false; }

  ModelGroupCategoryListModel::DataContainer new_models;
  for (const auto& object_ref : list) {
    const auto object = object_ref.toObject();
    if (object.empty()) { continue; }

    const auto uid = QString::number(object[QStringLiteral("id")].toInt());
    const auto name = object[QStringLiteral("name")].toString();

    auto info = model.load(uid);

    info.uid = uid;
    info.name = name;

    new_models.emplace_back(std::move(info));
  }

  model.emplaceBack(new_models);
  return true;
}

LoadRecommendModelGroupListRequest::LoadRecommendModelGroupListRequest(int page_index,
                                                                       int page_size,
                                                                       QObject* parent)
    : HttpRequest(parent)
    , page_index_(page_index)
    , page_size_(page_size) {}

int LoadRecommendModelGroupListRequest::getPageIndex() const {
  return page_index_;
}

int LoadRecommendModelGroupListRequest::getPageSize() const {
  return page_size_;
}

QByteArray LoadRecommendModelGroupListRequest::getRequestData() const {
  QJsonObject root{
    { QStringLiteral("limit"), page_size_ },
    { QStringLiteral("excludeAd"), true },
  };

  if (page_index_ > 0) {
    root[QStringLiteral("cursor")] = QString::number(page_index_);
  }

  return QJsonDocument{ std::move(root) }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadRecommendModelGroupListRequest::getUrlTail() const {
  switch (getApplicationType()) {
    case ApplicationType::CREATIVE_PRINT:
      return QStringLiteral("/api/cxy/v3/model/recommend");
    case ApplicationType::HALOT_BOX:
    default:
      return QStringLiteral("/api/cxy/v3/model/dlpRecommend");
  }
}

bool SyncHttpResponseDataToModel(const LoadRecommendModelGroupListRequest& request,
                                 ModelGroupInfoListModel& model) {
  const auto result = request.getResponseJson().value(QStringLiteral("result")).toObject();
  const auto group_list = result.value(QStringLiteral("list")).toArray();
  if (result.empty() || group_list.empty()) {
    return false;
  }

  ModelGroupInfoListModel::DataContainer new_models;
  for (const auto& group : group_list) {
    const auto group_info = group[QStringLiteral("model")].toObject();
    auto group_uid = group_info[QStringLiteral("id")].toString();
    auto author = group_info[QStringLiteral("userInfo")].toObject();

    auto info = model.load(group_uid);

    info.uid           = std::move(group_uid);
    info.name          = group_info[QStringLiteral("groupName")].toString();
    info.image         = group_info[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
    info.license       = group_info[QStringLiteral("license")].toString();
    info.price         = group_info[QStringLiteral("totalPrice")].toInt();
    info.creation_time = group_info[QStringLiteral("createTime")].toInt();
    info.collected     = group_info[QStringLiteral("isCollection")].toBool();
    info.liked         = group[QStringLiteral("isLike")].toBool();
    info.model_count   = group_info[QStringLiteral("modelCount")].toInt();
    info.author_name   = author[QStringLiteral("nickName")].toString();
    info.author_image  = author[QStringLiteral("avatar")].toString();

    new_models.emplace_back(std::move(info));
  }

  model.emplaceBack(new_models);
  return true;
}

LoadTypeModelGroupListRequest::LoadTypeModelGroupListRequest(const QString& type_id,
                                                             int page_index,
                                                             int page_size,
                                                             FilterType filter_type,
                                                             PayType pay_type,
                                                             const QString& last_group_uid,
                                                             QObject* parent)
    : HttpRequest(parent)
    , type_id_(type_id)
    , page_index_(page_index)
    , page_size_(page_size)
    , filter_type_(filter_type)
    , pay_type_(pay_type)
    , last_group_uid_(last_group_uid) {}

QString LoadTypeModelGroupListRequest::getTypeId() const {
  return type_id_;
}

int LoadTypeModelGroupListRequest::getPageIndex() const {
  return page_index_;
}

int LoadTypeModelGroupListRequest::getPageSize() const {
  return page_size_;
}

QByteArray LoadTypeModelGroupListRequest::getRequestData() const {
  return QJsonDocument{{
    // { QStringLiteral("cursor")    , filter_type_ == FilterType::NEWEST_UPLOAD ?
    //                                   last_group_uid_ : QString::number(page_index_) },
    { QStringLiteral("cursor")    , QString::number(page_index_)   },
    { QStringLiteral("limit")     , page_size_                     },
    { QStringLiteral("categoryId"), type_id_                       },
    { QStringLiteral("filterType"), static_cast<int>(filter_type_) },
    { QStringLiteral("isPay")     , static_cast<int>(pay_type_)    },
  }}.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadTypeModelGroupListRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/listCategory");
}

bool LoadTypeModelGroupListRequest::handleResponseData() {
  if (!HttpRequest::handleResponseData()) {
    return false;
  }

  if (filter_type_ == FilterType::NEWEST_UPLOAD) {
    auto root = getResponseJson();
    auto result = root[QStringLiteral("result")].toObject();
    result[QStringLiteral("nextCursor")] = page_index_ + 1;
    root[QStringLiteral("result")] = std::move(result);
    setResponseData(QJsonDocument{ root }.toJson());
    setResponseJson(std::move(root));
  }

  return true;
}

bool SyncHttpResponseDataToModel(const LoadTypeModelGroupListRequest& request,
                                 ModelGroupInfoListModel& model) {
  const auto root = request.getResponseJson();
  if (root.empty()) { return false; }

  const auto group_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
  if (group_list.empty()) { return false; }

  ModelGroupInfoListModel::DataContainer new_models;
  for (const auto& group : group_list) {
    if (!group.isObject()) { continue; }

    auto author = group[QStringLiteral("userInfo")];
    auto group_uid = group[QStringLiteral("id")].toString();

    auto info = model.load(group_uid);

    info.uid           = std::move(group_uid);
    info.name          = group[QStringLiteral("groupName")].toString();
    info.image         = group[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
    info.license       = group[QStringLiteral("license")].toString();
    info.price         = group[QStringLiteral("totalPrice")].toInt();
    info.creation_time = group[QStringLiteral("createTime")].toInt();
    info.collected     = group[QStringLiteral("isCollection")].toBool();
    info.liked         = group[QStringLiteral("isLike")].toBool();
    info.model_count   = group[QStringLiteral("modelCount")].toInt();
    info.author_name   = author[QStringLiteral("nickName")].toString();
    info.author_image  = author[QStringLiteral("avatar")].toString();

    new_models.emplace_back(std::move(info));
  }

  model.emplaceBack(new_models);
  return true;
}

SearchModelRequest::SearchModelRequest(const QString& keyword,
                                       int page_index,
                                       int page_size,
                                       QObject* parent)
    : HttpRequest(parent)
    , keyword_(keyword)
    , page_index_(page_index)
    , page_size_(page_size) {}

QString SearchModelRequest::getKeyword() const {
  return keyword_;
}

int SearchModelRequest::getPageIndex() const {
  return page_index_;
}

int SearchModelRequest::getPageSize() const {
  return page_size_;
}

QByteArray SearchModelRequest::getRequestData() const {
  return QJsonDocument{{
    { QStringLiteral("page")    , page_index_ },
    { QStringLiteral("pageSize"), page_size_  },
    { QStringLiteral("keyword") , keyword_    },
  }}.toJson(QJsonDocument::JsonFormat::Compact);
}

QString SearchModelRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/search/model");
}

bool SyncHttpResponseDataToModel(const SearchModelRequest& request,
                                 ModelGroupInfoListModel& model) {
  return SyncHttpResponseDataToModel(
    reinterpret_cast<const LoadTypeModelGroupListRequest&>(request), model);
}

LoadModelGroupInfoRequest::LoadModelGroupInfoRequest(const QString& group_id, QObject* parent)
    : HttpRequest(parent)
    , group_id_(group_id) {}

QByteArray LoadModelGroupInfoRequest::getRequestData() const {
  return QJsonDocument{ QJsonObject{
    { QStringLiteral("id"), group_id_ },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadModelGroupInfoRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/modelGroupInfo");
}

bool SyncHttpResponseDataToModel(const LoadModelGroupInfoRequest& request,
                                 ModelGroupInfoModel& model) {
  const auto root = request.getResponseJson();
  if (root.empty()) { return false; }

  const auto result = root[QStringLiteral("result")].toObject();
  if (result.empty()) { return false; }

  const auto group_info = result[QStringLiteral("groupItem")];
  const auto model_list = result[QStringLiteral("modelList")].toArray();
  const auto author_info = group_info[QStringLiteral("userInfo")];
  auto group_uid = group_info[QStringLiteral("id")].toString();

  model.setUid(std::move(group_uid));
  model.setName(group_info[QStringLiteral("groupName")].toString());
  model.setImage(group_info[QStringLiteral("covers")][0][QStringLiteral("url")].toString());
  model.setLicense(group_info[QStringLiteral("license")].toString());
  model.setPrice(group_info[QStringLiteral("totalPrice")].toInt());
  model.setCreationTime(group_info[QStringLiteral("createTime")].toInt());
  model.setCollected(group_info[QStringLiteral("isCollection")].toBool());
  model.setLiked(group_info[QStringLiteral("isLike")].toBool());
  model.setAuthorName(author_info[QStringLiteral("nickName")].toString());
  model.setAuthorImage(author_info[QStringLiteral("avatar")].toString());
  model.setModelCount(group_info[QStringLiteral("modelCount")].toInt());

  auto models = model.models().lock();
  decltype(models)::element_type::DataContainer new_models;

  for (const auto& item : model_list) {
    auto model_uid = item[QStringLiteral("id")].toString();
    auto info = models->load(model_uid);

    info.uid = std::move(model_uid);
    info.name = item[QStringLiteral("fileName")].toString();
    info.image = item[QStringLiteral("coverUrl")].toString();
    info.size = item[QStringLiteral("fileSize")].toInt();

    new_models.emplace_back(std::move(info));
  }

  models->emplaceBack(new_models);

  return false;
}

LoadModelGroupFileListInfoRequest::LoadModelGroupFileListInfoRequest(const QString& group_id,
                                                                     int count,
                                                                     QObject* parent)
    : HttpRequest(parent)
    , group_id_(group_id)
    , count_(count) {}

QByteArray LoadModelGroupFileListInfoRequest::getRequestData() const {
  return QJsonDocument{{
    { QStringLiteral("limit"), count_ },
    { QStringLiteral("modelId"), group_id_ },
  }}.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadModelGroupFileListInfoRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/fileList");
}

bool SyncHttpResponseDataToModel(const LoadModelGroupFileListInfoRequest& request,
                                 ModelGroupInfoModel& model) {
  const auto root = request.getResponseJson();
  if (root.empty()) { return false; }

  const auto list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
  if (list.empty()) { return false; }

  auto models = model.models().lock();
  decltype(models)::element_type::DataContainer new_models;

  for (const auto& item : list) {
    auto uid = item[QStringLiteral("id")].toString();
    auto info = models->load(uid);

    info.uid   = std::move(uid);
    info.name  = item[QStringLiteral("fileName")].toString();
    info.size  = item[QStringLiteral("fileSize")].toInt();
    info.image = item[QStringLiteral("coverUrl")].toString();

    new_models.emplace_back(std::move(info));
  }

  models->emplaceBack(new_models);

  return true;
}

LikeModelGroupRequest::LikeModelGroupRequest(const QString& uid,
    bool like,
    QObject* parent)
    : HttpRequest(parent)
    , uid_(uid)
    , like_(like) {}

QString LikeModelGroupRequest::getUid() const {
    return uid_;
}


bool LikeModelGroupRequest::isLike() const {
    return like_;
}

QByteArray LikeModelGroupRequest::getRequestData() const {
    return QJsonDocument{ {
      { QStringLiteral("id")    , uid_     },
      { QStringLiteral("action"), like_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LikeModelGroupRequest::getUrlTail() const {
    return QStringLiteral("/api/cxy/v3/model/modelGroupLike");
}


CollectModelGroupRequest::CollectModelGroupRequest(const QString& uid,
                                                   bool collect,
                                                   QObject* parent)
    : HttpRequest(parent)
    , uid_(uid)
    , collect_(collect) {}

QString CollectModelGroupRequest::getUid() const {
  return uid_;
}

bool CollectModelGroupRequest::isCollect() const {
  return collect_;
}


QByteArray CollectModelGroupRequest::getRequestData() const {
  return QJsonDocument{{
    { QStringLiteral("id")    , uid_     },
    { QStringLiteral("action"), collect_ },
  }}.toJson(QJsonDocument::JsonFormat::Compact);
}

QString CollectModelGroupRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/modelGroupCollection");
}


// ---------- model library service [end] ----------

// ---------- model library author service [begin] ----------

LoadModelGroupAuthorInfoRequest::LoadModelGroupAuthorInfoRequest(const QString& author_id, QObject* parent)
    : HttpRequest(parent)
    , author_id_(author_id) {}

QByteArray LoadModelGroupAuthorInfoRequest::getRequestData() const {
    QJsonObject jsonObject;
    jsonObject["userId"] = QJsonValue::fromVariant(QString(author_id_).toLongLong());
    QJsonDocument jsonDocument(jsonObject);
    return jsonDocument.toJson(QJsonDocument::Compact);
}
QString LoadModelGroupAuthorInfoRequest::getUrlTail() const {
    return QStringLiteral("/api/cxy/v2/user/getOtherInfo");

}

LoadModelGroupAuthorModelListRequest::LoadModelGroupAuthorModelListRequest(
    const QString& user_id,
    int page_index,
    int page_size,
    int filter_type,
    const QString& keyword,
    QObject* parent)
    : HttpRequest(parent)
    , page_index_(page_index)
    , page_size_(page_size)
    , filter_type_(filter_type)
    , keyword_(keyword)
    , user_id_(user_id) {}

QByteArray LoadModelGroupAuthorModelListRequest::getRequestData() const {
    QJsonObject json{
        {"page", static_cast<int>(page_index_)},
        {"pageSize", static_cast<int>(page_size_)},
        {"filterType", static_cast<int>(filter_type_)},
        {"userId", QJsonValue::fromVariant(QString(user_id_).toLongLong())},
        {"keyword",keyword_},
    };
   // if (!keyword_.isEmpty())json["keyword"] = keyword_;
    return QJsonDocument{ json }.toJson(QJsonDocument::JsonFormat::Compact);
}
QString LoadModelGroupAuthorModelListRequest::getUrlTail() const {
    return QStringLiteral("/api/cxy/v3/model/listSharePage");

}

//---------- model library author service [end] ----------

// ---------- model service [begin] ----------

DeleteModelRequest::DeleteModelRequest(const QString& id, QObject* parent)
    : HttpRequest(parent)
    , id_(id) {}

QByteArray DeleteModelRequest::getRequestData() const {
  return QStringLiteral(R"({"id":"%1"})").arg(id_).toUtf8();
}

QString DeleteModelRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/modelGroupDelete");
}

CreateModelGroupRequest::CreateModelGroupRequest(bool is_original,
                                                 bool is_share,
                                                 int color_index,
                                                 const QString& category_id,
                                                 const QString& group_name,
                                                 const QString& group_desc,
                                                 const QString& license,
                                                 const std::list<FileInfo>& file_list,
                                                 QObject* parent)
    : HttpRequest(parent)
    , is_original_(is_original)
    , is_share_(is_share)
    , color_index_(color_index)
    , category_id_(category_id)
    , group_name_(group_name)
    , group_desc_(group_desc)
    , license_(license)
    , file_list_(file_list) {}

QByteArray CreateModelGroupRequest::getRequestData() const {
  QJsonObject root{
    {
      QStringLiteral("groupItem"), QJsonObject{
        { QStringLiteral("categoryId"), category_id_ },
        { QStringLiteral("groupName") , group_name_ },
        { QStringLiteral("groupDesc") , group_desc_ },
        { QStringLiteral("modelColor"), color_index_ },
        { QStringLiteral("isOriginal"), is_original_ },
        { QStringLiteral("isShared")  , is_share_ },
        { QStringLiteral("license")   , license_ },
        { QStringLiteral("covers")    , QJsonArray{} },
      }
    },
  };

  QJsonArray model_list;
  for (const auto& file_info : file_list_) {
    model_list.push_back(QJsonObject{
      { QStringLiteral("fileKey") , file_info.key },
      { QStringLiteral("fileName"), file_info.name },
      { QStringLiteral("fileSize"), file_info.size },
    });
  }
  root[QStringLiteral("modelList")] = std::move(model_list);

  return QJsonDocument{ std::move(root) }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString CreateModelGroupRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/modelGroupCreate");
}

LoadUploadedModelGroupListReuquest::LoadUploadedModelGroupListReuquest(size_t   page_index,
                                                                       size_t   page_size,
                                                                       QString  keyword,
                                                                       size_t   begin_time,
                                                                       size_t   end_time,
                                                                       QObject* parent)
    : HttpRequest(parent)
    , page_index_(page_index)
    , page_size_(page_size)
    , keyword_(keyword)
    , begin_time_(begin_time)
    , end_time_(end_time) {}

size_t LoadUploadedModelGroupListReuquest::getPageIndex() const {
  return page_index_;
}

size_t LoadUploadedModelGroupListReuquest::getPageSize() const {
  return page_size_;
}

QString LoadUploadedModelGroupListReuquest::getKeyword() const {
  return keyword_;
}

size_t LoadUploadedModelGroupListReuquest::getBeginTime() const {
  return begin_time_;
}

size_t LoadUploadedModelGroupListReuquest::getEndTime() const {
  return end_time_;
}

QByteArray LoadUploadedModelGroupListReuquest::getRequestData() const {
  QJsonObject json{
    { QStringLiteral("page")      , static_cast<int>(page_index_)  },
    { QStringLiteral("pageSize")  , static_cast<int>(page_size_)   },
  };

  if (!keyword_.isEmpty()) {
    json[QStringLiteral("keyword")] = keyword_;
  }

  if (begin_time_ != 0) {
    json[QStringLiteral("begin")] = static_cast<int>(begin_time_);
  }

  if (end_time_ != 0) {
    json[QStringLiteral("end")] = static_cast<int>(end_time_);
  }

  return QJsonDocument{ json }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadUploadedModelGroupListReuquest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/listUploadPage");
}

bool SyncHttpResponseDataToModel(const LoadUploadedModelGroupListReuquest& request,
                                 ModelGroupInfoListModel& model) {
  const auto root = request.getResponseJson();
  if (root.empty()) { return false; }

  const auto group_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
  if (group_list.empty()) { return false; }

  ModelGroupInfoListModel::DataContainer new_models;
  for (const auto& group : group_list) {
    if (!group.isObject()) { continue; }

    const auto author = group[QStringLiteral("userInfo")];
    const auto group_uid = group[QStringLiteral("id")].toString();

    auto info = model.load(group_uid);

    info.uid           = std::move(group_uid);
    info.name          = group[QStringLiteral("groupName")].toString();
    info.image         = group[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
    info.model_count   = group[QStringLiteral("modelCount")].toInt();
    info.license       = group[QStringLiteral("license")].toString();
    info.price         = group[QStringLiteral("totalPrice")].toInt();
    info.creation_time = group[QStringLiteral("createTime")].toInt();
    info.collected     = group[QStringLiteral("isCollection")].toBool();
    info.liked         = group[QStringLiteral("isLike")].toBool();
    info.author_name   = author[QStringLiteral("nickName")].toString();
    info.author_image  = author[QStringLiteral("avatar")].toString();

    new_models.emplace_back(std::move(info));
  }

  model.emplaceBack(new_models);
  return true;
}

LoadCollectedModelGroupListRequest::LoadCollectedModelGroupListRequest(size_t     page_index,
                                                                       size_t     page_size,
                                                                       QString    user_id,
                                                                       FilterType filter_type,
                                                                       size_t     begin_time,
                                                                       size_t     end_time,
                                                                       QObject*   parent)
    : HttpRequest(parent)
    , page_index_(page_index)
    , page_size_(page_size)
    , user_id_(user_id)
    , filter_type_(filter_type)
    , begin_time_(begin_time)
    , end_time_(end_time) {}

size_t LoadCollectedModelGroupListRequest::getPageIndex() const {
  return page_index_;
}

size_t LoadCollectedModelGroupListRequest::getPageSize() const {
  return page_size_;
}

QString LoadCollectedModelGroupListRequest::getUserId() const {
  return user_id_;
}

LoadCollectedModelGroupListRequest::FilterType
LoadCollectedModelGroupListRequest::getFilterType() const {
  return filter_type_;
}

size_t LoadCollectedModelGroupListRequest::getBeginTime() const {
  return begin_time_;
}

size_t LoadCollectedModelGroupListRequest::getEndTime() const {
  return end_time_;
}

QByteArray LoadCollectedModelGroupListRequest::getRequestData() const {
  QJsonObject json{
    { QStringLiteral("page")      , static_cast<int>(page_index_)  },
    { QStringLiteral("pageSize")  , static_cast<int>(page_size_)   },
    { QStringLiteral("filterType"), static_cast<int>(filter_type_) },
  };

  if (!user_id_.isEmpty()) {
    json[QStringLiteral("userId")] = user_id_;
  }

  if (begin_time_ != 0) {
    json[QStringLiteral("begin")] = static_cast<int>(begin_time_);
  }

  if (end_time_ != 0) {
    json[QStringLiteral("end")] = static_cast<int>(end_time_);
  }

  return QJsonDocument{ json }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadCollectedModelGroupListRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v3/model/listCollectPageV2");
}

bool SyncHttpResponseDataToModel(const LoadCollectedModelGroupListRequest& request,
                                 ModelGroupInfoListModel& model) {
  return SyncHttpResponseDataToModel(
    reinterpret_cast<const LoadUploadedModelGroupListReuquest&>(request), model);
}

LoadPurchasedModelGroupListReuquest::LoadPurchasedModelGroupListReuquest(size_t      page_index,
                                                                         size_t      page_size,
                                                                         QString     order_id,
                                                                         OrderStatus order_status,
                                                                         size_t      begin_time,
                                                                         size_t      end_time,
                                                                         QObject*    parent)
    : HttpRequest(parent)
    , page_index_(page_index)
    , page_size_(page_size)
    , order_id_(order_id)
    , order_status_(order_status)
    , begin_time_(begin_time)
    , end_time_(end_time) {}

size_t LoadPurchasedModelGroupListReuquest::getPageIndex() const {
  return page_index_;
}

size_t LoadPurchasedModelGroupListReuquest::getPageSize() const {
  return page_size_;
}

QString LoadPurchasedModelGroupListReuquest::getOrderId() const {
  return order_id_;
}

LoadPurchasedModelGroupListReuquest::OrderStatus
LoadPurchasedModelGroupListReuquest::getOrderStatus() const {
  return order_status_;
}

size_t LoadPurchasedModelGroupListReuquest::getBeginTime() const {
  return begin_time_;
}

size_t LoadPurchasedModelGroupListReuquest::getEndTime() const {
  return end_time_;
}

QByteArray LoadPurchasedModelGroupListReuquest::getRequestData() const {
  QJsonObject json{
    { QStringLiteral("page")      , static_cast<int>(page_index_)  },
    { QStringLiteral("pageSize")  , static_cast<int>(page_size_)   },
  };

  if (!order_id_.isEmpty()) {
    json[QStringLiteral("orderId")] = order_id_;
  }

  if (order_status_ != OrderStatus::ANY) {
    json[QStringLiteral("payStatus")] = static_cast<int>(order_status_);
  }

  if (begin_time_ != 0) {
    json[QStringLiteral("begin")] = static_cast<int>(begin_time_);
  }

  if (end_time_ != 0) {
    json[QStringLiteral("end")] = static_cast<int>(end_time_);
  }

  return QJsonDocument{ json }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadPurchasedModelGroupListReuquest::getUrlTail() const {
    return QStringLiteral("/api/cxy/v3/model/modelOrderListPage");
  /*return QStringLiteral("/api/cxy/v3/model/modelOrderListV2");*/
}

bool SyncHttpResponseDataToModel(const LoadPurchasedModelGroupListReuquest& request,
                                 ModelGroupInfoListModel& model) {
  const auto json = request.getResponseJson();
  if (json.empty()) { return false; }

  const auto order_list = json[QStringLiteral("result")][QStringLiteral("list")].toArray();
  if (order_list.empty()) { return false; }

  for (const auto& order : order_list) {
    if (!order.isObject()) { continue; }

    const auto sub_order_list = order[QStringLiteral("orderList")].toArray();
    if (sub_order_list.empty()) { continue; }

    ModelGroupInfoListModel::DataContainer new_models;
    for (const auto& sub_order : sub_order_list) {
      if (!sub_order.isObject()) { continue; }

      const auto author = sub_order[QStringLiteral("userInfo")];
      const auto group_uid = sub_order[QStringLiteral("modelId")].toString();

      auto info = model.load(group_uid);

      info.uid           = std::move(group_uid);
      info.name          = sub_order[QStringLiteral("groupName")].toString();
      info.image         = sub_order[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
      info.model_count   = sub_order[QStringLiteral("modelCount")].toInt();
      info.license       = sub_order[QStringLiteral("license")].toString();
      info.price         = sub_order[QStringLiteral("totalPrice")].toInt();
      info.creation_time = sub_order[QStringLiteral("createTime")].toInt();
      info.collected     = sub_order[QStringLiteral("isCollection")].toBool();
      info.liked         = sub_order[QStringLiteral("isLike")].toBool();
      info.author_name   = author[QStringLiteral("nickName")].toString();
      info.author_image  = author[QStringLiteral("avatar")].toString();

      new_models.emplace_back(std::move(info));
    }

    model.emplaceBack(new_models);
  }

  return true;
}

// ---------- model service [end] ----------

}  // namespace cxcloud
