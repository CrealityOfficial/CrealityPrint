#include "cxcloud/service/profile_service.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>

#include <cpr/api.h>

#include <qtusercore/module/quazipfile.h>
#include <qtusercore/module/systemutil.h>
#include <qtusercore/string/resourcesfinder.h>
#include <qtusercore/util/settings.h>

#include "cxcloud/net/common_request.h"
#include "cxcloud/net/profile_request.h"
#include "cxcloud/tool/settings.h"

namespace cxcloud {

  ProfileService::ProfileService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , profile_dir_path_{}
      , user_profile_dir_path_{}
      , printer_dir_name_{ QStringLiteral("printer") }
      , printer_file_name_{ QStringLiteral("machineList.json") }
      , material_file_name_{ QStringLiteral("materialList.json") }
      , material_template_dir_name_{ QStringLiteral("material_templates") }
      , process_template_dir_name_{ QStringLiteral("process_templates") }
      , engine_version_{ "1.6.0"_v }
      , printer_update_model_{ std::make_unique<PrinterUpdateListModel>() } {
    using This = std::remove_pointer<decltype(this)>::type;

    connect(this, &This::profileDirPathChanged, this, &This::printerDirPathChanged);
    connect(this, &This::printerDirNameChanged, this, &This::printerDirPathChanged);

    connect(this, &This::userProfileDirPathChanged, this, &This::userPrinterDirPathChanged);
    connect(this, &This::printerDirNameChanged, this, &This::userPrinterDirPathChanged);

    connect(this, &This::profileDirPathChanged, this, &This::printerFilePathChanged);
    connect(this, &This::printerFileNameChanged, this, &This::printerFilePathChanged);

    connect(this, &This::profileDirPathChanged, this, &This::materialFilePathChanged);
    connect(this, &This::materialFileNameChanged, this, &This::materialFilePathChanged);

    connect(this, &This::profileDirPathChanged, this, &This::materialTemplateDirPathChanged);
    connect(this, &This::materialTemplateDirNameChanged,
            this, &This::materialTemplateDirPathChanged);

    connect(this, &This::profileDirPathChanged, this, &This::processTemplateDirPathChanged);
    connect(this, &This::processTemplateDirNameChanged, this, &This::processTemplateDirPathChanged);
  }

  auto ProfileService::initialize() -> void {
    initialized_ = true;
  }

  auto ProfileService::uninitialize() -> void {
    initialized_ = false;

    using namespace std::chrono;
    if (async_future_.valid() && async_future_.wait_for(0ms) != std::future_status::ready) {
      // std::future::wait() will throw error when std::future::vaild() return false in MSVC.
      // So check the state first here.
      async_future_.wait();
    }
  }





  auto ProfileService::getProfileDirPath() const -> QString {
    return profile_dir_path_;
  }

  auto ProfileService::setProfileDirPath(const QString& path) -> void {
    if (profile_dir_path_ != path) {
      profile_dir_path_ = path;
      profileDirPathChanged();
    }
  }

  auto ProfileService::getUserProfileDirPath() const -> QString {
      return user_profile_dir_path_;
  }

  auto ProfileService::setUserProfileDirPath(const QString& path) -> void {
    if (user_profile_dir_path_ != path) {
      user_profile_dir_path_ = path;
      userProfileDirPathChanged();
    }
  }

  auto ProfileService::getPrinterDirName() const -> QString {
    return printer_dir_name_;
  }

  auto ProfileService::setPrinterDirName(const QString& name) -> void {
    if (printer_dir_name_ != name) {
      printer_dir_name_ = name;
      printerDirNameChanged();
    }
  }

  auto ProfileService::getPrinterFileName() const -> QString {
    return printer_file_name_;
  }

  auto ProfileService::setPrinterFileName(const QString& name) -> void {
    if (printer_file_name_ != name) {
      printer_file_name_ = name;
      printerFileNameChanged();
    }
  }

  auto ProfileService::getMaterialFileName() const -> QString {
    return material_file_name_;
  }

