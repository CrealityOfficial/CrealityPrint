#include "cxcloud/net/project_request.h"

#include <map>
#include <mutex>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <qtusercore/module/quazipfile.h>

namespace cxcloud {

  std::mutex project_group_uid_map_mutex;
  std::map<QString, QString> project_group_uid_map{};

  auto GetModelGroupUidCache(const QString& project_uid) -> QString {
    auto lock = std::unique_lock<std::mutex>{ project_group_uid_map_mutex };
    auto iter = project_group_uid_map.find(project_uid);
    if (iter == project_group_uid_map.cend()) {
      return {};
    }
    return iter->second;
  }

  template <typename... _Types>
  auto SetModelGroupUidCache(_Types&&... values) -> void {
    auto lock = std::unique_lock<std::mutex>{ project_group_uid_map_mutex };
    project_group_uid_map.emplace(std::forward<_Types>(values)...);
  }





  std::mutex project_uid_name_map_mutex;
  std::map<QString, QString> project_uid_name_map{};

  auto GetProjectNameCache(const QString& project_uid) -> QString {
    auto lock = std::unique_lock<std::mutex>{ project_uid_name_map_mutex };
    auto iter = project_uid_name_map.find(project_uid);
    if (iter == project_uid_name_map.cend()) {
      return project_uid;
    }
    return iter->second;
  }

  template <typename... _Types>
  auto SetProjectNameCache(_Types&&... values) -> void {
    auto lock = std::unique_lock<std::mutex>{ project_uid_name_map_mutex };
    project_uid_name_map.emplace(std::forward<_Types>(values)...);
  }





  LoadProjectListRequest::LoadProjectListRequest(const QString& group_uid,
                                                 int            page_index,
                                                 int            page_size,
                                                 QObject*       parent)
      : HttpRequest{ parent }
      , group_uid_{ group_uid }
      , page_index_{ page_index }
      , page_size_{ page_size } {}

  auto LoadProjectListRequest::getGroupUid() const -> QString {
    return group_uid_;
  }

  auto LoadProjectListRequest::getPageIndex() const -> int {
    return page_index_;
  }

  auto LoadProjectListRequest::getPageSize() const -> int {
    return page_size_;
  }

