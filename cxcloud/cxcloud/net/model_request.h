#pragma once

#ifndef CXCLOUD_NET_MODEL_REQUEST_H_
#define CXCLOUD_NET_MODEL_REQUEST_H_

#include "cxcloud/interface.hpp"
#include "cxcloud/model/download_model.h"
#include "cxcloud/model/model_info_model.h"
#include "cxcloud/net/common_request.h"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

// ---------- model library service [begin] ----------

  class CXCLOUD_API LoadModelGroupCategoryListRequest : public LoadCategoryListRequest {
    Q_OBJECT;

   public:
    explicit LoadModelGroupCategoryListRequest(QObject* parent = nullptr);
    virtual ~LoadModelGroupCategoryListRequest() = default;
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadModelGroupCategoryListRequest& request,
                                               ModelGroupCategoryListModel& model) -> bool;





  class CXCLOUD_API LoadRecommendModelGroupListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadRecommendModelGroupListRequest(int      page_index,
                                                int      page_size,
                                                QObject* parent = nullptr);
    virtual ~LoadRecommendModelGroupListRequest() = default;

   public:
    auto getPageIndex() const -> int;
    auto getPageSize() const -> int;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    int page_index_{ 0 };
    int page_size_{ 24 };
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadRecommendModelGroupListRequest& request,
                                               ModelGroupInfoListModel& model) -> bool;





  class CXCLOUD_API LoadTypeModelGroupListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    enum class FilterType : int {
      MOST_LIKE     = 1,
      MOST_COLLECT  = 2,
      NEWEST_UPLOAD = 3,
      MOST_COMMENTS = 4,
      MOST_BUY      = 5,
      MOST_PRINT    = 6,
      MOST_DOWNLOAD = 7,
      MOST_POPULAR  = 8,
    };

   public:
    enum class PayType : int {
      MODEL_ALL  = 0,
      MODEL_TOLL = 1,
      MODEL_FREE = 2,
    };

   public:
    /// @param last_group_uid only used when filter_type = FilterType::NEWEST_UPLOAD
    explicit LoadTypeModelGroupListRequest(const QString& type_id,
                                           int            page_index,
                                           int            page_size,
                                           FilterType     filter_type = FilterType::NEWEST_UPLOAD,
                                           PayType        pay_type    = PayType::MODEL_ALL,
                                           const QString& last_group_uid = {},
                                           QObject*       parent         = nullptr);
    virtual ~LoadTypeModelGroupListRequest() = default;

   public:
    auto getTypeId() const -> QString;
    auto getPageIndex() const -> int;
    auto getPageSize() const -> int;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;
    auto handleResponseData() -> bool override;

   protected:
    QString type_id_{};
    int page_index_{ 0 };
    int page_size_{ 24 };
    FilterType filter_type_{};
    PayType pay_type_{};
    QString last_group_uid_{};
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadTypeModelGroupListRequest& request,
                                               ModelGroupInfoListModel&             model) -> bool;





  class CXCLOUD_API SearchModelRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit SearchModelRequest(const QString& keyword,
                                int            page_index,
                                int            page_size,
                                QObject*       parent = nullptr);
    virtual ~SearchModelRequest() = default;

   public:
    auto getKeyword() const -> QString;
    auto getPageIndex() const -> int;
    auto getPageSize() const -> int;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString keyword_{};
    int page_index_{ 0 };
    int page_size_{ 24 };
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const SearchModelRequest& request,
                                               ModelGroupInfoListModel&  model) -> bool;





  class CXCLOUD_API LoadModelGroupInfoRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadModelGroupInfoRequest(const QString& group_uid, QObject* parent = nullptr);
    virtual ~LoadModelGroupInfoRequest() = default;

   public:
    auto getGroupUid() const -> QString;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString group_uid_{};
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadModelGroupInfoRequest& request,
                                               ModelGroupInfoModel&             model) -> bool;

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadModelGroupInfoRequest& request,
                                               ModelGroupInfoListModel&         model) -> bool;





  class CXCLOUD_API LoadModelGroupFileListInfoRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadModelGroupFileListInfoRequest(const QString& group_id,
                                               int            count,
                                               QObject*       parent = nullptr);
    virtual ~LoadModelGroupFileListInfoRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString group_id_{};
    int count_{ 0 };
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadModelGroupFileListInfoRequest& request,
                                               ModelGroupInfoModel& model) -> bool;





  class CXCLOUD_API CollectModelGroupRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit CollectModelGroupRequest(const QString& uid, bool collect, QObject* parent = nullptr);
    virtual ~CollectModelGroupRequest() = default;

   public:
    auto getUid() const -> QString;
    auto isCollect() const -> bool;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
    bool collect_{ false };
  };





  class CXCLOUD_API LikeModelGroupRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LikeModelGroupRequest(const QString& uid, bool like, QObject* parent = nullptr);
    virtual ~LikeModelGroupRequest() = default;

   public:
    auto getUid() const -> QString;
    auto isLike() const -> bool;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
    bool like_{ false };
  };





  class CXCLOUD_API ShareModelGroupRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit ShareModelGroupRequest(const QString& uid, QObject* parent = nullptr);
    virtual ~ShareModelGroupRequest() = default;

   public:
    auto getUid() const -> QString;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
  };





  class CXCLOUD_API DownloadModelGroupRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit DownloadModelGroupRequest(const QString& uid, QObject* parent = nullptr);
    virtual ~DownloadModelGroupRequest() = default;

   public:
    auto getUid() const -> QString;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
  };





  class CXCLOUD_API DownloadModelGroupSuccessRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit DownloadModelGroupSuccessRequest(const QString& uid, QObject* parent = nullptr);
    virtual ~DownloadModelGroupSuccessRequest() = default;

   public:
    auto getUid() const -> QString;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString uid_{};
  };

