#pragma once

#ifndef CXCLOUD_SERVICE_UPGRADE_SERVICE_H_
#define CXCLOUD_SERVICE_UPGRADE_SERVICE_H_

#include <QtCore/QPointer>

#include "cxcloud/interface.hpp"
#include "cxcloud/net/http_request.h"
#include "cxcloud/tool/version.h"
#include "cxcloud/service/base_service.hpp"

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
  void initialize() override;
  void uninitialize() override;

public:  // local version
  Version getLocalVersion() const;
  QString getLocalVersionString() const;
  Q_PROPERTY(QString localVersion READ getLocalVersionString CONSTANT);

public:  // skipped version
  Version getSkippedVersion() const;
  void setSkippedVersion(Version version);
  QString getSkippedVersionString() const;
  void setSkippedVersionString(QString version);
  Q_SIGNAL void skippedVersionChanged();
  Q_PROPERTY(QString skippedVersion
             READ getSkippedVersionString
             WRITE setSkippedVersionString
             NOTIFY skippedVersionChanged);

public:  // online version
  bool isAutoCheck() const;
  void setAutoCheck(bool auto_check);
  Q_SIGNAL void isAutoCheckChanged();
  Q_PROPERTY(bool autoCheck READ isAutoCheck WRITE setAutoCheck NOTIFY isAutoCheckChanged);

  bool isUpgradable() const;
  Q_INVOKABLE void checkUpgradeable();
  Q_SIGNAL void checkUpgradableFinished();
  Q_PROPERTY(bool upgradable READ isUpgradable NOTIFY checkUpgradableFinished);

  Version getOnlineVersion() const;
  QString getOnlineVersionString() const;
  Q_PROPERTY(QString onlineVersion READ getOnlineVersionString NOTIFY checkUpgradableFinished);

  QString getOnlineVersionName() const;
  Q_PROPERTY(QString onlineVersionName READ getOnlineVersionName NOTIFY checkUpgradableFinished);

  QString getOnlineVersionDescription() const;
  Q_PROPERTY(QString onlineVersionDescription
             READ getOnlineVersionDescription
             NOTIFY checkUpgradableFinished);

  QString getOnlineVersionDownloadUrl() const;
  Q_PROPERTY(QString onlineVersionDownloadUrl
             READ getOnlineVersionDownloadUrl
             NOTIFY checkUpgradableFinished);

public:  // upgrade task
  Q_INVOKABLE void startDownloadInstaller();
  Q_INVOKABLE void cancelDownloadInstaller();
  Q_SIGNAL void downloadInstallerProgressUpadate(const QVariant& progress);
  Q_SIGNAL void downloadInstallerSuccessed();
  Q_SIGNAL void downloadInstallerFailed();

  /// @brief you may need to connect this function as a signal
  ///        who can notify you to quit the application after the installer opened.
  Q_SLOT void openInstaller() const;

private:
  Version local_version_;

  Version online_version_;
  QString online_version_name_;
  QString online_version_desc_;
  QString online_version_url_;

  QPointer<HttpRequest> download_request_;
  QString installer_download_dir_;
  QString installer_path_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_UPGRADE_SERVICE_H_