  auto LoadProjectListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("modelGroupId"), group_uid_ },
      { QStringLiteral("page"), page_index_ },
      { QStringLiteral("pageSize"), page_size_ },
      { QStringLiteral("filterType"), 11 },  // current author first
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadProjectListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/3mfList");
  }

  auto SyncHttpResponseDataToModel(const LoadProjectListRequest& request,
                                   ProjectListModel&             model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto project_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (project_list.empty()) {
      return false;
    }

    for (const auto& project : project_list) {
      if (!project.isObject()) {
        continue;
      }

      auto uid = project[QStringLiteral("id")].toString();
      const auto user_info = project[QStringLiteral("userInfo")].toObject();

      auto data = model.load(uid);

      data.uid                   = std::move(uid);
      data.name                  = project[QStringLiteral("name")].toString();
      data.image                 = project[QStringLiteral("thumbnail")].toString();
      data.printer_name          = project[QStringLiteral("printerName")].toString();
      data.layer_height          = project[QStringLiteral("layerHeight")].toString();
      data.sparse_infill_density = project[QStringLiteral("sparseInfillDensity")].toString();
      data.plate_count           = project[QStringLiteral("plateCount")].toInt();
      data.print_duraction       = project[QStringLiteral("printTime")].toInt();
      data.material_weight       = project[QStringLiteral("materialWeight")].toInt();
      data.author_id             = user_info[QStringLiteral("userId")].toVariant().toString();
      data.author_name           = user_info[QStringLiteral("nickName")].toString();
      data.author_image          = user_info[QStringLiteral("avatar")].toString();
      data.creation_timepoint    = project[QStringLiteral("createTime")].toInt();
      // data.suffix                = project[QStringLiteral("")].toString();
      // data.url                   = project[QStringLiteral("")].toString();

      SetModelGroupUidCache(data.uid, request.getGroupUid());
      SetProjectNameCache(data.uid, data.name);

      model.emplaceBack(std::move(data));
    }

    return true;
  }

  auto SyncHttpResponseDataToModel(const LoadProjectListRequest& request,
                                   ModelGroupInfoModel&          model) -> bool {
    return SyncHttpResponseDataToModel(request, *model.projects().lock());
  }






  LoadUploadedProjectListRequest::LoadUploadedProjectListRequest(int      page_index,
                                                                 int      page_size,
                                                                 QObject* parent)
      : HttpRequest{ parent }, page_index_{ page_index }, page_size_{ page_size } {}

  auto LoadUploadedProjectListRequest::getPageIndex() const -> int {
    return page_index_;
  }

  auto LoadUploadedProjectListRequest::getPageSize() const -> int {
    return page_size_;
  }

  auto LoadUploadedProjectListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("page"), page_index_ },
      { QStringLiteral("pageSize"), page_size_ },
      { QStringLiteral("filterType"), 3 },  // newest upload
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadUploadedProjectListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/print/3mfList");
  }

  auto SyncHttpResponseDataToModel(const LoadUploadedProjectListRequest& request,
                                   ProjectListModel&                     model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto project_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (project_list.empty()) {
      return false;
    }

    for (const auto& project : project_list) {
      if (!project.isObject()) {
        continue;
      }

      auto uid = project[QStringLiteral("id")].toString();
      const auto user_info = project[QStringLiteral("userInfo")].toObject();

      auto data = model.load(uid);

      data.uid                   = std::move(uid);
      data.name                  = project[QStringLiteral("name")].toString();
      data.image                 = project[QStringLiteral("thumbnail")].toString();
      data.model_group_uid       = project[QStringLiteral("modelGroupId")].toString();
      data.printer_name          = project[QStringLiteral("printerName")].toString();
      data.layer_height          = project[QStringLiteral("layerHeight")].toString();
      data.sparse_infill_density = project[QStringLiteral("sparseInfillDensity")].toString();
      data.plate_count           = project[QStringLiteral("plateCount")].toInt();
      data.print_duraction       = project[QStringLiteral("printTime")].toInt();
      data.material_weight       = project[QStringLiteral("materialWeight")].toInt();
      data.author_id             = user_info[QStringLiteral("userId")].toVariant().toString();
      data.author_name           = user_info[QStringLiteral("nickName")].toString();
      data.author_image          = user_info[QStringLiteral("avatar")].toString();
      data.creation_timepoint    = project[QStringLiteral("createTime")].toInt();
      // data.suffix                = project[QStringLiteral("")].toString();
      // data.url                   = project[QStringLiteral("")].toString();

      SetModelGroupUidCache(data.uid, data.model_group_uid);
      SetProjectNameCache(data.uid, data.name);

      model.emplaceBack(std::move(data));
    }

    return true;
  }





  CreateProjectRequest::CreateProjectRequest(const QString&                model_group_uid,
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
                                             QObject*                      parent)
      : HttpRequest{ parent }
      , model_group_uid_{ model_group_uid }
      , project_name_{ project_name }
      , file_key_{ file_key }
      , file_size_{ file_size }
      , printer_internal_name_{ printer_internal_name }
      , layer_height_{ layer_height }
      , sparse_infill_density_{ sparse_infill_density }
      , print_duraction_{ print_duraction }
      , material_weight_{ material_weight }
      , printable_{ printable }
      , plates_{ plates } {}

  auto CreateProjectRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{
      QJsonObject{
        { QStringLiteral("modelGroupId"), model_group_uid_ },
        { QStringLiteral("name"), project_name_ },
        { QStringLiteral("filekey"), file_key_ },
        { QStringLiteral("size"), static_cast<int>(file_size_) },
        { QStringLiteral("printerInternalName"), printer_internal_name_ },
        { QStringLiteral("layerHeight"), QString::number(layer_height_) },
        { QStringLiteral("sparseInfillDensity"),
          QStringLiteral("%1%").arg(QString::number(sparse_infill_density_)) },
        { QStringLiteral("printTime"), static_cast<int>(print_duraction_) },
        { QStringLiteral("materialWeight"), static_cast<int>(material_weight_) },
        { QStringLiteral("isCanPrint"), printable_ },
        { QStringLiteral("plateList"),
          [this] {
            auto array = QJsonArray{};
            for (size_t index = 0; index < plates_.size(); ++index) {
              const auto& plate = plates_.at(index);
              array.push_back(QJsonObject{
                  { QStringLiteral("index"), static_cast<int>(index) },
                  { QStringLiteral("name"), plate.name },
                  { QStringLiteral("thumbnail"), plate.image },
              });
            }
            return array;
          }() },
      }
    }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto CreateProjectRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/upload3mf");
  }





  DeleteProjectRequest::DeleteProjectRequest(const QString& uid, QObject* parent)
      : HttpRequest{ parent }, uid_{ uid } {}

  auto DeleteProjectRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), uid_ }
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto DeleteProjectRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/3mfDel");
  }





  LoadProjectDownloadUrlRequest::LoadProjectDownloadUrlRequest(const QString& uid, QObject* parent)
      : HttpRequest{ parent }, uid_{ uid } {}

  auto LoadProjectDownloadUrlRequest::getDownloadUrl() const -> QString {
    return responseJson()[QStringLiteral("result")][QStringLiteral("downloadUrl")].toString();
  }

  auto LoadProjectDownloadUrlRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), uid_ }
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadProjectDownloadUrlRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v3/model/3mfDownload");
  }

}  // namespace cxcloud
