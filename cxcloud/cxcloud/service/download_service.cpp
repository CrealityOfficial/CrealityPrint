#include "cxcloud/service/download_service.h"

#ifndef __APPLE__
#  include <filesystem>
#  include <fstream>
#else
#  include <boost/filesystem/fstream.hpp>
#  include <boost/filesystem/operations.hpp>
#  include <boost/filesystem/path.hpp>
#endif

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <qtusercore/module/cxopenandsavefilemanager.h>
#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/model_request.h"
#include "cxcloud/tool/function.h"

namespace cxcloud {

  namespace filesystem {

#ifndef __APPLE__
    using ifstream = std::ifstream;
    using ofstream = std::ofstream;
    using namespace std::filesystem;
#else
    using namespace boost::filesystem;
#endif

  }  // namespace filesystem

  DownloadService::DownloadService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , download_thread_pool_{ std::make_shared<ThreadPool>() }
      , download_group_model_{ std::make_unique<GroupListModel>() } {}

  auto DownloadService::initialize() -> void {
    syncCacheFromLocal();
    resetExpiredModel();
  }

  auto DownloadService::uninitialize() -> void {
    resetExpiredModel();
    syncCacheToLocal();
  }

  auto DownloadService::getOpenFileHandler() const -> std::function<void(const QString&)> {
    return open_file_handler_;
  }

  auto DownloadService::setOpenFileHandler(std::function<void(const QString&)> handler) -> void {
    open_file_handler_ = handler;
  }

  auto DownloadService::getOpenFileListHandler() const -> std::function<void(const QStringList&)> {
    return open_file_list_handler_;
  }

  auto DownloadService::setOpenFileListHandler(std::function<void(const QStringList&)> handler)
      -> void {
    open_file_list_handler_ = handler;
  }

  auto DownloadService::getCacheDirPath() const -> QString {
    return cache_dir_path_;
  }

  auto DownloadService::setCacheDirPath(const QString& path) -> void {
    cache_dir_path_ = path;
  }

  auto DownloadService::getMaxDownloadThreadCount() const -> size_t {
    return max_download_thread_count_;
  }

  auto DownloadService::setMaxDownloadThreadCount(size_t count) -> void {
    download_thread_pool_      = std::make_shared<ThreadPool>(count);
    max_download_thread_count_ = count;
  }

  auto DownloadService::getDownloadTaskModel() const -> QAbstractListModel* {
    return download_group_model_.get();
  }

  auto DownloadService::checkModelDownloaded(const QString& group_uid,
                                             const QString& model_uid) const -> bool {
    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    if (!download_group_model_->load(group_uid).items->find(model_uid)) {
      return false;
    }

    return checkModelState(group_uid, model_uid, { Task::State::FINISHED });
  }

  auto DownloadService::checkModelGroupDownloaded(const QString& group_uid) const -> bool {
    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    return !checkModelGroupState(group_uid, { Task::State::UNREADY,
                                              Task::State::READY,
                                              Task::State::DOWNLOADING,
                                              Task::State::PAUSED,
                                              Task::State::FAILED, });
  }

  auto DownloadService::checkModelDownloading(const QString& group_uid,
                                              const QString& model_uid) const -> bool {
    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    if (!download_group_model_->load(group_uid).items->find(model_uid)) {
      return false;
    }

    return checkModelState(group_uid, model_uid, { Task::State::DOWNLOADING });
  }

  auto DownloadService::checkModelGroupDownloading(const QString& group_uid) const -> bool {
    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    return !checkModelGroupState(group_uid, { Task::State::UNREADY,
                                              Task::State::READY,
                                              Task::State::PAUSED,
                                              Task::State::FAILED,
                                              Task::State::FINISHED,
                                              Task::State::BROKEN, });
  }

