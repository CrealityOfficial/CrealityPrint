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
#include "cxcloud/service/base_service.h"
#include "cxcloud/tool/thread_pool.h"

namespace cxcloud {

  class CXCLOUD_API DownloadService final : public BaseService {
    Q_OBJECT;

   public:
    using GroupListModel = DownloadGroupListModel;
    using GroupList      = GroupListModel::DataContainer;
    using Group          = GroupListModel::Data;

    using ModelListModel = decltype(Group::items)::element_type;
    using ModelList      = ModelListModel::DataContainer;
    using Model          = ModelListModel::Data;

   public:
    explicit DownloadService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~DownloadService() = default;

   public:
    auto initialize() -> void override;
    auto uninitialize() -> void override;

    auto getOpenFileHandler() const -> std::function<void(const QString&)>;
    auto setOpenFileHandler(std::function<void(const QString&)> handler) -> void;

    auto getOpenFileListHandler() const -> std::function<void(const QStringList&)>;
    auto setOpenFileListHandler(std::function<void(const QStringList&)> handler) -> void;

    auto getCacheDirPath() const -> QString;
    auto setCacheDirPath(const QString& path) -> void;

    /// @return 0 if automatically determined by library
    auto getMaxDownloadThreadCount() const -> size_t;
    auto setMaxDownloadThreadCount(size_t count) -> void;

   public:  // qt model
    auto getDownloadTaskModel() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* downloadTaskModel READ getDownloadTaskModel CONSTANT);

   public:  // local cache
    Q_INVOKABLE bool checkModelDownloaded(const QString& group_uid, const QString& model_uid) const;
    Q_INVOKABLE bool checkModelGroupDownloaded(const QString& group_uid) const;

    Q_INVOKABLE bool checkModelDownloading(const QString& group_uid,
                                           const QString& model_uid) const;
    Q_INVOKABLE bool checkModelGroupDownloading(const QString& group_uid) const;

    /// @brief !checkModelDownloaded && !checkModelDownloading
    /// @see checkModelDownloaded
    /// @see checkModelDownloading
    Q_INVOKABLE bool checkModelDownloadable(const QString& group_uid,
                                            const QString& model_uid) const;
    Q_INVOKABLE bool checkModelGroupDownloadable(const QString& group_uid) const;

    Q_INVOKABLE bool checkModelImportLater(const QString& group_uid,
                                           const QString& model_uid) const;
    Q_INVOKABLE bool checkModelGroupImportLater(const QString& group_uid) const;

    Q_INVOKABLE void markModelImportLater(const QString& group_uid,
                                          const QString& model_uid,
                                          bool           import_later);
    Q_INVOKABLE void markModelGroupImportLater(const QString& group_uid, bool import_later);

    /// @param uid_map_list javascript var like: [
    ///   {
    ///      group_uid: "xxx",
    ///      model_uid: "yyy"
    ///   },
    ///   ...
    /// ]
    Q_INVOKABLE void deleteModelCache(const QVariantList& uid_map_list);
    Q_INVOKABLE void deleteModelCache(const QString& group_uid, const QString& model_uid);
    Q_INVOKABLE void deleteModelGroupCache(const QString& group_uid);

    /// @param uid_map_list javascript var like: [
    ///   {
    ///      group_uid: "xxx",
    ///      model_uid: "yyy"
    ///   },
    ///   ...
    /// ]
    Q_INVOKABLE void importModelCache(const QVariantList& uid_map_list);
    Q_INVOKABLE void importModelCache(const QString& group_uid, const QString& model_uid);
    Q_INVOKABLE void importModelGroupCache(const QString& group_uid);

    Q_INVOKABLE bool exportModelCache(const QString& group_uid,
                                      const QString& model_uid,
                                      const QString& dir_path);

    Q_INVOKABLE void openModelCacheDir() const;

   public:  // download promise
    Q_INVOKABLE void makeModelGroupDownloadPromise(const QString& group_uid,
                                                   bool           import_later = false);
    Q_INVOKABLE void makeModelDownloadPromise(const QString& group_uid,
                                              const QString& model_uid,
                                              bool           import_later = false);
    Q_INVOKABLE void fulfillsAllDonwloadPromise();
    Q_INVOKABLE void breachAllDonwloadPromise();

   public:  // download task
    Q_INVOKABLE void startModelGroupDownloadTask(const QString& group_uid,
                                                 bool           import_later = false);
    Q_INVOKABLE void startModelDownloadTask(const QString& group_uid,
                                            const QString& model_uid,
                                            bool           import_later = false);
    Q_INVOKABLE void cancelModelDownloadTask(const QString& group_uid, const QString& model_uid);

   private:
    struct Promise {
      bool    entire_group{ false };
      bool    import_later{ false };
      QString group_uid{};
      QString model_uid{};
    };

    struct Task {
      using State   = Model::State;
      using Request = DownloadModelRequest;

      std::unique_ptr<Request> request{ nullptr };
      bool                     import_later{ false };
    };

   private:
    auto loadGroupCacheName(const QString& group_uid) const -> QString;
    auto loadModelCacheName(const QString& group_uid, const QString& model_uid) const -> QString;
    auto loadGroupCacheDirPath() const -> QString;
    auto loadModelCacheDirPath(const QString& group_uid) const -> QString;
    auto loadGroupCachePath(const QString& group_uid) const -> QString;
    auto loadModelCachePath(const QString& group_uid, const QString& model_uid) const -> QString;
    auto loadCacheInfoPath() const -> QString;

    auto syncCacheFromLocal() -> void;
    auto syncCacheToLocal() const -> void;
    auto resetExpiredModel() const -> void;

    /// @brief Check whether there are models in the group whose state in the state list.
    /// @param states target state list
    /// @return true if found any model's state is in the state list, or false
    auto checkModelGroupState(const QString&         group_uid,
                              std::list<Task::State> states) const -> bool;

    /// @brief Check model's state is in the state list.
    /// @param states target state list
    /// @return true if model's state is in the state list, or false.
    auto checkModelState(const QString&         group_uid,
                         const QString&         model_uid,
                         std::list<Task::State> states) const -> bool;

    /// @param download_all_later Download all models in group after request info finished.
    ///                           If you just want to download part of models in group,
    ///                           set their state to DownloadItemListModel::Data::State::READY
    ///                           before call this function.
    /// @param import_later Import all successful downloaded models.
    auto startDownloadInfoRequest(const QString& group_uid,
                                  bool           download_all_later = false,
                                  bool           import_later       = false) -> void;
    auto startDownloadTaskRequest(const QString& group_uid,
                                  const QString& model_uid,
                                  bool           import_later = false) -> void;

    Q_SLOT void onDownloadProgressUpdated(const QString& group_uid, const QString& model_uid);
    Q_SLOT void onDownloadFinished(const QString& group_uid, const QString& model_uid);

   private:
    QString cache_dir_path_;
    size_t  max_download_thread_count_{ 0 };

    std::function<void(const QString&)>     open_file_handler_;
    std::function<void(const QStringList&)> open_file_list_handler_;

    std::list<Promise>      promise_list_;
    std::map<QString, Task> model_task_map_;

    std::shared_ptr<ThreadPool>     download_thread_pool_;
    std::unique_ptr<GroupListModel> download_group_model_;
  };

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_DOWNLOAD_SERVICE_H_
