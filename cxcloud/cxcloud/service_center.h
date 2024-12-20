#pragma once

#ifndef CXCLOUD_SERVICE_CENTER_H_
#define CXCLOUD_SERVICE_CENTER_H_

#include <functional>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQml/QJSEngine>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"
#include "cxcloud/model/server_list_model.h"
#include "cxcloud/service/account_service.h"
#include "cxcloud/service/download_service.h"
#include "cxcloud/service/model_library_service.h"
#include "cxcloud/service/model_service.h"
#include "cxcloud/service/printer_service.h"
#include "cxcloud/service/profile_service.h"
#include "cxcloud/service/project_service.h"
#include "cxcloud/service/slice_service.h"
#include "cxcloud/service/upgrade_service.h"
#include "cxcloud/tool/http_component.h"
#include "cxcloud/tool/job_executor.h"

namespace cxcloud {

  class CXCLOUD_API ServiceCenter : public QObject {
    Q_OBJECT;

    ServiceCenter(ServiceCenter&&) = delete;
    ServiceCenter(const ServiceCenter&) = delete;
    auto operator=(ServiceCenter&&) -> ServiceCenter& = delete;
    auto operator=(const ServiceCenter&) -> ServiceCenter& = delete;

   public:
    explicit ServiceCenter(QObject* parent = nullptr);
    explicit ServiceCenter(Version local_version, QObject* parent = nullptr);
    virtual ~ServiceCenter() = default;

   public:  // module interface
    auto initialize() -> void;
    auto uninitialize() -> void;
    auto loadUserSettings() -> void;

    auto getApplicationType() const -> ApplicationType;
    auto setApplicationType(ApplicationType type) -> void;

    auto getInterfaceType() const -> InterfaceType;
    auto setInterfaceType(InterfaceType type) -> void;

    /// @param handler (file_path)
    auto setOpenFileHandler(std::function<void(const QString&)> handler) -> void;

    /// @param handler (file_path_list)
    auto setOpenFileListHandler(std::function<void(const QStringList&)> handler) -> void;

    /// @param handler (project_path)
    auto setOpenProjectHandler(std::function<void(const QString&)> handler) -> void;

    /// @param saver (save_dir)->file_path
    auto setSelectedModelCombineSaver(std::function<QString(const QString&)> saver) -> void;

    /// @param saver (save_dir)->file_path_list
    auto setSelectedModelUncombineSaver(std::function<QStringList(const QString&)> saver) -> void;

    /// @param saver (save_dir)->file_path
    auto setCurrentSliceSaver(std::function<QString(const QString&)> saver) -> void;

    auto getVaildSliceFileSuffixList() const -> QStringList;
    /// @note old list model will stay after the suffix list updated
    auto setVaildSliceFileSuffixList(const QStringList& list) -> void;

    auto isOnlineSliceFileCompressed() const -> bool;
    auto setOnlineSliceFileCompressed(bool compressed) -> void;

    auto getModelCacheDirPath() const -> QString;
    auto setModelCacheDirPath(const QString& path) -> void;

    /// @return 0 if automatically determined by library
    auto getMaxDownloadThreadCount() const -> size_t;
    auto setMaxDownloadThreadCount(size_t count) -> void;

    /// @return empty string when disabled
    auto getSslCaInfoFilePath() const -> QString;
    /// @param path set an empty string if disabled
    auto setSslCaInfoFilePath(const QString& path) -> void;

    auto getJsEngine() const -> QPointer<QJSEngine>;
    auto setJsEngine(QPointer<QJSEngine> engine) -> void;

   public:  // base
    auto getServerType() const -> ServerType;
    auto setServerType(ServerType server_type) -> void;
    auto getServerTypeIndex() const -> int;
    auto setServerTypeIndex(int server_type_index) -> void;
    Q_SIGNAL void serverTypeChanged();
    Q_PROPERTY(int serverType
               READ getServerTypeIndex
               WRITE setServerTypeIndex
               NOTIFY serverTypeChanged);

    auto getRealServerType() const -> RealServerType;
    auto getRealServerTypeIndex() const -> int;
    Q_SIGNAL void realServerTypeChanged() const;
    Q_PROPERTY(int realServerType READ getRealServerTypeIndex NOTIFY realServerTypeChanged);

    Q_INVOKABLE int toRealServerType(int server_type_index) const;
    Q_INVOKABLE int toServerType(int real_server_type_index) const;

    auto getServerList() const -> ServerListModel::DataContainer;
    auto setServerList(ServerListModel::DataContainer list) -> void;
    auto getServerListModel() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* serverListModel READ getServerListModel CONSTANT);

    auto getApiUrl() const -> QString;
    Q_PROPERTY(QString apiUrl READ getApiUrl CONSTANT);

    auto getWebUrl() const -> QString;
    Q_PROPERTY(QString webUrl READ getWebUrl CONSTANT);