  auto DownloadService::checkModelDownloadable(const QString& group_uid,
                                               const QString& model_uid) const -> bool {
    if (!download_group_model_->find(group_uid)) {
      return true;
    }

    if (!download_group_model_->load(group_uid).items->find(model_uid)) {
      return true;
    }

    return checkModelState(group_uid, model_uid, { Task::State::UNREADY,
                                                   Task::State::READY,
                                                   Task::State::PAUSED,
                                                   Task::State::FAILED, });
  }

  auto DownloadService::checkModelGroupDownloadable(const QString& group_uid) const -> bool {
    if (!download_group_model_->find(group_uid)) {
      return true;
    }

    return checkModelGroupState(group_uid, { Task::State::UNREADY,
                                             Task::State::READY,
                                             Task::State::PAUSED,
                                             Task::State::FAILED, });
  }

  auto DownloadService::checkModelImportLater(const QString& group_uid,
                                              const QString& model_uid) const -> bool {
    std::ignore = group_uid;

    const auto iter = model_task_map_.find(model_uid);
    if (iter == model_task_map_.cend()) {
      return false;
    }

    if (!iter->second.import_later) {
      return false;
    }

    return true;
  }

  auto DownloadService::checkModelGroupImportLater(const QString& group_uid) const -> bool {
    auto group = download_group_model_->load(group_uid);
    if (group.uid.isEmpty()) {
      return false;
    }

    if (!group.items) {
      return false;
    }

    for (const auto& model : group.items->rawData()) {
      if (model.uid.isEmpty()) {
        continue;
      }

      if (!checkModelImportLater(group.uid, model.uid)) {
        return false;
      }
    }

    return true;
  }

  auto DownloadService::markModelImportLater(const QString& group_uid,
                                             const QString& model_uid,
                                             bool           import_later) -> void {
    std::ignore = group_uid;

    const auto iter = model_task_map_.find(model_uid);
    if (iter == model_task_map_.cend()) {
      return;
    }

    iter->second.import_later = import_later;
  }

  auto DownloadService::markModelGroupImportLater(const QString& group_uid,
                                                  bool           import_later) -> void {
    auto group = download_group_model_->load(group_uid);
    if (group.uid.isEmpty()) {
      return;
    }

    if (!group.items) {
      return;
    }

    for (const auto& model : group.items->rawData()) {
      if (model.uid.isEmpty()) {
        continue;
      }

      markModelImportLater(group.uid, model.uid, import_later);
    }
  }

  auto DownloadService::deleteModelCache(const QVariantList& uid_map_list) -> void {
    auto deleted_any = bool{ false };

    for (const auto& uid_map_var : uid_map_list) {
      if (uid_map_var.type() != QVariant::Type::Map) {
        continue;
      }

      const auto uid_map       = uid_map_var.toMap();
      const auto group_uid_var = uid_map[QStringLiteral("group_uid")];
      const auto model_uid_var = uid_map[QStringLiteral("model_uid")];
      if (group_uid_var.type() != QVariant::Type::String ||
          model_uid_var.type() != QVariant::Type::String) {
        continue;
      }

      deleteModelCache(group_uid_var.toString(), model_uid_var.toString());
      deleted_any = true;
    }

    if (deleted_any) {
      syncCacheToLocal();
    }
  }

  auto DownloadService::deleteModelCache(const QString& group_uid,
                                         const QString& model_uid) -> void {
    if (!download_group_model_->find(group_uid)) {
      return;
    }

    const auto group_info = download_group_model_->load(group_uid);
    if (!group_info.items->find(model_uid)) {
      return;
    }

    auto model_info     = group_info.items->load(model_uid);
    model_info.state    = Task::State::UNREADY;
    model_info.progress = 0;
    group_info.items->update(std::move(model_info));

    QFile{ loadModelCachePath(group_uid, model_uid) }.remove();

    bool download_any{ false };
    for (const auto& other_model_info : group_info.items->rawData()) {
      if (other_model_info.state != Task::State::UNREADY) {
        download_any = true;
        break;
      }
    }

    if (!download_any) {
      QDir{}.rmdir(loadGroupCachePath(group_uid));
      download_group_model_->erase(group_uid);
    }
  }