  auto ProfileService::setMaterialFileName(const QString& name) -> void {
    if (material_file_name_ != name) {
      material_file_name_ = name;
      materialFileNameChanged();
    }
  }

  auto ProfileService::getMaterialTemplateDirName() const -> QString {
    return material_template_dir_name_;
  }

  auto ProfileService::setMaterialTemplateDirName(const QString& name) -> void {
    if (material_template_dir_name_ != name) {
      material_template_dir_name_ = name;
      materialTemplateDirNameChanged();
    }
  }

  auto ProfileService::getProcessTemplateDirName() const -> QString {
    return process_template_dir_name_;
  }

  auto ProfileService::setProcessTemplateDirName(const QString& name) -> void {
    if (process_template_dir_name_ != name) {
      process_template_dir_name_ = name;
      processTemplateDirNameChanged();
    }
  }

  auto ProfileService::getEngineVersion() const -> Version {
    return engine_version_;
  }

  auto ProfileService::setEngineVersion(const Version& version) -> void {
    if (engine_version_ != version) {
      engine_version_ = version;
      engineVersionChanged();
    }
  }

  auto ProfileService::getEngineVersionString() const -> QString {
    return engine_version_.toString();
  }

  auto ProfileService::setEngineVersionString(const QString& version) -> void {
    setEngineVersion(Version::FromString(version));
  }





  auto ProfileService::getPrinterDirPath() const -> QString {
    return QStringLiteral("%1/%2").arg(getProfileDirPath(), getPrinterDirName());
  }

  auto ProfileService::getUserPrinterDirPath() const -> QString {
    return QStringLiteral("%1/%2").arg(getUserProfileDirPath(), getPrinterDirName());
  }

  auto ProfileService::getPrinterFilePath() const -> QString {
    return QStringLiteral("%1/%2").arg(getProfileDirPath(), getPrinterFileName());
  }

  auto ProfileService::getMaterialFilePath() const -> QString {
    return QStringLiteral("%1/%2").arg(getProfileDirPath(), getMaterialFileName());
  }

  auto ProfileService::getMaterialTemplateDirPath() const -> QString {
    return QStringLiteral("%1/%2").arg(getProfileDirPath(), getMaterialTemplateDirName());
  }

  auto ProfileService::getProcessTemplateDirPath() const -> QString {
    return QStringLiteral("%1/%2").arg(getProfileDirPath(), getProcessTemplateDirName());
  }





  auto ProfileService::isAutoCheckOfficalProfile() const -> bool {
    return CloudSettings{}.value(QStringLiteral("auto_check_profile_updateable"), false).toBool();
  }

  auto ProfileService::setAutoCheckOfficalProfile(bool auto_check) -> void {
    static const auto key = QStringLiteral("auto_check_profile_updateable");
    auto settings = CloudSettings{};
    if (settings.value(key, false).toBool() != auto_check) {
      settings.setValue(key, auto_check);
      autoCheckOfficalProfileChanged();
    }
  }

