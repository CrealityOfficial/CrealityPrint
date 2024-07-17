#pragma once

#ifndef CXCLOUD_NET_PROFILE_REQUEST_H_
#define CXCLOUD_NET_PROFILE_REQUEST_H_

#include "cxcloud/interface.hpp"
#include "cxcloud/model/profile_model.h"
#include "cxcloud/tool/http_component.h"

namespace cxcloud {

  class CXCLOUD_API LoadPrinterProfileRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadPrinterProfileRequest(const Version& engine_version, QObject* parent = nullptr);
    virtual ~LoadPrinterProfileRequest() = default;

   public:  // request
    auto getEngineVersion() const -> Version;
    auto setEngineVersion(const Version& version) -> void;

   public:  // response
    auto profile() const -> const Profile&;
    auto getProfile() const -> Profile;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    Version engine_version_{};
    Profile profile_{};
  };





  class CXCLOUD_API CheckPrinterUpdatableRequest : public HttpRequest {
    Q_OBJECT;

   public:
    using Printer = Profile::Printer;

   public:
    explicit CheckPrinterUpdatableRequest(QObject* parent = nullptr);
    explicit CheckPrinterUpdatableRequest(const Version& engine_version,
                                          const std::list<Printer>& checked_printer_list,
                                          QObject* parent = nullptr);
    explicit CheckPrinterUpdatableRequest(const Version& engine_version,
                                          std::list<Printer>&& checked_printer_list,
                                          QObject* parent = nullptr);
    virtual ~CheckPrinterUpdatableRequest() = default;

   public:  // request
    auto getEngineVersion() const -> Version;
    auto setEngineVersion(const Version& version) -> void;

    /// @result importent members: [
    ///   internal_name,
    ///   nozzles,
    ///   profile_version,
    /// ]
    /// other members will be stored internally
    auto checkedPrinterList() const -> const std::list<Printer>&;
    auto getCheckedPrinterList() const -> std::list<Printer>;
    auto setCheckedPrinterList(const std::list<Printer>& printer_list) -> void;
    auto setCheckedPrinterList(std::list<Printer>&& printer_list) -> void;

   public:  // response
    /// @result importent members: [
    ///   engine_version,
    ///   internal_name,
    ///   nozzles,
    ///   profile_version,
    ///   profile_zip_url,
    /// ]
    /// other members will keep the same as the checked printer with the same internal_name
    auto updatablePrinterList() const -> const std::list<Printer>&;
    auto getUpdatablePrinterList() const -> std::list<Printer>;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    Version engine_version_{};
    std::list<Printer> checked_printer_list_{};
    std::list<Printer> updatable_printers_{};
  };





  class CXCLOUD_API LoadUserProfileRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadUserProfileRequest(QObject* parent = nullptr);
    virtual ~LoadUserProfileRequest() = default;

   public:  // response
    auto profile() const -> const Profile&;
    auto getProfile() const -> Profile;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    Profile profile_{};
  };





  class CXCLOUD_API PreUploadProfile : public HttpRequest {
    Q_OBJECT;

   public:
    explicit PreUploadProfile(const QString& printer_internal_name,
                              const QStringList& printer_nozzles,
                              const QString& md5,
                              QObject* parent = nullptr);
    virtual ~PreUploadProfile() = default;

   public:  // response
    auto uploadPolicy() const -> const UploadPolicy&;
    auto getUploadPolicy() const -> UploadPolicy;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    UploadPolicy policy_{};
    QString printer_internal_name_{};
    QStringList printer_nozzles_{};
    QString md5_{};
  };





  class CXCLOUD_API LoadPrinterMaterialRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadPrinterMaterialRequest(QObject* parent = nullptr);
    virtual ~LoadPrinterMaterialRequest() = default;

   public:  // response
    auto materials() -> std::list<PrinterMaterial>&;
    auto materials() const -> const std::list<PrinterMaterial>&;
    auto getMaterials() const -> std::list<PrinterMaterial>;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    std::list<PrinterMaterial> materials_{};
  };





  class CXCLOUD_API LoadMaterialTemplateRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadMaterialTemplateRequest(const Version& engine_version, QObject* parent = nullptr);
    virtual ~LoadMaterialTemplateRequest() = default;

   public:  // request
    auto getEngineVersion() const -> Version;
    auto setEngineVersion(const Version& version) -> void;

   public:  // response
    auto materialTemplates() -> std::list<MaterialTemplate>&;
    auto materialTemplates() const -> const std::list<MaterialTemplate>&;
    auto getMaterialTemplates() const -> std::list<MaterialTemplate>;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    Version engine_version_{};
    std::list<MaterialTemplate> material_templates_{};
  };





  class CXCLOUD_API LoadProcessTemplateRequest : public HttpRequest {
    Q_OBJECT;

   public:
    explicit LoadProcessTemplateRequest(const Version& engine_version, QObject* parent = nullptr);
    virtual ~LoadProcessTemplateRequest() = default;

   public:  // request
    auto getEngineVersion() const -> Version;
    auto setEngineVersion(const Version& version) -> void;

   public:  // response
    auto processTemplates() -> std::list<ProcessTemplate>&;
    auto processTemplates() const -> const std::list<ProcessTemplate>&;
    auto getProcessTemplates() const -> std::list<ProcessTemplate>;

   protected:
    auto getRequestData() const -> QByteArray override;
    auto getUrlTail() const -> QString override;

    auto callWhenSuccessed() -> void override;

   private:
    Version engine_version_{};
    std::list<ProcessTemplate> process_templates_{};
  };

}  // namespace cxcloud

#endif  // CXCLOUD_NET_PROFILE_REQUEST_H_
