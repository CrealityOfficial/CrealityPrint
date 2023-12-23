#pragma once

#ifndef CXCLOUD_SERVICE_GCODE_SERVICE_H_
#define CXCLOUD_SERVICE_GCODE_SERVICE_H_

#include <functional>
#include <memory>

#include "cxcloud/interface.hpp"
#include "cxcloud/model/slice_model.h"
#include "cxcloud/service/base_service.hpp"

namespace cxcloud {

class CXCLOUD_API SliceService : public BaseService {
  Q_OBJECT;

public:
  explicit SliceService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
  virtual ~SliceService() = default;

public:
  void setCurrentSliceSaver(std::function<QString(const QString&)> saver);
  void setOpenFileHandler(std::function<void(const QString&)> handler);

  bool isOnlineSliceFileCompressed() const;
  void setOnlineSliceFileCompressed(bool compressed);

public:
  QStringList getVaildSuffixList() const;
  /// @note old list model will stay after the suffix list updated
  void setVaildSuffixList(const QStringList& list);
  Q_SIGNAL void vaildSuffixListChanged();
  Q_PROPERTY(QStringList vaildSuffixList READ getVaildSuffixList NOTIFY vaildSuffixListChanged);

  QAbstractItemModel* getUploadedSliceListModel() const;
  Q_PROPERTY(QAbstractItemModel* uploadedSliceListModel READ getUploadedSliceListModel CONSTANT);
  Q_INVOKABLE void clearUploadedSliceListModel() const;
  Q_INVOKABLE void loadUploadedSliceList(int page_index, int page_size);
  Q_SIGNAL void loadUploadedSliceListSuccessed(const QVariant& json_string,
                                               const QVariant& page_index,
                                               const QVariant& page_size);

  QAbstractItemModel* getCloudSliceListModel() const;
  Q_PROPERTY(QAbstractItemModel* cloudSliceListModel READ getCloudSliceListModel CONSTANT);
  Q_INVOKABLE void clearCloudSliceListModel() const;
  Q_INVOKABLE void loadCloudSliceList(int page_index, int page_size);
  Q_SIGNAL void loadCloudSliceListSuccessed(const QVariant& json_string,
                                            const QVariant& page_index,
                                            const QVariant& page_size);

  Q_INVOKABLE void uploadSlice(const QString& slice_name);
  Q_SIGNAL void uploadSliceSuccessed();
  Q_SIGNAL void uploadSliceFailed();
  Q_SIGNAL void uploadSliceProgressUpdated(const QVariant& progress);

  Q_INVOKABLE void importSlice(const QString& uid);
  Q_SIGNAL void importSliceSuccessed(const QString& uid);
  Q_SIGNAL void importSliceFailed(const QString& uid);

  Q_INVOKABLE void deleteSlice(const QString& uid);
  Q_SIGNAL void deleteSliceSuccessed(const QVariant& uid);
  Q_SIGNAL void deleteSliceFailed(const QVariant& uid);

private:
  const QString cache_dir_name_{};
  QStringList vaild_suffix_list_{};
  bool online_slice_file_compressed_{ false };

  std::function<QString(const QString&)> current_slice_saver_{ nullptr };
  std::function<void(const QString&)> open_file_handler_{ nullptr };

  std::unique_ptr<SliceListModel> uploaded_slice_list_model_{ nullptr };
  std::unique_ptr<SliceListModel> cloud_slice_list_model_{ nullptr };
};

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_GCODE_SERVICE_H_
