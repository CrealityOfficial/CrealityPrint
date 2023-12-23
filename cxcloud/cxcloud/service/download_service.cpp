#include "cxcloud/service/download_service.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <qtusercore/module/cxopenandsavefilemanager.h>
#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/tool/function.h"

namespace cxcloud {

namespace {

using GroupInfo = DownloadGroupListModel::Data;
using ItemInfo = DownloadItemListModel::Data;
using ItemState = ItemInfo::State;

} // namespace

DownloadService::DownloadService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : BaseService(http_session, parent)
    , download_thread_pool_(std::make_shared<boost::asio::thread_pool>())
    , download_group_model_(std::make_unique<DownloadGroupListModel>()) {}

void DownloadService::initialize() {
  syncCacheFromLocal();
  resetExpiredModel();
}

void DownloadService::uninitialize() {
  resetExpiredModel();
  syncCacheToLocal();
}

std::function<void(const QString&)> DownloadService::getOpenFileHandler() const {
  return open_file_handler_;
}

void DownloadService::setOpenFileHandler(std::function<void(const QString&)> handler) {
  open_file_handler_ = handler;
}

std::function<void(const QStringList&)> DownloadService::getOpenFileListHandler() const {
  return open_file_list_handler_;
}

void DownloadService::setOpenFileListHandler(std::function<void(const QStringList&)> handler) {
  open_file_list_handler_ = handler;
}

QString DownloadService::getCacheDirPath() const {
  return cache_dir_path_;
}

void DownloadService::setCacheDirPath(const QString& path) {
  cache_dir_path_ = path;
}

size_t DownloadService::getMaxDownloadThreadCount() const {
  return max_download_thread_count_;
}

void DownloadService::setMaxDownloadThreadCount(size_t count) {
  download_thread_pool_ = std::make_shared<boost::asio::thread_pool>(count);
  max_download_thread_count_ = count;
}

bool DownloadService::checkModelDownloaded(const QString& group_id, const QString& model_id) {
  if (!download_group_model_->find(group_id)) {
    return false;
  }

  if (!download_group_model_->load(group_id).items->find(model_id)) {
    return false;
  }

  return checkModelState(group_id, model_id, { ItemState::FINISHED });
}

bool DownloadService::checkModelGroupDownloaded(const QString& group_id) {
  if (!download_group_model_->find(group_id)) {
    return false;
  }

  return !checkModelGroupState(group_id, { ItemState::UNREADY,
                                           ItemState::READY,
                                           ItemState::DOWNLOADING,
                                           ItemState::PAUSED,
                                           ItemState::FAILED, });
}

bool DownloadService::checkModelDownloading(const QString& group_id, const QString& model_id) {
  if (!download_group_model_->find(group_id)) {
    return false;
  }

  if (!download_group_model_->load(group_id).items->find(model_id)) {
    return false;
  }

  return checkModelState(group_id, model_id, { ItemState::DOWNLOADING });
}

bool DownloadService::checkModelGroupDownloading(const QString& group_id) {
  if (!download_group_model_->find(group_id)) {
    return false;
  }

  return !checkModelGroupState(group_id, { ItemState::UNREADY,
                                           ItemState::READY,
                                           ItemState::PAUSED,
                                           ItemState::FAILED,
                                           ItemState::FINISHED, });
}

bool DownloadService::checkModelDownloadable(const QString& group_id, const QString& model_id) {
  if (!download_group_model_->find(group_id)) {
    return true;
  }

  if (!download_group_model_->load(group_id).items->find(model_id)) {
    return true;
  }

  return checkModelState(group_id, model_id, { ItemState::UNREADY,
                                               ItemState::READY,
                                               ItemState::PAUSED,
                                               ItemState::FAILED, });
}

bool DownloadService::checkModelGroupDownloadable(const QString& group_id) {
  if (!download_group_model_->find(group_id)) {
    return true;
  }

  return checkModelGroupState(group_id, { ItemState::UNREADY,
                                          ItemState::READY,
                                          ItemState::PAUSED,
                                          ItemState::FAILED, });
}

void DownloadService::deleteModelCache(const QStringList& group_model_list) {
  // group_model_list : { "group_id#model_id", ... }
  // @see DownloadManageDialog.qml

  static auto const DELIMITER = QStringLiteral("#");

  bool deleted_any{ false };
  for (const auto& group_model_id : group_model_list) {
    if (!group_model_id.contains(DELIMITER)) {
      continue;
    }

    const auto id_list = group_model_id.split(DELIMITER);
    if (id_list.size() != 2) {
      continue;
    }

    const auto& group_id = id_list.at(0);
    const auto& model_id = id_list.at(1);

    deleteModelCache(group_id, model_id);
    deleted_any = true;
  }

  if (deleted_any) {
    syncCacheToLocal();
  }
}

void DownloadService::deleteModelCache(const QString& group_id, const QString& model_id) {
  if (!download_group_model_->find(group_id)) {
    return;
  }

  auto const group_info = download_group_model_->load(group_id);
  if (!group_info.items->find(model_id)) {
    return;
  }

  auto model_info = group_info.items->load(model_id);
  model_info.state = ItemState::UNREADY;
  model_info.progress = 0;
  group_info.items->update(std::move(model_info));

  QFile{ loadModelCachePath(group_id, model_id) }.remove();

  bool download_any{ false };
  for (auto const& other_model_info : group_info.items->rawData()) {
    if (other_model_info.state != ItemState::UNREADY) {
      download_any = true;
      break;
    }
  }

  if (!download_any) {
    QDir{}.rmdir(loadGroupCachePath(group_id));
    download_group_model_->erase(group_id);
  }
}

void DownloadService::importModelCache(const QStringList& group_model_list) {
  // group_model_list : { "group_id#model_id", ... }
  // @see DownloadManageDialog.qml

  static const auto DELIMITER = QStringLiteral("#");

  QStringList file_list;

  for (const auto& group_model_id : group_model_list) {
    if (!group_model_id.contains(DELIMITER)) {
      continue;
    }

    const auto id_list = group_model_id.split(DELIMITER);
    if (id_list.size() != 2) {
      continue;
    }

    const auto& group_id = id_list.at(0);
    const auto& model_id = id_list.at(1);

    file_list.push_back(loadModelCachePath(group_id, model_id));
  }

  if (open_file_list_handler_) { open_file_list_handler_(file_list); }
}

void DownloadService::importModelCache(const QString& group_id, const QString& model_id) {
  if (open_file_handler_) {
    open_file_handler_(loadModelCachePath(group_id, model_id));
  }
}

bool DownloadService::exportModelCache(const QString& group_id,
                                       const QString& model_id,
                                       const QString& dir_path) {
  if (!download_group_model_->find(group_id)) {
    return false;
  }

  const auto group_info = download_group_model_->load(group_id);
  if (!group_info.items->find(model_id)) {
    return false;
  }

  auto file_name = group_info.items->load(model_id).name;
  ReplaceIllegalCharacters(file_name);

  const auto src_path = loadModelCachePath(group_id, model_id);
  const auto dst_path = QStringLiteral("%1/%2.stl").arg(QUrl{ dir_path }.toLocalFile())
                                                   .arg(std::move(file_name));

  return qtuser_core::copyFile(src_path, dst_path, true);
}

void DownloadService::openModelCacheDir() const {
  auto path = loadGroupCacheDirPath();
  if (!QFileInfo{ path }.exists()) {
    QDir{}.mkpath(path);
  }
  // ¼æÈÝMac
  QUrl url = QUrl::fromLocalFile(path);
  QDesktopServices::openUrl(url);
}

void DownloadService::makeModelGroupDownloadPromise(const QString& group_id) {
  promise_list_.emplace_back(Promise{
    true,
    group_id,
  });
}

void DownloadService::makeModelDownloadPromise(const QString& group_id, const QString& model_id) {
  promise_list_.emplace_back(Promise{
    false,
    group_id,
    model_id,
  });
}

void DownloadService::fulfillsAllDonwloadPromise() {
  for (const auto& promise : promise_list_) {
    if (promise.is_group) {
      startModelGroupDownloadTask(promise.group_id);
    } else {
      startModelDownloadTask(promise.group_id, promise.model_id);
    }
  }

  promise_list_.clear();
}

void DownloadService::breachAllDonwloadPromise() {
  promise_list_.clear();
}

void DownloadService::startModelGroupDownloadTask(const QString& group_id, bool is_model_import) {
  // update model data : init group data if not exist
  if (!download_group_model_->find(group_id)) {
    download_group_model_->pushFront(GroupInfo{
      group_id,
      {},
      {},
      std::make_shared<DownloadItemListModel>(),
    });
  }

  auto group_info = download_group_model_->load(group_id);

  // check the group info, and request for it if not got
  if (group_info.name.isEmpty()) {
    startDownloadInfoRequest(group_id, true, is_model_import);
    return;
  }

  // if the group info allready got, try to download each model
  if (!group_info.items->rawData().empty()) {
    for (const auto& model_info : group_info.items->rawData()) {
      // if the model is downloading or downloaded now, skip it
      if (model_info.state == ItemState::DOWNLOADING ||
          model_info.state == ItemState::FINISHED) {
        continue;
      }

      const auto model_id = model_info.uid;

      // check the model download task, and start it if not started
      if (task_request_map_.find(model_id) == task_request_map_.cend()) {
        startDownloadTaskRequest(group_id, model_id, is_model_import);
      }
    }
  }
}

void DownloadService::startModelDownloadTask(const QString& group_id,
                                             const QString& model_id,
                                             bool is_model_import
        ) {
  // update model data : init group data if not exist
  if (!download_group_model_->find(group_id)) {
    download_group_model_->pushFront(GroupInfo{
      group_id,
      {},
      {},
      std::make_shared<DownloadItemListModel>(),
    });
  }

  // update model data : set the model's state to ItemState::READY
  auto group_info = download_group_model_->load(group_id);
  if (!group_info.items->find(model_id)) {
    group_info.items->pushFront(ItemInfo{
      model_id,
      {},
      {},
      {},
      0,
      {},
      0,
      ItemState::READY,
      0,
    });

  } else {
    // if it's downloading or downloaded now, skip it
    auto model_info = group_info.items->load(model_id);
    if (model_info.state == ItemState::DOWNLOADING ||
        model_info.state == ItemState::FINISHED) {
      return;
    }

    model_info.state = ItemState::READY;
  }

  // check the group info, and request for it if not got
  if (group_info.name.isEmpty()) {
    startDownloadInfoRequest(group_id, false, is_model_import);
    return;
  }

  // check the model download task, and start it if not started
  if (task_request_map_.find(model_id) == task_request_map_.cend()) {
    startDownloadTaskRequest(group_id, model_id, is_model_import);
  }
}

void DownloadService::cancelModelDownloadTask(const QString& group_id,
                                              const QString& model_id) {
  const auto iter = task_request_map_.find(model_id);
  if (iter != task_request_map_.cend() && iter->second && !iter->second->finished()) {
    iter->second->setCancelDownloadLater(true);
  }
}

QAbstractListModel* DownloadService::getDownloadTaskModel() const {
  return download_group_model_.get();
}

QString DownloadService::loadGroupCacheName(const QString& group_id) const {
  auto const group_info = download_group_model_->load(group_id);
  return QStringLiteral("%1").arg(group_info.local_name);
}

QString DownloadService::loadModelCacheName(const QString& group_id,
                                            const QString& model_id) const {
  auto const group_info = download_group_model_->load(group_id);
  auto const model_info = group_info.items->load(model_id);
  return QStringLiteral("%1/%2").arg(group_info.local_name).arg(model_info.local_name);
}

QString DownloadService::loadGroupCacheDirPath() const {
   return QStringLiteral("%1/cache").arg(getCacheDirPath());
}

QString DownloadService::loadModelCacheDirPath(const QString& group_id) const {
  return QStringLiteral("%1/%2").arg(loadGroupCacheDirPath()).arg(loadGroupCacheName(group_id));
}

QString DownloadService::loadGroupCachePath(const QString& group_id) const {
  return loadModelCacheDirPath(group_id);
}

QString DownloadService::loadModelCachePath(const QString& group_id,
                                            const QString& model_id) const {
  return QStringLiteral("%1/%2.stl").arg(loadGroupCacheDirPath())
                                    .arg(loadModelCacheName(group_id, model_id));
}

QString DownloadService::loadCacheInfoPath() const {
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
//           "image": "xxxx",
//           "size": 792484,
//           "date": "2023-06-26",
//           "downloaded": true
//         },
//         ......
//       ]
//     },
//     ......
//   ]
// }

void DownloadService::syncCacheFromLocal() {
  // read file data, parse json format

  std::ifstream ifstream{ loadCacheInfoPath().toStdString(), std::ios::in | std::ios::binary };
  rapidjson::IStreamWrapper wrapper{ ifstream };
  rapidjson::Document document;
  if (document.ParseStream(wrapper).HasParseError()) {
    return;
  }

  // reset model

  download_group_model_->clear();

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
        !group.HasMember("local_name") || !group["local_name"].IsString() ||
        !group.HasMember("model_list") || !group["model_list"].IsArray()) {
      continue;
    }

    // group info

    const auto download_item_model = std::make_shared<DownloadItemListModel>();
    download_group_model_->pushBack({
      group["id"].GetString(),
      group["name"].GetString(),
      group["local_name"].GetString(),
      download_item_model
    });

    // models info in group

    for (const auto& model : group["model_list"].GetArray()) {
      if (!model.IsObject()) {
        continue;
      }

      if (!model.HasMember("id") || !model["id"].IsString() ||
          !model.HasMember("name") || !model["name"].IsString() ||
          !model.HasMember("local_name") || !model["local_name"].IsString() ||
          !model.HasMember("image") || !model["image"].IsString() ||
          !model.HasMember("size") || !model["size"].IsUint() ||
          !model.HasMember("date") || !model["date"].IsString() ||
          !model.HasMember("downloaded") || !model["downloaded"].IsBool()) {
        continue;
      }

      download_item_model->pushBack({
        model["id"].GetString(),
        model["name"].GetString(),
        model["local_name"].GetString(),
        model["image"].GetString(),
        model["size"].GetUint(),
        model["downloaded"].GetBool() ? model["date"].GetString() : QString{},
        0,
        model["downloaded"].GetBool() ? ItemState::FINISHED : ItemState::UNREADY,
        0
      });
    }
  }
}

