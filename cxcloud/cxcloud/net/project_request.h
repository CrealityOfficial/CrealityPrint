#pragma once

#include "cxcloud/interface.hpp"
#include "cxcloud/model/model_info_model.h"
#include "cxcloud/model/project_model.h"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  auto GetModelGroupUidCache(const QString& project_uid) -> QString;
  auto GetProjectNameCache(const QString& project_uid) -> QString;





  class CXCLOUD_API LoadProjectListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadProjectListRequest(const QString& group_uid,
                                    int            page_index,
                                    int            page_size,
                                    QObject*       parent = nullptr);
    virtual ~LoadProjectListRequest() = default;

   public:
    auto getGroupUid() const -> QString;
    auto getPageIndex() const -> int;
    auto getPageSize() const -> int;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    QString group_uid_{};
    int page_index_{ 0 };
    int page_size_{ 24 };
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadProjectListRequest& request,
                                               ProjectListModel&             model) -> bool;

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadProjectListRequest& request,
                                               ModelGroupInfoModel&          model) -> bool;





  class CXCLOUD_API LoadUploadedProjectListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadUploadedProjectListRequest(int      page_index,
                                            int      page_size,
                                            QObject* parent = nullptr);
    virtual ~LoadUploadedProjectListRequest() = default;

   public:
    auto getPageIndex() const -> int;
    auto getPageSize() const -> int;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    int page_index_{ 0 };
    int page_size_{ 24 };
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadUploadedProjectListRequest& request,
                                               ProjectListModel&                     model) -> bool;





  class CXCLOUD_API CreateProjectRequest : public HttpRequest {
    Q_OBJECT;

   public:
    struct PlateInfo{
      QString name{};
      QString image{};
    };

   public:
    explicit CreateProjectRequest(const QString&                model_group_uid,
                                  const QString&                project_name,
                                  const QString&                file_key,
                                  size_t                        file_size,
                                  const QString&                printer_internal_name,
                                  float                         layer_height,
                                  float                         sparse_infill_density,
                                  size_t                        print_duraction,
                                  size_t                        material_weight,
                                  bool                          printable,
                                  const std::vector<PlateInfo>& plates,
                                  QObject*                      parent = nullptr);
    virtual ~CreateProjectRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    QString                model_group_uid_{};
    QString                project_name_{};
    QString                file_key_{};
    size_t                 file_size_{ 0 };
    QString                printer_internal_name_{ QStringLiteral("CR-K1") };
    float                  layer_height_{ 0.2f };
    float                  sparse_infill_density_{ 15.f };
    size_t                 print_duraction_{ 0 };
    size_t                 material_weight_{ 0 };
    bool                   printable_{ false };
    std::vector<PlateInfo> plates_{};
  };





  class CXCLOUD_API DeleteProjectRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit DeleteProjectRequest(const QString& uid, QObject* parent = nullptr);
    virtual ~DeleteProjectRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
  };





  class CXCLOUD_API LoadProjectDownloadUrlRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadProjectDownloadUrlRequest(const QString& uid, QObject* parent = nullptr);
    virtual ~LoadProjectDownloadUrlRequest() = default;

   public:
    auto getDownloadUrl() const -> QString;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
  };

}  // namespace cxcloud
