#include "cxcloud/service/model_library_service.h"

#include <QtCore/QFileInfo>
#include <QtQml/QQmlEngine>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stream.h>
#include <rapidjson/prettywriter.h>

#include <qtusercore/module/systemutil.h>

#include "cxcloud/net/model_request.h"
#include "cxcloud/tool/function.h"

namespace cxcloud {

ModelLibraryService::ModelLibraryService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : BaseService(http_session, parent)
    , category_model_(std::make_unique<ModelGroupCategoryListModel>())
    , random_model_(std::make_unique<ModelGroupInfoListModel>())
    , recommend_model_(std::make_unique<ModelGroupInfoListModel>())
    , history_model_(std::make_unique<ModelGroupInfoListModel>())
    , search_model_(std::make_unique<ModelGroupInfoListModel>())
    , focus_group_(std::make_unique<ModelGroupInfoModel>()) {
  QQmlEngine::setObjectOwnership(focus_group_.get(), QQmlEngine::ObjectOwnership::CppOwnership);
}

void ModelLibraryService::initialize() {
  syncHistoryFromLocal();
  loadRandomModelGroupList(24);
  // loadModelGroupCategoryList();
}

void ModelLibraryService::uninitialize() {
  syncHistoryToLocal();
}

void ModelLibraryService::setModelGroupUrlCreater(std::function<QString(const QString&)> creater) {
  model_group_url_creater_ = creater;
}

QString ModelLibraryService::getCacheDirPath() const {
  return cache_dir_path_;
}

void ModelLibraryService::setCacheDirPath(const QString& path) {
  cache_dir_path_ = path;
  history_file_path_ = QStringLiteral("%1/model_history.json").arg(getCacheDirPath());
}

QString ModelLibraryService::getHistoryFilePath() const {
  return history_file_path_;
}

QAbstractListModel* ModelLibraryService::getModelGroupCategoryModelList() {
  return category_model_.get();
}

void ModelLibraryService::clearModelGroupCategoryList() {
  category_model_->clear();
}

void ModelLibraryService::loadModelGroupCategoryList() {
  auto* request = createRequest<LoadModelGroupCategoryListRequest>();

  request->setSuccessedCallback([this, request]() {
    SyncHttpResponseDataToModel(*request, *category_model_);
    Q_EMIT loadModelGroupCategoryListSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()));
  });

  HttpPost(request);
}

QAbstractListModel* ModelLibraryService::getSearchModelGroupListModel() const {
  return search_model_.get();
}

void ModelLibraryService::clearSearchModelGroupList() {
  search_model_->clear();
}

void ModelLibraryService::searchModelGroup(const QString& keyword, int page, int size) {
  auto* request = createRequest<SearchModelRequest>(keyword, page, size);
  request->setSuccessedCallback([this, request, page]() {
    SyncHttpResponseDataToModel(*request, *search_model_);
    Q_EMIT searchModelGroupSuccessed(QVariant::fromValue<QString>(request->getResponseData()),
                                     QVariant::fromValue(page));
  });
  HttpPost(request);
}

QAbstractListModel* ModelLibraryService::getHistoryModelGroupListModel() const {
  return history_model_.get();
}

void ModelLibraryService::clearHistoryModelGroupList() {
  // history_model_->clear();
}

void ModelLibraryService::pushHistoryModelGroup(const QString& uid,
                                                const QString& name,
                                                const QString& image,
                                                unsigned int   price,
                                                unsigned int   creation_time,
                                                bool           collected,
                                                const QString& author_name,
                                                const QString& author_image,
                                                const QString& author_id,
                                                unsigned int   model_count) {
  auto& data = history_model_->rawData();
  auto data_begin = data.begin();
  auto data_end = data.end();

  auto iter = std::find_if(data_begin, data_end, [&uid](const auto& info) {
    return info.uid == uid;
  });

  if (iter == data_end) {
    history_model_->pushFront({
      uid,
      name,
      image,
      {},  // licence
      price,
      creation_time,
      collected,
      {},  // liked
      author_name,
      author_image,
      author_id,
      model_count,
      {},  // models
    });

  } else {
    // iter->uid = uid;
    iter->name = name;
    iter->image = image;
    iter->price = price;
    iter->author_name = author_name;
    iter->author_image = author_image;
    iter->author_id = author_id;

    auto const index = std::distance(data_begin, iter);
    if (iter != data_begin) {
      std::rotate(data_begin, iter, iter + 1);
      history_model_->syncRawData(0, index);
    } else {
      history_model_->syncRawData(index, index);
    }
  }

  syncHistoryToLocal();
}

