#include "cxcloud/net/model_request.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace cxcloud {

// ---------- model library service [begin] ----------

  LoadModelGroupCategoryListRequest::LoadModelGroupCategoryListRequest(QObject* parent)
      : LoadCategoryListRequest{ Type::MODEL_V3, parent } {}

  auto SyncHttpResponseDataToModel(const LoadModelGroupCategoryListRequest& request,
                                   ModelGroupCategoryListModel&             model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto result = root[QStringLiteral("result")].toObject();
    if (result.empty()) {
      return false;
    }

    const auto list = result[QStringLiteral("list")].toArray();
    if (list.empty()) {
      return false;
    }

    auto new_models = ModelGroupCategoryListModel::DataContainer{};
    for (const auto& object_ref : list) {
      const auto object = object_ref.toObject();
      if (object.empty()) {
        continue;
      }

      const auto uid = QString::number(object[QStringLiteral("id")].toInt());
      const auto name = object[QStringLiteral("name")].toString();

      auto info = model.load(uid);

      info.uid = uid;
      info.name = name;

      new_models.emplace_back(std::move(info));
    }

    model.emplaceBack(std::move(new_models));
    return true;
  }





  LoadRecommendModelGroupListRequest::LoadRecommendModelGroupListRequest(int      page_index,
                                                                         int      page_size,
                                                                         QObject* parent)
      : HttpRequest{ parent }, page_index_{ page_index }, page_size_{ page_size } {}

  auto LoadRecommendModelGroupListRequest::getPageIndex() const -> int {
    return page_index_;
  }

  auto LoadRecommendModelGroupListRequest::getPageSize() const -> int {
    return page_size_;
  }

  auto LoadRecommendModelGroupListRequest::getRequestData() const -> QByteArray {
    static const auto UNDEFINED = QJsonValue{ QJsonValue::Type::Undefined };

    QJsonObject root{
      { QStringLiteral("limit"), page_size_ },
      { QStringLiteral("excludeAd"), true },
    };

    if (page_index_ > 0) {
      root[QStringLiteral("cursor")] = QString::number(page_index_);
    }

    return QJsonDocument{ std::move(root) }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadRecommendModelGroupListRequest::getUrlTail() const -> QString {
    switch (getApplicationType()) {
      case ApplicationType::CREALITY_PRINT: {
        return QStringLiteral("/api/cxy/v3/model/recommend");
      }
      case ApplicationType::HALOT_BOX:
      default: {
        return QStringLiteral("/api/cxy/v3/model/dlpRecommend");
      }
    }
  }

  auto SyncHttpResponseDataToModel(const LoadRecommendModelGroupListRequest& request,
                                   ModelGroupInfoListModel&                  model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto group_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (group_list.empty()) {
      return false;
    }

    auto new_models = ModelGroupInfoListModel::DataContainer{};
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
      info.status        =
          static_cast<ModelGroupInfo::Status>(group[QStringLiteral("status")].toInt());
      info.collected     = group_info[QStringLiteral("isCollection")].toBool();
      info.restricted    =
          group_info[QStringLiteral("maturityRating")].toString() == QStringLiteral("restricted");
      info.liked         = group[QStringLiteral("isLike")].toBool();
      info.author_name   = author[QStringLiteral("nickName")].toString();
      info.author_image  = author[QStringLiteral("avatar")].toString();
      info.model_count   = group_info[QStringLiteral("modelCount")].toInt();
      info.project_count = group[QStringLiteral("model3mfCount")].toInt();

      new_models.emplace_back(std::move(info));
    }

    model.emplaceBack(new_models);
    return true;
  }





  LoadTypeModelGroupListRequest::LoadTypeModelGroupListRequest(const QString& type_id,
                                                               int            page_index,
                                                               int            page_size,
                                                               FilterType     filter_type,
                                                               PayType        pay_type,
                                                               const QString& last_group_uid,
                                                               QObject*       parent)
      : HttpRequest{ parent }
      , type_id_{ type_id }
      , page_index_{ page_index }
      , page_size_{ page_size }
      , filter_type_{ filter_type }
      , pay_type_{ pay_type }
      , last_group_uid_{ last_group_uid } {}

  auto LoadTypeModelGroupListRequest::getTypeId() const -> QString {
    return type_id_;
  }

  auto LoadTypeModelGroupListRequest::getPageIndex() const -> int {
    return page_index_;
  }

  auto LoadTypeModelGroupListRequest::getPageSize() const -> int {
    return page_size_;
  }

  auto LoadTypeModelGroupListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("cursor")    , QString::number(page_index_)   },
      { QStringLiteral("limit")     , page_size_                     },
      { QStringLiteral("categoryId"), type_id_                       },
      { QStringLiteral("filterType"), static_cast<int>(filter_type_) },
      { QStringLiteral("isPay")     , static_cast<int>(pay_type_)    },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadTypeModelGroupListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/listCategory");
  }

  auto LoadTypeModelGroupListRequest::handleResponseData() -> bool {
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

  auto SyncHttpResponseDataToModel(const LoadTypeModelGroupListRequest& request,
                                   ModelGroupInfoListModel&             model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto group_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (group_list.empty()) {
      return false;
    }

    auto new_models = ModelGroupInfoListModel::DataContainer{};
    for (const auto& group : group_list) {
      if (!group.isObject()) {
        continue;
      }

      auto author = group[QStringLiteral("userInfo")];
      auto group_uid = group[QStringLiteral("id")].toString();

      auto info = model.load(group_uid);

      info.uid           = std::move(group_uid);
      info.name          = group[QStringLiteral("groupName")].toString();
      info.image         = group[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
      info.license       = group[QStringLiteral("license")].toString();
      info.price         = group[QStringLiteral("totalPrice")].toInt();
      info.creation_time = group[QStringLiteral("createTime")].toInt();
      info.status        =
          static_cast<ModelGroupInfo::Status>(group[QStringLiteral("status")].toInt());
      info.collected     = group[QStringLiteral("isCollection")].toBool();
      info.restricted    =
          group[QStringLiteral("maturityRating")].toString() == QStringLiteral("restricted");
      info.liked         = group[QStringLiteral("isLike")].toBool();
      info.author_name   = author[QStringLiteral("nickName")].toString();
      info.author_image  = author[QStringLiteral("avatar")].toString();
      info.model_count   = group[QStringLiteral("modelCount")].toInt();
      info.project_count = group[QStringLiteral("model3mfCount")].toInt();

      new_models.emplace_back(std::move(info));
    }

    model.emplaceBack(new_models);
    return true;
  }





  SearchModelRequest::SearchModelRequest(const QString& keyword,
                                         int            page_index,
                                         int            page_size,
                                         QObject*       parent)
      : HttpRequest{ parent }
      , keyword_{ keyword }
      , page_index_{ page_index }
      , page_size_{ page_size } {}

  auto SearchModelRequest::getKeyword() const -> QString {
    return keyword_;
  }

  auto SearchModelRequest::getPageIndex() const -> int {
    return page_index_;
  }

  auto SearchModelRequest::getPageSize() const -> int {
    return page_size_;
  }

  auto SearchModelRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("page")    , page_index_ },
      { QStringLiteral("pageSize"), page_size_  },
      { QStringLiteral("keyword") , keyword_    },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto SearchModelRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/search/model");
  }

  auto SyncHttpResponseDataToModel(const SearchModelRequest& request,
                                   ModelGroupInfoListModel&  model) -> bool {
    return SyncHttpResponseDataToModel(
        reinterpret_cast<const LoadTypeModelGroupListRequest&>(request), model);
  }





  LoadModelGroupInfoRequest::LoadModelGroupInfoRequest(const QString& group_uid, QObject* parent)
      : HttpRequest{ parent } , group_uid_{ group_uid } {}

  auto LoadModelGroupInfoRequest::getGroupUid() const -> QString {
    return group_uid_;
  }

  auto LoadModelGroupInfoRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), group_uid_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadModelGroupInfoRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelGroupDetail");
  }

  auto SyncHttpResponseDataToModel(const LoadModelGroupInfoRequest& request,
                                   ModelGroupInfoModel&             model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto result = root[QStringLiteral("result")].toObject();
    if (result.empty()) {
      return false;
    }

    const auto group_info  = result[QStringLiteral("groupItem")];
    const auto model_list  = result[QStringLiteral("modelList")].toArray();
    const auto author_info = group_info[QStringLiteral("userInfo")];
    auto       group_uid   = group_info[QStringLiteral("id")].toString();

    model.setUid(std::move(group_uid));
    model.setName(group_info[QStringLiteral("groupName")].toString());
    model.setImage(group_info[QStringLiteral("covers")][0][QStringLiteral("url")].toString());
    model.setLicense(group_info[QStringLiteral("license")].toString());
    model.setPrice(group_info[QStringLiteral("totalPrice")].toInt());
    model.setCreationTime(group_info[QStringLiteral("createTime")].toInt());
    model.setStatus(
        static_cast<ModelGroupInfo::Status>(group_info[QStringLiteral("status")].toInt()));
    model.setCollected(group_info[QStringLiteral("isCollection")].toBool());
    model.setLiked(group_info[QStringLiteral("isLike")].toBool());
    model.setPurchased(group_info[QStringLiteral("isPurchased")].toBool());
    model.setRestricted(
        group_info[QStringLiteral("maturityRating")].toString() == QStringLiteral("restricted"));
    model.setAuthorName(author_info[QStringLiteral("nickName")].toString());
    model.setAuthorImage(author_info[QStringLiteral("avatar")].toString());
    model.setModelCount(group_info[QStringLiteral("modelCount")].toInt());
    model.setProjectCount(group_info[QStringLiteral("model3mfCount")].toInt());

    auto models             = model.models().lock();
    auto new_models         = decltype(models)::element_type::DataContainer{};
    auto model_broken_count = model.getModelCount();

    for (const auto& item : model_list) {
      auto model_uid = item[QStringLiteral("id")].toString();
      auto info      = models->load(model_uid);

      info.uid    = std::move(model_uid);
      info.name   = item[QStringLiteral("fileName")].toString();
      info.format = item[QStringLiteral("fileFormat")].toString();
      info.image  = item[QStringLiteral("coverUrl")].toString();
      info.size   = item[QStringLiteral("fileSize")].toInt();
      info.broken = item[QStringLiteral("isBroken")].toBool(false);

      if (!info.broken) {
        model_broken_count = std::max(static_cast<int>(model_broken_count) - 1, 0);
      }

      new_models.emplace_back(std::move(info));
    }

    model.setModelBrokenCount(model_broken_count);
    models->emplaceBack(new_models);

    return false;
  }

  auto SyncHttpResponseDataToModel(const LoadModelGroupInfoRequest& request,
                                   ModelGroupInfoListModel&         model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto result = root[QStringLiteral("result")].toObject();
    if (result.empty()) {
      return false;
    }

    const auto group_info = result[QStringLiteral("groupItem")];
    auto       group_uid  = group_info[QStringLiteral("id")].toString();

    auto info = model.load(group_uid);
    if (info.uid.isEmpty()) {
      return false;
    }

    info.uid                = std::move(group_uid);
    info.model_count        = group_info[QStringLiteral("modelCount")].toInt();
    info.model_broken_count = std::max(static_cast<int>(info.model_count), 0);
    info.project_count      = group_info[QStringLiteral("model3mfCount")].toInt();
    info.price              = group_info[QStringLiteral("totalPrice")].toInt();
    info.purchased          = group_info[QStringLiteral("isPurchased")].toBool();

    const auto model_info_array = result[QStringLiteral("modelList")].toArray();
    for (const auto& model_info : model_info_array) {
      if (!model_info[QStringLiteral("isBroken")].toBool(false)) {
        info.model_broken_count = std::max(static_cast<int>(info.model_broken_count) - 1, 0);
      }
    }

    model.update(std::move(info));
    return true;
  }





  LoadModelGroupFileListInfoRequest::LoadModelGroupFileListInfoRequest(
      const QString& group_id, int count, QObject* parent)
      : HttpRequest{ parent }, group_id_{ group_id }, count_{ count } {}

  auto LoadModelGroupFileListInfoRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("limit"), count_ },
      { QStringLiteral("modelId"), group_id_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadModelGroupFileListInfoRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/fileList");
  }

  auto SyncHttpResponseDataToModel(const LoadModelGroupFileListInfoRequest& request,
                                   ModelGroupInfoModel&                     model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (list.empty()) {
      return false;
    }

    auto models = model.models().lock();
    auto new_models = decltype(models)::element_type::DataContainer{};
    auto model_broken_count = model.getModelCount();

    for (const auto& item : list) {
      auto uid = item[QStringLiteral("id")].toString();
      auto info = models->load(uid);

      info.uid    = std::move(uid);
      info.name   = item[QStringLiteral("fileName")].toString();
      info.format = item[QStringLiteral("fileFormat")].toString();
      info.size   = item[QStringLiteral("fileSize")].toInt();
      info.image  = item[QStringLiteral("coverUrl")].toString();
      info.broken = item[QStringLiteral("isBroken")].toBool(false);

      if (!info.broken) {
        model_broken_count = std::max(static_cast<int>(model_broken_count) - 1, 0);
      }

      new_models.emplace_back(std::move(info));
    }

    model.setModelBrokenCount(model_broken_count);
    models->emplaceBack(new_models);

    return true;
  }





  CollectModelGroupRequest::CollectModelGroupRequest(const QString& uid,
                                                     bool           collect,
                                                     QObject*       parent)
      : HttpRequest{ parent }, uid_{ uid }, collect_{ collect } {}

  auto CollectModelGroupRequest::getUid() const -> QString {
    return uid_;
  }

  auto CollectModelGroupRequest::isCollect() const -> bool {
    return collect_;
  }

  auto CollectModelGroupRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("id")    , uid_     },
      { QStringLiteral("action"), collect_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto CollectModelGroupRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelGroupCollection");
  }





  LikeModelGroupRequest::LikeModelGroupRequest(const QString& uid, bool like, QObject* parent)
      : HttpRequest{ parent }, uid_{ uid }, like_{ like } {}

  auto LikeModelGroupRequest::getUid() const -> QString {
    return uid_;
  }

  auto LikeModelGroupRequest::isLike() const -> bool {
    return like_;
  }

  auto LikeModelGroupRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("id")    , uid_  },
      { QStringLiteral("action"), like_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LikeModelGroupRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelGroupLike");
  }





  ShareModelGroupRequest::ShareModelGroupRequest(const QString& uid, QObject* parent)
      : HttpRequest{ parent }, uid_{ uid } {}

  auto ShareModelGroupRequest::getUid() const -> QString {
    return uid_;
  }

  auto ShareModelGroupRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), uid_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto ShareModelGroupRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelGroupShareIt");
  }





  DownloadModelGroupRequest::DownloadModelGroupRequest(const QString& uid, QObject* parent)
      : HttpRequest{ parent }, uid_{ uid } {}

  auto DownloadModelGroupRequest::getUid() const -> QString {
      return uid_;
  }

  auto DownloadModelGroupRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("modelId")  , uid_ },
      { QStringLiteral("trailType"), 2    },  // 1:try out 2:give up
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto DownloadModelGroupRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/memberDownload");
  }





  DownloadModelGroupSuccessRequest::DownloadModelGroupSuccessRequest(
      const QString& uid, QObject* parent)
      : HttpRequest{ parent }, uid_{ uid } {}

  auto DownloadModelGroupSuccessRequest::getUid() const -> QString {
    return uid_;
  }

  auto DownloadModelGroupSuccessRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("modelId"), uid_ },
      { QStringLiteral("status") , 2    },  // 2:sucess 3:fail
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto DownloadModelGroupSuccessRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/downloadSuccess");
  }