  auto DownloadService::deleteModelGroupCache(const QString& group_uid) -> void {
    if (!download_group_model_->find(group_uid)) {
      return;
    }

    const auto group_info = download_group_model_->load(group_uid);
    if (group_info.uid.isEmpty()) {
      return;
    }

    QDir{ loadGroupCachePath(group_uid) }.removeRecursively();
  }

  auto DownloadService::importModelCache(const QVariantList& uid_map_list) -> void {
    auto file_list = QStringList{};

    for (const auto& uid_map_var : uid_map_list) {
      if (uid_map_var.type() != QVariant::Type::Map) {
        continue;
      }

      const auto uid_map       = uid_map_var.toMap();
      const auto group_uid_var = uid_map[QStringLiteral("group_uid")];
      const auto model_uid_var = uid_map[QStringLiteral("model_uid")];
      if (group_uid_var.type() != QVariant::Type::String ||
          model_uid_var.type() != QVariant::Type::String) {
        continue;
      }

      file_list.push_back(loadModelCachePath(group_uid_var.toString(), model_uid_var.toString()));
    }

    if (open_file_list_handler_ && !file_list.empty()) {
      open_file_list_handler_(file_list);
    }
  }

  auto DownloadService::importModelCache(const QString& group_uid,
                                         const QString& model_uid) -> void {
    if (open_file_handler_) {
      open_file_handler_(loadModelCachePath(group_uid, model_uid));
    }
  }

  auto DownloadService::importModelGroupCache(const QString& group_uid) -> void {
    if (!download_group_model_->find(group_uid)) {
      return;
    }

    const auto group_info = download_group_model_->load(group_uid);
    if (group_info.uid.isEmpty()) {
      return;
    }

    auto file_list = QStringList{};
    for (const auto& model_info : group_info.items->rawData()) {
      const auto model_uid = model_info.uid;
      if (model_info.uid.isEmpty()) {
        continue;
      }

      if (model_info.state == Task::State::FINISHED) {
        file_list.push_back(loadModelCachePath(group_uid, model_uid));
      }
    }

    if (open_file_list_handler_ && !file_list.empty()) {
      open_file_list_handler_(file_list);
    }
  }

  auto DownloadService::exportModelCache(const QString& group_uid,
                                         const QString& model_uid,
                                         const QString& dir_path) -> bool {
    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    const auto group_info = download_group_model_->load(group_uid);
    if (!group_info.items->find(model_uid)) {
      return false;
    }

    auto model_info = group_info.items->load(model_uid);
    ReplaceIllegalCharacters(model_info.name);

    const auto src_path = loadModelCachePath(group_uid, model_uid);
    const auto dst_path = QStringLiteral("%1/%2%3").arg(
        QUrl{ dir_path }.toLocalFile(), std::move(model_info.name), std::move(model_info.format));

    return qtuser_core::copyFile(src_path, dst_path, true);
  }

