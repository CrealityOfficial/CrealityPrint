#pragma once

#ifndef CXCLOUD_SERVICE_SLICE_SERVICE_H_
#define CXCLOUD_SERVICE_SLICE_SERVICE_H_

#include <functional>
#include <memory>

#include "cxcloud/define.h"
#include "cxcloud/interface.hpp"
#include "cxcloud/model/slice_model.h"
#include "cxcloud/net/oss_request.h"
#include "cxcloud/service/base_service.h"

namespace cxcloud {

  class CXCLOUD_API SliceService : public BaseService {
    Q_OBJECT;

   public:
    explicit SliceService(std::weak_ptr<HttpSession> http_session, QObject* parent = nullptr);
    virtual ~SliceService() = default;

   public:
    auto setCurrentSliceSaver(std::function<QString(const QString&)> saver) -> void;
    auto setOpenFileHandler(std::function<void(const QString&)> handler) -> void;

    auto isOnlineSliceFileCompressed() const -> bool;
    auto setOnlineSliceFileCompressed(bool compressed) -> void;

    auto getWebUrlGetter() const -> std::function<QString(void)>;
    auto setWebUrlGetter(std::function<QString(void)> getter) -> void;

   public:  // about slice file
    auto getVaildSuffixList() const -> QStringList;
    /// @note old list model will stay after the suffix list updated
    auto setVaildSuffixList(const QStringList& list) -> void;
    Q_SIGNAL void vaildSuffixListChanged();
    Q_PROPERTY(QStringList vaildSuffixList READ getVaildSuffixList NOTIFY vaildSuffixListChanged);

    auto getUploadedSliceListModel() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* uploadedSliceListModel READ getUploadedSliceListModel CONSTANT);
    Q_INVOKABLE void clearUploadedSliceListModel() const;
    Q_INVOKABLE void loadUploadedSliceList(int page_index, int page_size);
    Q_INVOKABLE void deleteUploadedSlice(const QString& uid);

    auto getCloudSliceListModel() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* cloudSliceListModel READ getCloudSliceListModel CONSTANT);
    Q_INVOKABLE void clearCloudSliceListModel() const;
    Q_INVOKABLE void loadCloudSliceList(int page_index, int page_size);
    Q_INVOKABLE void deleteCloudSlice(const QString& uid);

    Q_INVOKABLE void uploadSlice(const QString& slice_name);
    Q_INVOKABLE void cancelUploadSlice();
    Q_SIGNAL void uploadSliceProgressUpdated(const QVariant& progress);
    Q_SIGNAL void uploadSliceSuccessed();
    Q_SIGNAL void uploadSliceCanceled();
    Q_SIGNAL void uploadSliceFailed();

    Q_INVOKABLE void importSlice(const QString& uid);
    Q_SIGNAL void importSliceSuccessed(const QString& uid);
    Q_SIGNAL void importSliceFailed(const QString& uid);

    Q_INVOKABLE void openPrintSliceWebpage(const QString& uid);

   private:
    const QString cache_dir_name_{};
    QStringList vaild_suffix_list_{};
    bool online_slice_file_compressed_{ false };

    std::function<QString(const QString&)> current_slice_saver_{ nullptr };
    std::function<void(const QString&)> open_file_handler_{ nullptr };
    std::function<QString(void)> web_url_getter_{ nullptr };

    std::unique_ptr<SliceListModel> uploaded_slice_list_model_{ nullptr };
    std::unique_ptr<SliceListModel> cloud_slice_list_model_{ nullptr };

    QPointer<UploadFileRequest> upload_request_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_SERVICE_SLICE_SERVICE_H_
