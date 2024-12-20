#include "cxcloud/service/model_library_service.h"

#include <optional>
#ifndef __APPLE__
#  include <filesystem>
#  include <fstream>
#endif

#include <QtCore/QFileInfo>
#include <QtQml/QQmlEngine>

#ifdef __APPLE__
#  include <boost/filesystem/fstream.hpp>
#  include <boost/filesystem/operations.hpp>
#  include <boost/filesystem/path.hpp>
#endif

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <qtusercore/module/systemutil.h>

namespace cxcloud {

  ModelLibraryService::ModelLibraryService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , category_model_{ std::make_unique<ModelGroupCategoryListModel>() }
      , random_model_{ std::make_unique<ModelGroupInfoListModel>() }
      , recommend_model_{ std::make_unique<ModelGroupInfoListModel>() }
      , history_model_{ std::make_unique<ModelGroupInfoListModel>() }
      , search_model_{ std::make_unique<ModelGroupInfoListModel>() }
      , focus_group_{ std::make_unique<ModelGroupInfoModel>() } {
    QQmlEngine::setObjectOwnership(focus_group_.get(), QQmlEngine::ObjectOwnership::CppOwnership);
  }

  auto ModelLibraryService::initialize() -> void {
    syncHistoryFromLocal();
    loadRandomModelGroupList(24);
    // loadModelGroupCategoryList();
  }

  auto ModelLibraryService::uninitialize() -> void {
    syncHistoryToLocal();
  }

  auto ModelLibraryService::setServerTypeGetter(std::function<ServerType(void)> getter) -> void {
    server_type_getter_ = getter;
  }

  auto ModelLibraryService::onServerTypeChanged() -> void {
    syncHistoryFromLocal();
  }

  auto ModelLibraryService::setModelGroupUrlCreater(
      std::function<QString(const QString&)> creater) -> void {
    model_group_url_creater_ = creater;
  }

  auto ModelLibraryService::getCacheDirPath() const -> QString {
    return cache_dir_path_;
  }

  auto ModelLibraryService::setCacheDirPath(const QString& path) -> void {
    cache_dir_path_ = path;
    history_file_path_ = QStringLiteral("%1/model_history.json").arg(getCacheDirPath());
  }

  auto ModelLibraryService::getHistoryFilePath() const -> QString {
    return history_file_path_;
  }

  auto ModelLibraryService::getModelGroupCategoryModelList() const -> QAbstractListModel* {
    return category_model_.get();
  }

  auto ModelLibraryService::clearModelGroupCategoryList() -> void {
    category_model_->clear();
  }

  auto ModelLibraryService::loadModelGroupCategoryList() -> void {
    auto request = createRequest<LoadModelGroupCategoryListRequest>();

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *category_model_);
      loadModelGroupCategoryListSuccessed(QVariant::fromValue<QString>(request->getResponseData()));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::getSearchModelGroupListModel() const -> QAbstractListModel* {
    return search_model_.get();
  }

  auto ModelLibraryService::clearSearchModelGroupList() -> void {
    search_model_->clear();
  }

