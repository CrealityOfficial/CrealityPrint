#include "cxcloud/net/printer_request.h"

#include <QtCore/QTime>
#include <QtCore/QVariant>

namespace cxcloud {

GetPrinterTypeListRequest::GetPrinterTypeListRequest(QObject* parent) : HttpRequest(parent) {}

QString GetPrinterTypeListRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v2/device/printerTypeList");
}

GetDeviceListRequest::GetDeviceListRequest(
  int pageIndex,
  int pageSize,
  std::function<void(const QVariant&, const QVariant&)> info_updater,
  std::function<void(const QVariant&, const QVariant&)> realtime_info_updater,
  std::function<void(const QVariant&, const QVariant&)> position_info_updater,
  std::function<void(const QVariant&, const QVariant&)> status_updater,
  QObject* parent)
    : HttpRequest(parent)
    , m_pageIndex(pageIndex)
    , m_pageSize(pageSize)
    , info_updater_(info_updater)
    , realtime_info_updater_(realtime_info_updater)
    , position_info_updater_(position_info_updater)
    , status_updater_(status_updater) {}

GetDeviceListRequest::~GetDeviceListRequest() {}

void GetDeviceListRequest::callWhenSuccessed() {
  QJsonObject result = getResponseJson().value(QStringLiteral("result")).toObject();
  int count = result.value("count").toInt();
  if (count == 0) return;

  QList<DeviceInfo> infos;
  QJsonArray objs = result.value("list").toArray();

  for (QJsonValueRef ref : objs) {
    DeviceInfo info;
    QJsonObject object = ref.toObject();

    info.productKey = object.value("productKey").toString();
    info.deviceName = object.value("deviceName").toString();
    info.iotId = object.value("iotId").toString();
    info.tbId = object.value("tbId").toString();
    info.iotType = object.value("iotType").toInt();

    infos.append(info);
  }

  for (const DeviceInfo& info : infos) {
    if (info.iotType == 1) {
      HttpPost(new QueryDeviceInfoResponse(DeviceQueryType::dqt_property_status,
                                           info,
                                           info_updater_,
                                           realtime_info_updater_,
                                           position_info_updater_,
                                           status_updater_,
                                           this->parent()));

      HttpPost(new QueryDeviceInfoResponse(DeviceQueryType::dqt_status,
                                           info,
                                           info_updater_,
                                           realtime_info_updater_,
                                           position_info_updater_,
                                           status_updater_,
                                           this->parent()));
    } else if (info.iotType == 2) {
      {
        auto* request = new QueryDeviceInfoResponse(DeviceQueryType::dqt_property_iot,
                                                    info,
                                                    info_updater_,
                                                    realtime_info_updater_,
                                                    position_info_updater_,
                                                    status_updater_,
                                                    this->parent());
        request->setParameters(cpr::Parameters{
          {"keys",
           "netIP,tfCard,state,active,model,connect,printId,boxVersion,upgradeStatus,upgrade,"
           "bedTemp2,nozzleTemp2,fan,layer,setPosition,video"}});
        HttpGet(request);
      }

      {
        auto* request = new QueryDeviceInfoResponse(DeviceQueryType::dqt_property_iot_realtime,
                                                    info,
                                                    info_updater_,
                                                    realtime_info_updater_,
                                                    position_info_updater_,
                                                    status_updater_,
                                                    this->parent());
        request->setParameters(cpr::Parameters{
          {"keys",
           "nozzleTemp,bedTemp,curFeedratePct,dProgress,printProgress,printJobTime,printLeftTime"},
          {"useStrictDataTypes", "true"}});
        HttpGet(request);
      }

      HttpPost(new QueryDeviceInfoResponse(DeviceQueryType::dqt_property_iot_position,
                                           info,
                                           info_updater_,
                                           realtime_info_updater_,
                                           position_info_updater_,
                                           status_updater_,
                                           this->parent()));
    }
  }
}

QByteArray GetDeviceListRequest::getRequestData() const {
  return QString("{\"page\":%1, \"pageSize\":%2}").arg(m_pageIndex).arg(m_pageSize).toUtf8();
}