  auto ProfileService::checkOfficalProfileUpdateable() -> void {
    if (!initialized_) {
      return;
    }

    using namespace std::chrono;
    if (async_future_.valid() && async_future_.wait_for(0ms) != std::future_status::ready) {
      return;
    }

    async_future_ = std::async(std::launch::async, [this]() {
      // printer list
      auto series_list = decltype(local_profile_.series_list){};
      auto updateable_printer_list = decltype(updateable_printer_list_){};
      auto overrideable_printer_list = decltype(overrideable_printer_list_){};
      auto deleteable_printer_list = decltype(deleteable_printer_list_){};
      auto update_info_list = decltype(printer_update_model_)::element_type::DataContainer{};
      do {
        if (!initialized_) {
          return;
        }

        auto request = createRequest<LoadPrinterProfileRequest>(getEngineVersion());
        request->syncPost();
        if (request->getResponseState() != HttpErrorCode::OK) {
          break;
        }

        if (!initialized_) {
          return;
        }

        auto& online_profile = request->profile();
        series_list = std::move(online_profile.series_list);

        if (local_profile_.printer_list.empty()) {
          if (const auto path = getPrinterFilePath(); !LoadPrinterProfile(local_profile_, path)) {
            // if there is no local printer, every online printer is overrideable.
            overrideable_printer_list = online_profile.printer_list;
            break;
          }
        }

        const auto user_added_uids = [] {
          auto settings = VersionServerSettings{};
          settings.beginGroup(QStringLiteral("PresetMachines"));
          auto uid_list = settings.childGroups();
          settings.endGroup();
          return std::set<QString>{ uid_list.begin(), uid_list.end() };
        }();

        // record deleteable printers
        for (const auto& local : local_profile_.printer_list) {
          auto online_existed{ false };
          for (const auto& online : online_profile.printer_list) {
            if (online.uid == local.uid) {
              online_existed = true;
              break;
            }
          }

          if (!online_existed) {
            deleteable_printer_list.emplace_back(local);
          }
        }

        // record updateable printers, overrideable printers and printer update info
        for (const auto& online : online_profile.printer_list) {
          auto local_existed{ false };

          for (const auto& local : local_profile_.printer_list) {
            if (online.uid != local.uid) {
              continue;
            }

            local_existed = true;

            const auto version_different = online.profile_version != local.profile_version;
            const auto user_added = user_added_uids.find(local.uid) != user_added_uids.cend();
            const auto updateable = version_different && user_added;
            const auto overrideable = version_different && !user_added;

            update_info_list.emplace_back(PrinterUpdateListModel::Data{
              local.uid,  // uid
              local,      // local
              online,     // online
              updateable, // updateable
              false,      // updating
            });

            if (overrideable) {
              overrideable_printer_list.emplace_back(online);
            }

            if (updateable) {
              updateable_printer_list.emplace_back(online);
            }

            break;
          }

          // brand new printer online
          if (!local_existed) {
            overrideable_printer_list.emplace_back(online);
          }
        }
      } while (false);

      // material list
      auto material_list = decltype(material_list_){};
      do {
        if (!initialized_) {
          return;
        }

        auto request = createRequest<LoadPrinterMaterialRequest>();
        request->syncPost();
        if (request->getResponseState() != HttpErrorCode::OK) {
          break;
        }

        if (!initialized_) {
          return;
        }

        std::swap(material_list, request->materials());
      } while (false);

      // material template
      auto material_template_list = decltype(material_template_list_){};
      do {
        if (!initialized_) {
          return;
        }

        auto request = createRequest<LoadMaterialTemplateRequest>(getEngineVersion());
        request->syncPost();
        if (request->getResponseState() != HttpErrorCode::OK) {
          break;
        }

        if (!initialized_) {
          return;
        }

        std::swap(material_template_list, request->materialTemplates());
      } while (false);

      // process template
      auto process_template_list = decltype(process_template_list_){};
      do {
        if (!initialized_) {
          return;
        }

        auto request = createRequest<LoadProcessTemplateRequest>(getEngineVersion());
        request->syncPost();
        if (request->getResponseState() != HttpErrorCode::OK) {
          break;
        }

        if (!initialized_) {
          return;
        }

        std::swap(process_template_list, request->processTemplates());
      } while (false);

      QMetaObject::invokeMethod(this,
        [this,
         series_list = std::move(series_list),
         updateable_printer_list = std::move(updateable_printer_list),
         overrideable_printer_list = std::move(overrideable_printer_list),
         deleteable_printer_list = std::move(deleteable_printer_list),
         update_info_list = std::move(update_info_list),
         material_list = std::move(material_list),
         material_template_list = std::move(material_template_list),
         process_template_list = std::move(process_template_list)] {

          series_list_ = std::move(series_list);
          updateable_printer_list_ = std::move(updateable_printer_list);
          overrideable_printer_list_ = std::move(overrideable_printer_list);
          deleteable_printer_list_ = std::move(deleteable_printer_list);
          printer_update_model_->clear();
          printer_update_model_->pushBack(std::move(update_info_list));

          material_list_ = std::move(material_list);
          material_template_list_ = std::move(material_template_list);
          process_template_list_ = std::move(process_template_list);

          checkOfficalProfileFinished(isOfficalProfileUpdateable(), isOfficalProfileOverrideable());

          if (isOfficalProfileOverrideable()) {
            overrideOfficalProfile();
          }
        }, Qt::ConnectionType::QueuedConnection);
    });
  }

