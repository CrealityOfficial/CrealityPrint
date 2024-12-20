#include "cxcloud/model/profile_model.h"

#ifndef __APPLE__
#  include <filesystem>
#  include <fstream>
#endif

#include <optional>

#ifdef __APPLE__
#  include <boost/filesystem/fstream.hpp>
#  include <boost/filesystem/operations.hpp>
#  include <boost/filesystem/path.hpp>
#endif

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <qtusercore/module/systemutil.h>

#include "cxcloud/tool/json_tool.h"

namespace cxcloud {

  namespace {

    auto PrinterProfileToJsonDocument(const Profile& profile)
        -> std::optional<rapidjson::Document> {
      if (profile.printer_list.empty() || profile.series_list.empty()) {
        return std::nullopt;
      }

      auto document = rapidjson::Document{ rapidjson::Type::kObjectType };
      auto& allocator = document.GetAllocator();
      auto& root = document;

      auto printer_array = rapidjson::Value{ rapidjson::Type::kArrayType };
      for (const auto& printer : profile.printer_list) {
        auto printer_object = rapidjson::Value{ rapidjson::Type::kObjectType };

        auto engine_version = rapidjson::Value{ rapidjson::Type::kStringType };
        engine_version.Set(printer.engine_version.toString().toUtf8().constData(), allocator);
        printer_object.AddMember("engineVersion", std::move(engine_version), allocator);

        auto series_id = rapidjson::Value{ rapidjson::Type::kNumberType };
        series_id.Set(printer.series_id);
        printer_object.AddMember("seriesId", std::move(series_id), allocator);

        auto series_type = rapidjson::Value{ rapidjson::Type::kNumberType };
        series_type.Set(printer.series_type);
        printer_object.AddMember("type", std::move(series_type), allocator);

        auto internal_name = rapidjson::Value{ rapidjson::Type::kStringType };
        internal_name.Set(printer.internal_name.toUtf8().constData(), allocator);
        printer_object.AddMember("printerIntName", std::move(internal_name), allocator);

        auto external_name = rapidjson::Value{ rapidjson::Type::kStringType };
        external_name.Set(printer.external_name.toUtf8().constData(), allocator);
        printer_object.AddMember("name", std::move(external_name), allocator);

        auto thumbnail = rapidjson::Value{ rapidjson::Type::kStringType };
        thumbnail.Set(printer.thumbnail.toUtf8().constData(), allocator);
        printer_object.AddMember("thumbnail", std::move(thumbnail), allocator);

        auto x_size = rapidjson::Value{ rapidjson::Type::kNumberType };
        x_size.Set(printer.x_size);
        printer_object.AddMember("xSize", std::move(x_size), allocator);

        auto y_size = rapidjson::Value{ rapidjson::Type::kNumberType };
        y_size.Set(printer.y_size);
        printer_object.AddMember("ySize", std::move(y_size), allocator);

        auto z_size = rapidjson::Value{ rapidjson::Type::kNumberType };
        z_size.Set(printer.z_size);
        printer_object.AddMember("zSize", std::move(z_size), allocator);

        auto nozzle_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (const auto& nozzle : printer.nozzles) {
          auto nozzle_string = rapidjson::Value{ rapidjson::Type::kStringType };
          nozzle_string.Set(nozzle.toUtf8().constData(), allocator);
          nozzle_array.PushBack(std::move(nozzle_string), allocator);
        }
        printer_object.AddMember("nozzleDiameter", std::move(nozzle_array), allocator);

        auto material_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (const auto& material : printer.materials) {
          auto material_string = rapidjson::Value{ rapidjson::Type::kStringType };
          material_string.Set(material.toUtf8().constData(), allocator);
          material_array.PushBack(std::move(material_string), allocator);
        }
        printer_object.AddMember("supMaterialType", std::move(material_array), allocator);

        auto profile_version = rapidjson::Value{ rapidjson::Type::kStringType };
        profile_version.Set(printer.profile_version.toUtf8().constData(), allocator);
        printer_object.AddMember("version", std::move(profile_version), allocator);

        auto profile_zip_url = rapidjson::Value{ rapidjson::Type::kStringType };
        profile_zip_url.Set(printer.profile_zip_url.toUtf8().constData(), allocator);
        printer_object.AddMember("zipUrl", std::move(profile_zip_url), allocator);

        auto profile_show_version = rapidjson::Value{ rapidjson::Type::kStringType };
        profile_show_version.Set(printer.show_version.toUtf8().constData(), allocator);
        printer_object.AddMember("showVersion", std::move(profile_show_version), allocator);

        auto profile_change_log = rapidjson::Value{ rapidjson::Type::kStringType };
        profile_change_log.Set(printer.change_log.toUtf8().constData(), allocator);
        printer_object.AddMember("descriptionI18n", std::move(profile_change_log), allocator);

        printer_array.PushBack(std::move(printer_object), allocator);
      }
      root.AddMember("printerList", std::move(printer_array), allocator);

      auto series_array = rapidjson::Value{ rapidjson::Type::kArrayType };
      for (const auto& series : profile.series_list) {
        auto series_object = rapidjson::Value{ rapidjson::Type::kObjectType };

        auto id = rapidjson::Value{ rapidjson::Type::kNumberType };
        id.Set(series.id);
        series_object.AddMember("id", std::move(id), allocator);

        auto name = rapidjson::Value{ rapidjson::Type::kStringType };
        name.Set(series.name.toUtf8().constData(), allocator);
        series_object.AddMember("name", std::move(name), allocator);

        auto type = rapidjson::Value{ rapidjson::Type::kNumberType };
        type.Set(series.type);
        series_object.AddMember("type", std::move(type), allocator);

        series_array.PushBack(std::move(series_object), allocator);
      }
      root.AddMember("series", std::move(series_array), allocator);

      return std::make_optional<rapidjson::Document>(std::move(document));
    }