  auto DownloadService::openModelCacheDir() const -> void {
    auto path = loadGroupCacheDirPath();

    if (!QFileInfo{ path }.exists()) {
      QDir{}.mkpath(path);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
  }

  auto DownloadService::makeModelGroupDownloadPromise(const QString& group_uid,
                                                      bool           import_later) -> void {
    promise_list_.emplace_back(Promise{
        true,          // entire_group
        import_later,  // import_later
        group_uid,     // group_uid
    });
  }

  auto DownloadService::makeModelDownloadPromise(const QString& group_uid,
                                                 const QString& model_uid,
                                                 bool           import_later) -> void {
    promise_list_.emplace_back(Promise{
        false,         // entire_group
        import_later,  // import_later
        group_uid,     // group_uid
        model_uid,     // model_uid
    });
  }

  auto DownloadService::fulfillsAllDonwloadPromise() -> void {
    for (const auto& promise : promise_list_) {
      if (promise.entire_group) {
        startModelGroupDownloadTask(promise.group_uid, promise.import_later);
      } else {
        startModelDownloadTask(promise.group_uid, promise.model_uid, promise.import_later);
      }
    }

    promise_list_.clear();
  }

  auto DownloadService::breachAllDonwloadPromise() -> void {
    promise_list_.clear();
  }

  auto DownloadService::startModelGroupDownloadTask(const QString& group_uid,
                                                    bool           import_later) -> void {
    // update model data : init group data if not exist
    if (!download_group_model_->find(group_uid)) {
      download_group_model_->pushFront(Group{
        group_uid,                           // uid
        {},                                  // name
        {},                                  // local_name
        std::make_shared<ModelListModel>(),  // items
      });
    }

    auto group_info = download_group_model_->load(group_uid);

    // check the group info, and request for it if not got
    if (group_info.name.isEmpty()) {
      startDownloadInfoRequest(group_uid, true, import_later);
      return;
    }

    // if the group info allready got, try to download each model
    if (!group_info.items->rawData().empty()) {
      for (const auto& model_info : group_info.items->rawData()) {
        // if the model is no need to download again, skip it
        if (model_info.state == Task::State::DOWNLOADING ||
            model_info.state == Task::State::FINISHED ||
            model_info.state == Task::State::BROKEN) {
          continue;
        }

        const auto& model_uid = model_info.uid;

        // check the model download task, and start it if not started
        if (model_task_map_.find(model_uid) == model_task_map_.cend()) {
          startDownloadTaskRequest(group_uid, model_uid, import_later);
        }
      }
    }
  }

  auto DownloadService::startModelDownloadTask(const QString& group_uid,
                                               const QString& model_uid,
                                               bool           import_later) -> void {
    // update model data : init group data if not exist
    if (!download_group_model_->find(group_uid)) {
      download_group_model_->pushFront(Group{
        group_uid,                           // uid
        {},                                  // name
        {},                                  // local_name
        std::make_shared<ModelListModel>(),  // items
      });
    }

    // update model data : set the model's state to Task::State::READY
    // @see startDownloadInfoRequest
    auto group_info = download_group_model_->load(group_uid);
    if (!group_info.items->find(model_uid)) {
      group_info.items->pushFront(Model{
        model_uid,           // uid
        {},                  // name
        {},                  // format
        {},                  // local_name
        {},                  // image
        0,                   // size
        {},                  // date
        0,                   // speed
        Task::State::READY,  // state
        0,                   // progress
      });

    } else {
      // if it's no need to download again, skip it
      auto model_info = group_info.items->load(model_uid);
      if (model_info.state == Task::State::DOWNLOADING ||
          model_info.state == Task::State::FINISHED ||
          model_info.state == Task::State::BROKEN) {
        return;
      }

      model_info.state = Task::State::READY;
    }

    // check the group info, and request for it if not got
    if (group_info.name.isEmpty()) {
      startDownloadInfoRequest(group_uid, false, import_later);
      return;
    }

    // check the model download task, and start it if not started
    if (model_task_map_.find(model_uid) == model_task_map_.cend()) {
      startDownloadTaskRequest(group_uid, model_uid, import_later);
    }
  }

  auto DownloadService::cancelModelDownloadTask(const QString& group_uid,
                                                const QString& model_uid) -> void {
    std::ignore = group_uid;

    const auto iter = model_task_map_.find(model_uid);
    if (iter != model_task_map_.cend() &&
        iter->second.request &&
        !iter->second.request->finished()) {
      iter->second.request->setCancelDownloadLater(true);
    }
  }

  auto DownloadService::loadGroupCacheName(const QString& group_uid) const -> QString {
    const auto group_info = download_group_model_->load(group_uid);
    return QStringLiteral("%1").arg(group_info.local_name);
  }

  auto DownloadService::loadModelCacheName(const QString& group_uid,
                                           const QString& model_uid) const -> QString {
    const auto group_info = download_group_model_->load(group_uid);
    if (group_info.uid.isEmpty()) {
      return {};
    }

    const auto model_info = group_info.items->load(model_uid);
    if (model_info.uid.isEmpty()) {
      return {};
    }

    return QStringLiteral("%1/%2%3").arg(
        group_info.local_name, model_info.local_name, model_info.format);
  }

  auto DownloadService::loadGroupCacheDirPath() const -> QString {
    return QStringLiteral("%1/cache").arg(getCacheDirPath());
  }

  auto DownloadService::loadModelCacheDirPath(const QString& group_uid) const -> QString {
    return QStringLiteral("%1/%2").arg(loadGroupCacheDirPath(), loadGroupCacheName(group_uid));
  }

  auto DownloadService::loadGroupCachePath(const QString& group_uid) const -> QString {
    return loadModelCacheDirPath(group_uid);
  }

  auto DownloadService::loadModelCachePath(const QString& group_uid,
                                           const QString& model_uid) const -> QString {
    return QStringLiteral("%1/%2").arg(loadGroupCacheDirPath(),
                                       loadModelCacheName(group_uid, model_uid));
  }

  auto DownloadService::loadCacheInfoPath() const -> QString {
    return QStringLiteral("%1/cache_info.json").arg(getCacheDirPath());
  }

  // example of cache_info.json in local disk:
  // {
  //   "group_list": [
  //     {
  //       "id": "645cadb3716e5ffc533139bb",
  //       "name": "test_model_group???",
  //       "local_name": "test_model_group____1"
  //       "model_list": [
  //         {
  //           "id": "645cadb3716e5ffc533139be",
  //           "name": "test_///model_1",
  //           "local_name": "test____model_1",
  //           "format": ".stl",
  //           "image": "xxxx",
  //           "size": 792484,
  //           "date": "2023-06-26",
  //           "state": 4
  //         },
  //         ......
  //       ]
  //     },
  //     ......
  //   ]
  // }

  auto DownloadService::syncCacheFromLocal() -> void {
    // read file data, parse json format
    const auto file_path   = filesystem::path{ loadCacheInfoPath().toStdWString() };
    auto       file_stream = filesystem::ifstream{ file_path, std::ios::binary };
    if (!file_stream.is_open()) {
      return;
    }

    auto json_stream   = rapidjson::IStreamWrapper{ file_stream };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return;
    }

    // read info from local cache to model

    auto& root = json_document;
    if (!root.HasMember("group_list") || !root["group_list"].IsArray()) {
      return;
    }

    auto group_list = GroupList{};
    for (const auto& group : root["group_list"].GetArray()) {
      if (!group.IsObject()) {
        continue;
      }

      if (!group.HasMember("id") || !group["id"].IsString() ||
          !group.HasMember("name") || !group["name"].IsString() ||
          !group.HasMember("local_name") || !group["local_name"].IsString() ||
          !group.HasMember("model_list") || !group["model_list"].IsArray()) {
        continue;
      }

      // models info in group

      auto model_list = ModelList{};
      for (const auto& model : group["model_list"].GetArray()) {
        if (!model.IsObject()) {
          continue;
        }

        if (!model.HasMember("id") || !model["id"].IsString() ||
            !model.HasMember("name") || !model["name"].IsString() ||
            !model.HasMember("format") || !model["format"].IsString() ||
            !model.HasMember("local_name") || !model["local_name"].IsString() ||
            !model.HasMember("image") || !model["image"].IsString() ||
            !model.HasMember("size") || !model["size"].IsUint() ||
            !model.HasMember("date") || !model["date"].IsString()) {
          continue;
        }

        auto state = Model::State::UNREADY;

        // adapt old data
        if (model.HasMember("state") && model["state"].IsInt()) {
          state = static_cast<Model::State>(model["state"].GetInt());
        } else if (model.HasMember("downloaded") && model["downloaded"].IsBool()) {
          const auto downloaded = model["downloaded"].GetBool();
          state                 = downloaded ? Task::State::FINISHED : Task::State::UNREADY;
        }

        model_list.emplace_back(Model{
          model["id"].GetString(),                                                 // uid
          model["name"].GetString(),                                               // name
          model["format"].GetString(),                                             // format
          model["local_name"].GetString(),                                         // local_name
          model["image"].GetString(),                                              // image
          model["size"].GetUint(),                                                 // size
          state == Task::State::FINISHED ? model["date"].GetString() : QString{},  // date
          0,                                                                       // speed
          state,                                                                   // state
          0,                                                                       // progress
        });
      }

      group_list.emplace_back(Group{
        group["id"].GetString(),                                  // uid
        group["name"].GetString(),                                // name
        group["local_name"].GetString(),                          // local_name
        std::make_shared<ModelListModel>(std::move(model_list)),  // items
      });
    }

    download_group_model_->clear();
    while (!group_list.empty()) {
      download_group_model_->pushBack(std::move(group_list.front()));
      group_list.pop_front();
    }
  }

