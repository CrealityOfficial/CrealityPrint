#pragma once

#ifndef CXCLOUD_NET_PRINTER_REQUEST_H_
#define CXCLOUD_NET_PRINTER_REQUEST_H_

#include <functional>

#include "cxcloud/interface.hpp"
#include "cxcloud/net/http_request.h"

namespace cxcloud {

class CXCLOUD_API GetPrinterTypeListRequest : public HttpRequest {
  Q_OBJECT;

public:
  explicit GetPrinterTypeListRequest(QObject* parent = nullptr);
  virtual ~GetPrinterTypeListRequest() = default;

protected:
  QString getUrlTail() const override;
};

class CXCLOUD_API GetDeviceListRequest : public HttpRequest {
  Q_OBJECT;

public:
  GetDeviceListRequest(
		int pageIndex,
		int pageSize,
		std::function<void(const QVariant&, const QVariant&)> info_updater = nullptr,
		std::function<void(const QVariant&, const QVariant&)> realtime_info_updater = nullptr,
		std::function<void(const QVariant&, const QVariant&)> position_info_updater = nullptr,
		std::function<void(const QVariant&, const QVariant&)> status_updater = nullptr,
		QObject* parent = nullptr);
  virtual ~GetDeviceListRequest();

protected:
  void callWhenSuccessed() override;
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;

protected:
  int m_pageIndex;
  int m_pageSize;

	std::function<void(const QVariant&, const QVariant&)> info_updater_;
	std::function<void(const QVariant&, const QVariant&)> realtime_info_updater_;
	std::function<void(const QVariant&, const QVariant&)> position_info_updater_;
	std::function<void(const QVariant&, const QVariant&)> status_updater_;
};

enum class CXCLOUD_API DeviceQueryType {
  dqt_property_status,
  dqt_status,
  dqt_property_iot,
  dqt_property_iot_realtime,
  dqt_property_iot_position
};

struct DeviceInfo {
  QString productKey;
  QString deviceName;
  QString iotId;
  QString tbId;
  int iotType;
};

class CXCLOUD_API QueryDeviceInfoResponse : public HttpRequest {
  Q_OBJECT;

public:
  QueryDeviceInfoResponse(
		DeviceQueryType type,
		const DeviceInfo& deviceInfo,
		std::function<void(const QVariant&, const QVariant&)> info_updater = nullptr,
		std::function<void(const QVariant&, const QVariant&)> realtime_info_updater = nullptr,
		std::function<void(const QVariant&, const QVariant&)> position_info_updater = nullptr,
		std::function<void(const QVariant&, const QVariant&)> status_updater = nullptr,
		QObject* parent = nullptr);
  virtual ~QueryDeviceInfoResponse();

  QString getUrl() const override;

protected:
  void callWhenSuccessed() override;
  QByteArray getRequestData() const override;
  QString getUrlTail() const override;

  cpr::Parameters createParameters(const QString& action,
                                   const QString& accessKeyId,
                                   const QString& accessToken);

protected:
  DeviceInfo m_deviceInfo;
  DeviceQueryType m_type;
  bool m_postStage;

  bool use_iot_url_;

	std::function<void(const QVariant&, const QVariant&)> info_updater_;
	std::function<void(const QVariant&, const QVariant&)> realtime_info_updater_;
	std::function<void(const QVariant&, const QVariant&)> position_info_updater_;
	std::function<void(const QVariant&, const QVariant&)> status_updater_;
};

}  // namespace cxcloud

#endif  // CXCLOUD_NET_PRINTER_REQUEST_H_