    auto JsonDocumentToPrinterProfile(const rapidjson::Document& document)
        -> std::optional<Profile> {
      auto& root = document;
      if (!root.IsObject()) {
        return std::nullopt;
      }

      if (!root.HasMember("printerList") || !root["printerList"].IsArray() ||
          !root.HasMember("series") || !root["series"].IsArray() ) {
        return std::nullopt;
      }

      auto printer_array = root["printerList"].GetArray();
      auto series_array = root["series"].GetArray();
      if (printer_array.Empty() || series_array.Empty()) {
        return std::nullopt;
      }

      return std::make_optional<Profile>({
        [&printer_array]() {  // printer_list
          auto printer_list = decltype(Profile::printer_list){};
          using Nozzles = decltype(Profile::Printer::nozzles);
          using Materials = decltype(Profile::Printer::materials);
          for (const auto& printer : printer_array) {
            const auto internal_name = JsonGet<QString>(printer, "printerIntName");
            const auto nozzles = JsonGet<Nozzles>(printer, "nozzleDiameter");
            if (!internal_name || !nozzles) {
              continue;
            }

            printer_list.emplace_back(Profile::Printer{
              [&internal_name, &nozzles] {                                      // uid
                auto uid = *internal_name;
                for (const auto& nozzle : *nozzles) {
                  uid.push_back(QStringLiteral("-%1").arg(nozzle));
                }
                return uid;
              }(),
                                                                                // engine_version
              Version{ JsonGet<QString>(printer, "engineVersion").value_or(QString{}) },
              JsonGet<int>(printer, "seriesId").value_or(0),                    // series_id
              JsonGet<int>(printer, "type").value_or(0),                        // series_type
              *internal_name,                                                   // internal_name
              JsonGet<QString>(printer, "name").value_or(QString{}),            // external_name
              JsonGet<QString>(printer, "thumbnail").value_or(QString{}),       // thumbnail
              JsonGet<double>(printer, "xSize").value_or(0.),                   // x_size
              JsonGet<double>(printer, "ySize").value_or(0.),                   // y_size
              JsonGet<double>(printer, "zSize").value_or(0.),                   // z_size
              *nozzles,                                                         // nozzles
              JsonGet<Materials>(printer, "supMaterialType").value_or(Materials{}),  // materials
              JsonGet<QString>(printer, "version").value_or(QString{}),         // profile_version
              JsonGet<QString>(printer, "zipUrl").value_or(QString{}),          // profile_zip_url
              JsonGet<QString>(printer, "showVersion").value_or(QString{}),     // show_version
              JsonGet<QString>(printer, "descriptionI18n").value_or(QString{}), // change_log
            });
          }
          return printer_list;
        }(),
        [&series_array]() {   // series_list
          auto series_list = decltype(Profile::series_list){};
          for (const auto& series : series_array) {
            series_list.emplace_back(Profile::Series{
              JsonGet<int>(series, "id").value_or(0),                // id
              JsonGet<QString>(series, "name").value_or(QString{}),  // name
              JsonGet<int>(series, "type").value_or(0),              // type
            });
          }
          return series_list;
        }(),
      });
    }