  auto DownloadService::syncCacheToLocal() const -> void {
    // read info from model to json

    auto json_document = rapidjson::Document{ rapidjson::Type::kObjectType };

    auto& allocator = json_document.GetAllocator();
    auto& root      = json_document;

    auto group_list = rapidjson::Value{ rapidjson::Type::kArrayType };
    for (const auto& group_info : download_group_model_->rawData()) {
      const auto& model_info_list = group_info.items->rawData();
      if (model_info_list.empty()) {
        continue;
      }

      // if none of model has been downloaded in this group, skip it

      bool downloaded_any{ false };
      for (const auto& model_info : model_info_list) {
        if (model_info.state == Task::State::FINISHED) {
          downloaded_any = true;
          break;
        }
      }

      if (!downloaded_any) {
        continue;
      }

      // group info

      auto group = rapidjson::Value{ rapidjson::Type::kObjectType };

      auto group_uid = rapidjson::Value{ rapidjson::Type::kStringType };
      group_uid.Set(group_info.uid.toUtf8().constData(), allocator);
      group.AddMember("id", std::move(group_uid), allocator);

      auto group_name = rapidjson::Value{ rapidjson::Type::kStringType };
      group_name.Set(group_info.name.toUtf8().constData(), allocator);
      group.AddMember("name", std::move(group_name), allocator);

      auto group_local_name = rapidjson::Value{ rapidjson::Type::kStringType };
      group_local_name.Set(group_info.local_name.toUtf8().constData(), allocator);
      group.AddMember("local_name", std::move(group_local_name), allocator);

      auto model_list = rapidjson::Value{ rapidjson::Type::kArrayType };
      for (const auto& model_info : model_info_list) {
        // models info in group

        auto model = rapidjson::Value{ rapidjson::Type::kObjectType };

        auto model_uid = rapidjson::Value{ rapidjson::Type::kStringType };
        model_uid.Set(model_info.uid.toUtf8().constData(), allocator);
        model.AddMember("id", std::move(model_uid), allocator);

        auto model_name = rapidjson::Value{ rapidjson::Type::kStringType };
        model_name.Set(model_info.name.toUtf8().constData(), allocator);
        model.AddMember("name", std::move(model_name), allocator);

        auto model_local_name = rapidjson::Value{ rapidjson::Type::kStringType };
        model_local_name.Set(model_info.local_name.toUtf8().constData(), allocator);
        model.AddMember("local_name", std::move(model_local_name), allocator);

        auto model_format = rapidjson::Value{ rapidjson::Type::kStringType };
        model_format.Set(model_info.format.toUtf8().constData(), allocator);
        model.AddMember("format", std::move(model_format), allocator);

        auto model_image = rapidjson::Value{ rapidjson::Type::kStringType };
        model_image.Set(model_info.image.toUtf8().constData(), allocator);
        model.AddMember("image", std::move(model_image), allocator);

        auto model_size = rapidjson::Value{ rapidjson::Type::kNumberType };
        model_size.Set(static_cast<unsigned int>(model_info.size));
        model.AddMember("size", std::move(model_size), allocator);

        auto model_date = rapidjson::Value{ rapidjson::Type::kStringType };
        model_date.Set(model_info.date.toUtf8().constData(), allocator);
        model.AddMember("date", std::move(model_date), allocator);

        auto model_state = rapidjson::Value{ rapidjson::Type::kNumberType };
        model_state.Set(static_cast<int>(model_info.state));
        model.AddMember("state", std::move(model_state), allocator);

        model_list.PushBack(std::move(model), allocator);
      }

      group.AddMember("model_list", std::move(model_list), allocator);

      group_list.PushBack(std::move(group), allocator);
    }

    root.AddMember("group_list", group_list, allocator);

    // write file data
    const auto file_path = filesystem::path{ loadCacheInfoPath().toStdWString() };
    filesystem::create_directories(file_path.parent_path());
    auto file_stream = filesystem::ofstream{ file_path, std::ios::binary };
    if (!file_stream.is_open()) {
      return;
    }

    auto json_stream = rapidjson::OStreamWrapper{ file_stream };
    auto json_writer = rapidjson::PrettyWriter<rapidjson::OStreamWrapper>{ json_stream };
#if defined(QT_DEBUG)
    json_writer.SetIndent(' ', 2);
#endif
    json_document.Accept(json_writer);
  }

