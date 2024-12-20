#pragma once

#ifndef CXCLOUD_SERVICE_PRINTER_SERVICE_H_
#define CXCLOUD_SERVICE_PRINTER_SERVICE_H_

#include <functional>
#include <map>
#include <memory>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"
#include "cxcloud/model/printer_model.h"
#include "cxcloud/service/base_service.h"

namespace cxcloud {

  class CXCLOUD_API PrinterService : public BaseService {
    Q_OBJECT;

   public:
    explicit PrinterService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~PrinterService() = default;

   public:
    auto initialize() -> void override;

    auto getServerTypeGetter() const -> std::function<ServerType()>;
    auto setServerTypeGetter(std::function<ServerType()> getter) -> void;

   public:
    auto getPrinterListModel() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* printerListModel READ getPrinterListModel CONSTANT);
    Q_INVOKABLE void loadPrinterList(int page_index, int page_size);
    Q_INVOKABLE void clearPrinterListModel() const;

    Q_INVOKABLE void openCloudPrintWebpage();

   private:
    std::function<ServerType()> server_type_getter_{ nullptr };
    std::unique_ptr<PrinterListModel> printer_list_model_{ nullptr };
    std::map<QString, QString> device_name_image_map_{};
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_PRINTER_SERVICE_H_
