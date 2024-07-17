#pragma once

#ifndef CXCLOUD_SERVICE_PROFILE_SERVICE_H_
#define CXCLOUD_SERVICE_PROFILE_SERVICE_H_

#include <functional>
#include <future>
#include <memory>

#include <QtCore/QJsonObject>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/profile_model.h"
#include "cxcloud/service/base_service.h"
#include "cxcloud/tool/version.h"

namespace cxcloud {

  class CXCLOUD_API ProfileService : public BaseService {
    Q_OBJECT;

   public:
    explicit ProfileService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~ProfileService() = default;

   public:
    auto initialize() -> void override;
    auto uninitialize() -> void override;

   public:  // wirteable basic properties
    auto getProfileDirPath() const -> QString;
    auto setProfileDirPath(const QString& path) -> void;
    Q_SIGNAL void profileDirPathChanged() const;
    Q_PROPERTY(QString profileDirPath READ getProfileDirPath NOTIFY profileDirPathChanged);

    auto getUserProfileDirPath() const -> QString;
    auto setUserProfileDirPath(const QString& path) -> void;
    Q_SIGNAL void userProfileDirPathChanged() const;
    Q_PROPERTY(QString userProfileDirPath
               READ getUserProfileDirPath
               NOTIFY userProfileDirPathChanged);

    auto getPrinterDirName() const -> QString;
    auto setPrinterDirName(const QString& name) -> void;
    Q_SIGNAL void printerDirNameChanged() const;
    Q_PROPERTY(QString printerDirName READ getPrinterDirName NOTIFY printerDirNameChanged);

    auto getPrinterFileName() const -> QString;
    auto setPrinterFileName(const QString& name) -> void;
    Q_SIGNAL void printerFileNameChanged() const;
    Q_PROPERTY(QString printerFileName READ getPrinterFileName NOTIFY printerFileNameChanged);

    auto getMaterialFileName() const -> QString;
    auto setMaterialFileName(const QString& name) -> void;
    Q_SIGNAL void materialFileNameChanged() const;
    Q_PROPERTY(QString materialFileName READ getMaterialFileName NOTIFY materialFileNameChanged);

    auto getMaterialTemplateDirName() const -> QString;
    auto setMaterialTemplateDirName(const QString& name) -> void;
    Q_SIGNAL void materialTemplateDirNameChanged() const;
    Q_PROPERTY(QString materialTemplateDirName
               READ getMaterialTemplateDirName
               NOTIFY materialTemplateDirNameChanged);

    auto getProcessTemplateDirName() const -> QString;
    auto setProcessTemplateDirName(const QString& name) -> void;
    Q_SIGNAL void processTemplateDirNameChanged() const;
    Q_PROPERTY(QString processTemplateDirName
               READ getProcessTemplateDirName
               NOTIFY processTemplateDirNameChanged);

    auto getEngineVersion() const -> Version;
    auto setEngineVersion(const Version& version) -> void;
    auto getEngineVersionString() const -> QString;
    auto setEngineVersionString(const QString& version) -> void;
    Q_SIGNAL void engineVersionChanged() const;
    Q_PROPERTY(QString engineVersion READ getEngineVersionString NOTIFY engineVersionChanged);

   public:  // readonly basic properties
    /// @brief profileDirPath/printerDirName
    auto getPrinterDirPath() const -> QString;
    Q_SIGNAL void printerDirPathChanged() const;
    Q_PROPERTY(QString printerDirPath READ getPrinterDirPath NOTIFY printerDirPathChanged);

    /// @brief profileDirPath/printerFileName
    auto getPrinterFilePath() const -> QString;
    Q_SIGNAL void printerFilePathChanged() const;
    Q_PROPERTY(QString printerFilePath READ getPrinterFilePath NOTIFY printerFilePathChanged);

    /// @brief profileDirPath/materialFileName
    auto getMaterialFilePath() const -> QString;
    Q_SIGNAL void materialFilePathChanged() const;
    Q_PROPERTY(QString materialFilePath READ getMaterialFilePath NOTIFY materialFilePathChanged);

    /// @brief profileDirPath/materialTemplateDirName
    auto getMaterialTemplateDirPath() const -> QString;
    Q_SIGNAL void materialTemplateDirPathChanged() const;
    Q_PROPERTY(QString materialTemplateDirPath
               READ getMaterialTemplateDirPath
               NOTIFY materialTemplateDirPathChanged);

