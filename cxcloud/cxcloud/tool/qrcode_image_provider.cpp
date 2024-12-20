#include "cxcloud/tool/qrcode_image_provider.h"

#include <QtGui/QPainter>

#include <qrencode.h>

namespace {

  auto UrlToImage(const QString& url, size_t width, size_t height) -> QImage {
    QImage image{ static_cast<int>(width), static_cast<int>(height), QImage::Format::Format_ARGB32 };
    QPainter painter{ &image };
    painter.setBrush(Qt::GlobalColor::white);
    painter.setPen(Qt::PenStyle::NoPen);
    painter.drawRect(0, 0, width, height);
    if (url.isEmpty()) {
      return image;
    }

    auto* qrcode = QRcode_encodeString(url.toUtf8().constData(),
                                       2,  // version
                                       QRecLevel::QR_ECLEVEL_Q,
                                       QRencodeMode::QR_MODE_8,
                                       1);  // case sensitive

    painter.setBrush(Qt::GlobalColor::black);
    const auto qrcode_width = std::max<size_t>(qrcode->width, 1);
    const auto point_width = static_cast<double>(width) / static_cast<double>(qrcode_width);
    const auto point_height = static_cast<double>(height) / static_cast<double>(qrcode_width);
    for (size_t y = 0; y < qrcode_width; ++y) {
      for (size_t x = 0; x < qrcode_width; ++x) {
        if (qrcode->data[y * qrcode_width + x] & 0x01) {
          QRectF rect{ x * point_width, y * point_height, point_width, point_height };
          painter.drawRects(&rect, 1);
        }
      }
    }

    QRcode_free(qrcode);
    return image;
  }

}  // namespace

namespace cxcloud {

  QrcodeImageProvider::QrcodeImageProvider(QObject* parent)
      : QObject{ parent }
      , QQuickImageProvider{ QQuickImageProvider::ImageType::Image } {}

  auto QrcodeImageProvider::getUrl() const -> QString {
    return url_;
  }

  auto QrcodeImageProvider::setUrl(const QString& url) -> void {
    url_ = url;
  }

  auto QrcodeImageProvider::requestImage(const QString& id,
                                         QSize* size,
                                         const QSize& requested_size) -> QImage {
    static QSize const DEFAULT_SIZE{ 150, 150 };

    auto image_size = requested_size;
    if (requested_size.width() <= 0 || requested_size.height() <= 0) {
      image_size = DEFAULT_SIZE;
    }

    return UrlToImage(url_, image_size.width(), image_size.height());
  }

}  // namespace cxcloud
