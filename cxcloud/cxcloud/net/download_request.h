#pragma once

#ifndef CXCLOUD_NET_DOWNLOAD_REQUEST_H_
#define CXCLOUD_NET_DOWNLOAD_REQUEST_H_

#include <memory>

#include <QTime>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/download_model.h"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  class CXCLOUD_API DownloadModelRequest : public HttpRequest {
    Q_OBJECT;

    friend CXCLOUD_API bool SyncHttpResponseDataToModel(const DownloadModelRequest&,
                                                        DownloadItemListModel&);

   public:
    explicit DownloadModelRequest(
      const QString& group_id,
      const QString& model_id,
      const QString& name,
      std::weak_ptr<ThreadPool> thread_pool = std::shared_ptr<ThreadPool>{ nullptr },
      QObject* parent = nullptr);
    virtual ~DownloadModelRequest() = default;

   public:
    Q_SIGNAL void progressUpdated(const QString& group_id, const QString& model_id);
    Q_SIGNAL void downloadFinished(const QString& group_id, const QString& model_id);

    bool finished() const;
    bool successed() const;

   protected:
    void callWhenSuccessed() override;
    void callWhenFailed() override;
    bool isAutoDelete() const override;
    QByteArray getRequestData() const override;
    QString getUrlTail() const override;

    QString getDownloadDstPath() const override;
    QString getDownloadSrcUrl() const override;
    void callWhenProgressUpdated() override;

   protected:
    QString group_id_{};
    QString model_id_{};
    QString download_path_{};
    QString download_url_{};

    bool finished_{ false };
    bool successed_{ false };

    uint32_t progress_{};

    QTime time_{};
    int downloaded_{};
    uint32_t speed_{};

    std::shared_ptr<ThreadPool> thread_pool_shared_{ nullptr };
    std::weak_ptr<ThreadPool> thread_pool_weak_{ thread_pool_shared_ };
  };

  CXCLOUD_API bool SyncHttpResponseDataToModel(const DownloadModelRequest& request,
                                               DownloadItemListModel& model);

  class CXCLOUD_API UnlimitedDownloadModelRequest : public DownloadModelRequest {
    Q_OBJECT;

   public:
    using DownloadModelRequest::DownloadModelRequest;
    virtual ~UnlimitedDownloadModelRequest() = default;

   protected:
    QByteArray getRequestData() const override;
    QString getUrlTail() const override;
  };

}  // namespace cxcloud

#endif  // CXCLOUD_NET_DOWNLOAD_REQUEST_H_
