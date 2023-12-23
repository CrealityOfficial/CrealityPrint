#pragma once

#ifndef CXCLOUD_TOOL_DOWNLOAD_SERVICE_H_
#define CXCLOUD_TOOL_DOWNLOAD_SERVICE_H_

#include <functional>
#include <list>
#include <map>
#include <memory>

#include <QtCore/QAbstractListModel>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "cxcloud/model/download_model.h"
#include "cxcloud/net/download_request.h"
#include "cxcloud/net/model_request.h"
#include "cxcloud/service/base_service.hpp"

#if (__cplusplus >= 201703L)
namespace boost::asio {
#else
namespace boost {
namespace asio {
#endif

class thread_pool;

#if (__cplusplus >= 201703L)
}  // namespace boost::asio
#else
}  // namespace asio
}  // namespace boost
#endif

namespace cxcloud {

class CXCLOUD_API DownloadService final : public BaseService {
  Q_OBJECT;

public:
  explicit DownloadService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
  virtual ~DownloadService() = default;

public:
  void initialize() override;
  void uninitialize() override;

  std::function<void(const QString&)> getOpenFileHandler() const;
  void setOpenFileHandler(std::function<void(const QString&)> handler);

  std::function<void(const QStringList&)> getOpenFileListHandler() const;
  void setOpenFileListHandler(std::function<void(const QStringList&)> handler);

  QString getCacheDirPath() const;
  void setCacheDirPath(const QString& path);

  /// @return 0 if automatically determined by library
  size_t getMaxDownloadThreadCount() const;
  void setMaxDownloadThreadCount(size_t count);

public:  // local cache
  Q_INVOKABLE bool checkModelDownloaded(const QString& group_id, const QString& model_id);
  Q_INVOKABLE bool checkModelGroupDownloaded(const QString& group_id);

  Q_INVOKABLE bool checkModelDownloading(const QString& group_id, const QString& model_id);
  Q_INVOKABLE bool checkModelGroupDownloading(const QString& group_id);

  /// @brief !checkModelDownloaded && !checkModelDownloading
  /// @see checkModelDownloaded
  /// @see checkModelDownloading
  Q_INVOKABLE bool checkModelDownloadable(const QString& group_id, const QString& model_id);
  Q_INVOKABLE bool checkModelGroupDownloadable(const QString& group_id);

  Q_INVOKABLE void deleteModelCache(const QStringList& group_model_list);
  Q_INVOKABLE void deleteModelCache(const QString& group_id, const QString& model_id);
  Q_INVOKABLE void importModelCache(const QStringList& group_model_list);
  Q_INVOKABLE void importModelCache(const QString& group_id, const QString& model_id);
  Q_INVOKABLE bool exportModelCache(const QString& group_id,
                                    const QString& model_id,
                                    const QString& dir_path);

  Q_INVOKABLE void openModelCacheDir() const;

public:  // download promise
  Q_INVOKABLE void makeModelGroupDownloadPromise(const QString& group_id);
  Q_INVOKABLE void makeModelDownloadPromise(const QString& group_id, const QString& model_id);
  Q_INVOKABLE void fulfillsAllDonwloadPromise();
  Q_INVOKABLE void breachAllDonwloadPromise();

public:  // download task
  Q_INVOKABLE void startModelGroupDownloadTask(const QString& group_id, bool is_model_import = false);
  Q_INVOKABLE void startModelDownloadTask(const QString& group_id, const QString& model_id, bool is_model_import = false);
  Q_INVOKABLE void cancelModelDownloadTask(const QString& group_id, const QString& model_id);

public:
  Q_PROPERTY(QAbstractListModel* downloadTaskModel READ getDownloadTaskModel CONSTANT);
  QAbstractListModel* getDownloadTaskModel() const;

private:
  QString loadGroupCacheName(const QString& group_id) const;
  QString loadModelCacheName(const QString& group_id, const QString& model_id) const;
  QString loadGroupCacheDirPath() const;
  QString loadModelCacheDirPath(const QString& group_id) const;
  QString loadGroupCachePath(const QString& group_id) const;
  QString loadModelCachePath(const QString& group_id, const QString& model_id) const;
  QString loadCacheInfoPath() const;

  void syncCacheFromLocal();
  void syncCacheToLocal() const;
  void resetExpiredModel() const;

  /// @brief check whether there are models in the group whose state in the state list
  /// @param group_id id of group
  /// @param states target state list
  /// @return true if found any model's state is in the state list, or false
  bool checkModelGroupState(const QString& group_id,
                            std::list<DownloadItemListModel::Data::State> states);

  /// @brief check model's state is in the state list
  /// @param group_id id of model's group
  /// @param model_id id of model
  /// @param states target state list
  /// @return true if model's state is in the state list, or false
  bool checkModelState(const QString& group_id,
                       const QString& model_id,
                       std::list<DownloadItemListModel::Data::State> states);

  Q_SLOT void onDownloadProgressUpdated(const QString& group_id, const QString& model_id);
  Q_SLOT void onDownloadFinished(const QString& group_id, const QString& model_id, bool is_model_import = false);

  void startDownloadInfoRequest(const QString& group_id, bool download_later = false, bool is_model_import = false);
  void startDownloadTaskRequest(const QString& group_id, const QString& model_id, bool is_model_import = false);

private:
  QString cache_dir_path_;
  size_t max_download_thread_count_{ 0 };

  std::function<void(const QString&)> open_file_handler_;
  std::function<void(const QStringList&)> open_file_list_handler_;

  struct Promise {
    bool is_group{false};
    QString group_id{};
    QString model_id{};
  };

  std::list<Promise> promise_list_;

  std::shared_ptr<boost::asio::thread_pool> download_thread_pool_;
  std::map<QString, std::unique_ptr<DownloadModelRequest>> task_request_map_;

  std::unique_ptr<DownloadGroupListModel> download_group_model_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_DOWNLOAD_SERVICE_H_
