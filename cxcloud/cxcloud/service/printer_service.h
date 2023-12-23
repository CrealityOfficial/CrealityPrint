#pragma once

#ifndef CXCLOUD_SERVICE_PRINTER_SERVICE_H_
#define CXCLOUD_SERVICE_PRINTER_SERVICE_H_

#include <functional>

#include "cxcloud/interface.hpp"
#include "cxcloud/service/base_service.hpp"

namespace cxcloud {

class CXCLOUD_API PrinterService : public BaseService {
  Q_OBJECT;

public:
  explicit PrinterService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
  virtual ~PrinterService() = default;

public:
  Q_INVOKABLE void loadPrinterList(int page_index, int page_size);
  Q_SIGNAL void loadPrinterListSuccessed(const QVariant& json_string);
  Q_SIGNAL void printerListInfoBotained(const QVariant& json_string);
  Q_SIGNAL void printerBaseInfoBotained(const QVariant& json_string, const QVariant& printer_name);
  Q_SIGNAL void printerRealtimeInfoBotained(const QVariant& json_string,
                                            const QVariant& printer_name);
  Q_SIGNAL void printerPostitonInfoBotained(const QVariant& json_string,
                                            const QVariant& printer_name);
  Q_SIGNAL void printerStateInfoBotained(const QVariant& json_string, const QVariant& printer_name);

  Q_INVOKABLE void openCloudPrintWebpage();

private:
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_PRINTER_SERVICE_H_