  auto ProfileService::getPrinterUpdateListModel() const -> PrinterUpdateListModel* {
    return printer_update_model_.get();
  }

  auto ProfileService::getPrinterUpdateListModelObject() const -> QObject* {
    return getPrinterUpdateListModel();
  }

  auto ProfileService::isOfficalProfileUpdateable() -> bool {
    return !updateable_printer_list_.empty();
  }

  auto ProfileService::updateOfficalPrinter(const QString& uid) -> void {
    auto info = printer_update_model_->load(uid);
    if (info.uid.isEmpty() || !info.updateable) {
      updateOfficalPrinterFinished(uid, false);
      return;
    }

    const auto requested = asyncDownloadOfficialPrinter(info.online,
      [this, info, uid]() mutable {  // successed callback
        // update local data
        for (auto& printer : local_profile_.printer_list) {
          if (printer.uid == uid) {
            printer = info.online;
          }
        }

        SavePrinterProfile(local_profile_, getPrinterFilePath());

        // update model data
        info.updateable = false;
        info.local = info.online;
        printer_update_model_->update(std::move(info));

        updateOfficalPrinterFinished(uid, true);

        // check if all finished
        for (const auto& data : printer_update_model_->rawData()) {
          if (data.updateable) {
            return;
          }
        }

        updateOfficalPrinterFinished({}, true);

      }, [this, info, uid] {  // failed callback
        printer_update_model_->update(std::move(info));
        updateOfficalPrinterFinished(uid, false);
      });

    if (!requested) {
      updateOfficalPrinterFinished(uid, false);
      return;
    }

    info.updating = true;
    printer_update_model_->update(std::move(info));
  }

  auto ProfileService::isOfficalProfileOverrideable() -> bool {
    return !overrideable_printer_list_.empty() ||
           !deleteable_printer_list_.empty() ||
           !material_list_.empty() ||
           !material_template_list_.empty() ||
           !process_template_list_.empty();
  }

  auto ProfileService::overrideOfficalProfile() -> void {
    auto successed{ true };

    // material list
    if (!material_list_.empty()) {
      if (SavePrinterMaterial(material_list_, getMaterialFilePath())) {
        material_list_.clear();
      } else {
        successed = false;
      }
    }

    // material template
    if (!material_template_list_.empty()) {
      if (SaveMaterialTemplates(material_template_list_, getMaterialTemplateDirPath())) {
        material_template_list_.clear();
      } else {
        successed = false;
      }
    }

    // process template
    if (!process_template_list_.empty()) {
      if (SaveProcessTemplates(process_template_list_, getProcessTemplateDirPath())) {
        process_template_list_.clear();
      } else {
        successed = false;
      }
    }

    if (!initialized_) {
      return;
    }

    using namespace std::chrono;
    if (async_future_.valid() && async_future_.wait_for(0ms) != std::future_status::ready) {
      overrideOfficalProfileFinished(false);
      return;
    }

    async_future_ = std::async(std::launch::async, [this, successed]() mutable {
      auto local_profile = local_profile_;

      // update series list
      local_profile.series_list = series_list_;

      // erise history printers
      for (const auto& history_printer : deleteable_printer_list_) {
        auto& list = local_profile.printer_list;
        for (auto iter = list.cbegin(); iter != list.cend(); ++iter) {
          if (iter->uid == history_printer.uid) {
            list.erase(iter);
            break;
          }
        }
      }

      // emplace new printers
      for (const auto& online_printer : overrideable_printer_list_) {
        if (!initialized_) {
          successed = false;
          break;
        }

        if (!syncDownloadOfficialPrinter(online_printer)) {
          continue;
        }

        if (!initialized_) {
          successed = false;
          break;
        }

        auto local_existed{ false };
        for (auto& local_printer : local_profile.printer_list) {
          if (local_printer.uid == online_printer.uid) {
            local_printer = online_printer;
            local_existed = true;
            break;
          }
        }

        if (!local_existed) {
          local_profile.printer_list.emplace_back(online_printer);
        }
      }

      if (!initialized_) {
        return;
      }

      QMetaObject::invokeMethod(this,
        [this, successed, local_profile = std::move(local_profile)]() mutable {
          local_profile_ = std::move(local_profile);

          if (SavePrinterProfile(local_profile_, getPrinterFilePath())) {
            overrideable_printer_list_.clear();
            deleteable_printer_list_.clear();
            series_list_.clear();
          } else {
            successed = false;
          }

          overrideOfficalProfileFinished(successed);
        }, Qt::ConnectionType::QueuedConnection);
    });
  }

