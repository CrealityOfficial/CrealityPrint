#pragma once

#ifndef CXCLOUD_SERVICE_CENTER_H_
#define CXCLOUD_SERVICE_CENTER_H_

#include <functional>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "cxcloud/define.hpp"
#include "cxcloud/interface.hpp"
#include "cxcloud/model/base_model.h"
#include "cxcloud/net/http_request.h"
#include "cxcloud/service/account_service.h"
#include "cxcloud/service/download_service.h"
#include "cxcloud/service/model_library_service.h"
#include "cxcloud/service/model_service.h"
#include "cxcloud/service/printer_service.h"
#include "cxcloud/service/slice_service.h"
#include "cxcloud/service/upgrade_service.h"

namespace cxcloud {

class CXCLOUD_API ServiceCenter : public QObject {
  Q_OBJECT;

  ServiceCenter(ServiceCenter&&) = delete;
  ServiceCenter(const ServiceCenter&) = delete;
  ServiceCenter& operator=(ServiceCenter&&) = delete;
  ServiceCenter& operator=(const ServiceCenter&) = delete;

public:
  explicit ServiceCenter(QObject* parent = nullptr);
  explicit ServiceCenter(Version local_version, QObject* parent = nullptr);
  virtual ~ServiceCenter() = default;

public:  // module interface
  void initialize();
  void uninitialize();
  void loadUserSettings();

  ApplicationType getApplicationType() const;
  void setApplicationType(ApplicationType type);

  /// @param handler (file_path)
  void setOpenFileHandler(std::function<void(const QString&)> handler);

  /// @param handler (file_path_list)
  void setOpenFileListHandler(std::function<void(const QStringList&)> handler);

  /// @param saver (save_dir)->file_path
  void setSelectedModelCombineSaver(std::function<QString(const QString&)> saver);

  /// @param saver (save_dir)->file_path_list
  void setSelectedModelUncombineSaver(std::function<QStringList(const QString&)> saver);

  /// @param saver (save_dir)->file_path
  void setCurrentSliceSaver(std::function<QString(const QString&)> saver);

  QStringList getVaildSliceFileSuffixList() const;
  /// @note old list model will stay after the suffix list updated
  void setVaildSliceFileSuffixList(const QStringList& list);

  bool isOnlineSliceFileCompressed() const;
  void setOnlineSliceFileCompressed(bool compressed);

  QString getModelCacheDirPath() const;
  void setModelCacheDirPath(const QString& path);

  /// @return 0 if automatically determined by library
  size_t getMaxDownloadThreadCount() const;
  void setMaxDownloadThreadCount(size_t count);

public:  // base
  ServerType getServerType() const;
  void setServerType(ServerType server_type);
  int getServerTypeIndex() const;
  void setServerTypeIndex(int server_type);
  Q_SIGNAL void serverTypeChanged();
  Q_PROPERTY(int serverType
             READ getServerTypeIndex
             WRITE setServerTypeIndex
             NOTIFY serverTypeChanged);

  QAbstractListModel* getServerListModel();
  Q_PROPERTY(QAbstractListModel* serverListModel READ getServerListModel CONSTANT);

  QString getUrl() const;
  Q_PROPERTY(QString url READ getUrl CONSTANT);

  QString getModelGroupUrlHead() const;
  QString loadModelGroupUrl(const QString& group_uid);
  Q_PROPERTY(QString modelGroupUrlHead READ getModelGroupUrlHead CONSTANT);

public:  // sub servive
  std::weak_ptr<AccountService> getAccountService() const;
  QObject* getAccountServiceObject() const;
  Q_PROPERTY(QObject* accountService READ getAccountServiceObject CONSTANT);

  std::weak_ptr<ModelService> getModelService() const;
  QObject* getModelServiceObject() const;
  Q_PROPERTY(QObject* modelService READ getModelServiceObject CONSTANT);

  std::weak_ptr<SliceService> getSliceService() const;
  QObject* getSliceServiceObject() const;
  Q_PROPERTY(QObject* sliceService READ getSliceServiceObject CONSTANT);

  std::weak_ptr<PrinterService> getPrinterService() const;
  QObject* getPrinterServiceObject() const;
  Q_PROPERTY(QObject* printerService READ getPrinterServiceObject CONSTANT);

  std::weak_ptr<ModelLibraryService> getModelLibraryService() const;
  QObject* getModelLibraryServiceObject() const;
  Q_PROPERTY(QObject* modelLibraryService READ getModelLibraryServiceObject CONSTANT);

  std::weak_ptr<DownloadService> getDownloadService() const;
  QObject* getDownloadServiceObject() const;
  Q_PROPERTY(QObject* downloadService READ getDownloadServiceObject CONSTANT);

  std::weak_ptr<UpgradeService> getUpgradeService() const;
  QObject* getUpgradeServiceObject() const;
  Q_PROPERTY(QObject* upgradeService READ getUpgradeServiceObject CONSTANT);

private:
  std::unique_ptr<ServerListModel> server_list_model_{ nullptr };

  std::shared_ptr<HttpSession> http_session_{ nullptr };

  std::shared_ptr<AccountService> account_service_{ nullptr };
  std::shared_ptr<ModelService> model_service_{ nullptr };
  std::shared_ptr<SliceService> slice_service_{ nullptr };
  std::shared_ptr<PrinterService> printer_service_{ nullptr };
  std::shared_ptr<ModelLibraryService> model_library_service_{ nullptr };
  std::shared_ptr<DownloadService> download_service_{ nullptr };
  std::shared_ptr<UpgradeService> upgrade_service_{ nullptr };

  std::function<void(const QString&)> open_file_handler_{ nullptr };
  std::function<void(const QStringList&)> open_file_list_handler_{ nullptr };
  std::function<QString(const QString&)> model_group_url_creater_{ nullptr };
  std::function<QString(const QString&)> selected_model_combine_saver_{ nullptr };
  std::function<QStringList(const QString&)> selected_model_uncombine_saver_{ nullptr };
  std::function<QString(const QString&)> current_slice_saver_{ nullptr };

  QStringList vaild_slice_file_suffix_list_{};
  bool online_slice_file_compressed_{ false };
  QString model_cache_dir_path_{};
  size_t max_download_thread_count_{ 0 };
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_CENTER_H_
