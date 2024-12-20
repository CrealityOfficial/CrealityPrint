#include "cxcloud/service/project_service.h"

#ifndef __APPLE__
#  include <filesystem>
#endif

#include <QtCore/QDir>

#ifdef __APPLE__
#  include <boost/filesystem/operations.hpp>
#endif

#include <qtusercore/module/quazipfile.h>
#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/project_request.h"

namespace cxcloud {

  namespace {

#ifdef __APPLE__
      namespace filesystem = boost::filesystem;
#else
      namespace filesystem = std::filesystem;
#endif

  }  // namespace

  ProjectService::ProjectService(std::weak_ptr<HttpSession> http_session,
                                 std::weak_ptr<JobExecutor> job_executor,
                                 QObject*                   parent)
      : BaseService{ http_session, job_executor, parent }
      , cache_dir_name_{ QStringLiteral("temp_upload_project") }
      , uploaded_project_list_model_{ std::make_unique<ProjectListModel>(getVaildSuffixList()) } {}

  auto ProjectService::setCurrentProjectSaver(
      std::function<QString(const QString&)> saver) -> void {
    current_project_saver_ = saver;
  }

  auto ProjectService::setOpenFileHandler(std::function<void(const QString&)> handler) -> void {
    open_file_handler_ = handler;
  }

  auto ProjectService::getVaildSuffixList() const -> QStringList {
    return vaild_suffix_list_;
  }

  auto ProjectService::setVaildSuffixList(const QStringList& list) -> void {
    vaild_suffix_list_ = list;
    uploaded_project_list_model_->setVaildSuffixList(list);
    vaildSuffixListChanged();
  }

  auto ProjectService::getUploadedProjectListModel() const -> QAbstractItemModel* {
    return uploaded_project_list_model_.get();
  }

  auto ProjectService::clearUploadedProjectListModel() const -> void {
    uploaded_project_list_model_->clear();
  }

  auto ProjectService::loadUploadedProjectList(int page_index, int page_size) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    auto request = createRequest<LoadUploadedProjectListRequest>(page_index, page_size);

    request->setSuccessedCallback([this, request]() {
      SyncHttpResponseDataToModel(*request, *uploaded_project_list_model_);
    });

    request->asyncPost();
  }

  auto ProjectService::deleteUploadedProject(const QString& project_uid) -> void {
    auto request = createRequest<DeleteProjectRequest>(project_uid);

    request->setSuccessedCallback([this, project_uid]() {
      const auto model_group_uid = GetModelGroupUidCache(project_uid);
      if (model_group_uid.isEmpty()) {
        return;
      }

      uploadedProjectDeleted(model_group_uid, project_uid);
      uploaded_project_list_model_->erase(project_uid);
    });

    request->asyncPost();
  }

  auto ProjectService::uploadProject(const QString& model_group_id) -> void {
    if (upload_request_) {
      return;
    }

    const auto cache_dir_path = qtuser_core::getOrCreateAppDataLocation(cache_dir_name_);
    const auto cache_dir_cleaner = [cache_dir_path]() {
      QDir{ cache_dir_path }.removeRecursively();
    };

    current_project_saver_ = [](auto&&) { return QStringLiteral("E:/file/model/coin.3mf"); };
    if (!current_project_saver_) {
      return;
    }

    const auto project_file_path = current_project_saver_(cache_dir_path);
    if (!filesystem::exists(project_file_path.toStdString())) {
      return;
    }

    const auto project_file_size = filesystem::file_size(project_file_path.toStdString());
    const auto project_file_suffix = project_file_path.split(QStringLiteral(".")).last();
    const auto project_file_name = project_file_path.split(QStringLiteral("/")).last();

    auto upload_file_path = project_file_path;
    auto upload_file_size = project_file_size;
    auto upload_file_suffix = project_file_suffix;

    upload_request_ = createRequest<UploadFileRequest>(QStringList{ upload_file_path },
                                                       std::move(upload_file_suffix),
                                                       UploadFileRequest::FileType::FILE);

    upload_request_->setProgressUpdatedCallback([this]() {
      uploadProjectProgressUpdated(upload_request_->getCurrentProgress());
    });

    upload_request_->setSuccessedCallback([this,
                                           cache_dir_cleaner,
                                           model_group_id,
                                           project_file_name,
                                           upload_file_path,
                                           upload_file_size]() {
      if (upload_request_->isCancelDownloadLater()) {
        uploadProjectCanceled();
        return;
      }

      cache_dir_cleaner();

      auto create_request =
          createRequest<CreateProjectRequest>(model_group_id,
                                              project_file_name,
                                              upload_request_->getOnlineFilePath(upload_file_path),
                                              upload_file_size,
                                              QString{ "CR-K1" },
                                              0.2f,
                                              15.f,
                                              70ull,
                                              100ull,
                                              false,
                                              std::vector<CreateProjectRequest::PlateInfo>{
                                                {
                                                  "DefaultPlate",
                                                  ""
                                                }
                                              });

      create_request->setSuccessedCallback([this, cache_dir_cleaner, create_request]() {
        uploadProjectSuccessed();
      });

      create_request->setFailedCallback([this, cache_dir_cleaner, create_request]() {
        uploadProjectFailed();
      });

      create_request->asyncPost();
    });

    upload_request_->setFailedCallback([this, cache_dir_cleaner]() {
      cache_dir_cleaner();
      uploadProjectFailed();
    });

    upload_request_->asyncRequest();
  }

  auto ProjectService::cancelUploadProject() -> void {
    if (!upload_request_) {
      return;
    }

    upload_request_->setCancelDownloadLater(true);
  }

  auto ProjectService::importProject(const QString& uid) -> void {
    if (download_request_) {
      return;
    }

    auto request = createRequest<LoadProjectDownloadUrlRequest>(uid);

    request->setSuccessedCallback([this, request, uid]() {
      if (download_request_) {
        return;
      }

      const auto dir_path = qtuser_core::getOrCreateAppDataLocation(cache_dir_name_);
      const auto file_path = QStringLiteral("%1/%2").arg(dir_path, GetProjectNameCache(uid));
      if (auto path = filesystem::path{ file_path.toStdWString() }; filesystem::exists(path)) {
        filesystem::remove(path);
      }

      download_request_ = createRequest<DownloadRequest>(request->getDownloadUrl(), file_path);

      download_request_->setSuccessedCallback([this, uid] {
        if (!open_file_handler_) {
          importProjectFailed(uid);
          return;
        }

        open_file_handler_(download_request_->getDownloadDstPath());
        importProjectSuccessed(uid);
      });

      download_request_->setFailedCallback([this, uid] {
        importProjectFailed(uid);
      });

      atatchJob(download_request_->getJob());
      download_request_->asyncDownload();
    });

    request->setFailedCallback([this, uid] {
      importProjectFailed(uid);
    });

    request->asyncPost();
  }

  auto ProjectService::deleteModelGroupProject(const QString& model_group_uid) -> void {
    auto request = createRequest<LoadUploadedProjectListRequest>(0, 1024);

    request->setSuccessedCallback([this, request, model_group_uid]() {
      SyncHttpResponseDataToModel(*request, *uploaded_project_list_model_);
      for (auto& project : uploaded_project_list_model_->rawData()) {
        if (project.model_group_uid == model_group_uid) {
          deleteUploadedProject(project.uid);
        }
      }
    });

    request->asyncPost();
  }

}  // namespace cxcloud
