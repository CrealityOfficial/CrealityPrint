#include "cxcloud/net/slice_request.h"

#include <QtCore/QVariant>

#include <buildinfo.h>

#include <qtusercore/module/quazipfile.h>

namespace cxcloud {

LoadSliceListRequest::LoadSliceListRequest(bool is_upload,
                                           int page_index,
                                           int page_size,
                                           QObject* parent)
    : HttpRequest(parent)
    , is_upload_(is_upload)
    , page_index_(page_index)
    , page_size_(page_size) {}

bool LoadSliceListRequest::isUpload() const {
  return is_upload_;
}

int LoadSliceListRequest::getPageIndex() const {
  return page_index_;
}

int LoadSliceListRequest::getPageSize() const {
  return page_size_;
}

QByteArray LoadSliceListRequest::getRequestData() const {
  return QJsonDocument{ {
    { QStringLiteral("page"), page_index_ },
    { QStringLiteral("pageSize"), page_size_ },
    { QStringLiteral("isUpload"), is_upload_ },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString LoadSliceListRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v2/gcode/ownerList");
}

bool SyncHttpResponseDataToModel(const LoadSliceListRequest& request, SliceListModel& model) {
  const auto root = request.getResponseJson();
  if (root.empty()) { return false; }

  const auto slice_list = root["result"]["list"].toArray();
  if (slice_list.empty()) { return false; }

  const auto COMPRESSED_FORMAT = qtuser_core::CompressedFileFormat();

  for (const auto& slice : slice_list) {
    if (!slice.isObject()) { continue; }

    const auto uid = slice["id"].toString();

    auto data = model.load(uid);

    data.uid = std::move(uid);
    data.name = slice["name"].toString();
    data.image = slice["thumbnail"].toString();
    data.url = slice["downloadLink"].toString();

    const auto url_sub_list = data.url.split(".");
    for (auto iter = url_sub_list.rbegin(); iter != url_sub_list.rend(); ++iter) {
      auto suffix = *iter;
      if (suffix == COMPRESSED_FORMAT) { continue; }
      data.suffix = std::move(suffix);
      break;
    }

    model.emplaceBack(std::move(data));
  }

  return true;
}





CreateSliceRequest::CreateSliceRequest(const QString& name, const QString& key, QObject* parent)
    : HttpRequest(parent), name_(name), key_(key) {}

QByteArray CreateSliceRequest::getRequestData() const {
  return QJsonDocument{ QJsonObject{
    {
      QStringLiteral("list"),
      QJsonArray{
        QJsonObject{
          { QStringLiteral("name"), name_ },
          { QStringLiteral("filekey"), key_ },
          { QStringLiteral("type"), [this]() -> int {
            switch (getApplicationType()) {
              case ApplicationType::CREATIVE_PRINT:
                return 1;
              case ApplicationType::HALOT_BOX:
              default:
                return 2;
            }
          }() },
        },
      }
    },
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString CreateSliceRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v2/gcode/uploadGcode");
}





DeleteSliceRequest::DeleteSliceRequest(const QString& id, QObject* parent)
    : HttpRequest(parent), uid_(id) {}

QByteArray DeleteSliceRequest::getRequestData() const {
  return QJsonDocument{ QJsonObject{
    { QStringLiteral("id"), uid_ }
  } }.toJson(QJsonDocument::JsonFormat::Compact);
}

QString DeleteSliceRequest::getUrlTail() const {
  return QStringLiteral("/api/cxy/v2/gcode/deleteGcode");
}





DownloadSliceRequest::DownloadSliceRequest(const QString& dir_path,
                                           const QString& file_name,
                                           const QString& file_suffix,
                                           const QString& download_url,
                                           QObject* parent)
    : HttpRequest(parent)
    , dir_path_(dir_path)
    , file_name_(file_name)
    , file_suffix_(file_suffix)
    , download_url_(download_url)
    , file_path_(QStringLiteral("%1/%2.%3").arg(dir_path_).arg(file_name_).arg(file_suffix_)) {}

QString DownloadSliceRequest::getFilePath() const {
  return file_path_;
}

QString DownloadSliceRequest::getDownloadDstPath() const {
  return file_path_;
}

QString DownloadSliceRequest::getDownloadSrcUrl() const {
  return download_url_;
}

}  // namespace cxcloud
