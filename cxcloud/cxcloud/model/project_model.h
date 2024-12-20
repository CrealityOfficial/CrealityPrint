#pragma once

#include <QtCore/QStringList>

#include <qtusercore/plugin/datalistmodel.h>

#include "cxcloud/interface.hpp"

namespace cxcloud {

  struct ProjectInfo {
    QString uid{};
    QString name{};
    QString image{};
    QString model_group_uid{};
    QString printer_name{};
    QString layer_height{};
    QString sparse_infill_density{};
    size_t plate_count{ 0ull };
    size_t print_duraction{ 0ull };
    size_t material_weight{ 0ull };
    size_t creation_timepoint{ 0ull };
    QString author_id{};
    QString author_name{};
    QString author_image{};
    QString suffix{ QStringLiteral("3mf") };
    QString url{};
  };

  class CXCLOUD_API ProjectListModel : public qtuser_qml::DataListModel<ProjectInfo> {
    Q_OBJECT;

   public:
    using SuperType::SuperType;
    explicit ProjectListModel(const QStringList& vaild_suffix_list, QObject* parent = nullptr);
    explicit ProjectListModel(QStringList&& vaild_suffix_list, QObject* parent = nullptr);

    virtual ~ProjectListModel() = default;

   public:
    /// @brief Only vaild suffix will pushBack into list.
    ///        If the list is empty, it means all suffix is vaild.
    auto getVaildSuffixList() const -> QStringList;
    auto setVaildSuffixList(const QStringList& vaild_suffix_list) -> void;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   protected:
    auto pushBack(const Data& data) -> bool override;
    auto pushBack(Data&& data) -> bool override;

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,
      NAME,
      IMAGE,
      MODEL_GROUP_UID,
      PRINTER_NAME,
      LAYER_HEIGHT,
      SPARSE_INFILL_DENSITY,
      PLATE_COUNT,
      PRINT_DURACTION,
      MATERIAL_WEIGHT,
      AUTHOR_ID,
      AUTHOR_NAME,
      AUTHOR_IMAGE,
      CREATION_TIMEPOINT,
      SUFFIX,
      URL,
    };

    QStringList vaild_suffix_list_;
  };

}  // namespace cxcloud