void ModelLibraryService::loadHistoryModelGroupList(int page_index, int page_size) {
  history_model_->setVaildSize(page_index * page_size);

  rapidjson::Document document;
  document.SetObject();

  auto& allocator = document.GetAllocator();

  rapidjson::Value group_list;
  group_list.SetArray();

  const size_t loaded_size = (page_index - 1) * page_size;
  const size_t total_size = history_model_->getSize();
  const auto begin = history_model_->rawData().cbegin() + (std::min)(loaded_size, total_size);

  const size_t unloaded_size = page_size;
  const size_t left_size = std::distance(begin, history_model_->rawData().cend());
  const auto end = begin + (std::min)(left_size, unloaded_size);

  for (auto iter = begin; iter != end; ++iter) {
    const auto& group_info = *iter;

    rapidjson::Value group;
    group.SetObject();

    rapidjson::Value id;
    id.SetString(group_info.uid.toUtf8(), allocator);
    group.AddMember("id", std::move(id), allocator);

    rapidjson::Value name;
    name.SetString(group_info.name.toUtf8(), allocator);
    group.AddMember("name", std::move(name), allocator);

    rapidjson::Value image;
    image.SetString(group_info.image.toUtf8(), allocator);
    group.AddMember("image", std::move(image), allocator);

    rapidjson::Value price;
    price.SetUint(group_info.price);
    group.AddMember("price", std::move(price), allocator);

    rapidjson::Value creation_time;
    creation_time.SetUint(group_info.creation_time);
    group.AddMember("creation_time", std::move(creation_time), allocator);

    rapidjson::Value collected;
    collected.SetBool(group_info.collected);
    group.AddMember("collected", std::move(collected), allocator);

    rapidjson::Value author_name;
    author_name.SetString(group_info.author_name.toUtf8(), allocator);
    group.AddMember("author_name", std::move(author_name), allocator);

    rapidjson::Value author_image;
    author_image.SetString(group_info.author_image.toUtf8(), allocator);
    group.AddMember("author_image", std::move(author_image), allocator);

    rapidjson::Value author_id;
    author_id.SetString(group_info.author_id.toUtf8(), allocator);
    group.AddMember("author_id", std::move(author_id), allocator);

    rapidjson::Value model_count;
    model_count.SetUint(group_info.model_count);
    group.AddMember("model_count", std::move(model_count), allocator);

    group_list.PushBack(std::move(group), allocator);
  }

  document.AddMember("group_list", group_list, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer{ buffer };

  document.Accept(writer);

  QString json_string{ QByteArray{ buffer.GetString(), static_cast<int>(buffer.GetSize()) } };
  Q_EMIT loadHistoryModelGroupListSuccessed(QVariant::fromValue(std::move(json_string)),
                                            QVariant::fromValue(page_index));
}

QAbstractListModel* ModelLibraryService::getRandomModelGroupListModel() const {
  return random_model_.get();
}

void ModelLibraryService::setRandomModelGroupListMaxSize(int size) {
  random_model_->setMaxSize(static_cast<size_t>(size));
}

void ModelLibraryService::clearRandomModelGroupList() {
  random_model_->clear();
}

void ModelLibraryService::loadRandomModelGroupList(int page_size) {
  static bool Successed_Loaded = false;
  if (Successed_Loaded) { return; }

  auto* request{ createRequest<LoadRecommendModelGroupListRequest>(0, page_size) };

  request->setSuccessedCallback([this, request, page_size]() {
    Successed_Loaded = true;

    SyncHttpResponseDataToModel(*request, *random_model_);

    Q_EMIT loadRandomModelGroupListSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()), QVariant::fromValue(page_size));
  });

  HttpPost(request);
}

QAbstractListModel* ModelLibraryService::getRecommendModelGroupListModel() const {
  return recommend_model_.get();
}

void ModelLibraryService::clearRecommendModelGroupList() {
  recommend_model_->clear();
}