  auto ProfileService::updateProfileLanguage() -> void {
    auto request = createRequest<LoadPrinterProfileRequest>(getEngineVersion());

    request->setSuccessedCallback([this, request] {
      if (!request) {
        updateProfileLanguageFinished(false);
        return;
      }

      const auto& online_profile = request->profile();

      // update change_logs in updateable printer list model

      auto&& updated_items = decltype(printer_update_model_)::element_type::DataContainer{};

      for (const auto& online_printer : online_profile.printer_list) {
        auto&& item = printer_update_model_->load(online_printer.uid);
        if (item.uid.isEmpty()) {
          continue;
        }

        item.online.change_log = online_printer.change_log;
        updated_items.emplace_back(std::move(item));
      }

      printer_update_model_->update(std::move(updated_items));

      // update names in local series list

      const auto printer_file_path = getPrinterFilePath();
      auto&& local_profile = decltype(local_profile_){};
      if (!LoadPrinterProfile(local_profile, printer_file_path)) {
        updateProfileLanguageFinished(false);
        return;
      }

      for (auto& local_series : local_profile.series_list) {
        for (const auto& online_series : online_profile.series_list) {
          if (local_series.id == online_series.id) {
            local_series.name = online_series.name;
          }
        }
      }

      SavePrinterProfile(std::move(local_profile), printer_file_path);
      updateProfileLanguageFinished(true);
    });

    request->setFailedCallback([this] {
      updateProfileLanguageFinished(false);
    });

    request->asyncPost();
  }





  auto ProfileService::downloadUserProfile() -> void {
    auto request = createRequest<LoadUserProfileRequest>();

    request->setSuccessedCallback([this, request]() {
      std::map<QString, Profile::Printer> name_printer_map;

      for (const auto& printer : request->profile().printer_list) {
        auto iter = name_printer_map.find(printer.internal_name);
        if (iter == name_printer_map.cend()) {
          name_printer_map.emplace(std::make_pair(printer.internal_name, printer));
          continue;
        }

        if (printer.profile_version > iter->second.profile_version) {
          iter->second = printer;
        }
      }

      for (const auto& pair : name_printer_map) {
        syncDownloadUserPrinter(pair.second);
      }
    });

    request->setFailedCallback([]() {
      // TODO
    });

    request->asyncPost();
  }

