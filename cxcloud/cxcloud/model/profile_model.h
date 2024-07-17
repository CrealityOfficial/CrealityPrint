#pragma once

#ifndef CXCLOUD_MODEL_PROFILE_MODEL_H_
#define CXCLOUD_MODEL_PROFILE_MODEL_H_

#include <list>
#include <set>
#include <map>

#include <QtCore/QString>
#include <QtCore/QByteArray>

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"
#include "cxcloud/tool/version.h"

namespace cxcloud {

  struct Profile {
    struct Printer {
      QString uid;  // unique in set, {internal_name}-{nozzles[...]}...
      Version engine_version;
      int series_id;
      int series_type;
      QString internal_name;
      QString external_name;
      QString thumbnail;
      double x_size;
      double y_size;
      double z_size;
      std::list<QString> nozzles;
      std::set<QString> materials;
      QString profile_version;
      QString profile_zip_url;
      QString show_version;
      QString change_log;
    };

    struct Series {
      int id;  // unique in set
      QString name;
      int type;
    };

    std::list<Printer> printer_list{};
    std::list<Series> series_list{};
  };

  struct UploadPolicy {
    QString callback;
    int expire_at;
    QString host;
    QString key;
    QString oss_access_key_id;
    QString policy;
    QString signature;
  };

  struct PrinterMaterial {
    QString id;
    QString brand;
    QString name;
    QString type;
    QString support_diameters;
    int rank;
  };

  struct MaterialTemplate {
    QString name;
    QString type;
    std::map<QString, QString> engine_data;
  };

  struct ProcessTemplate {
    QString name;
    std::map<QString, QString> engine_data;
  };

  auto PrinterProfileToJsonBytes(const Profile& profile) -> QByteArray;
  auto JsonBytesToPrinterProfile(const QByteArray& bytes) -> Profile;
  auto JsonBytesToUserProfile(const QByteArray& bytes) -> Profile;
  auto JsonBytesToUploadPolicy(const QByteArray& bytes) -> UploadPolicy;
  auto JsonBytesToMaterials(const QByteArray& bytes) -> std::list<PrinterMaterial>;
  auto JsonBytesToMaterialTemplates(const QByteArray& bytes) -> std::list<MaterialTemplate>;
  auto JsonBytesToProcessTemplates(const QByteArray& bytes) -> std::list<ProcessTemplate>;

  auto SavePrinterProfile(const Profile& profile, const QString& path) -> bool;
  auto LoadPrinterProfile(Profile& profile, const QString& path) -> bool;
  auto SavePrinterMaterial(const std::list<PrinterMaterial>& materials,
                           const QString& path) -> bool;
  auto SaveMaterialTemplates(const std::list<MaterialTemplate>& templates,
                             const QString& path) -> bool;
  auto SaveProcessTemplates(const std::list<ProcessTemplate>& templates,
                            const QString& path) -> bool;

  struct PrinterUpdateInfo {
    using Printer = Profile::Printer;

    QString uid{};
    Printer local{};
    Printer online{};
    bool updateable{ false };
    bool updating{ false };
  };

  class CXCLOUD_API PrinterUpdateListModel : public qtuser_qml::DataListModel<PrinterUpdateInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;
    virtual ~PrinterUpdateListModel() = default;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,
      DISPLAY_NAME,
      LOCAL_VERSION,
      ONLINE_VERSION,
      CHANGE_LOG,
      UPDATEABLE,
      UPDATING,
    };

   private:

  };

}  // namespace cxcloud

#endif  // CXCLOUD_MODEL_PROFILE_MODEL_H_