QString GetDeviceListRequest::getUrlTail() const {
  return "/api/cxy/v2/device/onwerDeviceList";
}





QueryDeviceInfoResponse::QueryDeviceInfoResponse(
  DeviceQueryType type,
  const DeviceInfo& deviceInfo,
  std::function<void(const QVariant&, const QVariant&)> info_updater,
  std::function<void(const QVariant&, const QVariant&)> realtime_info_updater,
  std::function<void(const QVariant&, const QVariant&)> position_info_updater,
  std::function<void(const QVariant&, const QVariant&)> status_updater,
  QObject* parent)
    : HttpRequest(parent)
    , info_updater_(info_updater)
    , realtime_info_updater_(realtime_info_updater)
    , position_info_updater_(position_info_updater)
    , status_updater_(status_updater)
    , m_deviceInfo(deviceInfo)
    , m_type(type)
    , m_postStage(true)
    , use_iot_url_(false) {}

QueryDeviceInfoResponse::~QueryDeviceInfoResponse() {}

QString QueryDeviceInfoResponse::getUrl() const {
  if (use_iot_url_) {
    return "https://iot.cn-shanghai.aliyuncs.com/";
  }

  return HttpRequest::getUrl();
}

void QueryDeviceInfoResponse::callWhenSuccessed() {
  QByteArray bytes = getResponseData();
  QJsonObject jsonObject = getResponseJson();

  QString data(bytes);
  if (m_postStage) {
    switch (m_type) {
      case DeviceQueryType::dqt_property_status: {
        const QJsonObject& aliyunInfo = jsonObject.value("aliyunInfo").toObject();
        QString accessKeyId = aliyunInfo.value("accessKeyId").toString();
        QString secretAccessKey = aliyunInfo.value("secretAccessKey").toString();
        QString sessionToken = aliyunInfo.value("sessionToken").toString();

        use_iot_url_ = true;
        setParameters(createParameters("QueryDevicePropertyStatus", accessKeyId, sessionToken));
        HttpGet(this);
      } break;
      case DeviceQueryType::dqt_status: {
        const QJsonObject& aliyunInfo = jsonObject.value("aliyunInfo").toObject();
        QString accessKeyId = aliyunInfo.value("accessKeyId").toString();
        QString secretAccessKey = aliyunInfo.value("secretAccessKey").toString();
        QString sessionToken = aliyunInfo.value("sessionToken").toString();

        use_iot_url_ = true;
        setParameters(createParameters("GetDeviceStatus", accessKeyId, sessionToken));
        HttpGet(this);
      } break;
      case DeviceQueryType::dqt_property_iot: {
        // invokeCloudUserInfoFunc("updateDeviceInformation_NewIOT",
        //                         QVariant::fromValue(data),
        //                         QVariant::fromValue(m_deviceInfo.deviceName));
        info_updater_(QVariant::fromValue(data), QVariant::fromValue(m_deviceInfo.deviceName));
      } break;
      case DeviceQueryType::dqt_property_iot_realtime: {
        // invokeCloudUserInfoFunc("updateDeviceInformation_NewIOT_RealTime",
        //                         QVariant::fromValue(data),
        //                         QVariant::fromValue(m_deviceInfo.deviceName));
        realtime_info_updater_(QVariant::fromValue(data),
                               QVariant::fromValue(m_deviceInfo.deviceName));
      } break;
      case DeviceQueryType::dqt_property_iot_position: {
        // invokeCloudUserInfoFunc("updateDeviceInformation_NewIOT_Position",
        //                         QVariant::fromValue(data),
        //                         QVariant::fromValue(m_deviceInfo.deviceName));
        position_info_updater_(QVariant::fromValue(data),
                               QVariant::fromValue(m_deviceInfo.deviceName));
      } break;
      default:
        break;
    }

    m_postStage = false;
  } else {
    switch (m_type) {
      case DeviceQueryType::dqt_property_status: {
        // invokeCloudUserInfoFunc("updateDeviceInformation_NewIOT",
        //                         QVariant::fromValue(data),
        //                         QVariant::fromValue(m_deviceInfo.deviceName));
        info_updater_(QVariant::fromValue(data), QVariant::fromValue(m_deviceInfo.deviceName));
      } break;
      case DeviceQueryType::dqt_status: {
        // invokeCloudUserInfoFunc("updateDeviceStatus",
        //                         QVariant::fromValue(data),
        //                         QVariant::fromValue(m_deviceInfo.deviceName));
        status_updater_(QVariant::fromValue(data), QVariant::fromValue(m_deviceInfo.deviceName));
      } break;
      case DeviceQueryType::dqt_property_iot:
        break;
      case DeviceQueryType::dqt_property_iot_realtime:
        break;
      case DeviceQueryType::dqt_property_iot_position:
        break;
      default:
        break;
    }
  }
}