void DownloadService::syncCacheToLocal() const {
  // read info from model to json

  rapidjson::Document document;
  document.SetObject();

  auto& allocator = document.GetAllocator();

  rapidjson::Value group_list;
  group_list.SetArray();

  for (const auto& group_info : download_group_model_->rawData()) {
    const auto& model_info_list = group_info.items->rawData();
    if (model_info_list.empty()) {
      continue;
    }

    // if none of model has been downloaded in this group, skip it

    bool downloaded_any{ false };
    for (const auto& model_info : model_info_list) {
      if (model_info.state == ItemState::FINISHED) {
        downloaded_any = true;
        break;
      }
    }

    if (!downloaded_any) {
      continue;
    }

    // group info

    rapidjson::Value group;
    group.SetObject();

    rapidjson::Value group_id;
    group_id.SetString(group_info.uid.toUtf8(), allocator);
    group.AddMember("id", std::move(group_id), allocator);

    rapidjson::Value group_name;
    group_name.SetString(group_info.name.toUtf8(), allocator);
    group.AddMember("name", std::move(group_name), allocator);

    rapidjson::Value group_local_name;
    group_local_name.SetString(group_info.local_name.toUtf8(), allocator);
    group.AddMember("local_name", std::move(group_local_name), allocator);

    rapidjson::Value model_list;
    model_list.SetArray();

    for (const auto& model_info : model_info_list) {
      // models info in group

      rapidjson::Value model;
      model.SetObject();

      rapidjson::Value model_id;
      model_id.SetString(model_info.uid.toUtf8(), allocator);
      model.AddMember("id", std::move(model_id), allocator);

      rapidjson::Value model_name;
      model_name.SetString(model_info.name.toUtf8(), allocator);
      model.AddMember("name", std::move(model_name), allocator);

      rapidjson::Value model_local_name;
      model_local_name.SetString(model_info.local_name.toUtf8(), allocator);
      model.AddMember("local_name", std::move(model_local_name), allocator);

      rapidjson::Value model_image;
      model_image.SetString(model_info.image.toUtf8(), allocator);
      model.AddMember("image", std::move(model_image), allocator);

      rapidjson::Value model_size;
      model_size.SetUint(model_info.size);
      model.AddMember("size", std::move(model_size), allocator);

      rapidjson::Value model_date;
      model_date.SetString(model_info.date.toUtf8(), allocator);
      model.AddMember("date", std::move(model_date), allocator);

      rapidjson::Value model_downloaded;
      model_downloaded.SetBool(model_info.state == ItemState::FINISHED);
      model.AddMember("downloaded", std::move(model_downloaded), allocator);

      model_list.PushBack(std::move(model), allocator);
    }

    group.AddMember("model_list", std::move(model_list), allocator);

    group_list.PushBack(std::move(group), allocator);
  }

  document.AddMember("group_list", group_list, allocator);

  // write file data

  std::ofstream ofstream{ loadCacheInfoPath().toStdString(), std::ios::out | std::ios::binary };
  rapidjson::OStreamWrapper wrapper{ ofstream };
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer{ wrapper };

#ifdef QT_DEBUG
  writer.SetIndent(' ', 2);
#endif // QT_DEBUG

  document.Accept(writer);
}