void ModelLibraryService::loadRecommendModelGroupList(int page_index, int page_size) {
  auto* request = createRequest<LoadRecommendModelGroupListRequest>(page_index, page_size);

  request->setSuccessedCallback([this, request, page_index]() {
    SyncHttpResponseDataToModel(*request, *recommend_model_);
    Q_EMIT loadRecommendModelGroupListSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()), QVariant::fromValue(page_index));
  });

  HttpPost(request);
}

std::unique_ptr<ModelGroupInfoListModel>& ModelLibraryService::findOrMakeTypeModel(
    const QString& type_id) const {
  auto iter = type_model_map_.find(type_id);
  if (iter == type_model_map_.cend()) {
    auto model = std::make_unique<ModelGroupInfoListModel>();
    QQmlEngine::setObjectOwnership(model.get(), QQmlEngine::ObjectOwnership::CppOwnership);
    type_model_map_.insert(std::make_pair(type_id, std::move(model)));

    iter = type_model_map_.find(type_id);
  }

  return iter->second;
}

QAbstractListModel* ModelLibraryService::getTypeModelGroupListModel(const QString& type_id) const {
  return findOrMakeTypeModel(type_id).get();
}

void ModelLibraryService::clearTypeModelGroupList(const QString& type_id) {
  auto iter = type_model_map_.find(type_id);
  if (iter == type_model_map_.cend()) { return; }
  return iter->second->clear();
}

void ModelLibraryService::loadTypeModelGroupList(const QString& type_id,
                                                 int page_index,
                                                 int page_size,
                                                 int filter_id,
                                                 int pay_type
    ) {
  if (page_index == 1 && !last_type_model_group_uid_.isEmpty()) {
    last_type_model_group_uid_.clear();
  }

  using filter_t = LoadTypeModelGroupListRequest::FilterType;
  static std::map<int, filter_t> enumMap = {
        {1, filter_t::MOST_LIKE},
        {2, filter_t::MOST_COLLECT},
        {3, filter_t::NEWEST_UPLOAD},
        {4, filter_t::MOST_COMMENTS},
        {5, filter_t::MOST_BUY},
        {6, filter_t::MOST_PRINT},
        {7, filter_t::MOST_DOWNLOAD},
        {8, filter_t::MOST_POPULAR}
  };
  auto it = enumMap.find(filter_id);

  using pay_t = LoadTypeModelGroupListRequest::PayType;
  static std::map<int, pay_t> enumPay = {
        {0, pay_t::MODEL_ALL},
        {1, pay_t::MODEL_TOLL},
        {2, pay_t::MODEL_FREE}
  };
  auto pt = enumPay.find(pay_type);

  auto* request = createRequest<LoadTypeModelGroupListRequest>(type_id,
                                                               page_index,
                                                               page_size,
                                                             //  filter_t::NEWEST_UPLOAD,
                                                               it->second,
                                                               pt->second,
                                                               last_type_model_group_uid_);

  request->setSuccessedCallback([this, request]() {
    auto& type_model = findOrMakeTypeModel(request->getTypeId());

    SyncHttpResponseDataToModel(*request, *type_model);
    Q_EMIT loadTypeModelGroupListSuccessed(QVariant::fromValue<QString>(request->getResponseData()),
                                           QVariant::fromValue(request->getPageIndex()));

    last_type_model_group_uid_ = type_model->rawData().back().uid;
  });

  HttpPost(request);
}

QObject* ModelLibraryService::getFocusModelGroup() const {
  return focus_group_.get();
}

void ModelLibraryService::setFocusModelGroupUid(const QString& group_uid) {
  focus_group_->models().lock()->clear();
  focus_group_->setUid(group_uid);

  if (group_uid.isEmpty()) { return; }

  auto* info_request = createRequest<LoadModelGroupInfoRequest>(group_uid);

  info_request->setSuccessedCallback([this, info_request]() {
    SyncHttpResponseDataToModel(*info_request, *focus_group_);

    Q_EMIT loadModelGroupInfoSuccessed(
      QVariant::fromValue<QString>(info_request->getResponseData()));

    auto* file_request = createRequest<LoadModelGroupFileListInfoRequest>(
      focus_group_->getUid(), focus_group_->getModelCount());

    file_request->setSuccessedCallback([this, file_request]() {
      SyncHttpResponseDataToModel(*file_request, *focus_group_);

      Q_EMIT loadModelGroupFileListInfoSuccessed(
        QVariant::fromValue<QString>(file_request->getResponseData()));
    });

    HttpPost(file_request);
  });

  HttpPost(info_request);
}