  void ProfileService::uploadUserProfile(const QString& printer_unique_name,
                                         const QString& printer_internal_name,
                                         const QStringList& nozzle_diameter) {
    auto profile_path = QStringLiteral("%1/%2").arg(getUserPrinterDirPath(), printer_unique_name);
    auto zip_file = QStringLiteral("%1.zip").arg(profile_path);
    qtuser_core::compressDir(zip_file, profile_path);

    auto request = createRequest<PreUploadProfile>(
      printer_internal_name, nozzle_diameter, qtuser_core::calculateFileMD5(zip_file));

    request->setSuccessedCallback(
      [request, profile_path = std::move(profile_path), zip_file = std::move(zip_file)]() {
        const auto& policy = request->uploadPolicy();
        cpr::PostCallback(
          [](cpr::Response response) {},
          cpr::Url{ policy.host.toStdString() },
          cpr::Header{
            { "Content-Type", "multipart/form-data" },
          },
          cpr::Multipart{
            { "OSSAccessKeyId", policy.oss_access_key_id.toStdString() },
            { "Signature", policy.signature.toStdString() },
            { "Policy", policy.policy.toStdString() },
            { "Key", policy.key.toStdString() },
            { "Callback", policy.callback.toStdString() },
            { "file", cpr::File{ zip_file.toStdString() } },
          },
          cpr::ProgressCallback{ [](cpr::cpr_off_t download_total,
                                    cpr::cpr_off_t download_now,
                                    cpr::cpr_off_t upload_total,
                                    cpr::cpr_off_t upload_now,
                                    intptr_t userdata) {
            return true;
          } });
      });

    request->setFailedCallback([]() {
      // TODO
    });

    request->asyncPost();
  }





  auto ProfileService::syncDownloadUserPrinter(const Profile::Printer& printer) -> bool {
    return downloadPrinterPackage(printer, true, true, nullptr, nullptr);
  }

  auto ProfileService::syncDownloadOfficialPrinter(const Profile::Printer& printer) -> bool {
    return downloadPrinterPackage(printer, false, true, nullptr, nullptr);
  }

  auto ProfileService::asyncDownloadUserPrinter(
      const Profile::Printer& printer,
      std::function<void(void)> successed_callback,
      std::function<void(void)> failed_callback) -> bool {
    return downloadPrinterPackage(printer, true, false, successed_callback, failed_callback);
  }

  auto ProfileService::asyncDownloadOfficialPrinter(
      const Profile::Printer& printer,
      std::function<void(void)> successed_callback,
      std::function<void(void)> failed_callback) -> bool {
    return downloadPrinterPackage(printer, false, false, successed_callback, failed_callback);
  }

  auto ProfileService::downloadPrinterPackage(const Profile::Printer& printer,
                                              bool is_user_printer,
                                              bool need_sync_download,
                                              std::function<void(void)> successed_callback,
                                              std::function<void(void)> failed_callback) -> bool {
    asyncDownloadPrinterImage(printer);

    const auto& zip_file_url = printer.profile_zip_url;
    const auto unzip_dir_name = printer.uid;
    auto zip_file = QTemporaryFile{};
    if (!zip_file.open()) {
      return false;
    }

    const auto zip_file_path = QStringLiteral("%1.zip").arg(zip_file.fileName());
    const auto unzip_dir_path = QStringLiteral("%1/%2/").arg(
      is_user_printer ? getUserPrinterDirPath() : getPrinterDirPath(), unzip_dir_name);

    auto request = createRequest<DownloadRequest>(zip_file_url, zip_file_path);

    const auto real_successed_callback = [successed_callback,
                                          is_user_printer,
                                          zip_file_path,
                                          unzip_dir_path] {
      if (!is_user_printer) {
        QDir{ unzip_dir_path }.removeRecursively();
      }

      qtuser_core::extractDir(zip_file_path, unzip_dir_path);

      if (successed_callback) {
        successed_callback();
      }
    };

    const auto real_failed_callback = [failed_callback] {
      if (failed_callback) {
        failed_callback();
      }
    };

    if (need_sync_download) {
      request->syncDownload();
      const auto successed = request->getResponseState() == HttpErrorCode::OK;
      successed ? real_successed_callback() : real_failed_callback();
      return successed;
    }

    request->setSuccessedCallback(real_successed_callback);
    request->setFailedCallback(real_failed_callback);
    request->asyncDownload();
    return true;
  }

  auto ProfileService::asyncDownloadPrinterImage(const Profile::Printer& printer) -> void {
    const auto path = QStringLiteral("%1/machineImages/%2.png").arg(getProfileDirPath(),
                                                                    printer.internal_name);
    if (QFileInfo{ path }.exists()) {
      return;
    }

    auto request = createRequest<DownloadRequest>(printer.thumbnail, path);
    request->asyncDownload();
  }

}  // namespace cxcloud
