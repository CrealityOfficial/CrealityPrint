#include "cxcloud/service/printer_service.h"

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QDesktopServices>

#include "cxcloud/net/printer_request.h"
#include "cxcloud/net/common_request.h"

namespace cxcloud {

  PrinterService::PrinterService(std::weak_ptr<HttpSession> http_session, QObject* parent)
      : BaseService{ http_session, parent }
      , printer_list_model_{ std::make_unique<PrinterListModel>() } {}

  auto PrinterService::initialize() -> void {
    auto config_request = createRequest<LoadPrinterConfigListRequest>();

    config_request->setSuccessedCallback([this, config_request]() {
      SyncHttpResponseDataToModel(*config_request, device_name_image_map_);
    });

    config_request->asyncPost();
  }

  auto PrinterService::getServerTypeGetter() const -> std::function<ServerType()> {
    return server_type_getter_;
  }

  auto PrinterService::setServerTypeGetter(std::function<ServerType()> getter) -> void {
    server_type_getter_ = getter;
  }

  auto PrinterService::getPrinterListModel() const -> QAbstractItemModel* {
    return printer_list_model_.get();
  }

  auto PrinterService::loadPrinterList(int page_index, int page_size) -> void {
    if (page_index <= 0) {
      page_index = 1;
    }

    auto list_request = createRequest<LoadPrinterListRequest>(page_index, page_size);

    list_request->setSuccessedCallback([this, list_request]() {
      SyncHttpResponseDataToModel(*list_request, *printer_list_model_);

      for (auto printer : printer_list_model_->rawData()) {
        auto status_request = createRequest<LoadPrinterStatusRequest>(printer.uid);
        status_request->setSuccessedCallback([this,
                                              status_request,
                                              printer = std::move(printer)]() mutable {
          SyncHttpResponseDataToModel(*status_request, printer);

          const auto iter = device_name_image_map_.find(printer.device_name);
          if (iter != device_name_image_map_.cend()) {
            printer.image = iter->second;
          }

          printer_list_model_->emplaceBack(std::move(printer));
        });
        status_request->asyncGet();
      }
    });

    list_request->asyncPost();
  }

  auto PrinterService::clearPrinterListModel() const -> void {
    printer_list_model_->clear();
  }

  auto PrinterService::openCloudPrintWebpage() -> void {
    auto request = createRequest<SsoTokenRequest>();

    request->setSuccessedCallback([this, request]() {
      QDesktopServices::openUrl(QUrl{ QStringLiteral("%1/my-print?pageType=job&slice-token=%2")
        .arg([](ServerType server_type) {
            switch (server_type) {
              case ServerType::MAINLAND_CHINA: {
                return QStringLiteral("https://www.crealitycloud.cn");
              }
              default: {
                return QStringLiteral("https://www.crealitycloud.com");
              }
            }
          }(server_type_getter_ ? server_type_getter_() : ServerType::OTHERS))
        .arg(request->getSsoToken()) });
    });

    request->asyncPost();
  }

}  // namespace cxcloud
