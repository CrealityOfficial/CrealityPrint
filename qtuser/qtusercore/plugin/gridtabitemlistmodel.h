#pragma once

#ifndef QTUSERCORE_PLIGIN_PROFILEGROUPLISTMODEL_H_
#define QTUSERCORE_PLIGIN_PROFILEGROUPLISTMODEL_H_

#include <mutex>
#include <type_traits>

#include "qtusercore/plugin/datalistmodel.h"

namespace qtuser_qml {

  /// @see property *itemModel* in CrealityUI/components/GridTabView.qml
  struct GridTabItem {
    QString button_text{};
    QString button_image{};
    QString button_checked_image{};
    int button_row{ 0 };
    int button_column{ 0 };
    int button_column_span{ 1 };
  };

  /// @brief model of GridTabView
  /// @see CrealityUI/components/GridTabView.qml
  /// @note must derived this class
  /// @tparam _ItemData *_Data* of qtuser_qml::DataListModel, must have a *GridTabItem* member *ui*
  template <typename _ItemData>
  class GridTabItemListModel : public DataListModel<_ItemData> {
    static_assert(std::is_same<decltype(_ItemData::ui), GridTabItem>::value,
                  "*_ItemData* must have a *GridTabItem* member *ui*");

   public:
    using SuperType = DataListModel<_ItemData>;

    enum class Role : int {
      UI_BUTTON_TEXT = Qt::ItemDataRole::UserRole,
      UI_BUTTON_IMAGE,
      UI_BUTTON_CHECKED_IMAGE,
      UI_BUTTON_ROW,
      UI_BUTTON_COLUMN,
      UI_BUTTON_COLUMN_SPAN,
    };

   public:
    using SuperType::SuperType;
    virtual ~GridTabItemListModel() override = default;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   protected:
    /// @brief use for derived class's first role
    static constexpr auto FirstUserRoleIndex() -> int {
      return static_cast<int>(Role::UI_BUTTON_COLUMN_SPAN) + 1;
    };

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
  };

}  // namespace qtuser_qml

#endif  // QTUSERCORE_PLIGIN_PROFILEGROUPLISTMODEL_H_

#include "qtusercore/plugin/gridtabitemlistmodel.inl"
