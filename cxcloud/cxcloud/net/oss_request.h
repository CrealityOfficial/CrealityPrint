#pragma once

#ifndef CXCLOUD_NET_OSS_REQUEST_H_
#define CXCLOUD_NET_OSS_REQUEST_H_

#include <functional>

#include <QtCore/QPointer>
#include <QtCore/QTime>

#include "cxcloud/interface.hpp"
#include "cxcloud/net/http_request.h"

namespace cxcloud {

class OssRequest;

CXCLOUD_API void StartOssRequest(QPointer<OssRequest> request);

class CXCLOUD_API OssRequest : public HttpRequest {
  Q_OBJECT;

  friend CXCLOUD_API void StartOssRequest(QPointer<OssRequest>);

public:
  explicit OssRequest(QObject* parent = nullptr);
  virtual ~OssRequest() = default;

protected:
  QString getUrlTail() const override;
  QByteArray getRequestData() const override;
  void callWhenResponsed() override;

protected:
  virtual QString getRealUrlTail() const;
  virtual QByteArray getRealRequestData() const;
  virtual void handleRealTask();

protected:
  QString getAccessKeyId() const;
  QString getSecretAccessKey() const;
  QString getSessionToken() const;
  QString getEndPoint() const;
  QString getFileBucket() const;
  QString getFilePrefixPath() const;
  QString getInternalBucket() const;
  QString getInternalPrefixPath() const;

  virtual bool initOssConfig() const;

private:
  bool isReadyToHandleRealTask() const;

private:
  enum class State : int {
    INFO,
    BUCKET,
    REAL,
  };

  State state_;

  QString access_key_id_;
  QString secret_access_key_;
  QString session_token_;
  QString end_point_;
  QString file_bucket_;
  QString file_prefix_path_;
  QString internal_bucket_;
  QString internal_prefix_path_;
};

class CXCLOUD_API UploadFileRequest : public OssRequest {
  Q_OBJECT;

public:
  enum class FileType : int {
    INTERNAL,
    FILE,
    IMAGE,
    VIDEO,
  };

public:
  /// @param file_list local file full path list
  /// @param suffix online file suffix
  explicit UploadFileRequest(const QStringList& file_list = {},
                             const QString& suffix = {},
                             FileType file_type = FileType::INTERNAL,
                             QObject* parent = nullptr);
  virtual ~UploadFileRequest() = default;

public:
  QString getSuffix() const;
  void setSuffix(const QString& suffix);

  QStringList getFileList() const;
  void setFileList(const QStringList& file_list);

  QString getOnlineFilePath(const QString& local_file_path);

protected:
  void handleRealTask() override;
  void callWhenProgressUpdated() override;
  bool initOssConfig() const override;

private:
  QStringList file_list_;
  QString suffix_;
  FileType file_type_;

  std::map<QString, QString> local_online_path_map_;

  std::mutex mutex_;
  std::atomic<long long> total_size_;
  std::atomic<long long> uploaded_size_;
  QTime time_;

  size_t finished_count_;
  size_t successed_count_;
};

class CXCLOUD_API GcodeUploadRequest : public OssRequest {
  Q_OBJECT
public:
  GcodeUploadRequest(const QString& name, const QString& fileName, QObject* parent = nullptr);
  virtual ~GcodeUploadRequest();

  const QString& getName() const;
  const QString& getFileKey() const;

protected:
  void handleRealTask() override;
  QString getRealUrlTail() const override;
protected slots:
  void uploadProgress(int value);
  void uploadResult(bool success);

protected:
  QString m_fileName;
  QString m_name;
  QString m_fileKey;
};

}  // namespace cxcloud

#endif  // CXCLOUD_NET_OSS_REQUEST_H_