void ModelLibraryService::loadModelGroupInfo(const QString& group_uid) {
  auto* request = createRequest<LoadModelGroupInfoRequest>(group_uid);

  request->setSuccessedCallback([this, request]() {
    Q_EMIT loadModelGroupInfoSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()));
  });

  HttpPost(request);
}

void ModelLibraryService::loadModelGroupFileListInfo(const QString& group_uid, int count) {
  auto* request = createRequest<LoadModelGroupFileListInfoRequest>(group_uid, count);

  request->setSuccessedCallback([this, request, group_uid]() {
    Q_EMIT loadModelGroupFileListInfoSuccessed(
      QVariant::fromValue<QString>(request->getResponseData()));
  });

  HttpPost(request);
}

void ModelLibraryService::collectModelGroup(const QString& group_uid, bool collect) {
  auto* request = createRequest<CollectModelGroupRequest>(group_uid, collect);

   request->setSuccessedCallback([this, request]() {
    const auto uid = request->getUid();
    const auto collect = request->isCollect();

    collect ? Q_EMIT modelGroupCollected(QVariant::fromValue(uid))
            : Q_EMIT modelGroupUncollected(QVariant::fromValue(uid));

    if (uid == focus_group_->getUid()) {
      focus_group_->setCollected(collect);
    }

    loadModelGroupInfo(uid);
  });

  HttpPost(request);
}

void ModelLibraryService::likeModelGroup(const QString& group_uid, bool like) {
    auto* request = createRequest<LikeModelGroupRequest>(group_uid, like);

    request->setSuccessedCallback([this, request]() {
        const auto uid = request->getUid();
        const auto like = request->isLike();

        like ? Q_EMIT modelGroupLiked(QVariant::fromValue(uid))
            : Q_EMIT modelGroupUnliked(QVariant::fromValue(uid));

        if (uid == focus_group_->getUid()) {
            focus_group_->setLiked(like);
        }

        loadModelGroupInfo(uid);
        });

    HttpPost(request);
}

void ModelLibraryService::uncollectModelGroup(const QString& group_uid) {
  collectModelGroup(group_uid, false);
}

void ModelLibraryService::unlikeModelGroup(const QString& group_uid) {
    likeModelGroup(group_uid, false);
}

void ModelLibraryService::loadModelGroupAuthorInfo(const QString& author_id) {
    auto* request = createRequest<LoadModelGroupAuthorInfoRequest>(author_id);

    request->setSuccessedCallback([this, request]() {
        Q_EMIT loadModelGroupAuthorInfoSuccessed(
            QVariant::fromValue<QString>(request->getResponseData()));
        });

    HttpPost(request);
}

void ModelLibraryService::loadModelGroupAuthorModelList(const QString& user_id, int page_index, int page_size, int filter_type, const QString& keyword) {
    auto* request = createRequest<LoadModelGroupAuthorModelListRequest>(user_id, page_index, page_size, filter_type, keyword);
    request->setSuccessedCallback([this, request]() {
        Q_EMIT loadModelGroupAuthorModelListSuccessed(
            QVariant::fromValue<QString>(request->getResponseData()));
        });

    HttpPost(request);
}

QString ModelLibraryService::createModelGroupUrl(const QString& group_uid) const {
  if (model_group_url_creater_) {
    return model_group_url_creater_(group_uid);
  }

  return {};
}

void ModelLibraryService::shareModelGroup(const QString& group_uid) {
  if (model_group_url_creater_) {
    qtuser_core::copyString2Clipboard(model_group_url_creater_(group_uid));
    Q_EMIT shareModelGroupSuccessed();
  }
}

// example of model_history.json in local disk:
// {
//   "group_list": [  // orderly
//     {
//       "id": "645cadb3716e5ffc533139bb",
//       "name": "test_group",
//       "image" : "xxx",
//       "price": 1000,
//       "author_name": "test_user",
//       "author_image": "xxx"
//     },
//     ...
//   ]
// }

