#include "cxcloud/net/profile_request.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace cxcloud {

  LoadPrinterProfileRequest::LoadPrinterProfileRequest(const Version& engine_version,
                                                       QObject* parent)
      : HttpRequest{ parent }, engine_version_{ engine_version } {}

  auto LoadPrinterProfileRequest::getEngineVersion() const -> Version {
    return engine_version_;
  }

  auto LoadPrinterProfileRequest::setEngineVersion(const Version& version) -> void {
    engine_version_ = version;
  }

  auto LoadPrinterProfileRequest::profile() const -> const Profile& {
    return profile_;
  }

  auto LoadPrinterProfileRequest::getProfile() const -> Profile {
    return profile_;
  }

  auto LoadPrinterProfileRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("engineVersion"), getEngineVersion().toString() },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadPrinterProfileRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/official/printerList");
  }

  auto LoadPrinterProfileRequest::callWhenSuccessed() -> void {
    auto json = getResponseJson()[QStringLiteral("result")].toObject();
    auto json_bytes = QJsonDocument{ std::move(json) }.toJson(QJsonDocument::JsonFormat::Compact);
    profile_ = JsonBytesToPrinterProfile(std::move(json_bytes));
  }





  CheckPrinterUpdatableRequest::CheckPrinterUpdatableRequest(QObject* parent)
      : HttpRequest{ parent } {}

  CheckPrinterUpdatableRequest::CheckPrinterUpdatableRequest(
    const Version& engine_version, const std::list<Printer>& checked_printer_list, QObject* parent)
      : HttpRequest{ parent }
      , engine_version_{ engine_version }
      , checked_printer_list_{ checked_printer_list } {}

  CheckPrinterUpdatableRequest::CheckPrinterUpdatableRequest(
    const Version& engine_version, std::list<Printer>&& checked_printer_list, QObject* parent)
      : HttpRequest{ parent }
      , engine_version_{ engine_version }
      , checked_printer_list_{ std::move(checked_printer_list) } {}

  auto CheckPrinterUpdatableRequest::getEngineVersion() const -> Version {
    return engine_version_;
  }

  auto CheckPrinterUpdatableRequest::setEngineVersion(const Version& version) -> void {
    engine_version_ = version;
  }

  auto CheckPrinterUpdatableRequest::checkedPrinterList() const -> const std::list<Printer>& {
    return checked_printer_list_;
  }

  auto CheckPrinterUpdatableRequest::getCheckedPrinterList() const -> std::list<Printer> {
    return checked_printer_list_;
  }

  auto CheckPrinterUpdatableRequest::setCheckedPrinterList(
      const std::list<Printer>& printer_list) -> void {
    checked_printer_list_ = printer_list;
  }

  auto CheckPrinterUpdatableRequest::setCheckedPrinterList(
      std::list<Printer>&& printer_list) -> void {
    checked_printer_list_ = std::move(printer_list);
  }

  auto CheckPrinterUpdatableRequest::updatablePrinterList() const -> const std::list<Printer>& {
    return updatable_printers_;
  }

  auto CheckPrinterUpdatableRequest::getUpdatablePrinterList() const -> std::list<Printer> {
    return updatable_printers_;
  }

  auto CheckPrinterUpdatableRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("engineVersion"), getEngineVersion().toString() },
      { QStringLiteral("printers"), [this]() {
        QJsonArray printer_array;
        for (const auto& printer : updatablePrinterList()) {
          printer_array.push_back(QJsonValue{ QJsonObject{
            { QStringLiteral("nozzleDiameter"), [&nozzles = printer.nozzles]() {
              QJsonArray nozzle_array;
              for (const auto& nozzle : nozzles) {
                nozzle_array.push_back(QJsonValue{ nozzle });
              }
              return nozzle_array;
            }() },
            { QStringLiteral("printerIntName"), printer.internal_name },
            { QStringLiteral("version"), printer.profile_version },
          } });
        }
        return printer_array;
      }() },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto CheckPrinterUpdatableRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/official/profileUpdateCheck");
  }

  auto CheckPrinterUpdatableRequest::callWhenSuccessed() -> void {
    const auto& root = getResponseJson();
    if (root.empty()) {
      return;
    }

    const auto& printer_array =
      root[QStringLiteral("result")][QStringLiteral("updateList")].toArray();
    if (!printer_array.empty()) {
      return;
    }

    updatable_printers_.clear();

    for (const auto& printer : printer_array) {
      if (!printer.isObject()) {
        continue;
      }

      auto internal_name = printer[QStringLiteral("printerIntName")].toString();

      for (const auto& checked_printer : checkedPrinterList()) {
        if (checked_printer.internal_name != internal_name) {
          continue;
        }

        auto nozzles = [printer]() {
          auto nozzles = decltype(Printer::nozzles){};
          for (const auto& nozzle : printer[QStringLiteral("nozzleDiameter")].toArray()) {
            if (nozzle.isString()) {
              nozzles.emplace_back(nozzle.toString());
            }
          }
          return nozzles;
        }();

        updatable_printers_.emplace_back(Printer{
          [printer, &internal_name, &nozzles] {                           // uid
            auto uid = internal_name;
            for (const auto& nozzle : nozzles) {
              uid.push_back(QStringLiteral("-%1").arg(nozzle));
            }
            return uid;
          }(),
          Version{ printer[QStringLiteral("engineVersion")].toString() },  // engine_version
          checked_printer.series_id,                                       // series_id
          checked_printer.series_type,                                     // series_type
          std::move(internal_name),                                        // internal_name
          checked_printer.external_name,                                   // external_name
          checked_printer.thumbnail,                                       // thumbnail
          checked_printer.x_size,                                          // x_size
          checked_printer.y_size,                                          // y_size
          checked_printer.z_size,                                          // z_size
          std::move(nozzles),                                              // nozzles
          checked_printer.materials,                                       // materials
          printer[QStringLiteral("version")].toString(),                   // profile_version
          printer[QStringLiteral("zipUrl")].toString(),                    // profile_zip_url
        });
      }
    }
  }





  LoadUserProfileRequest::LoadUserProfileRequest(QObject* parent) : HttpRequest{parent} {}

  auto LoadUserProfileRequest::profile() const -> const Profile& {
    return profile_;
  }

  auto LoadUserProfileRequest::getProfile() const -> Profile {
    return profile_;
  }

  auto LoadUserProfileRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{QJsonObject{}}.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadUserProfileRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/user/list");
  }

  auto LoadUserProfileRequest::callWhenSuccessed() -> void {
    auto json = getResponseJson()[QStringLiteral("result")].toObject();
    auto json_bytes = QJsonDocument{std::move(json)}.toJson(QJsonDocument::JsonFormat::Compact);
    profile_ = JsonBytesToUserProfile(std::move(json_bytes));
  }





  PreUploadProfile::PreUploadProfile(const QString& printer_internal_name,
                                     const QStringList& printer_nozzle,
                                     const QString& md5,
                                     QObject* parent)
      : HttpRequest{ parent }
      , printer_internal_name_{ printer_internal_name }
      , printer_nozzles_{ printer_nozzle }
      , md5_{ md5 } {}

  auto PreUploadProfile::uploadPolicy() const -> const UploadPolicy& {
    return policy_;
  }

  auto PreUploadProfile::getUploadPolicy() const -> UploadPolicy {
    return policy_;
  }

  auto PreUploadProfile::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), QStringLiteral("") },
      { QStringLiteral("md5"), md5_ },
      { QStringLiteral("name"), printer_internal_name_ },
      { QStringLiteral("nozzleDiameter"),
        [&nozzles = printer_nozzles_] {
          auto nozzle_array = QJsonArray{};
          for (const auto& nozzle : nozzles) {
            nozzle_array.push_back(QJsonValue{ nozzle });
          }
          return nozzle_array;
        }() },
      { QStringLiteral("printerIntName"), printer_internal_name_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto PreUploadProfile::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/user/preUploadProfile");
  }

  auto PreUploadProfile::callWhenSuccessed() -> void {
    auto json = getResponseJson()[QStringLiteral("result")].toObject();
    auto json_bytes = QJsonDocument{ std::move(json) }.toJson(QJsonDocument::JsonFormat::Compact);
    policy_ = JsonBytesToUploadPolicy(std::move(json_bytes));
  }





  LoadPrinterMaterialRequest::LoadPrinterMaterialRequest(QObject* parent)
      : HttpRequest{ parent } {}

  auto LoadPrinterMaterialRequest::materials() -> std::list<PrinterMaterial>& {
    return materials_;
  }

  auto LoadPrinterMaterialRequest::materials() const -> const std::list<PrinterMaterial>& {
    return materials_;
  }

  auto LoadPrinterMaterialRequest::getMaterials() const -> std::list<PrinterMaterial> {
    return materials_;
  }

  auto LoadPrinterMaterialRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("pageSize"), 100 },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadPrinterMaterialRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/official/materialList");
  }

  auto LoadPrinterMaterialRequest::callWhenSuccessed() -> void {
    auto json = getResponseJson()[QStringLiteral("result")].toObject();
    auto json_bytes = QJsonDocument{std::move(json)}.toJson(QJsonDocument::JsonFormat::Compact);
    materials_ = JsonBytesToMaterials(std::move(json_bytes));
  }





  LoadMaterialTemplateRequest::LoadMaterialTemplateRequest(const Version& engine_version,
                                                           QObject* parent)
      : HttpRequest{ parent }, engine_version_{ engine_version } {}

  auto LoadMaterialTemplateRequest::getEngineVersion() const -> Version {
    return engine_version_;
  }

  auto LoadMaterialTemplateRequest::setEngineVersion(const Version& version) -> void {
    engine_version_ = version;
  }

  auto LoadMaterialTemplateRequest::materialTemplates() -> std::list<MaterialTemplate>& {
    return material_templates_;
  }

  auto LoadMaterialTemplateRequest::materialTemplates() const -> const std::list<MaterialTemplate>& {
    return material_templates_;
  }

  auto LoadMaterialTemplateRequest::getMaterialTemplates() const -> std::list<MaterialTemplate> {
    return material_templates_;
  }

  auto LoadMaterialTemplateRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("engineVersion"), getEngineVersion().toString() },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadMaterialTemplateRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/official/material/tmplList");
  }

  auto LoadMaterialTemplateRequest::callWhenSuccessed() -> void {
    auto json = getResponseJson()[QStringLiteral("result")].toObject();
    auto json_bytes = QJsonDocument{ std::move(json) }.toJson(QJsonDocument::JsonFormat::Compact);
    material_templates_ = JsonBytesToMaterialTemplates(std::move(json_bytes));
  }





  LoadProcessTemplateRequest::LoadProcessTemplateRequest(const Version& engine_version,
                                                         QObject* parent)
      : HttpRequest{ parent }, engine_version_{ engine_version } {}

  auto LoadProcessTemplateRequest::getEngineVersion() const -> Version {
    return engine_version_;
  }

  auto LoadProcessTemplateRequest::setEngineVersion(const Version& version) -> void {
    engine_version_ = version;
  }

  auto LoadProcessTemplateRequest::processTemplates() -> std::list<ProcessTemplate>& {
    return process_templates_;
  }

  auto LoadProcessTemplateRequest::processTemplates() const -> const std::list<ProcessTemplate>& {
    return process_templates_;
  }

  auto LoadProcessTemplateRequest::getProcessTemplates() const -> std::list<ProcessTemplate> {
    return process_templates_;
  }

  auto LoadProcessTemplateRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("engineVersion"), getEngineVersion().toString() },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadProcessTemplateRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/slice/profile/official/process/tmplList");
  }

  auto LoadProcessTemplateRequest::callWhenSuccessed() -> void {
    auto json = getResponseJson()[QStringLiteral("result")].toObject();
    auto json_bytes = QJsonDocument{ std::move(json) }.toJson(QJsonDocument::JsonFormat::Compact);
    process_templates_ = JsonBytesToProcessTemplates(std::move(json_bytes));
  }

}  // namespace cxcloud