  auto DownloadService::resetExpiredModel() const -> void {
    if (download_group_model_->isEmpty()) {
      return;
    }

    auto expired_group_uid_list = std::list<QString>{};
    for (auto& group_info : download_group_model_->rawData()) {
      auto downloaded_any = false;

      for (auto& model_info : group_info.items->rawData()) {
        // only check the models which downloaded by us
        if (model_info.state != Task::State::FINISHED) {
          continue;
        }

        if (!QFileInfo{ loadModelCachePath(group_info.uid, model_info.uid) }.exists()) {
          model_info.state = Task::State::UNREADY;
          continue;
        }

        downloaded_any = true;
      }

      if (!downloaded_any) {
        expired_group_uid_list.emplace_back(group_info.uid);
      }
    }

    for (const auto& group_uid : expired_group_uid_list) {
      download_group_model_->erase(group_uid);
    }

    return;
  }

  auto DownloadService::checkModelGroupState(const QString&         group_uid,
                                             std::list<Task::State> states) const -> bool {
    if (group_uid.isEmpty() || states.empty()) {
      return false;
    }

    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    const auto group_info = download_group_model_->load(group_uid);
    for (const auto& model_info : group_info.items->rawData()) {
      for (const auto& state : states) {
        if (model_info.state == state) {
          return true;
        }
      }
    }

    return false;
  }