  auto ModelLibraryService::searchModelGroup(const QString& keyword, int page, int size) -> void {
    if (keyword.isEmpty()) {
      search_model_->clear();
      searchModelGroupSuccessed(QVariant::fromValue(QStringLiteral("{}")),
                                QVariant::fromValue(page));
      return;
    }

    auto request = createRequest<SearchModelRequest>(keyword, page, size);

    request->setSuccessedCallback([this, request, page]() {
      SyncHttpResponseDataToModel(*request, *search_model_);
      searchModelGroupSuccessed(QVariant::fromValue<QString>(request->getResponseData()),
                                QVariant::fromValue(page));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::getHistoryModelGroupListModel() const -> QAbstractListModel* {
    return history_model_.get();
  }

  auto ModelLibraryService::clearHistoryModelGroupList() -> void {
    // history_model_->clear();
  }

  auto ModelLibraryService::pushHistoryModelGroup(const QString& uid,
                                                  const QString& name,
                                                  const QString& image,
                                                  unsigned int   price,
                                                  unsigned int   creation_time,
                                                  bool           collected,
                                                  const QString& author_name,
                                                  const QString& author_image,
                                                  const QString& author_id,
                                                  unsigned int   model_count) -> void {
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
        {},  // status
        collected,
        {},  // liked
        {},  // purchased
        {},  // restricted
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

      const auto index = std::distance(data_begin, iter);
      if (iter != data_begin) {
        std::rotate(data_begin, iter, iter + 1);
        history_model_->syncRawData(0, index);
      } else {
        history_model_->syncRawData(index, index);
      }
    }

    syncHistoryToLocal();
  }

  auto ModelLibraryService::loadHistoryModelGroupList(int page_index, int page_size) -> void {
    history_model_->setVaildSize(page_index * page_size);

    auto document = rapidjson::Document{ rapidjson::Type::kObjectType };

    auto& allocator = document.GetAllocator();

    auto group_list = rapidjson::Value{ rapidjson::Type::kArrayType };

    const size_t loaded_size = (page_index - 1) * page_size;
    const size_t total_size = history_model_->getSize();
    const auto begin = history_model_->rawData().cbegin() + (std::min)(loaded_size, total_size);

    const size_t unloaded_size = page_size;
    const size_t left_size = std::distance(begin, history_model_->rawData().cend());
    const auto end = begin + (std::min)(left_size, unloaded_size);

    for (auto iter = begin; iter != end; ++iter) {
      const auto& group_info = *iter;

      auto group = rapidjson::Value{ rapidjson::Type::kObjectType };

      auto uid = rapidjson::Value{ rapidjson::Type::kStringType };
      uid.Set(group_info.uid.toUtf8().constData(), allocator);
      group.AddMember("id", std::move(uid), allocator);

      auto name = rapidjson::Value{ rapidjson::Type::kStringType };
      name.Set(group_info.name.toUtf8().constData(), allocator);
      group.AddMember("name", std::move(name), allocator);

      auto image = rapidjson::Value{ rapidjson::Type::kStringType };
      image.Set(group_info.image.toUtf8().constData(), allocator);
      group.AddMember("image", std::move(image), allocator);

      auto price = rapidjson::Value{ rapidjson::Type::kNumberType };
      price.Set(static_cast<unsigned int>(group_info.price));
      group.AddMember("price", std::move(price), allocator);

      auto creation_time = rapidjson::Value{ rapidjson::Type::kNumberType };
      creation_time.Set(static_cast<unsigned int>(group_info.creation_time));
      group.AddMember("creation_time", std::move(creation_time), allocator);

      auto collected = rapidjson::Value{ group_info.collected ? rapidjson::Type::kTrueType
                                                              : rapidjson::Type::kFalseType };
      group.AddMember("collected", std::move(collected), allocator);

      auto author_name = rapidjson::Value{ rapidjson::Type::kStringType };
      author_name.Set(group_info.author_name.toUtf8().constData(), allocator);
      group.AddMember("author_name", std::move(author_name), allocator);

      auto author_image = rapidjson::Value{ rapidjson::Type::kStringType };
      author_image.Set(group_info.author_image.toUtf8().constData(), allocator);
      group.AddMember("author_image", std::move(author_image), allocator);

      auto author_id = rapidjson::Value{ rapidjson::Type::kStringType };
      author_id.Set(group_info.author_id.toUtf8().constData(), allocator);
      group.AddMember("author_id", std::move(author_id), allocator);

      auto model_count = rapidjson::Value{ rapidjson::Type::kNumberType };
      model_count.Set(static_cast<unsigned int>(group_info.model_count));
      group.AddMember("model_count", std::move(model_count), allocator);

      group_list.PushBack(std::move(group), allocator);
    }

    document.AddMember("group_list", group_list, allocator);

    auto buffer = rapidjson::StringBuffer{};
    auto writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>{ buffer };

    document.Accept(writer);

    auto json_bytes = QByteArray{ buffer.GetString(), static_cast<int>(buffer.GetSize()) };
    auto json_string = QString{ std::move(json_bytes) };
    loadHistoryModelGroupListSuccessed(QVariant::fromValue(std::move(json_string)),
                                       QVariant::fromValue(page_index));
  }

  auto ModelLibraryService::getRandomModelGroupListModel() const -> QAbstractListModel* {
    return random_model_.get();
  }

  auto ModelLibraryService::setRandomModelGroupListMaxSize(int size) -> void {
    random_model_->setMaxSize(static_cast<size_t>(size));
  }

  auto ModelLibraryService::clearRandomModelGroupList() -> void {
    random_model_->clear();
  }

  auto ModelLibraryService::loadRandomModelGroupList(int page_size) -> void {
    static bool Successed_Loaded = false;
    if (Successed_Loaded) { return; }

    auto request = createRequest<LoadRecommendModelGroupListRequest>(0, page_size);

    request->setSuccessedCallback([this, request]() {
      if (Successed_Loaded) { return; }
      Successed_Loaded = true;
      SyncHttpResponseDataToModel(*request, *random_model_);
    });

    request->asyncPost();
  }

  auto ModelLibraryService::getRecommendModelGroupListModel() const -> QAbstractListModel* {
    return recommend_model_.get();
  }

  auto ModelLibraryService::clearRecommendModelGroupList() -> void {
    recommend_model_->clear();
  }

  auto ModelLibraryService::loadRecommendModelGroupList(int page_index, int page_size) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    auto request = createRequest<LoadRecommendModelGroupListRequest>(page_index, page_size);

    request->setSuccessedCallback([this, request, page_index]() {
      SyncHttpResponseDataToModel(*request, *recommend_model_);
      loadRecommendModelGroupListSuccessed(
        QVariant::fromValue<QString>(request->getResponseData()), QVariant::fromValue(page_index));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::findOrMakeTypeModel(
      const QString& type_id) const -> std::unique_ptr<ModelGroupInfoListModel>& {
    auto iter = type_model_map_.find(type_id);
    if (iter == type_model_map_.cend()) {
      auto model = std::make_unique<ModelGroupInfoListModel>();
      QQmlEngine::setObjectOwnership(model.get(), QQmlEngine::ObjectOwnership::CppOwnership);
      iter = type_model_map_.emplace(std::make_pair(type_id, std::move(model))).first;
    }

    return iter->second;
  }

  auto ModelLibraryService::getTypeModelGroupListModel(
      const QString& type_id) const -> QAbstractListModel* {
    return findOrMakeTypeModel(type_id).get();
  }

  auto ModelLibraryService::clearTypeModelGroupList(const QString& type_id) -> void {
    auto iter = type_model_map_.find(type_id);
    if (iter == type_model_map_.cend()) {
      return;
    }

    return iter->second->clear();
  }

  auto ModelLibraryService::loadTypeModelGroupList(const QString& type_id,
                                                   int page_index,
                                                   int page_size,
                                                   int filter_type,
                                                   int pay_type) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    if (page_index == 1 && !last_type_model_group_uid_.isEmpty()) {
      last_type_model_group_uid_.clear();
    }

    using filter_t = LoadTypeModelGroupListRequest::FilterType;
    using pay_t = LoadTypeModelGroupListRequest::PayType;

    auto request = createRequest<LoadTypeModelGroupListRequest>(type_id,
                                                                page_index,
                                                                page_size,
                                                                static_cast<filter_t>(filter_type),
                                                                static_cast<pay_t>(pay_type),
                                                                last_type_model_group_uid_);

    request->setSuccessedCallback([this, request]() {
      auto& type_model = findOrMakeTypeModel(request->getTypeId());

      SyncHttpResponseDataToModel(*request, *type_model);
      loadTypeModelGroupListSuccessed(QVariant::fromValue<QString>(request->getResponseData()),
                                      QVariant::fromValue(request->getPageIndex()));

      const auto& raw_data = type_model->rawData();
      last_type_model_group_uid_ = !raw_data.empty() ? raw_data.back().uid : QString{};
    });

    request->asyncPost();
  }

  auto ModelLibraryService::getFocusModelGroup() const -> QObject* {
    return focus_group_.get();
  }

  auto ModelLibraryService::setFocusModelGroupUid(const QString& group_uid) -> void {
    focus_group_->models().lock()->clear();
    focus_group_->setUid(group_uid);

    if (group_uid.isEmpty()) {
      return;
    }

    auto info_request = createRequest<LoadModelGroupInfoRequest>(group_uid);

    info_request->setSuccessedCallback([this, info_request]() {
      SyncHttpResponseDataToModel(*info_request, *focus_group_);

      loadModelGroupInfoSuccessed(QVariant::fromValue<QString>(info_request->getResponseData()));

      auto file_request = createRequest<LoadModelGroupFileListInfoRequest>(
          focus_group_->getUid(), focus_group_->getModelCount());

      file_request->setSuccessedCallback([this, file_request]() {
        SyncHttpResponseDataToModel(*file_request, *focus_group_);

        loadModelGroupFileListInfoSuccessed(
            QVariant::fromValue<QString>(file_request->getResponseData()));
      });

      file_request->asyncPost();
    });

    info_request->asyncPost();
  }

  auto ModelLibraryService::loadModelGroupInfo(const QString& group_uid) -> void {
    auto request = createRequest<LoadModelGroupInfoRequest>(group_uid);

    request->setSuccessedCallback([this, request]() {
      loadModelGroupInfoSuccessed(QVariant::fromValue<QString>(request->getResponseData()));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::loadModelGroupFileListInfo(const QString& group_uid,
                                                       int count) -> void {
    auto request = createRequest<LoadModelGroupFileListInfoRequest>(group_uid, count);

    request->setSuccessedCallback([this, request, group_uid]() {
      loadModelGroupFileListInfoSuccessed(QVariant::fromValue<QString>(request->getResponseData()));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::collectModelGroup(const QString& group_uid, bool collect) -> void {
    auto request = createRequest<CollectModelGroupRequest>(group_uid, collect);

    request->setSuccessedCallback([this, request]() {
      const auto uid = request->getUid();
      const auto collect = request->isCollect();

      collect ? modelGroupCollected(QVariant::fromValue(uid))
              : modelGroupUncollected(QVariant::fromValue(uid));

      if (uid == focus_group_->getUid()) {
        focus_group_->setCollected(collect);
      }

      loadModelGroupInfo(uid);
    });

    request->asyncPost();
  }

  auto ModelLibraryService::likeModelGroup(const QString& group_uid, bool like) -> void {
    auto request = createRequest<LikeModelGroupRequest>(group_uid, like);

    request->setSuccessedCallback([this, request]() {
      const auto uid = request->getUid();
      const auto like = request->isLike();

      like ? modelGroupLiked(QVariant::fromValue(uid))
           : modelGroupUnliked(QVariant::fromValue(uid));

      if (uid == focus_group_->getUid()) {
        focus_group_->setLiked(like);
      }

      loadModelGroupInfo(uid);
    });

    request->asyncPost();
  }

  auto ModelLibraryService::shareModelGroup(const QString& group_uid) -> void {
    if (model_group_url_creater_) {
      qtuser_core::copyString2Clipboard(model_group_url_creater_(group_uid));
      shareModelGroupSuccessed();
    }

    auto request = createRequest<ShareModelGroupRequest>(group_uid);

    request->setSuccessedCallback([this, request]() {
      loadModelGroupInfo(request->getUid());
    });

    request->asyncPost();
  }

  auto ModelLibraryService::downloadModelGroup(const QString& group_uid) -> void {
    auto start_request = createRequest<DownloadModelGroupRequest>(group_uid);

    start_request->setSuccessedCallback([this, group_uid]() {
      auto finish_request = createRequest<DownloadModelGroupSuccessRequest>(group_uid);

      finish_request->setSuccessedCallback([this, group_uid]() {
        loadModelGroupInfo(group_uid);
      });

      finish_request->asyncPost();
    });

    start_request->asyncPost();
  }

  auto ModelLibraryService::uncollectModelGroup(const QString& group_uid) -> void {
    collectModelGroup(group_uid, false);
  }

  auto ModelLibraryService::unlikeModelGroup(const QString& group_uid) -> void {
    likeModelGroup(group_uid, false);
  }

  auto ModelLibraryService::loadModelGroupAuthorInfo(const QString& author_id) -> void {
    auto request = createRequest<LoadModelGroupAuthorInfoRequest>(author_id);

    request->setSuccessedCallback([this, request]() {
      loadModelGroupAuthorInfoSuccessed(QVariant::fromValue<QString>(request->getResponseData()));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::loadModelGroupAuthorModelList(const QString& user_id,
                                                          int page_index,
                                                          int page_size,
                                                          int filter_type,
                                                          const QString& keyword) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    auto request = createRequest<LoadModelGroupAuthorModelListRequest>(user_id,
                                                                       page_index,
                                                                       page_size,
                                                                       filter_type,
                                                                       keyword);

    request->setSuccessedCallback([this, request]() {
      loadModelGroupAuthorModelListSuccessed(
          QVariant::fromValue<QString>(request->getResponseData()));
    });

    request->asyncPost();
  }

  auto ModelLibraryService::createModelGroupUrl(const QString& group_uid) const -> QString {
    if (model_group_url_creater_) {
      return model_group_url_creater_(group_uid);
    }

    return {};
  }

  auto ModelLibraryService::getServerType() const -> ServerType {
    return server_type_getter_ ? server_type_getter_() : ServerType::MAINLAND_CHINA;
  }

// example of model_history.json in local disk:
// {
//   "server_list": [                           // unorderly
//     {
//       "type": 0,                             // ServerType
//       "group_list": [                        // orderly
//         {
//           "id": "645cadb3716e5ffc533139bb",  // ModelGroupInfo::uid
//           "name": "test_group",              // ModelGroupInfo::name
//           "image" : "xxx",                   // ModelGroupInfo::image
//           "price": 1000,                     // ModelGroupInfo::price
//           "author_name": "test_user",        // ModelGroupInfo::author_name
//           "author_image": "xxx"              // ModelGroupInfo::author_image
//           "author_id": "xxx"                 // ModelGroupInfo::author_id
//         },
//         ...
//       ]
//     },
//     ...
//   ]
// }

  namespace {

    inline auto ReadLocalHistoryJson(const QString& json_file_path)
        -> std::optional<rapidjson::Document> {
#ifdef __APPLE__
      const auto file_path = boost::filesystem::path{ json_file_path.toStdWString() };
      auto file_stream = boost::filesystem::ifstream{ file_path, std::ios::binary };
#else
      const auto file_path = std::filesystem::path{ json_file_path.toStdWString() };
      auto file_stream = std::ifstream{ file_path, std::ios::binary };
#endif
      if (!file_stream.is_open()) {
        return std::nullopt;
      }

      auto json_stream = rapidjson::IStreamWrapper{ file_stream };
      auto json_document = rapidjson::Document{};
      if (json_document.ParseStream(json_stream).HasParseError()) {
        return std::nullopt;
      }

      return json_document;
    }

    inline auto WriteLocalHistoryJson(const QString& json_file_path,
                                      const rapidjson::Document& json_document) -> bool {
#ifdef __APPLE__
      const auto file_path = boost::filesystem::path{ json_file_path.toStdWString() };
      boost::filesystem::create_directories(file_path.parent_path());
      auto file_stream = boost::filesystem::ofstream{ file_path, std::ios::binary };
#else
      const auto file_path = std::filesystem::path{ json_file_path.toStdWString() };
      std::filesystem::create_directories(file_path.parent_path());
      auto file_stream = std::ofstream{ file_path, std::ios::binary };
#endif
      if (!file_stream.is_open()) {
        return false;
      }

      auto json_stream = rapidjson::OStreamWrapper{ file_stream };
      auto json_writer = rapidjson::PrettyWriter<rapidjson::OStreamWrapper>{ json_stream };
#ifdef QT_DEBUG
      json_writer.SetIndent(' ', 2);
#endif // QT_DEBUG
      return json_document.Accept(json_writer);
    }

  }  // namespace

  auto ModelLibraryService::syncHistoryFromLocal() -> void {
    // read 'json document' from 'json file'

    auto json_document = ReadLocalHistoryJson(getHistoryFilePath());
    if (!json_document) {
      return;
    }

    auto& root = *json_document;
    if (!root.HasMember("server_list") || !root["server_list"].IsArray()) {
      return;
    }

    // create 'new model data' from 'json document' with 'model'

    using GroupListModel = decltype(history_model_)::element_type;
    using GroupList = GroupListModel::DataContainer;
    using Group = GroupList::value_type;

    auto group_list = GroupList{};
    const auto server_type = static_cast<int>(getServerType());
    for (const auto& server : root["server_list"].GetArray()) {
      if (!server.IsObject()) {
        continue;
      }

      if (!server.HasMember("type") || !server["type"].IsInt() ||
          server["type"].GetInt() != server_type) {
        continue;
      }

      if (!server.HasMember("group_list") || !server["group_list"].IsArray()) {
        continue;
      }

      for (const auto& group : server["group_list"].GetArray()) {
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

        group_list.emplace_back(Group{
          group["id"].GetString(),            // uid
          group["name"].GetString(),          // name
          group["image"].GetString(),         // image
          {},                                 // license
          group["price"].GetUint(),           // price
          group["creation_time"].GetUint(),   // creation_time
          {},                                 // status
          group["collected"].GetBool(),       // collected
          {},                                 // liked
          {},                                 // purchased
          {},                                 // restricted
          group["author_name"].GetString(),   // author_name
          group["author_image"].GetString(),  // author_image
          group["author_id"].GetString(),     // author_id
          group["model_count"].GetUint(),     // model_count
          {},                                 // models
        });
      }

      break;
    }

    // set 'new model data' into 'model'

    history_model_->clear();
    history_model_->pushBack(std::move(group_list));
  }

  auto ModelLibraryService::syncHistoryToLocal() const -> void {
    // read 'json document' from 'json file'

    const auto json_file_path = getHistoryFilePath();
    auto json_document = ReadLocalHistoryJson(json_file_path);
    if (!json_document) {
      return;
    }

    auto& allocator = json_document->GetAllocator();

    // create 'new json value' from 'model' with 'json document'

    auto group_list = rapidjson::Value{ rapidjson::Type::kArrayType };
    const auto server_type = static_cast<int>(getServerType());
    for (const auto& group_info : history_model_->rawData()) {
      auto group = rapidjson::Value{ rapidjson::Type::kObjectType };

      auto uid = rapidjson::Value{ rapidjson::Type::kStringType };
      uid.Set(group_info.uid.toUtf8().constData(), allocator);
      group.AddMember("id", std::move(uid), allocator);

      auto name = rapidjson::Value{ rapidjson::Type::kStringType };
      name.Set(group_info.name.toUtf8().constData(), allocator);
      group.AddMember("name", std::move(name), allocator);

      auto image = rapidjson::Value{ rapidjson::Type::kStringType };
      image.Set(group_info.image.toUtf8().constData(), allocator);
      group.AddMember("image", std::move(image), allocator);

      auto price = rapidjson::Value{ rapidjson::Type::kNumberType };
      price.Set(static_cast<unsigned int>(group_info.price));
      group.AddMember("price", std::move(price), allocator);

      auto creation_time = rapidjson::Value{ rapidjson::Type::kNumberType };
      creation_time.Set(static_cast<unsigned int>(group_info.creation_time));
      group.AddMember("creation_time", std::move(creation_time), allocator);

      auto collected = rapidjson::Value{ group_info.collected ? rapidjson::Type::kTrueType
                                                              : rapidjson::Type::kFalseType };
      group.AddMember("collected", std::move(collected), allocator);

      auto author_name = rapidjson::Value{ rapidjson::Type::kStringType };
      author_name.Set(group_info.author_name.toUtf8().constData(), allocator);
      group.AddMember("author_name", std::move(author_name), allocator);

      auto author_image = rapidjson::Value{ rapidjson::Type::kStringType };
      author_image.Set(group_info.author_image.toUtf8().constData(), allocator);
      group.AddMember("author_image", std::move(author_image), allocator);

      auto author_id = rapidjson::Value{ rapidjson::Type::kStringType };
      author_id.Set(group_info.author_id.toUtf8().constData(), allocator);
      group.AddMember("author_id", std::move(author_id), allocator);

      auto model_count = rapidjson::Value{ rapidjson::Type::kNumberType };
      model_count.Set(static_cast<unsigned int>(group_info.model_count));
      group.AddMember("model_count", std::move(model_count), allocator);

      group_list.PushBack(std::move(group), allocator);
    }

    // set 'new json value' into 'json document'

    auto& root = *json_document;
    if (!root.HasMember("server_list")) {
      root.AddMember("server_list", rapidjson::Value{ rapidjson::Type::kArrayType }, allocator);
    } else if (!root["server_list"].IsArray()) {
      root["server_list"].SetArray();
    }

    auto& server_list = root["server_list"];
    auto server_iter = server_list.Begin();
    for (; server_iter != server_list.End(); ++server_iter) {
      auto& server = *server_iter;
      if (!server.IsObject()) {
        continue;
      }

      if (!server.HasMember("type") || !server["type"].IsInt() ||
          server["type"].GetInt() != server_type) {
        continue;
      }

      if (!server.HasMember("group_list")) {
        server.AddMember("group_list", group_list.Move(), allocator);
        break;
      }

      if (!server["group_list"].IsArray()) {
        server["group_list"].SetArray();
      }

      server["group_list"].Swap(group_list.Move());
      break;
    }

    if (server_iter == server_list.End()) {
      auto server = rapidjson::Value{ rapidjson::Type::kObjectType };

      auto type = rapidjson::Value{ rapidjson::Type::kNumberType };
      type.Set(server_type);
      server.AddMember("type", type, allocator);

      server.AddMember("group_list", group_list.Move(), allocator);

      server_list.PushBack(server.Move(), allocator);
    }

    // write 'json document' into 'json file'

    WriteLocalHistoryJson(json_file_path, *json_document);
  }

}  // namespace cxcloud