void DownloadService::resetExpiredModel() const {
  if (download_group_model_->isEmpty()) {
    return;
  }

  std::list<QString> expired_group_id_list;
  for (auto& group_info : download_group_model_->rawData()) {
    auto downloaded_any = false;

    for (auto& model_info : group_info.items->rawData()) {
      // only check the models which downloaded by us
      if (model_info.state != ItemState::FINISHED) {
        continue;
      }

      if (!QFileInfo{ loadModelCachePath(group_info.uid, model_info.uid) }.exists()) {
        model_info.state = ItemState::UNREADY;
        continue;
      }

      downloaded_any = true;
    }

    if (!downloaded_any) {
      expired_group_id_list.emplace_back(group_info.uid);
    }
  }

  for (auto const& group_id : expired_group_id_list) {
    download_group_model_->erase(group_id);
  }

  return;
}

bool DownloadService::checkModelGroupState(const QString& group_id,
                                           std::list<ItemState> states) {
  if (group_id.isEmpty() || states.empty()) {
    return false;
  }

  if (!download_group_model_->find(group_id)) {
    return false;
  }

  const auto group_info = download_group_model_->load(group_id);
  for (const auto& model_info : group_info.items->rawData()) {
    for (const auto& state : states) {
      if (model_info.state == state) {
        return true;
      }
    }
  }

  return false;
}

