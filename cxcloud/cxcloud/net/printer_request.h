#pragma once

#ifndef CXCLOUD_NET_PRINTER_REQUEST_H_
#define CXCLOUD_NET_PRINTER_REQUEST_H_

#include <functional>
#include <map>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/printer_model.h"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  class CXCLOUD_API LoadPrinterListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadPrinterListRequest(int page_index,
                                    int page_size,
                                    QObject* parent = nullptr);
    virtual ~LoadPrinterListRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    int page_index_;
    int page_size_;
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadPrinterListRequest& request,
                                               PrinterListModel& model) -> bool;





  class CXCLOUD_API LoadPrinterConfigListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadPrinterConfigListRequest(QObject* parent = nullptr);
    virtual ~LoadPrinterConfigListRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadPrinterConfigListRequest& request,
                                               std::map<QString, QString>& model) -> bool;





  class CXCLOUD_API LoadPrinterStatusRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadPrinterStatusRequest(QString uid, QObject* parent = nullptr);
    virtual ~LoadPrinterStatusRequest() = default;

   public:
    QString getUid() const;

   protected:
    QString getUrlTail() const override;
    HttpParameters getParameters() const override;

   private:
    QString uid_;
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadPrinterStatusRequest& request,
                                               PrinterListModel::Data& model) -> bool;

}  // namespace cxcloud

#endif
