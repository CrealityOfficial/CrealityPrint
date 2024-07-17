#include "cxcloud/service/upgrade_service.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/common_request.h"
#include "cxcloud/tool/settings.h"

namespace cxcloud {

  UpgradeService::UpgradeService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : UpgradeService{ http_session, { 0, 0, 0, 0 }, parent } {}

  UpgradeService::UpgradeService(std::weak_ptr<HttpSession> http_session,
                                 Version local_version,
                                 QObject* parent)
      : BaseService{ http_session, parent }
      , local_version_{ std::move(local_version) }
      , online_version_{ 0, 0, 0, 0 } {}

  auto UpgradeService::initialize() -> void {
    if (isAutoCheck()) {
      checkUpgradeable();
    }
  }

  auto UpgradeService::uninitialize() -> void {
    if (download_request_) {
      download_request_->setCancelDownloadLater(true);
    }
  }

// ---------- local version [begin] ----------

  auto UpgradeService::getLocalVersion() const -> Version {
    return local_version_;
  }

  auto UpgradeService::getLocalVersionString() const -> QString {
    return local_version_.toString();
  }

// ---------- local version [end] ----------

// ---------- skipped version [begin] ----------

  auto UpgradeService::getSkippedVersion() const -> Version {
    return Version::FromString(getSkippedVersionString());
  }

  auto UpgradeService::setSkippedVersion(const Version& version) -> void {
    setSkippedVersionString(version.toString());
  }

  auto UpgradeService::getSkippedVersionString() const -> QString {
    auto local_version_string = local_version_.toString();
    auto version_string =
        CloudSettings{}.value(QStringLiteral("skipped_version"), local_version_string).toString();
    return Version{ version_string } > local_version_ ? std::move(version_string)
                                                      : std::move(local_version_string);
  }

  auto UpgradeService::setSkippedVersionString(const QString& version_string) -> void {
    CloudSettings{}.setValue(
        QStringLiteral("skipped_version"),
        Version{ version_string } > local_version_ ? version_string : local_version_.toString());
    skippedVersionChanged();
  }

// ---------- skipped version [end] ----------

// ---------- online version [begin] ----------

  auto UpgradeService::isAutoCheck() const -> bool {
    return CloudSettings{}.value(QStringLiteral("auto_check_upgradeable"), false).toBool();
  }

  auto UpgradeService::setAutoCheck(bool auto_check) -> void {
    CloudSettings{}.setValue(QStringLiteral("auto_check_upgradeable"), auto_check);
  }

  auto UpgradeService::isUpgradable() const -> bool {
    return online_version_ > getSkippedVersion();;
  }

  auto UpgradeService::checkUpgradeable() -> void {
    auto request = createRequest<CheckNewestVersionRequest>();

    request->setSuccessedCallback([this, request]() {
      if (request->exist()) {
        online_version_ = request->version();
        online_version_name_ = request->name();
        online_version_desc_ = request->description();
        online_version_url_ = request->url();
      } else {
        online_version_ = { 0, 0, 0, 0 };
        online_version_name_.clear();
        online_version_desc_.clear();
        online_version_url_.clear();
      }

      checkUpgradableFinished();
    });

    request->asyncPost();
  }

  auto UpgradeService::getOnlineVersion() const -> Version {
    return online_version_;
  }

  auto UpgradeService::getOnlineVersionString() const -> QString {
    return online_version_.toString();
  }

  auto UpgradeService::getOnlineVersionName() const -> QString {
    return online_version_name_;
  }

  auto UpgradeService::getOnlineVersionDescription() const -> QString {
    return online_version_desc_;
  }

  auto UpgradeService::getOnlineVersionDownloadUrl() const -> QString {
    return online_version_url_;
  }

// ---------- online version [end] ----------

// ---------- upgrade task [begin] ----------

  auto UpgradeService::startDownloadInstaller() -> void {
    if (download_request_) {
      return;
    }

    const auto file_name = online_version_url_.split(QStringLiteral("/")).last();
    installer_download_dir_ = qtuser_core::getOrCreateAppDataLocation("temp_installer");
    installer_path_ = QStringLiteral("%1/%2").arg(installer_download_dir_).arg(file_name);
    if (QFileInfo{ installer_path_ }.exists()) {
      QFile::remove(installer_path_);
    }

    download_request_ = createRequest<DownloadRequest>(online_version_url_, installer_path_);

    download_request_->setProgressUpdatedCallback([this]() {
      installer_size_ = static_cast<unsigned int>(download_request_->getTotalProgress());
      installerSizeChanged();

      downloaded_size_ = static_cast<unsigned int>(download_request_->getCurrentProgress());
      downloadedInstallerSizeChanged();

      const auto max = static_cast<double>(download_request_->getTotalProgress());
      const auto cur = static_cast<double>(download_request_->getCurrentProgress());
      download_progress_ = static_cast<unsigned int>(cur / max * 100u);
      downloadProgressChanged();

      using namespace std::chrono;
      if (auto time_point = steady_clock::now(); time_point - last_time_point_ >= 1s) {
        last_time_point_ = std::move(time_point);
        download_speed_ = downloaded_size_ - last_downloaded_size_;
        last_downloaded_size_ = downloaded_size_;
        downloadSpeedChanged();
      }
    });

    download_request_->setFailedCallback([this]() {
      downloadInstallerFailed();
      QDir{ installer_download_dir_ }.removeRecursively();
    });

    download_request_->setSuccessedCallback([this]() {
      if (download_request_->isCancelDownloadLater()) {
        downloadInstallerCanceled();
        return;
      }

      download_progress_ = 100u;
      downloadProgressChanged();
      downloadInstallerSuccessed();
    });

    installer_size_ = 0u;
    installerSizeChanged();

    downloaded_size_ = 0u;
    downloadedInstallerSizeChanged();

    download_progress_ = 0u;
    downloadProgressChanged();

    last_time_point_ = std::chrono::steady_clock::now();
    last_downloaded_size_ = 0u;
    download_speed_ = 0u;
    downloadSpeedChanged();

    download_request_->asyncDownload();
  }

  auto UpgradeService::cancelDownloadInstaller() -> void {
    if (!download_request_) {
      return;
    }

    download_request_->setCancelDownloadLater(true);
  }

  auto UpgradeService::getInstallerPath() const -> QString {
    return installer_path_;
  }

  auto UpgradeService::getInstallerSize() const -> unsigned int {
    return installer_size_;
  }

  auto UpgradeService::getDownloadedInstallerSize() const -> unsigned int {
    return downloaded_size_;
  }

  auto UpgradeService::getDownloadProgress() const -> unsigned int {
    return download_progress_;
  }

  auto UpgradeService::getDownloadSpeed() const -> unsigned int {
    return download_speed_;
  }

// ---------- upgrade task [end] ----------

}  // namespace cxcloud