QByteArray QueryDeviceInfoResponse::getRequestData() const {
  QString request = "{}";
  switch (m_type) {
    case DeviceQueryType::dqt_property_status:
    case DeviceQueryType::dqt_status:
      break;
    case DeviceQueryType::dqt_property_iot:
      break;
    case DeviceQueryType::dqt_property_iot_realtime:
      break;
    case DeviceQueryType::dqt_property_iot_position: {
      request = QString("{\"method\":\"get\",\"params\":{\"ReqPrinterPara\":1}}");
    } break;
    default:
      break;
  }

  return request.toUtf8();
}

QString QueryDeviceInfoResponse::getUrlTail() const {
  QString tail = "";
  switch (m_type) {
    case DeviceQueryType::dqt_property_status:
    case DeviceQueryType::dqt_status: {
      tail = "/api/cxy/account/v2/getAliyunInfo";
      break;
    }
    case DeviceQueryType::dqt_property_iot: {
      tail = "/api/rest/iotrouter/plugins/telemetry/" + m_deviceInfo.tbId + "/values/attributes";
    } break;
    case DeviceQueryType::dqt_property_iot_realtime: {
      tail = "/api/rest/iotrouter/" + m_deviceInfo.tbId + "/values/timeseries";
    } break;
    case DeviceQueryType::dqt_property_iot_position: {
      tail = "/api/rest/iotrouter/rpc/twoway/" + m_deviceInfo.tbId;
      break;
    }
    default:
      break;
  }

  return tail;
}

cpr::Parameters QueryDeviceInfoResponse::createParameters(const QString& action,
                                                          const QString& accessKeyId,
                                                          const QString& accessToken) {
  qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

  cpr::Parameters parameters = cpr::Parameters{};
  parameters.Add(cpr::Parameter{"Action", "QueryDevicePropertyStatus"});
  parameters.Add(cpr::Parameter{"Format", "JSON"});
  parameters.Add(cpr::Parameter{"Version", "2018-01-20"});
  parameters.Add(cpr::Parameter{"SignatureMethod", "HMAC-SHA1"});
  parameters.Add(cpr::Parameter{"SignatureNonce", QString::number(qrand()).toStdString()});
  parameters.Add(cpr::Parameter{"AccessKeyId", accessKeyId.toStdString()});
  parameters.Add(cpr::Parameter{"SecurityToken", accessToken.toStdString()});
  parameters.Add(cpr::Parameter{
    "Timestamp",
    QDateTime::currentDateTime().toUTC().toString("yyyy-MM-ddThh:mm:ssZ").toStdString()});
  parameters.Add(cpr::Parameter{"RegionId", "cn-shanghai"});
  parameters.Add(cpr::Parameter{"ProductKey", m_deviceInfo.productKey.toStdString()});
  parameters.Add(cpr::Parameter{"DeviceName", m_deviceInfo.deviceName.toStdString()});
  parameters.Add(cpr::Parameter{"IotId", m_deviceInfo.iotId.toStdString()});
  parameters.Add(cpr::Parameter{"Signature", m_deviceInfo.iotId.toStdString()});
  parameters.Add(cpr::Parameter{"SecurityToken", m_deviceInfo.iotId.toStdString()});
  parameters.Add(cpr::Parameter{"Timestamp", m_deviceInfo.iotId.toStdString()});

  return parameters;
}

}  // namespace cxcloud