// ---------- model library service [end] ----------





// ---------- model library author service [begin] ----------

  class CXCLOUD_API LoadModelGroupAuthorInfoRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadModelGroupAuthorInfoRequest(const QString& author_id, QObject* parent = nullptr);
    virtual ~LoadModelGroupAuthorInfoRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString author_id_{};
  };





  class CXCLOUD_API LoadModelGroupAuthorModelListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadModelGroupAuthorModelListRequest(const QString& user_id,
                                                  int            page_index  = 0,
                                                  int            page_size   = 24,
                                                  int            filter_type = 3,
                                                  const QString& keyword     = {},
                                                  QObject*       parent      = nullptr);
    virtual ~LoadModelGroupAuthorModelListRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    int page_index_{ 0 };
    int page_size_{ 24 };
    int filter_type_{ 3 };
    QString user_id_{};
    QString keyword_{};
  };

//---------- model library author service [end] ----------





// ---------- model service [begin] ----------

  class CXCLOUD_API DeleteModelRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit DeleteModelRequest(const QString& id, QObject* parent = nullptr);
    virtual ~DeleteModelRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   protected:
    QString id_{};
  };





  class CXCLOUD_API CreateModelGroupRequest : public HttpRequest {
    Q_OBJECT;

   public:
    struct FileInfo {
      QString key;
      QString name;
      int     size;
    };

   public:
    explicit CreateModelGroupRequest(bool                       is_original,
                                     bool                       is_share,
                                     int                        color_index,
                                     const QString&             category_id,
                                     const QString&             group_name,
                                     const QString&             group_desc,
                                     const QString&             license,
                                     const std::list<FileInfo>& file_list,
                                     QObject*                   parent = nullptr);
    virtual ~CreateModelGroupRequest() = default;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    bool                is_original_{ false };
    bool                is_share_{ false };
    int                 color_index_{ 0 };
    QString             category_id_{};
    QString             group_name_{};
    QString             group_desc_{};
    QString             license_{};
    std::list<FileInfo> file_list_{};
  };





  class CXCLOUD_API LoadUploadedModelGroupListReuquest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadUploadedModelGroupListReuquest(size_t   page_index = 0,
                                                size_t   page_size  = 24,
                                                QString  keyword    = {},
                                                size_t   begin_time = 0,
                                                size_t   end_time   = 0,
                                                QObject* parent     = nullptr);
    virtual ~LoadUploadedModelGroupListReuquest() = default;

   public:
    auto getPageIndex() const -> size_t;
    auto getPageSize() const -> size_t;
    auto getKeyword() const -> QString;
    auto getBeginTime() const -> size_t;
    auto getEndTime() const -> size_t;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    size_t  page_index_{ 0 };
    size_t  page_size_{ 24 };
    QString keyword_{};
    size_t  begin_time_{ 0 };
    size_t  end_time_{ 0 };
  };

  CXCLOUD_API bool SyncHttpResponseDataToModel(const LoadUploadedModelGroupListReuquest& request,
                                               ModelGroupInfoListModel& model);





  class CXCLOUD_API LoadCollectedModelGroupListRequest : public HttpRequest {
    Q_OBJECT;

   public:
    enum class FilterType : int {
      MOST_LIKED       = 1,
      MOST_COLLECTED   = 2,
      NEWEST_UPLOADED  = 3,
      MOST_COMMIT      = 4,
      MOST_PURCHASED   = 5,
      MOST_PRINTED     = 6,
      MOST_DOWNLOAD    = 7,
      NEWEST_COLLECTED = 8,
    };

   public:
    explicit LoadCollectedModelGroupListRequest(
        size_t     page_index  = 0,
        size_t     page_size   = 24,
        QString    user_id     = {},
        FilterType filter_type = FilterType::NEWEST_COLLECTED,
        size_t     begin_time  = 0,
        size_t     end_time    = 0,
        QObject*   parent      = nullptr);
    virtual ~LoadCollectedModelGroupListRequest() = default;

   public:
    auto getPageIndex() const -> size_t;
    auto getPageSize() const -> size_t;
    auto getUserId() const -> QString;
    auto getFilterType() const -> FilterType;
    auto getBeginTime() const -> size_t;
    auto getEndTime() const -> size_t;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    size_t     page_index_{ 0 };
    size_t     page_size_{ 24 };
    QString    user_id_{};
    FilterType filter_type_{ FilterType::MOST_COLLECTED };
    size_t     begin_time_{ 0 };
    size_t     end_time_{ 0 };
  };

  CXCLOUD_API bool SyncHttpResponseDataToModel(const LoadCollectedModelGroupListRequest& request,
                                               ModelGroupInfoListModel& model);





  class CXCLOUD_API LoadPurchasedModelGroupListReuquest : public HttpRequest {
    Q_OBJECT;

   public:
    enum class OrderStatus : int {
      ANY             = 0,
      NOT_PURCHASED   = 1,
      PURCHASED       = 2,
      PURCHASE_FAILED = 3,
    };

   public:
    explicit LoadPurchasedModelGroupListReuquest(size_t      page_index   = 0,
                                                 size_t      page_size    = 24,
                                                 QString     order_id     = {},
                                                 OrderStatus order_status = OrderStatus::PURCHASED,
                                                 size_t      begin_time   = 0,
                                                 size_t      end_time     = 0,
                                                 QObject*    parent       = nullptr);
    virtual ~LoadPurchasedModelGroupListReuquest() = default;

   public:
    auto getPageIndex() const -> size_t;
    auto getPageSize() const -> size_t;
    auto getOrderId() const -> QString;
    auto getOrderStatus() const -> OrderStatus;
    auto getBeginTime() const -> size_t;
    auto getEndTime() const -> size_t;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

   private:
    size_t      page_index_{ 0 };
    size_t      page_size_{ 24 };
    QString     order_id_{};
    OrderStatus order_status_{};
    size_t      begin_time_{ 0 };
    size_t      end_time_{ 0 };
  };

  CXCLOUD_API auto SyncHttpResponseDataToModel(const LoadPurchasedModelGroupListReuquest& request,
                                               ModelGroupInfoListModel& model) -> bool;

// ---------- model service [end] ----------

}  // namespace cxcloud

#endif  // CXCLOUD_NET_MODEL_REQUEST_H_
