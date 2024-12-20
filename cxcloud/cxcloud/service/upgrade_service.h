#pragma once

#ifndef CXCLOUD_SERVICE_UPGRADE_SERVICE_H_
#define CXCLOUD_SERVICE_UPGRADE_SERVICE_H_

#include <chrono>

#include <QtCore/QPointer>

#include "cxcloud/interface.hpp"
#include "cxcloud/service/base_service.h"
#include "cxcloud/tool/http_component.h"
#include "cxcloud/tool/version.h"

namespace cxcloud {

  class CXCLOUD_API UpgradeService : public BaseService {
    Q_OBJECT;

   public:
    explicit UpgradeService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    explicit UpgradeService(std::weak_ptr<HttpSession> http_session,
                            Version local_version,
                            QObject* parent = nullptr);
    virtual ~UpgradeService() = default;

   public:
    auto initialize() -> void override;
    auto uninitialize() -> void override;

   public:  // local version
    auto getLocalVersion() const -> Version;
    auto getLocalVersionString() const -> QString;
    Q_PROPERTY(QString localVersion READ getLocalVersionString CONSTANT);

   public:  // skipped version
    auto getSkippedVersion() const -> Version;
    auto setSkippedVersion(const Version& version) -> void;
    auto getSkippedVersionString() const -> QString;
    auto setSkippedVersionString(const QString& version_string) -> void;
    Q_SIGNAL void skippedVersionChanged();
    Q_PROPERTY(QString skippedVersion
               READ getSkippedVersionString
               WRITE setSkippedVersionString
               NOTIFY skippedVersionChanged);

   public:  // online version
    auto isAutoCheck() const -> bool;
    auto setAutoCheck(bool auto_check) -> void;
    Q_SIGNAL void isAutoCheckChanged();
    Q_PROPERTY(bool autoCheck READ isAutoCheck WRITE setAutoCheck NOTIFY isAutoCheckChanged);

    auto isUpgradable() const -> bool;
    Q_INVOKABLE void checkUpgradeable();
    Q_SIGNAL void checkUpgradableFinished();
    Q_PROPERTY(bool upgradable READ isUpgradable NOTIFY checkUpgradableFinished);

    auto getOnlineVersion() const -> Version;
    auto getOnlineVersionString() const -> QString;
    Q_PROPERTY(QString onlineVersion READ getOnlineVersionString NOTIFY checkUpgradableFinished);

    auto getOnlineVersionName() const -> QString;
    Q_PROPERTY(QString onlineVersionName READ getOnlineVersionName NOTIFY checkUpgradableFinished);

    auto getOnlineVersionDescription() const -> QString;
    Q_PROPERTY(QString onlineVersionDescription
               READ getOnlineVersionDescription
               NOTIFY checkUpgradableFinished);

    auto getOnlineVersionDownloadUrl() const -> QString;
    Q_PROPERTY(QString onlineVersionDownloadUrl
               READ getOnlineVersionDownloadUrl
               NOTIFY checkUpgradableFinished);

   public:  // upgrade task
    Q_INVOKABLE void startDownloadInstaller();
    Q_INVOKABLE void cancelDownloadInstaller();
    Q_SIGNAL void downloadInstallerSuccessed();
    Q_SIGNAL void downloadInstallerCanceled();
    Q_SIGNAL void downloadInstallerFailed();

    Q_INVOKABLE QString getInstallerPath() const;

    /// @return size with unit Byte
    auto getInstallerSize() const -> unsigned int;
    Q_SIGNAL void installerSizeChanged() const;
    Q_PROPERTY(unsigned int installerSize READ getInstallerSize NOTIFY installerSizeChanged);

    /// @return size with unit Byte
    auto getDownloadedInstallerSize() const -> unsigned int;
    Q_SIGNAL void downloadedInstallerSizeChanged() const;
    Q_PROPERTY(unsigned int downloadedInstallerSize
               READ getDownloadedInstallerSize
               NOTIFY downloadedInstallerSizeChanged);

    /// @return progress with unit %
    auto getDownloadProgress() const -> unsigned int;
    Q_SIGNAL void downloadProgressChanged() const;
    Q_PROPERTY(unsigned int downloadProgress
               READ getDownloadProgress
               NOTIFY downloadProgressChanged);

    /// @return speed with unit B/s
    auto getDownloadSpeed() const -> unsigned int;
    Q_SIGNAL void downloadSpeedChanged() const;
    Q_PROPERTY(unsigned int downloadSpeed READ getDownloadSpeed NOTIFY downloadSpeedChanged);

   private:
    Version local_version_{ 0, 0, 0, 0 };

    Version online_version_{ 0, 0, 0, 0 };
    QString online_version_name_{};
    QString online_version_desc_{};
    QString online_version_url_{};

    QString installer_download_dir_{};
    QString installer_path_{};

    QPointer<HttpRequest> download_request_{ nullptr };

    unsigned int installer_size_{ 0u };
    unsigned int downloaded_size_{ 0u };
    unsigned int download_progress_{ 0u };

    std::chrono::steady_clock::time_point last_time_point_;
    unsigned int last_downloaded_size_{ 0u };
    unsigned int download_speed_{ 0u };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_UPGRADE_SERVICE_H_