bool DownloadService::checkModelState(const QString& group_id,
                                      const QString& model_id,
                                      std::list<ItemState> states) {
  if (states.empty() || group_id.isEmpty() || model_id.isEmpty()) {
    return false;
  }

  if (!download_group_model_->find(group_id)) {
    return false;
  }

  const auto group_info = download_group_model_->load(group_id);
  if (!group_info.items->find(model_id)) {
    return false;
  }

  const auto item_state = group_info.items->load(model_id).state;

  for (const auto& state : states) {
    if (item_state == state) {
      return true;
    }
  }

  return false;
}

void DownloadService::onDownloadProgressUpdated(const QString& group_id,
                                                const QString& model_id) {
  const auto iter = task_request_map_.find(model_id);
  if (iter == task_request_map_.cend() || !download_group_model_->find(group_id)) {
    return;
  }

  auto items = download_group_model_->load(group_id).items;
  if (!items->find(model_id)) {
    return;
  }

  SyncHttpResponseDataToModel(*(iter->second), *items);
}

void DownloadService::onDownloadFinished(const QString& group_id, const QString& model_id,bool is_model_import) {
  auto iter = task_request_map_.find(model_id);
  if (iter == task_request_map_.cend()) {
    return;
  }

  const bool finished = !iter->second->isCancelDownloadLater();

  if (download_group_model_->find(group_id)) {
    auto models = download_group_model_->load(group_id).items;
    if (models->find(model_id)) {
      auto model = models->load(model_id);
      model.state = finished ? ItemState::FINISHED : ItemState::UNREADY;
      models->update(std::move(model));
    }
  }

  task_request_map_.erase(iter);

  if (!finished) {
    deleteModelCache(group_id, model_id);
  }

  syncCacheToLocal();
  if (is_model_import&&finished) importModelCache(group_id, model_id);
}