    auto getModelGroupUrlHead() const -> QString;
    auto loadModelGroupUrl(const QString& group_uid) -> QString;
    Q_PROPERTY(QString modelGroupUrlHead READ getModelGroupUrlHead CONSTANT);

    auto getModelRestriction() const -> ModelRestriction;
    auto setModelRestriction(ModelRestriction restriction) -> void;
    auto getModelRestrictionString() const -> QString;
    auto setModelRestrictionString(const QString& restriction) -> void;
    Q_SIGNAL void modelRestrictionChanged() const;
    Q_PROPERTY(QString modelRestriction
               READ getModelRestrictionString
               WRITE setModelRestrictionString
               NOTIFY modelRestrictionChanged);

    auto isRestrictedTipEnabled() const -> bool;
    auto setRestrictedTipEnabled(bool enabled) -> void;
    Q_SIGNAL void restrictedTipEnabledChanged() const;
    Q_PROPERTY(bool restrictedTipEnabled
               READ isRestrictedTipEnabled
               WRITE setRestrictedTipEnabled
               NOTIFY restrictedTipEnabledChanged);

    auto getJobExecutor() const -> std::weak_ptr<JobExecutor>;
    auto getJobExecutorObject() const -> QObject*;
    Q_PROPERTY(QObject* jobExecutor READ getJobExecutorObject CONSTANT);

   public:  // sub servive
    auto getAccountService() const -> std::weak_ptr<AccountService>;
    auto getAccountServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* accountService READ getAccountServiceObject CONSTANT);

    auto getModelService() const -> std::weak_ptr<ModelService>;
    auto getModelServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* modelService READ getModelServiceObject CONSTANT);

    auto getSliceService() const -> std::weak_ptr<SliceService>;
    auto getSliceServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* sliceService READ getSliceServiceObject CONSTANT);

    auto getProjectService() const -> std::weak_ptr<ProjectService>;
    auto getProjectServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* projectService READ getProjectServiceObject CONSTANT);

    auto getPrinterService() const -> std::weak_ptr<PrinterService>;
    auto getPrinterServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* printerService READ getPrinterServiceObject CONSTANT);

    auto getModelLibraryService() const -> std::weak_ptr<ModelLibraryService>;
    auto getModelLibraryServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* modelLibraryService READ getModelLibraryServiceObject CONSTANT);

    auto getDownloadService() const -> std::weak_ptr<DownloadService>;
    auto getDownloadServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* downloadService READ getDownloadServiceObject CONSTANT);

    auto getUpgradeService() const -> std::weak_ptr<UpgradeService>;
    auto getUpgradeServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* upgradeService READ getUpgradeServiceObject CONSTANT);

    auto getProfileService() const -> std::weak_ptr<ProfileService>;
    auto getProfileServiceObject() const -> QObject*;
    Q_PROPERTY(QObject* profileService READ getProfileServiceObject CONSTANT);

    Q_INVOKABLE bool checkNetworkAvailable() const;

   private:
    auto updateModelRestriction() -> void;

   private:
    std::unique_ptr<ServerListModel> server_list_model_{ nullptr };

    bool restricted_tip_enabled_{ true };

    std::shared_ptr<HttpSession> http_session_{ nullptr };
    std::shared_ptr<JobExecutor> job_executor_{ nullptr };

    std::shared_ptr<AccountService> account_service_{ nullptr };
    std::shared_ptr<ModelService> model_service_{ nullptr };
    std::shared_ptr<SliceService> slice_service_{ nullptr };
    std::shared_ptr<ProjectService> project_service_{ nullptr };
    std::shared_ptr<PrinterService> printer_service_{ nullptr };
    std::shared_ptr<ModelLibraryService> model_library_service_{ nullptr };
    std::shared_ptr<DownloadService> download_service_{ nullptr };
    std::shared_ptr<UpgradeService> upgrade_service_{ nullptr };
    std::shared_ptr<ProfileService> profile_service_{ nullptr };

    std::function<void(const QString&)> open_file_handler_{ nullptr };
    std::function<void(const QStringList&)> open_file_list_handler_{ nullptr };
    std::function<void(const QString&)> open_project_handler_{ nullptr };
    std::function<QString(const QString&)> model_group_url_creater_{ nullptr };
    std::function<QString(const QString&)> selected_model_combine_saver_{ nullptr };
    std::function<QStringList(const QString&)> selected_model_uncombine_saver_{ nullptr };
    std::function<QString(const QString&)> current_slice_saver_{ nullptr };

    InterfaceType interface_type_{ InterfaceType::RELEASE };
    QStringList vaild_slice_file_suffix_list_{};
    bool online_slice_file_compressed_{ false };
    QString model_cache_dir_path_{};
    size_t max_download_thread_count_{ 0 };
    QPointer<QJSEngine> js_engine_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_CENTER_H_
