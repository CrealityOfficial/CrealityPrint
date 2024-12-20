#include "cxcloud/net/printer_request.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace cxcloud {

  LoadPrinterListRequest::LoadPrinterListRequest(int page_index, int page_size, QObject* parent)
      : HttpRequest{ parent }, page_index_{ page_index }, page_size_{ page_size } {}

  auto LoadPrinterListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("page"), page_index_ },
      { QStringLiteral("pageSize"), page_size_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadPrinterListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/device/onwerDeviceList");
  }

  auto SyncHttpResponseDataToModel(const LoadPrinterListRequest& request,
                                   PrinterListModel& model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) { return false; }

    const auto printer_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (printer_list.empty()) { return false; }

    for (const auto& printer : printer_list) {
      if (!printer.isObject()) { continue; }

      auto uid = printer[QStringLiteral("tbId")].toString();
      auto data = model.load(uid);

      data.uid = std::move(uid);
      data.nickname = printer[QStringLiteral("nickName")].toString();
      data.device_id = printer[QStringLiteral("deviceName")].toString();

      model.emplaceBack(std::move(data));
    }

    return true;
  }





  LoadPrinterConfigListRequest::LoadPrinterConfigListRequest(QObject* parent)
      : HttpRequest{ parent } {}

  auto LoadPrinterConfigListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("isAll"), true },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadPrinterConfigListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/device/printerTypeListNew");
  }

  auto SyncHttpResponseDataToModel(const LoadPrinterConfigListRequest& request,
                                   std::map<QString, QString>& model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) { return false; }

    const auto printer_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (printer_list.empty()) { return false; }

    for (const auto& printer : printer_list) {
      if (!printer.isObject()) { continue; }

      auto name = printer[QStringLiteral("internalName")].toString();
      auto image = printer[QStringLiteral("thumbnail")].toString();

      model.emplace(std::make_pair(std::move(name), std::move(image)));
    }

    return true;
  }





  LoadPrinterStatusRequest::LoadPrinterStatusRequest(QString uid, QObject* parent)
      : HttpRequest{ parent }, uid_{ std::move(uid) }  {}

  auto LoadPrinterStatusRequest::getUid() const -> QString {
    return uid_;
  }

  auto LoadPrinterStatusRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/rest/iotrouter/plugins/telemetry/%1/values/attributes").arg(uid_);
  }

  HttpParameters LoadPrinterStatusRequest::getParameters() const {
    return {
      { QStringLiteral("keys"), QStringLiteral("connect,netIP,state,tfCard,model") },
    };
  }

  auto SyncHttpResponseDataToModel(const LoadPrinterStatusRequest& request,
                                   PrinterListModel::Data& model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) { return false; }

    const auto property_list = root[QStringLiteral("result")].toArray();
    if (property_list.empty()) { return false; }

    for (const auto& property : property_list) {
      if (!property.isObject()) { continue; }

      const auto key = property[QStringLiteral("key")].toString();

      if (key == QStringLiteral("connect")) {
        model.connected = property[QStringLiteral("value")].toInt() != 0;
      } else if (key == QStringLiteral("state")) {
        model.online = property[QStringLiteral("value")].toInt() != 0;
      } else if (key == QStringLiteral("tfCard")) {
        model.tf_card_exisit = property[QStringLiteral("value")].toInt() != 0;
      } else if (key == QStringLiteral("netIP")) {
        model.ip = property[QStringLiteral("value")].toString();
      } else if (key == QStringLiteral("model")) {
        model.device_name = property[QStringLiteral("value")].toString();
      }
    }

    return true;
  }

}  // namespace cxcloud
