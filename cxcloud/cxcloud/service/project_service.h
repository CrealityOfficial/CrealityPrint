#pragma once

#include <functional>
#include <memory>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/project_model.h"
#include "cxcloud/net/common_request.h"
#include "cxcloud/net/oss_request.h"
#include "cxcloud/service/base_service.h"

namespace cxcloud {

  class CXCLOUD_API ProjectService : public BaseService {
    Q_OBJECT;

   public:
    explicit ProjectService(std::weak_ptr<HttpSession> http_session,
                            std::weak_ptr<JobExecutor> job_executor,
                            QObject*                   parent = nullptr);
    virtual ~ProjectService() = default;

   public:
    auto setCurrentProjectSaver(std::function<QString(const QString&)> saver) -> void;
    auto setOpenFileHandler(std::function<void(const QString&)> handler) -> void;

   public:
    auto getVaildSuffixList() const -> QStringList;
    /// @note old list model will stay after the suffix list updated
    auto setVaildSuffixList(const QStringList& list) -> void;
    Q_SIGNAL void vaildSuffixListChanged();
    Q_PROPERTY(QStringList vaildSuffixList READ getVaildSuffixList NOTIFY vaildSuffixListChanged);

    auto getUploadedProjectListModel() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* uploadedProjectListModel
               READ getUploadedProjectListModel CONSTANT);
    Q_INVOKABLE void clearUploadedProjectListModel() const;
    Q_INVOKABLE void loadUploadedProjectList(int page_index, int page_size);
    Q_INVOKABLE void deleteUploadedProject(const QString& project_uid);
    Q_SIGNAL void uploadedProjectDeleted(const QString& model_group_uid,
                                         const QString& project_uid);

    Q_INVOKABLE void uploadProject(const QString& model_group_id);
    Q_INVOKABLE void cancelUploadProject();
    Q_SIGNAL void uploadProjectProgressUpdated(int progress);
    Q_SIGNAL void uploadProjectSuccessed();
    Q_SIGNAL void uploadProjectCanceled();
    Q_SIGNAL void uploadProjectFailed();

    Q_INVOKABLE void importProject(const QString& uid);
    Q_SIGNAL void importProjectSuccessed(const QString& uid);
    Q_SIGNAL void importProjectFailed(const QString& uid);

    Q_INVOKABLE void deleteModelGroupProject(const QString& model_group_uid);

   private:
    const QString cache_dir_name_{};
    QStringList vaild_suffix_list_{};

    std::function<QString(const QString&)> current_project_saver_{ nullptr };
    std::function<void(const QString&)> open_file_handler_{ nullptr };

    std::unique_ptr<ProjectListModel> uploaded_project_list_model_{ nullptr };

    QPointer<UploadFileRequest> upload_request_{ nullptr };
    QPointer<DownloadRequest> download_request_{ nullptr };
  };

}  // namespace cxcloud
