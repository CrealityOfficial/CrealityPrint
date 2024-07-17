#include "cxcloud/net/slice_request.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include <qtusercore/module/quazipfile.h>

namespace cxcloud {

  LoadSliceListRequest::LoadSliceListRequest(bool is_upload,
                                             int page_index,
                                             int page_size,
                                             QObject* parent)
      : HttpRequest{ parent }
      , page_index_{ page_index }
      , page_size_{ page_size }
      , is_upload_{ is_upload } {}

  auto LoadSliceListRequest::isUpload() const -> bool {
    return is_upload_;
  }

  auto LoadSliceListRequest::getPageIndex() const -> int {
    return page_index_;
  }

  auto LoadSliceListRequest::getPageSize() const -> int {
    return page_size_;
  }

  auto LoadSliceListRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ {
      { QStringLiteral("page"), page_index_ },
      { QStringLiteral("pageSize"), page_size_ },
      { QStringLiteral("isUpload"), is_upload_ },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto LoadSliceListRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/gcode/ownerList");
  }

  auto SyncHttpResponseDataToModel(const LoadSliceListRequest& request,
                                   SliceListModel& model) -> bool {
    const auto root = request.getResponseJson();
    if (root.empty()) {
      return false;
    }

    const auto slice_list = root[QStringLiteral("result")][QStringLiteral("list")].toArray();
    if (slice_list.empty()) {
      return false;
    }

    const auto COMPRESSED_FORMAT = qtuser_core::CompressedFileFormat();

    for (const auto& slice : slice_list) {
      if (!slice.isObject()) {
        continue;
      }

      const auto uid = slice[QStringLiteral("id")].toString();

      auto data = model.load(uid);

      data.uid = std::move(uid);
      data.name = slice[QStringLiteral("name")].toString();
      data.image = slice[QStringLiteral("thumbnail")].toString();
      data.url = slice[QStringLiteral("downloadLink")].toString();

      const auto url_sub_list = data.url.split(".");
      for (auto iter = url_sub_list.rbegin(); iter != url_sub_list.rend(); ++iter) {
        auto suffix = *iter;
        if (suffix == COMPRESSED_FORMAT) {
          continue;
        }

        data.suffix = std::move(suffix);
        break;
      }

      model.emplaceBack(std::move(data));
    }

    return true;
  }

  CreateSliceRequest::CreateSliceRequest(const QString& name, const QString& key, QObject* parent)
      : HttpRequest{ parent }, name_{ name }, key_{ key } {}

  auto CreateSliceRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      {
        QStringLiteral("list"), QJsonArray{
          QJsonObject{
            { QStringLiteral("name"), name_ },
            { QStringLiteral("filekey"), key_ },
            { QStringLiteral("type"), [this]() -> int {
              switch (getApplicationType()) {
                case ApplicationType::CREALITY_PRINT: {
                  return 1;
                }
                case ApplicationType::HALOT_BOX:
                default: {
                  return 2;
                }
              }
            }() },
          },
        }
      },
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto CreateSliceRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/gcode/uploadGcode");
  }





  DeleteSliceRequest::DeleteSliceRequest(const QString& id, QObject* parent)
      : HttpRequest{ parent }, uid_{ id } {}

  auto DeleteSliceRequest::getRequestData() const -> QByteArray {
    return QJsonDocument{ QJsonObject{
      { QStringLiteral("id"), uid_ }
    } }.toJson(QJsonDocument::JsonFormat::Compact);
  }

  auto DeleteSliceRequest::getUrlTail() const -> QString {
    return QStringLiteral("/api/cxy/v2/gcode/deleteGcode");
  }





  DownloadSliceRequest::DownloadSliceRequest(const QString& dir_path,
                                             const QString& file_name,
                                             const QString& file_suffix,
                                             const QString& download_url,
                                             QObject* parent)
      : HttpRequest{ parent }
      , dir_path_{ dir_path }
      , file_name_{ file_name }
      , file_suffix_{ file_suffix }
      , file_path_{ QStringLiteral("%1/%2.%3").arg(dir_path_).arg(file_name_).arg(file_suffix_) }
      , download_url_{ download_url } {}

  auto DownloadSliceRequest::getFilePath() const -> QString {
    return file_path_;
  }

  auto DownloadSliceRequest::getDownloadDstPath() const -> QString {
    return file_path_;
  }

  auto DownloadSliceRequest::getDownloadSrcUrl() const -> QString {
    return download_url_;
  }

}  // namespace cxcloud
