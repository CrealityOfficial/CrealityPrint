#pragma once

#ifndef CXCLOUD_NET_GCODE_REQUEST_H_
#define CXCLOUD_NET_GCODE_REQUEST_H_

#include "cxcloud/interface.hpp"
#include "cxcloud/model/slice_model.h"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  class CXCLOUD_API LoadSliceListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadSliceListRequest(bool is_upload,
                                  int page_index,
                                  int page_size,
                                  QObject* parent = nullptr);
    virtual ~LoadSliceListRequest() = default;

   public:
    auto isUpload() const -> bool;
    auto getPageIndex() const -> int;
    auto getPageSize() const -> int;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    int page_index_;
    int page_size_;
    bool is_upload_;
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadSliceListRequest& request,
                                               SliceListModel& model) -> bool;





  class CXCLOUD_API CreateSliceRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit CreateSliceRequest(const QString& name, const QString& key, QObject* parent = nullptr);
    virtual ~CreateSliceRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    QString name_;
    QString key_;
  };





  class CXCLOUD_API DeleteSliceRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit DeleteSliceRequest(const QString& uid, QObject* parent = nullptr);
    virtual ~DeleteSliceRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_;
  };





  class CXCLOUD_API DownloadSliceRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit DownloadSliceRequest(const QString& dir_path,
                                  const QString& file_name,
                                  const QString& file_suffix,
                                  const QString& download_url,
                                  QObject* parent = nullptr);
    virtual ~DownloadSliceRequest() = default;

   public:
    auto getFilePath() const -> QString;

   protected:
    auto getDownloadDstPath() const -> QString override;
    auto getDownloadSrcUrl() const -> QString override;

   protected:
    QString dir_path_;
    QString file_name_;
    QString file_suffix_;
    QString file_path_;
    QString download_url_;
  };

}  // namespace cxcloud

#endif  // CXCLOUD_NET_GCODE_REQUEST_H_
