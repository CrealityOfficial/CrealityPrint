#pragma once

#ifndef CXCLOUD_NET_COMMON_REQUEST_H_
#define CXCLOUD_NET_COMMON_REQUEST_H_

#include <QtCore/QDateTime>

#include "cxcloud/interface.hpp"
#include "cxcloud/tool/version.h"
#include "cxcloud/tool/http_component.h"
#include "cxcloud/tool/job_executor.h"

namespace cxcloud {

  class CXCLOUD_API SsoTokenRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit SsoTokenRequest(QObject* parent = nullptr);
    virtual ~SsoTokenRequest() = default;

   public:
    auto getSsoToken() const -> QString;

   protected:
    auto callWhenSuccessed() -> void override;
    auto getUrlTail() const -> QString override;

   private:
    QString token_{};
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
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

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
    virtual ~DownloadRequest() override;
    // virtual ~DownloadRequest() override = default;

   public:
    auto getDownloadSrcUrl() const -> QString override;
    auto getDownloadDstPath() const-> QString override;
    auto getServerFileName() const -> QString;

    auto getJob() const -> QPointer<Job>;

   protected:
    auto checkRequestData() const -> bool override;
    auto callWhenProgressUpdated() -> void override;

    auto handleResponseData() -> bool override;
    auto callWhenSuccessed() -> void override;

   private:
    auto onJobInterrupted() -> void;

   private:
    QString src_url_{};
    QString dst_path_{};
    QString server_file_name_{};
    mutable QPointer<Job> download_job_{ nullptr };
  };





  class CXCLOUD_API CheckNewestVersionRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit CheckNewestVersionRequest(QObject* parent = nullptr);
    virtual ~CheckNewestVersionRequest() = default;

   public:  // vaild when successed response
    auto exist() const -> bool;
    auto uid() const -> QString;
    auto name() const -> QString;
    auto version() const -> Version;
    auto time() const -> QDateTime;
    auto description() const -> QString;
    auto url() const -> QString;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;
    auto callWhenSuccessed() -> void override;

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