    auto JsonDocumentToUserProfile(const rapidjson::Document& document)
        -> std::optional<Profile> {
      auto& root = document;
      if (!root.IsObject()) {
        return std::nullopt;
      }

      if (!root.HasMember("list") || !root["list"].IsArray()){
        return std::nullopt;
      }

      auto printer_array = root["list"].GetArray();
      if (printer_array.Empty()) {
        return std::nullopt;
      }

      return std::make_optional<Profile>({
        [&printer_array]() {                     // printer_list
          auto printer_list = decltype(Profile::printer_list){};
          for (const auto& printer : printer_array) {
            using Nozzles = decltype(Profile::Printer::nozzles);
            const auto internal_name = JsonGet<QString>(printer, "printerIntName");
            const auto nozzles = JsonGet<Nozzles>(printer, "nozzleDiameter");
            if (!internal_name || !nozzles) {
              continue;
            }

            printer_list.emplace_back(Profile::Printer{
              [&printer, &internal_name, &nozzles] {                        // uid
                auto uid = *internal_name;
                for (const auto& nozzle : *nozzles) {
                  uid.push_back(QStringLiteral("-%1").arg(nozzle));
                }
                return uid;
              }(),
                                                                            // engine_version
              Version{ JsonGet<QString>(printer, "engineVersion").value_or(QString{}) },
              0,                                                            // series_id
              0,                                                            // series_type
              *internal_name,                                               // internal_name
              JsonGet<QString>(printer, "name").value_or(QString{}),        // external_name
              QStringLiteral(""),                                           // thumbnail
              0.0,                                                          // x_size
              0.0,                                                          // y_size
              0.0,                                                          // z_size
              std::move(*nozzles),                                          // nozzles
              {},                                                           // materials
              JsonGet<QString>(printer, "version").value_or(QString{}),     // profile_version
              JsonGet<QString>(printer, "zipUrl").value_or(QString{}),      // profile_zip_url
              {},                                                           // show_version
              {},                                                           // change_log
            });
          }
          return printer_list;
        }(),
        decltype(Profile::series_list){}  // series_list
      });
    }

    auto JsonDocumentToUploadPolicy(const rapidjson::Document& document)
        -> std::optional<UploadPolicy> {
      auto& root = document;
      if (!root.IsObject()) {
        return std::nullopt;
      }

      if (!root.HasMember("uploadPolicy") || !root["uploadPolicy"].IsObject()) {
        return std::nullopt;
      }

      auto result = root["uploadPolicy"].GetObject();

      return std::make_optional<UploadPolicy>({
        JsonGet<QString>(result, "Callback").value_or(QString{}),        // callback
        JsonGet<int>(result, "ExpireAt").value_or(0),                    // expire_at
        JsonGet<QString>(result, "Host").value_or(QString{}),            // host
        JsonGet<QString>(result, "Key").value_or(QString{}),             // key
        JsonGet<QString>(result, "OSSAccessKeyId").value_or(QString{}),  // oss_access_key_id
        JsonGet<QString>(result, "Policy").value_or(QString{}),          // policy
        JsonGet<QString>(result, "Signature").value_or(QString{}),       // signature
      });
    }

  }  // namespace





