#pragma once

#ifndef CXCLOUD_NET_COMMON_REQUEST_H_
#define CXCLOUD_NET_COMMON_REQUEST_H_

#include <QtCore/QDateTime>

#include "cxcloud/interface.hpp"
#include "cxcloud/tool/version.h"
#include "cxcloud/net/http_request.h"

namespace cxcloud {

class CXCLOUD_API SsoTokenRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit SsoTokenRequest(QObject* parent = nullptr);
  virtual ~SsoTokenRequest() = default;

public:
  QString getSsoUrl() const;

protected:
  void callWhenSuccessed() override;
  QString getUrlTail() const override;

private:
  QString sso_url_;
};

class CXCLOUD_API LoadCategoryListRequest : public HttpRequest {
  Q_OBJECT;

public:
  /// @brief 分类类型
  enum class Type : int {
    FRIENDS       = 1,  // 圈子
    MODEL         = 2,  // 模型
    LABEL         = 3,  // 标签
    PRODUCT       = 4,  // 产品分类
    FAQ           = 5,  // 常见问题
    DOCUMENTATION = 6,  // 说明文档
    MODEL_V3      = 7,  // 模型v3
  };

public:
  explicit LoadCategoryListRequest(Type type, QObject* parent = nullptr);
  virtual ~LoadCategoryListRequest() = default;

protected:
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;

protected:
  Type type_;
};

class CXCLOUD_API DownloadRequest : public HttpRequest {
  Q_OBJECT;

public:
  /// @param dst_path absolute path on local disk of file to be download
  explicit DownloadRequest(const QString& src_url,
                           const QString& dst_path,
                           QObject* parent = nullptr);
  virtual ~DownloadRequest() = default;

public:
  QString getDownloadSrcUrl() const override;
  QString getDownloadDstPath() const override;

private:
  QString src_url_;
  QString dst_path_;
};

class CXCLOUD_API CheckNewestVersionRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit CheckNewestVersionRequest(QObject* parent = nullptr);
  virtual ~CheckNewestVersionRequest() = default;

public:  // vaild when successed response
  bool exist() const;
  QString uid() const;
  QString name() const;
  Version version() const;
  QDateTime time() const;
  QString description() const;
  QString url() const;

protected:
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;
  void callWhenSuccessed() override;

private:
  int palform_;

  QString uid_;
  QString name_;
  Version version_;
  QDateTime time_;
  QString description_;
  QString url_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_NET_COMMON_REQUEST_H_