    /// @brief profileDirPath/processTemplateDirName
    auto getProcessTemplateDirPath() const -> QString;
    Q_SIGNAL void processTemplateDirPathChanged() const;
    Q_PROPERTY(QString processTemplateDirPath
               READ getProcessTemplateDirPath
               NOTIFY processTemplateDirPathChanged);

    /// @brief userProfileDirPath/printerDirName
    auto getUserPrinterDirPath() const -> QString;
    Q_SIGNAL void userPrinterDirPathChanged() const;
    Q_PROPERTY(QString userPrinterDirPath
               READ getUserPrinterDirPath
               NOTIFY userPrinterDirPathChanged);

   public:  // properties and methods for updating offical profile
    /// @brief only used as a flag
    auto isAutoCheckOfficalProfile() const -> bool;
    auto setAutoCheckOfficalProfile(bool auto_check) -> void;
    Q_SIGNAL void autoCheckOfficalProfileChanged() const;
    Q_PROPERTY(bool autoCheckUpdateable
               READ isAutoCheckOfficalProfile
               WRITE setAutoCheckOfficalProfile
               NOTIFY autoCheckOfficalProfileChanged);

    Q_INVOKABLE void checkOfficalProfileUpdateable();
    /// @param updateable see isOfficalProfileUpdateable()
    /// @param overrideable see isOfficalProfileUpdateable()
    Q_SIGNAL void checkOfficalProfileFinished(bool updateable, bool overrideable);

    auto getPrinterUpdateListModel() const -> PrinterUpdateListModel*;
    auto getPrinterUpdateListModelObject() const -> QObject*;
    Q_PROPERTY(QObject* printerUpdateListModel READ getPrinterUpdateListModelObject CONSTANT);
    /// @return true if some printers already used by user can be updated
    auto isOfficalProfileUpdateable() -> bool;
    /// @param uid printer uid who will be update
    Q_INVOKABLE void updateOfficalPrinter(const QString& uid);
    /// @brief if uid is empty and still successed, it means all printer update finished
    Q_SIGNAL void updateOfficalPrinterFinished(const QString& uid, bool successed);

    /// @return true if something in this list can be updated: [
    ///            printer unused by user,
    ///            printer list file,
    ///            material list file,
    ///            material templates,
    ///            process templates,
    ///         ]
    auto isOfficalProfileOverrideable() -> bool;
    Q_INVOKABLE void overrideOfficalProfile();
    Q_SIGNAL void overrideOfficalProfileFinished(bool successed);

    Q_INVOKABLE void updateProfileLanguage();
    Q_SIGNAL void updateProfileLanguageFinished(bool successed);

   public:  // properties and methods for synchronizing user profile
    Q_INVOKABLE void downloadUserProfile();
    Q_INVOKABLE void uploadUserProfile(const QString& printer_unique_name,
                                       const QString& printer_internal_name,
                                       const QStringList& nozzle_diameter);

   private:
    auto syncDownloadUserPrinter(const Profile::Printer& printer) -> bool;
    auto syncDownloadOfficialPrinter(const Profile::Printer& printer) -> bool;
    auto asyncDownloadUserPrinter(const Profile::Printer& printer,
                                  std::function<void(void)> successed_callback,
                                  std::function<void(void)> failed_callback) -> bool;
    auto asyncDownloadOfficialPrinter(const Profile::Printer& printer,
                                      std::function<void(void)> successed_callback,
                                      std::function<void(void)> failed_callback) -> bool;
    auto downloadPrinterPackage(const Profile::Printer& printer,
                                bool is_user_printer,
                                bool need_sync_download,
                                std::function<void(void)> successed_callback,
                                std::function<void(void)> failed_callback) -> bool;

    auto asyncDownloadPrinterImage(const Profile::Printer& printer) -> void;

   private:
    QString profile_dir_path_{};
    QString user_profile_dir_path_{};
    QString printer_dir_name_{};
    QString printer_file_name_{};
    QString material_file_name_{};
    QString material_template_dir_name_{};
    QString process_template_dir_name_{};
    Version engine_version_{ "0.0.0"_v };

    Profile local_profile_{};

    std::future<void> async_future_{};
    std::atomic<bool> initialized_{ false };

    std::list<Profile::Printer> updateable_printer_list_{};
    std::list<Profile::Printer> overrideable_printer_list_{};
    std::list<Profile::Printer> deleteable_printer_list_{};
    std::list<Profile::Series> series_list_{};
    std::list<PrinterMaterial> material_list_{};
    std::list<MaterialTemplate> material_template_list_{};
    std::list<ProcessTemplate> process_template_list_{};

    std::unique_ptr<PrinterUpdateListModel> printer_update_model_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_PROFILE_SERVICE_H_