  auto DownloadService::checkModelState(const QString&         group_uid,
                                        const QString&         model_uid,
                                        std::list<Task::State> states) const -> bool {
    if (states.empty() || group_uid.isEmpty() || model_uid.isEmpty()) {
      return false;
    }

    if (!download_group_model_->find(group_uid)) {
      return false;
    }

    const auto group_info = download_group_model_->load(group_uid);
    if (!group_info.items->find(model_uid)) {
      return false;
    }

    const auto item_state = group_info.items->load(model_uid).state;

    auto found_any = false;
    for (const auto& state : states) {
      if (item_state == state) {
        found_any = true;
        break;
      }
    }

    return found_any;
  }

  auto DownloadService::startDownloadInfoRequest(const QString& group_uid,
                                                 bool           download_all_later,
                                                 bool           import_later) -> void {
    auto info_request = createRequest<LoadModelGroupInfoRequest>(group_uid);

    info_request->setSuccessedCallback([this,
                                        info_request,
                                        group_uid,
                                        download_all_later,
                                        import_later]() {
      ModelGroupInfoModel info;
      SyncHttpResponseDataToModel(*info_request, info);
      auto group_info = download_group_model_->load(group_uid);
      group_info << info;

      auto group_local_name = group_info.name;
      ReplaceIllegalCharacters(group_local_name);
      ConvertNonRepeatingPath(loadGroupCacheDirPath(), group_local_name);
      group_info.local_name = std::move(group_local_name);
      download_group_model_->update(std::move(group_info));

      auto file_request = createRequest<LoadModelGroupFileListInfoRequest>(group_uid,
                                                                           info.getModelCount());
      file_request->setSuccessedCallback([this,
                                          file_request,
                                          group_uid,
                                          download_all_later,
                                          import_later]() mutable {
        auto info = ModelGroupInfoModel{};
        SyncHttpResponseDataToModel(*file_request, info);

        auto group_info = download_group_model_->load(group_uid);
        group_info << *info.models().lock();

        auto download_item_model = group_info.items;
        download_group_model_->update(std::move(group_info));

        for (auto& model_info : download_item_model->rawData()) {
          auto model_local_name = model_info.name;
          ReplaceIllegalCharacters(model_local_name);
          ConvertNonRepeatingPath(loadModelCacheDirPath(group_uid), model_local_name);
          model_info.local_name = std::move(model_local_name);

          if (!download_all_later && model_info.state != Task::State::READY) {
            continue;
          }

          if (model_info.state == Task::State::DOWNLOADING ||
              model_info.state == Task::State::FINISHED ||
              model_info.state == Task::State::BROKEN) {
            continue;
          }

          const auto& model_uid = model_info.uid;
          if (model_task_map_.find(model_uid) != model_task_map_.cend()) {
            continue;
          }

          if (model_info.state != Task::State::READY) {
            auto new_model_info  = model_info;
            new_model_info.state = Task::State::READY;
            download_item_model->update(std::move(new_model_info));
          }

          startDownloadTaskRequest(group_uid, model_uid, import_later);
        }
      });

      file_request->asyncPost();
    });

    info_request->asyncPost();
  }

