#include "cxcloud/service/printer_service.h"

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QDesktopServices>

#include "cxcloud/net/printer_request.h"
#include "cxcloud/net/common_request.h"
#include "cxcloud/net/oss_request.h"

namespace cxcloud {

PrinterService::PrinterService(std::weak_ptr<HttpSession> http_session, QObject* parent)
    : BaseService(http_session, parent) {}

void PrinterService::loadPrinterList(int page_index, int page_size) {
  QPointer<HttpRequest> request{nullptr};

  request = createRequest<GetPrinterTypeListRequest>();
  request->setSuccessedCallback([this, request]() {
    if (!request) { return; }
    Q_EMIT loadPrinterListSuccessed(QVariant::fromValue<QString>(request->getResponseData()));
  });
  HttpPost(request);

  request = createRequest<GetDeviceListRequest>(
    page_index,
    page_size,
    [this](auto&&... args) { printerBaseInfoBotained(std::forward<decltype(args)>(args)...); },
    [this](auto&&... args) { printerRealtimeInfoBotained(std::forward<decltype(args)>(args)...); },
    [this](auto&&... args) { printerPostitonInfoBotained(std::forward<decltype(args)>(args)...); },
    [this](auto&&... args) { printerStateInfoBotained(std::forward<decltype(args)>(args)...); });
  request->setResponsedCallback([this, request]() {
    if (!request) { return; }
    Q_EMIT printerListInfoBotained(QVariant::fromValue<QString>(request->getResponseData()));
  });
  HttpPost(request);
}

void PrinterService::openCloudPrintWebpage() {
  auto* request = createRequest<SsoTokenRequest>();
  request->setSuccessedCallback([this, request]() {
    QDesktopServices::openUrl(QUrl{ request->getSsoUrl() });
  });
  HttpPost(request);
}

}  // namespace cxcloud
