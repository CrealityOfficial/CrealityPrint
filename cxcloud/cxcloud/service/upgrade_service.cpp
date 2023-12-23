#include "cxcloud/service/upgrade_service.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QProcess>

#include <qtusercore/string/resourcesfinder.h>

#include "cxcloud/net/common_request.h"

namespace cxcloud {

UpgradeService::UpgradeService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : UpgradeService(http_session, { 0, 0, 0, 0 }, parent) {}

UpgradeService::UpgradeService(std::weak_ptr<HttpSession> http_session,
                               Version local_version,
                               QObject* parent)
    : BaseService(http_session, parent)
    , local_version_(std::move(local_version))
    , online_version_({ 0, 0, 0, 0 }) {}

void UpgradeService::initialize() {
  if (isAutoCheck()) {
    checkUpgradeable();
  }
}

void UpgradeService::uninitialize() {
  if (download_request_) {
    download_request_->setCancelDownloadLater(true);
  }
}

// ---------- local version [begin] ----------

Version UpgradeService::getLocalVersion() const {
  return local_version_;
}

QString UpgradeService::getLocalVersionString() const {
  return local_version_.toString();
}

// ---------- local version [end] ----------

// ---------- skipped version [begin] ----------

Version UpgradeService::getSkippedVersion() const {
  return Version::FromString(getSkippedVersionString());
}

void UpgradeService::setSkippedVersion(Version version) {
  setSkippedVersionString(version.toString());
}

QString UpgradeService::getSkippedVersionString() const {
  QSettings settings;
  settings.beginGroup(QStringLiteral("cloud_service"));
  auto version =
    settings.value(QStringLiteral("skipped_version"), local_version_.toString()).toString();
  settings.endGroup();
  return Version{ version } > local_version_ ? version : local_version_.toString();
}

void UpgradeService::setSkippedVersionString(QString version) {
  QSettings settings;
  settings.beginGroup(QStringLiteral("cloud_service"));
  settings.setValue(QStringLiteral("skipped_version"),
                    Version{ version } > local_version_ ? version : local_version_.toString());
  settings.endGroup();
  Q_EMIT skippedVersionChanged();
}

// ---------- skipped version [end] ----------

// ---------- online version [begin] ----------

bool UpgradeService::isAutoCheck() const {
  QSettings settings;
  settings.beginGroup(QStringLiteral("cloud_service"));
  auto auto_check = settings.value(QStringLiteral("auto_check_upgradeable"), false).toBool();
  settings.endGroup();
  return auto_check;
}

void UpgradeService::setAutoCheck(bool auto_check) {
  QSettings settings;
  settings.beginGroup(QStringLiteral("cloud_service"));
  settings.setValue(QStringLiteral("auto_check_upgradeable"), auto_check);
  settings.endGroup();
}

bool UpgradeService::isUpgradable() const {
  return online_version_ > getSkippedVersion();
}

void UpgradeService::checkUpgradeable() {
  auto* request = createRequest<CheckNewestVersionRequest>();
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
    Q_EMIT checkUpgradableFinished();
  });
  HttpPost(request);
}

Version UpgradeService::getOnlineVersion() const {
  return online_version_;
}

QString UpgradeService::getOnlineVersionString() const {
  return online_version_.toString();
}

QString UpgradeService::getOnlineVersionName() const {
  return online_version_name_;
}

QString UpgradeService::getOnlineVersionDescription() const {
  return online_version_desc_;
}

QString UpgradeService::getOnlineVersionDownloadUrl() const {
  return online_version_url_;
}

// ---------- online version [end] ----------

// ---------- upgrade task [begin] ----------

void UpgradeService::startDownloadInstaller() {
  if (!download_request_) { return; }

  const auto file_name = online_version_url_.split(QStringLiteral("/")).last();
  installer_download_dir_ = qtuser_core::getOrCreateAppDataLocation("temp_installer");
  installer_path_ = QStringLiteral("%1/%2").arg(installer_download_dir_).arg(file_name);
  if (QFileInfo{ installer_path_ }.exists()) {
    QFile::remove(installer_path_);
  }

  download_request_ = createRequest<DownloadRequest>(online_version_url_, installer_path_);
  download_request_->setProgressUpdatedCallback([this]() {
    const auto max = static_cast<double>(download_request_->getTotalProgress());
    const auto cur = static_cast<double>(download_request_->getCurrentProgress());
    const auto progress = static_cast<int>(cur / max * 100);
    Q_EMIT downloadInstallerProgressUpadate(QVariant::fromValue(progress));
  });
  download_request_->setFailedCallback([this]() {
    Q_EMIT downloadInstallerFailed();
    QDir{ installer_download_dir_ }.removeRecursively();
  });
  download_request_->setSuccessedCallback([this]() { Q_EMIT downloadInstallerSuccessed(); });

  HttpDownload(download_request_.data());
}

void UpgradeService::cancelDownloadInstaller() {
  if (!download_request_) { return; }
  download_request_->setCancelDownloadLater(true);
  download_request_ = nullptr;
}

void UpgradeService::openInstaller() const {
  if (QFileInfo{ installer_path_ }.exists()) {
    QProcess::startDetached(installer_path_);
  }
}

// ---------- upgrade task [end] ----------

}  // namespace cxcloud