void ModelLibraryService::syncHistoryFromLocal() {
  // read file data, parse json format

  std::ifstream ifstream { getHistoryFilePath().toStdString(), std::ios::in | std::ios::binary };
  rapidjson::IStreamWrapper wrapper{ ifstream };
  rapidjson::Document document;
  if (document.ParseStream(wrapper).HasParseError()) {
    return;
  }

  history_model_->clear();

  // read info from local cache to model

  if (!document.HasMember("group_list") || !document["group_list"].IsArray()) {
    return;
  }

  for (const auto& group : document["group_list"].GetArray()) {
    if (!group.IsObject()) {
      continue;
    }

    if (!group.HasMember("id") || !group["id"].IsString() ||
        !group.HasMember("name") || !group["name"].IsString() ||
        !group.HasMember("image") || !group["image"].IsString() ||
        !group.HasMember("price") || !group["price"].IsUint() ||
        !group.HasMember("creation_time") || !group["creation_time"].IsUint() ||
        !group.HasMember("collected") || !group["collected"].IsBool() ||
        !group.HasMember("author_name") || !group["author_name"].IsString() ||
        !group.HasMember("author_image") || !group["author_image"].IsString() ||
        !group.HasMember("author_id") || !group["author_id"].IsString() ||
        !group.HasMember("model_count") || !group["model_count"].IsUint()) {
      continue;
    }

    history_model_->pushBack({
      group["id"].GetString(),
      group["name"].GetString(),
      group["image"].GetString(),
      {},  // license
      group["price"].GetUint(),
      group["creation_time"].GetUint(),
      group["collected"].GetBool(),
      {},  // liked
      group["author_name"].GetString(),
      group["author_image"].GetString(),
      group["author_id"].GetString(),
      group["model_count"].GetUint(),
      {},  // models
    });
  }
}

void ModelLibraryService::syncHistoryToLocal() const {
  // read info from model to json

  rapidjson::Document document;
  document.SetObject();

  auto& allocator = document.GetAllocator();

  rapidjson::Value group_list;
  group_list.SetArray();

  for (const auto& group_info : history_model_->rawData()) {
    rapidjson::Value group;
    group.SetObject();

    rapidjson::Value id;
    id.SetString(group_info.uid.toUtf8(), allocator);
    group.AddMember("id", std::move(id), allocator);

    rapidjson::Value name;
    name.SetString(group_info.name.toUtf8(), allocator);
    group.AddMember("name", std::move(name), allocator);

    rapidjson::Value image;
    image.SetString(group_info.image.toUtf8(), allocator);
    group.AddMember("image", std::move(image), allocator);

    rapidjson::Value price;
    price.SetUint(group_info.price);
    group.AddMember("price", std::move(price), allocator);

    rapidjson::Value creation_time;
    creation_time.SetUint(group_info.creation_time);
    group.AddMember("creation_time", std::move(creation_time), allocator);

    rapidjson::Value collected;
    collected.SetBool(group_info.collected);
    group.AddMember("collected", std::move(collected), allocator);

    rapidjson::Value author_name;
    author_name.SetString(group_info.author_name.toUtf8(), allocator);
    group.AddMember("author_name", std::move(author_name), allocator);

    rapidjson::Value author_image;
    author_image.SetString(group_info.author_image.toUtf8(), allocator);
    group.AddMember("author_image", std::move(author_image), allocator);

    rapidjson::Value author_id;
    author_id.SetString(group_info.author_id.toUtf8(), allocator);
    group.AddMember("author_id", std::move(author_id), allocator);

    rapidjson::Value model_count;
    model_count.SetUint(group_info.model_count);
    group.AddMember("model_count", std::move(model_count), allocator);

    group_list.PushBack(std::move(group), allocator);
  }

  document.AddMember("group_list", group_list, allocator);

  // write file data

  std::ofstream ofstream{ getHistoryFilePath().toStdString(), std::ios::out | std::ios::binary };
  rapidjson::OStreamWrapper wrapper{ ofstream };
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer{ wrapper };

#ifdef QT_DEBUG
  writer.SetIndent(' ', 2);
#endif // QT_DEBUG

  document.Accept(writer);
}

}  // namespace cxcloud