void DownloadService::startDownloadInfoRequest(const QString& group_id, bool download_later, bool is_model_import) {
  auto* info_request = createRequest<LoadModelGroupInfoRequest>(group_id);
  info_request->setSuccessedCallback([this, info_request, download_later, group_id, is_model_import]() {
    ModelGroupInfoModel info;
    SyncHttpResponseDataToModel(*info_request, info);
    auto group_info = download_group_model_->load(group_id);
    group_info << info;

    auto group_local_name = group_info.name;
    ReplaceIllegalCharacters(group_local_name);
    ConvertNonRepeatingPath(loadGroupCacheDirPath(), group_local_name);
    group_info.local_name = std::move(group_local_name);
    download_group_model_->update(std::move(group_info));

    auto* file_request = createRequest<LoadModelGroupFileListInfoRequest>(group_id,
                                                                          info.getModelCount());
    file_request->setSuccessedCallback(
        [this, file_request, download_group = download_later, group_id, is_model_import]() mutable {
      ModelGroupInfoModel info;
      SyncHttpResponseDataToModel(*file_request, info);
      auto group_info = download_group_model_->load(group_id);
      group_info << *info.models().lock();
      auto download_item_model = group_info.items;
      download_group_model_->update(std::move(group_info));

      for (auto& model_info : download_item_model->rawData()) {
        auto model_local_name = model_info.name;
        ReplaceIllegalCharacters(model_local_name);
        ConvertNonRepeatingPath(loadModelCacheDirPath(group_id), model_local_name);
        model_info.local_name = std::move(model_local_name);

        if (!download_group && model_info.state != ItemState::READY) {
          continue;
        }

        if (model_info.state == ItemState::DOWNLOADING ||
            model_info.state == ItemState::FINISHED) {
          continue;
        }

        auto const& model_id = model_info.uid;
        if (task_request_map_.find(model_id) != task_request_map_.cend()) {
          continue;
        }

        if (model_info.state != ItemState::READY) {
          auto new_model_info = model_info;
          new_model_info.state = ItemState::READY;
          download_item_model->update(std::move(new_model_info));
        }

        startDownloadTaskRequest(group_id, model_id, is_model_import);
      }
    });

    HttpPost(file_request);
  });

  HttpPost(info_request);
}

void DownloadService::startDownloadTaskRequest(const QString& group_id, const QString& model_id, bool is_model_import) {
  auto request = std::make_unique<UnlimitedDownloadModelRequest>(
    group_id, model_id, loadModelCachePath(group_id, model_id), download_thread_pool_);

  getHttpSession().lock()->addRequest(request.get());

  connect(request.get(), &DownloadModelRequest::progressUpdated,
          this, &DownloadService::onDownloadProgressUpdated,
          Qt::ConnectionType::QueuedConnection);

  connect(request.get(), &DownloadModelRequest::downloadFinished,
      this, [this, is_model_import](const QString& group_id, const QString& model_id) {
          onDownloadFinished(group_id, model_id, is_model_import);
      },
          Qt::ConnectionType::QueuedConnection);

  HttpPost(request.get());

  task_request_map_.emplace(std::make_pair(model_id, std::move(request)));
}

} // namespace cxcloud
