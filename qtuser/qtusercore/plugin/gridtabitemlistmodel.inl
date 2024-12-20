#pragma once

#ifndef QTUSERCORE_PLIGIN_PROFILEGROUPLISTMODEL_INL_
#define QTUSERCORE_PLIGIN_PROFILEGROUPLISTMODEL_INL_

#include "qtusercore/plugin/gridtabitemlistmodel.h"

namespace qtuser_qml {

  template <typename _ItemData>
  auto GridTabItemListModel<_ItemData>::get(int index) const -> QVariantMap {
    const auto& datas = this->rawData();
    if (datas.size() <= index) {
      return {};
    }

    const auto& data = datas[index];

    return {
      { QByteArrayLiteral("ui_button_text"), QVariant::fromValue(data.ui.button_text) },
      { QByteArrayLiteral("ui_button_image"), QVariant::fromValue(data.ui.button_image) },
      { QByteArrayLiteral("ui_button_checked_image"),
          QVariant::fromValue(data.ui.button_checked_image) },
      { QByteArrayLiteral("ui_button_row"), QVariant::fromValue(data.ui.button_row) },
      { QByteArrayLiteral("ui_button_column"), QVariant::fromValue(data.ui.button_column) },
      { QByteArrayLiteral("ui_button_column_span"),
          QVariant::fromValue(data.ui.button_column_span) },
    };
  }

  template <typename _ItemData>
  auto GridTabItemListModel<_ItemData>::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = this->rawData();
    const auto data_index = index.row();
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    const auto& data = datas[data_index];
    switch (static_cast<Role>(role)) {
      case Role::UI_BUTTON_TEXT: {
        return QVariant::fromValue(data.ui.button_text);
      }
      case Role::UI_BUTTON_IMAGE: {
        return QVariant::fromValue(data.ui.button_image);
      }
      case Role::UI_BUTTON_CHECKED_IMAGE: {
        return QVariant::fromValue(data.ui.button_checked_image);
      }
      case Role::UI_BUTTON_ROW: {
        return QVariant::fromValue(data.ui.button_row);
      }
      case Role::UI_BUTTON_COLUMN: {
        return QVariant::fromValue(data.ui.button_column);
      }
      case Role::UI_BUTTON_COLUMN_SPAN: {
        return QVariant::fromValue(data.ui.button_column_span);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  template <typename _ItemData>
  auto GridTabItemListModel<_ItemData>::roleNames() const -> QHash<int, QByteArray> {
    static const QHash<int, QByteArray> ROLE_NAMES{
      { static_cast<int>(Role::UI_BUTTON_TEXT), QByteArrayLiteral("ui_button_text") },
      { static_cast<int>(Role::UI_BUTTON_IMAGE), QByteArrayLiteral("ui_button_image") },
      { static_cast<int>(Role::UI_BUTTON_CHECKED_IMAGE),
          QByteArrayLiteral("ui_button_checked_image") },
      { static_cast<int>(Role::UI_BUTTON_ROW), QByteArrayLiteral("ui_button_row") },
      { static_cast<int>(Role::UI_BUTTON_COLUMN), QByteArrayLiteral("ui_button_column") },
      { static_cast<int>(Role::UI_BUTTON_COLUMN_SPAN),
          QByteArrayLiteral("ui_button_column_span") },
    };

    return ROLE_NAMES;
  }

}  // namespace qtuser_qml


#endif  // QTUSERCORE_PLIGIN_PROFILEGROUPLISTMODEL_INL_