// ---------- model library service [end] ----------





// ---------- model library author service [begin] ----------

  LoadModelGroupAuthorInfoRequest::LoadModelGroupAuthorInfoRequest(const QString& author_id,
                                                                   QObject*       parent)
      : HttpRequest{ parent }, author_id_{ author_id } {}

  auto LoadModelGroupAuthorInfoRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("userId"), QJsonValue::fromVariant(author_id_.toLongLong()) },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadModelGroupAuthorInfoRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/user/getOtherInfo");
  }





  LoadModelGroupAuthorModelListRequest::LoadModelGroupAuthorModelListRequest(const QString& user_id,
                                                                             int page_index,
                                                                             int page_size,
                                                                             int filter_type,
                                                                             const QString& keyword,
                                                                             QObject*       parent)
      : HttpRequest{ parent }
      , page_index_{ page_index }
      , page_size_{ page_size }
      , filter_type_{ filter_type }
      , user_id_{ user_id }
      , keyword_{ keyword } {}

  auto LoadModelGroupAuthorModelListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("page"), static_cast<int>(page_index_) },
      { QStringLiteral("pageSize"), static_cast<int>(page_size_) },
      { QStringLiteral("filterType"), static_cast<int>(filter_type_) },
      { QStringLiteral("userId"), user_id_.toLongLong() },
      { QStringLiteral("keyword"), keyword_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadModelGroupAuthorModelListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/listSharePage");
  }