  auto PrinterProfileToJsonBytes(const Profile& profile) -> QByteArray {
    const auto json_document = PrinterProfileToJsonDocument(profile);
    if (!json_document) {
      return {};
    }

    auto json_buffer = rapidjson::StringBuffer{};
    auto json_writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>{ json_buffer };
    if (!json_document->Accept(json_writer)) {
      return {};
    }

    return QByteArray::fromRawData(json_buffer.GetString(), json_buffer.GetSize());
  }

  auto JsonBytesToPrinterProfile(const QByteArray& bytes) -> Profile {
    auto json_stream = rapidjson::StringStream{ bytes.constData() };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return {};
    }

    auto profile = JsonDocumentToPrinterProfile(json_document);
    if (!profile) {
      return {};
    }

    return std::move(*profile);
  }

  auto JsonBytesToUserProfile(const QByteArray& bytes) -> Profile {
    auto json_stream = rapidjson::StringStream{ bytes.constData() };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return {};
    }

    auto profile = JsonDocumentToUserProfile(json_document);
    if (!profile) {
      return {};
    }

    return std::move(*profile);
  }

  auto JsonBytesToUploadPolicy(const QByteArray& bytes) -> UploadPolicy {
    auto json_stream = rapidjson::StringStream{ bytes.constData() };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return {};
    }

    auto policy = JsonDocumentToUploadPolicy(json_document);
    if (!policy) {
      return {};
    }

    return std::move(*policy);
  }

  auto JsonBytesToMaterials(const QByteArray& bytes) -> std::list<PrinterMaterial> {
    auto materials = decltype(JsonBytesToMaterials({})){};

    auto json_stream = rapidjson::StringStream{bytes.constData()};
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return materials;
    }

    auto& root = json_document;
    if (!root.IsObject()) {
      return materials;
    }

    if (!root.HasMember("list") || !root["list"].IsArray()) {
      return materials;
    }

    for (const auto& printer : root["list"].GetArray()) {
      materials.emplace_back(PrinterMaterial{
        JsonGet<QString>(printer, "id").value_or(QString{}),            // id
        JsonGet<QString>(printer, "brand").value_or(QString{}),         // brand
        JsonGet<QString>(printer, "name").value_or(QString{}),          // name
        JsonGet<QString>(printer, "meterialType").value_or(QString{}),  // type
        JsonGet<QString>(printer, "diameter").value_or(QString{}),      // support_diameters
        JsonGet<int>(printer, "rank").value_or(0),                      // rank
      });
    }

    return materials;
  }

  auto JsonBytesToMaterialTemplates(const QByteArray& bytes) -> std::list<MaterialTemplate> {
    auto templates = decltype(JsonBytesToMaterialTemplates({})){};

    auto json_stream = rapidjson::StringStream{ bytes.constData() };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return templates;
    }

    auto& root = json_document;
    if (!root.IsObject()) {
      return templates;
    }

    if (!root.HasMember("list") || !root["list"].IsArray()) {
      return templates;
    }

    for (const auto& printer : root["list"].GetArray()) {
      templates.emplace_back(decltype(templates)::value_type{
        JsonGet<QString>(printer, "tmplName").value_or(QString{}),      // name
        JsonGet<QString>(printer, "meterialType").value_or(QString{}),  // type
        [&templates, &printer] {                                        // engine_data
          auto engine_data = decltype(templates.front().engine_data){};
          if (printer.HasMember("kvParam") && printer["kvParam"].IsObject()) {
            auto parameter_map = printer["kvParam"].GetObject();
            for (auto iter = parameter_map.begin(); iter != parameter_map.end(); ++iter) {
              if (iter->name.IsString() && iter->value.IsString()) {
                auto&& pair = std::make_pair(iter->name.GetString(), iter->value.GetString());
                engine_data.emplace(std::move(pair));
              }
            }
          }
          return engine_data;
        }(),
      });
    }

    return templates;
  }

  auto JsonBytesToProcessTemplates(const QByteArray& bytes) -> std::list<ProcessTemplate> {
    auto templates = decltype(JsonBytesToProcessTemplates({})){};

    auto json_stream = rapidjson::StringStream{ bytes.constData() };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return templates;
    }

    auto& root = json_document;
    if (!root.IsObject()) {
      return templates;
    }

    if (!root.HasMember("list") || !root["list"].IsArray()) {
      return templates;
    }

    for (const auto& printer : root["list"].GetArray()) {
      templates.emplace_back(decltype(templates)::value_type{
        JsonGet<QString>(printer, "tmplName").value_or(QString{}),  // name
        [&templates, &printer] {                                    // engine_data
          auto engine_data = decltype(templates.front().engine_data){};
          if (printer.HasMember("kvParam") && printer["kvParam"].IsObject()) {
            auto parameter_map = printer["kvParam"].GetObject();
            for (auto iter = parameter_map.begin(); iter != parameter_map.end(); ++iter) {
              if (iter->name.IsString() && iter->value.IsString()) {
                auto&& pair = std::make_pair(iter->name.GetString(), iter->value.GetString());
                engine_data.emplace(std::move(pair));
              }
            }
          }
          return engine_data;
        }(),
      });
    }

    return templates;
  }

  auto SaveJson2File(const std::optional<rapidjson::Document>& json_document, const QString& path) -> bool {
    if (!json_document) {
      return false;
    }

#ifdef __APPLE__
    const auto file_path = boost::filesystem::path{ path.toStdWString() };
    boost::filesystem::create_directories(file_path.parent_path());
    auto file_stream = boost::filesystem::ofstream{ file_path, std::ios::binary };
#else
    const auto file_path = std::filesystem::path{ path.toStdWString() };
    std::filesystem::create_directories(file_path.parent_path());
    auto file_stream = std::ofstream{ file_path, std::ios::binary };
#endif
    if (!file_stream.is_open()) {
      return false;
    }

    auto json_stream = rapidjson::OStreamWrapper{ file_stream };
    auto json_writer = rapidjson::PrettyWriter<rapidjson::OStreamWrapper>{ json_stream };
#ifdef QT_DEBUG
    json_writer.SetIndent(' ', 2);
#endif // QT_DEBUG

    return json_document->Accept(json_writer);
  }

  auto SavePrinterProfile(const Profile& profile, const QString& path) -> bool {
    return SaveJson2File(PrinterProfileToJsonDocument(profile), path);
  }

  auto LoadPrinterProfile(Profile& profile, const QString& path) -> bool {
#ifdef __APPLE__
    const auto file_path = boost::filesystem::path{ path.toStdWString() };
    auto file_stream = boost::filesystem::ifstream{ file_path, std::ios::binary };
#else
    const auto file_path = std::filesystem::path{ path.toStdWString() };
    auto file_stream = std::ifstream{ file_path, std::ios::binary };
#endif
    if (!file_stream.is_open()) {
      return false;
    }

    auto json_stream = rapidjson::IStreamWrapper{ file_stream };
    auto json_document = rapidjson::Document{};
    if (json_document.ParseStream(json_stream).HasParseError()) {
      return false;
    }

    auto profile_optional = JsonDocumentToPrinterProfile(json_document);
    if (!profile_optional) {
      return false;
    }

    profile = std::move(*profile_optional);
    return true;
  }

  auto SavePrinterMaterial(const std::list<PrinterMaterial>& templates,
                           const QString& path) -> bool {
    if (templates.empty()) {
      return false;
    }

    auto document = rapidjson::Document{ rapidjson::Type::kObjectType };
    auto& allocator = document.GetAllocator();

    auto series_array = rapidjson::Value{ rapidjson::Type::kArrayType };
    for (const auto& material : templates) {
      auto material_object = rapidjson::Value{ rapidjson::Type::kObjectType };

      auto id = rapidjson::Value{ material.id.toUtf8().constData(), allocator };
      material_object.AddMember("id", std::move(id), allocator);

      auto name = rapidjson::Value{ material.name.toUtf8().constData(), allocator };
      material_object.AddMember("name", std::move(name), allocator);

      auto type = rapidjson::Value{ material.type.toUtf8().constData(), allocator };
      material_object.AddMember("type", std::move(type), allocator);

      auto brand = rapidjson::Value{ material.brand.toUtf8().constData(), allocator };
      material_object.AddMember("brand", std::move(brand), allocator);

      auto support_diameters =
          rapidjson::Value{ material.support_diameters.toUtf8().constData(), allocator };
      material_object.AddMember("supportDiameters", std::move(support_diameters), allocator);

      material_object.AddMember("rank", std::move(material.rank), allocator);

      series_array.PushBack(std::move(material_object), allocator);
    }

    document.AddMember("materials", std::move(series_array), allocator);

    return SaveJson2File(std::make_optional<rapidjson::Document>(std::move(document)), path);
  }

  auto SaveMaterialTemplates(const std::list<MaterialTemplate>& templates,
                             const QString& path) -> bool {
    if (templates.empty()) {
      return false;
    }

    for (const auto& material_template : templates) {
      auto engine_data_object = rapidjson::Value{ rapidjson::Type::kObjectType };
      auto document = rapidjson::Document{ rapidjson::Type::kObjectType };
      auto& allocator = document.GetAllocator();

      for (const auto& data : material_template.engine_data) {
        rapidjson::Value key(data.first.toStdString().c_str(), allocator);
        rapidjson::Value value(data.second.toStdString().c_str(), allocator);
        engine_data_object.AddMember(key, value, allocator);
      }

      document.AddMember("engine_data", std::move(engine_data_object), allocator);
      const auto filename = QStringLiteral("%1/%2.json").arg(path, material_template.name);
      SaveJson2File(std::make_optional<rapidjson::Document>(std::move(document)), filename);
    }

    return true;
  }

  auto SaveProcessTemplates(const std::list<ProcessTemplate>& templates,
                            const QString& path) -> bool {
    if (templates.empty()) {
      return false;
    }

    for (const auto& process_template : templates) {
      auto engine_data_object = rapidjson::Value{ rapidjson::Type::kObjectType };
      auto document = rapidjson::Document{ rapidjson::Type::kObjectType };
      auto& allocator = document.GetAllocator();

      for (const auto& data : process_template.engine_data) {
        rapidjson::Value key(data.first.toStdString().c_str(), allocator);
        rapidjson::Value value(data.second.toStdString().c_str(), allocator);
        engine_data_object.AddMember(key, value, allocator);
      }

      document.AddMember("engine_data", std::move(engine_data_object), allocator);
      const auto filename = QStringLiteral("%1/%2.json").arg(path, process_template.name);
      SaveJson2File(std::make_optional<rapidjson::Document>(std::move(document)), filename);
    }

    return true;
  }





  auto PrinterUpdateListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.local.uid);
      }
      case DataRole::DISPLAY_NAME: {
        auto name = data.local.external_name;
        for (const auto& nozzle : data.local.nozzles) {
          name.push_back(QStringLiteral("-%1").arg(nozzle));
        }
        name.push_back(QStringLiteral(" nozzle"));
        return QVariant::fromValue(std::move(name));
      }
      case DataRole::LOCAL_VERSION: {
        return QVariant::fromValue(data.local.show_version);
      }
      case DataRole::ONLINE_VERSION: {
        return QVariant::fromValue(data.online.show_version);
      }
      case DataRole::CHANGE_LOG: {
        return QVariant::fromValue(data.online.change_log);
      }
      case DataRole::UPDATEABLE: {
        return QVariant::fromValue(data.updateable);
      }
      case DataRole::UPDATING: {
        return QVariant::fromValue(data.updating);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto PrinterUpdateListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = decltype(roleNames()){
      { static_cast<int>(DataRole::UID           ), QByteArrayLiteral("model_uid"           ) },
      { static_cast<int>(DataRole::DISPLAY_NAME  ), QByteArrayLiteral("model_display_name"  ) },
      { static_cast<int>(DataRole::LOCAL_VERSION ), QByteArrayLiteral("model_local_version" ) },
      { static_cast<int>(DataRole::ONLINE_VERSION), QByteArrayLiteral("model_online_version") },
      { static_cast<int>(DataRole::CHANGE_LOG    ), QByteArrayLiteral("model_change_log"    ) },
      { static_cast<int>(DataRole::UPDATEABLE    ), QByteArrayLiteral("model_updateable"    ) },
      { static_cast<int>(DataRole::UPDATING      ), QByteArrayLiteral("model_updating"      ) },
    };

    return ROLE_NAMES;
  }

}  // namespace cxcloud