  auto DownloadService::startDownloadTaskRequest(const QString& group_uid,
                                                 const QString& model_uid,
                                                 bool           import_later) -> void {
    auto request = std::make_unique<UnlimitedDownloadModelRequest>(
        group_uid, model_uid, loadModelCachePath(group_uid, model_uid), download_thread_pool_);

    getHttpSession().lock()->addRequest(request.get());

    connect(request.get(), &Task::Request::progressUpdated,
            this, &DownloadService::onDownloadProgressUpdated,
            Qt::ConnectionType::QueuedConnection);

    connect(request.get(), &Task::Request::downloadFinished,
            this, &DownloadService::onDownloadFinished,
            Qt::ConnectionType::QueuedConnection);

    request->asyncPost();

    model_task_map_.emplace(model_uid, Task{
      std::move(request),  // request
      import_later,        // import_later
    });
  }

  auto DownloadService::onDownloadProgressUpdated(const QString& group_uid,
                                                  const QString& model_uid) -> void {
    const auto iter = model_task_map_.find(model_uid);
    if (iter == model_task_map_.cend() || !download_group_model_->find(group_uid)) {
      return;
    }

    auto items = download_group_model_->load(group_uid).items;
    if (!items->find(model_uid)) {
      return;
    }

    SyncHttpResponseDataToModel(*(iter->second.request), *items);
  }

  auto DownloadService::onDownloadFinished(const QString& group_uid,
                                           const QString& model_uid) -> void {
    auto iter = model_task_map_.find(model_uid);
    if (iter == model_task_map_.cend()) {
      return;
    }

    const auto succssed    = iter->second.request->getResponseState() == HttpErrorCode::OK;
    const auto finished    = !iter->second.request->isCancelDownloadLater();
    const auto need_import = iter->second.import_later;

    if (download_group_model_->find(group_uid)) {
      auto models = download_group_model_->load(group_uid).items;
      if (models->find(model_uid)) {
        auto model  = models->load(model_uid);
        model.state = succssed && finished ? Task::State::FINISHED : Task::State::UNREADY;
        models->update(std::move(model));
      }
    }

    model_task_map_.erase(iter);

    if (!succssed || !finished) {
      deleteModelCache(group_uid, model_uid);
    } else if (need_import) {
      importModelCache(group_uid, model_uid);
    }

    syncCacheToLocal();
  }

}  // namespace cxcloud