//---------- model library author service [end] ----------





// ---------- model service [begin] ----------

  DeleteModelRequest::DeleteModelRequest(const QString& id, QObject* parent)
      : HttpRequest{ parent }, id_{ id } {}

  auto DeleteModelRequest::getRequestData() const -> QByteArray {
    return QStringLiteral(R"({"id":"%1"})").arg(id_).toUtf8();
  }

  auto DeleteModelRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelGroupDelete");
  }





  CreateModelGroupRequest::CreateModelGroupRequest(bool                       is_original,
                                                   bool                       is_share,
                                                   int                        color_index,
                                                   const QString&             category_id,
                                                   const QString&             group_name,
                                                   const QString&             group_desc,
                                                   const QString&             license,
                                                   const std::list<FileInfo>& file_list,
                                                   QObject*                   parent)
      : HttpRequest{ parent }
      , is_original_{ is_original }
      , is_share_{ is_share }
      , color_index_{ color_index }
      , category_id_{ category_id }
      , group_name_{ group_name }
      , group_desc_{ group_desc }
      , license_{ license }
      , file_list_{ file_list } {}

  auto CreateModelGroupRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      {
        QStringLiteral("groupItem"), QJsonObject{
          { QStringLiteral("categoryId"), category_id_ },
          { QStringLiteral("groupName") , group_name_  },
          { QStringLiteral("groupDesc") , group_desc_  },
          { QStringLiteral("modelColor"), color_index_ },
          { QStringLiteral("isOriginal"), is_original_ },
          { QStringLiteral("isShared")  , is_share_    },
          { QStringLiteral("license")   , license_     },
          { QStringLiteral("covers")    , QJsonArray{} },
        }
      },
      {
        QStringLiteral("modelList"), [this]() -> QJsonArray {
          QJsonArray model_list;
          for (const auto& file_info : file_list_) {
            model_list.push_back(QJsonObject{
              { QStringLiteral("fileKey") , file_info.key },
              { QStringLiteral("fileName"), file_info.name },
              { QStringLiteral("fileSize"), file_info.size },
            });
          }
          return model_list;
        }()
      },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto CreateModelGroupRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelGroupCreate");
  }





  LoadUploadedModelGroupListReuquest::LoadUploadedModelGroupListReuquest(size_t   page_index,
                                                                         size_t   page_size,
                                                                         QString  keyword,
                                                                         size_t   begin_time,
                                                                         size_t   end_time,
                                                                         QObject* parent)
      : HttpRequest{ parent }
      , page_index_{ page_index }
      , page_size_{ page_size }
      , keyword_{ keyword }
      , begin_time_{ begin_time }
      , end_time_{ end_time } {}

  auto LoadUploadedModelGroupListReuquest::getPageIndex() const -> size_t {
    return page_index_;
  }

  auto LoadUploadedModelGroupListReuquest::getPageSize() const -> size_t {
    return page_size_;
  }

  auto LoadUploadedModelGroupListReuquest::getKeyword() const -> QString {
    return keyword_;
  }

  auto LoadUploadedModelGroupListReuquest::getBeginTime() const -> size_t {
    return begin_time_;
  }

  auto LoadUploadedModelGroupListReuquest::getEndTime() const -> size_t {
    return end_time_;
  }

  auto LoadUploadedModelGroupListReuquest::getRequestData() const -> QByteArray {
    static const auto UNDEFINED = QJsonValue{ QJsonValue::Type::Undefined };

    return QJsonDocument{ QJsonObject{
      { QStringLiteral("page"), static_cast<int>(page_index_) },
      { QStringLiteral("pageSize"), static_cast<int>(page_size_) },
      { QStringLiteral("keyword"), keyword_.isEmpty() ? UNDEFINED : keyword_ },
      { QStringLiteral("begin"), begin_time_ == 0 ? UNDEFINED : static_cast<int>(begin_time_) },
      { QStringLiteral("end"), end_time_ == 0 ? UNDEFINED : static_cast<int>(end_time_) },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadUploadedModelGroupListReuquest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/listUploadPage");
  }

  auto SyncHttpResponseDataToModel(const LoadUploadedModelGroupListReuquest& request,
                                   ModelGroupInfoListModel&                  model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto group_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (group_list.empty()) {
      return false;
    }

    auto new_models = ModelGroupInfoListModel::DataContainer{};
    for (const auto& group : group_list) {
      if (!group.isObject()) {
        continue;
      }

      const auto author = group[QStringLiteral("userInfo")];
      const auto group_uid = group[QStringLiteral("id")].toString();

      auto info = model.load(group_uid);

      info.uid           = std::move(group_uid);
      info.name          = group[QStringLiteral("groupName")].toString();
      info.image         = group[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
      info.license       = group[QStringLiteral("license")].toString();
      info.price         = group[QStringLiteral("totalPrice")].toInt();
      info.creation_time = group[QStringLiteral("createTime")].toInt();
      info.status        =
          static_cast<ModelGroupInfo::Status>(group[QStringLiteral("status")].toInt());
      info.collected     = group[QStringLiteral("isCollection")].toBool();
      info.restricted    =
          group[QStringLiteral("maturityRating")].toString() == QStringLiteral("restricted");
      info.liked         = group[QStringLiteral("isLike")].toBool();
      info.author_name   = author[QStringLiteral("nickName")].toString();
      info.author_image  = author[QStringLiteral("avatar")].toString();
      info.model_count   = group[QStringLiteral("modelCount")].toInt();
      info.project_count = group[QStringLiteral("model3mfCount")].toInt();

      new_models.emplace_back(std::move(info));
    }

    model.emplaceBack(std::move(new_models));
    return true;
  }





  LoadCollectedModelGroupListRequest::LoadCollectedModelGroupListRequest(size_t     page_index,
                                                                         size_t     page_size,
                                                                         QString    user_id,
                                                                         FilterType filter_type,
                                                                         size_t     begin_time,
                                                                         size_t     end_time,
                                                                         QObject*   parent)
      : HttpRequest{ parent }
      , page_index_{ page_index }
      , page_size_{ page_size }
      , user_id_{ user_id }
      , filter_type_{ filter_type }
      , begin_time_{ begin_time }
      , end_time_{ end_time } {}

  auto LoadCollectedModelGroupListRequest::getPageIndex() const -> size_t {
    return page_index_;
  }

  auto LoadCollectedModelGroupListRequest::getPageSize() const -> size_t {
    return page_size_;
  }

  auto LoadCollectedModelGroupListRequest::getUserId() const -> QString {
    return user_id_;
  }

  auto LoadCollectedModelGroupListRequest::getFilterType() const -> FilterType {
    return filter_type_;
  }

  auto LoadCollectedModelGroupListRequest::getBeginTime() const -> size_t {
    return begin_time_;
  }

  auto LoadCollectedModelGroupListRequest::getEndTime() const -> size_t {
    return end_time_;
  }

  auto LoadCollectedModelGroupListRequest::getRequestData() const -> QByteArray {
    static const auto UNDEFINED = QJsonValue{ QJsonValue::Type::Undefined };

    return QJsonDocument{ QJsonObject{
      { QStringLiteral("page"), static_cast<int>(page_index_) },
      { QStringLiteral("pageSize"), static_cast<int>(page_size_) },
      { QStringLiteral("filterType"), static_cast<int>(filter_type_) },
      { QStringLiteral("userId"), user_id_.isEmpty() ? UNDEFINED : user_id_ },
      { QStringLiteral("begin"), begin_time_ == 0 ? UNDEFINED : static_cast<int>(begin_time_) },
      { QStringLiteral("end"), end_time_ == 0 ? UNDEFINED : static_cast<int>(end_time_) },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadCollectedModelGroupListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/listCollectPageV2");
  }

  auto SyncHttpResponseDataToModel(const LoadCollectedModelGroupListRequest& request,
                                   ModelGroupInfoListModel&                  model) -> bool {
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
      : HttpRequest{ parent }
      , page_index_{ page_index }
      , page_size_{ page_size }
      , order_id_{ order_id }
      , order_status_{ order_status }
      , begin_time_{ begin_time }
      , end_time_{ end_time } {}

  auto LoadPurchasedModelGroupListReuquest::getPageIndex() const -> size_t {
    return page_index_;
  }

  auto LoadPurchasedModelGroupListReuquest::getPageSize() const -> size_t {
    return page_size_;
  }

  auto LoadPurchasedModelGroupListReuquest::getOrderId() const -> QString {
    return order_id_;
  }

  auto LoadPurchasedModelGroupListReuquest::getOrderStatus() const -> OrderStatus {
    return order_status_;
  }

  auto LoadPurchasedModelGroupListReuquest::getBeginTime() const -> size_t {
    return begin_time_;
  }

  auto LoadPurchasedModelGroupListReuquest::getEndTime() const -> size_t {
    return end_time_;
  }

  auto LoadPurchasedModelGroupListReuquest::getRequestData() const -> QByteArray {
    static const auto UNDEFINED = QJsonValue{ QJsonValue::Type::Undefined };

    return QJsonDocument{ QJsonObject{
      { QStringLiteral("page"), static_cast<int>(page_index_)  },
      { QStringLiteral("pageSize"), static_cast<int>(page_size_)   },
      { QStringLiteral("orderId"), order_id_.isEmpty() ? UNDEFINED : order_id_ },
      { QStringLiteral("payStatus"), order_status_ == OrderStatus::ANY
                                          ? UNDEFINED : static_cast<int>(order_status_) },
      { QStringLiteral("begin"), begin_time_ == 0 ? UNDEFINED : static_cast<int>(begin_time_) },
      { QStringLiteral("end"), end_time_ == 0 ? UNDEFINED : static_cast<int>(end_time_) },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadPurchasedModelGroupListReuquest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/modelOrderListPage");
  }

  auto SyncHttpResponseDataToModel(const LoadPurchasedModelGroupListReuquest& request,
                                   ModelGroupInfoListModel&                   model) -> bool {
    const auto json = request.getResponseJson();
    if (json.empty()) {
      return false;
    }

    const auto order_list = json[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (order_list.empty()) {
      return false;
    }

    auto new_models = ModelGroupInfoListModel::DataContainer{};

    for (const auto& order : order_list) {
      if (!order.isObject()) {
        continue;
      }

      const auto group_uid = order[QStringLiteral("modelId")].toString();
      if (group_uid.isEmpty()) {
        continue;
      }

      auto info = model.load(group_uid);

      info.uid           = std::move(group_uid);
      info.name          = order[QStringLiteral("groupName")].toString();
      info.image         = order[QStringLiteral("covers")][0][QStringLiteral("url")].toString();
      info.restricted    =
          order[QStringLiteral("maturityRating")].toString() == QStringLiteral("restricted");
      info.price         = order[QStringLiteral("totalPrice")].toInt();
      info.model_count   = order[QStringLiteral("modelCount")].toInt();
      info.project_count = order[QStringLiteral("model3mfCount")].toInt();

      new_models.emplace_back(std::move(info));
    }

    model.emplaceBack(std::move(new_models));

    return true;
  }

// ---------- model service [end] ----------

}  // namespace cxcloud
