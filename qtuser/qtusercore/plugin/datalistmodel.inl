#pragma once

#ifndef QTUSERCORE_PLIGIN_DATALISTMODEL_INL_
#define QTUSERCORE_PLIGIN_DATALISTMODEL_INL_

#include "qtusercore/plugin/datalistmodel.h"

namespace qtuser_qml {

  template<typename _Data>
  DataListModel<_Data>::DataListModel(QObject* parent) : QAbstractListModel{ parent } {}

  template<typename _Data>
  DataListModel<_Data>::DataListModel(const DataContainer& data, QObject* parent)
      : QAbstractListModel{ parent }, datas_{ data } {}

  template<typename _Data>
  DataListModel<_Data>::DataListModel(DataContainer&& data, QObject* parent)
      : QAbstractListModel{ parent }, datas_{ std::move(data) } {}

  template<typename _Data>
  auto DataListModel<_Data>::get(int index) const -> QVariantMap {
    return {};
  }

  template<typename _Data>
  auto DataListModel<_Data>::getMaxSize() const -> size_t {
    return max_size_;
  }

  template<typename _Data>
  auto DataListModel<_Data>::setMaxSize(size_t size) -> void {
    max_size_ = size;
    if (datas_.size() > max_size_) {
      beginRemoveRows(QModelIndex{}, max_size_, datas_.size() - 1);
      datas_.erase(datas_.cbegin() + datas_.size(), datas_.cend());
      endRemoveRows();
    }
  }

  template<typename _Data>
  auto DataListModel<_Data>::getVaildSize() const -> size_t {
    return std::min<size_t>(vaild_size_, datas_.size());
  }

  template<typename _Data>
  auto DataListModel<_Data>::setVaildSize(size_t size) -> void {
    const auto old_size = std::min<size_t>(vaild_size_, datas_.size());
    const auto new_size = std::min<size_t>(size, datas_.size());

    const auto old_max_index = std::max<size_t>(0, old_size - 1);
    const auto new_max_index = std::max<size_t>(0, new_size - 1);

    if (new_size < old_size) {
      beginRemoveRows(QModelIndex{}, new_max_index, old_max_index);
      vaild_size_ = size;
      endRemoveRows();
    } else if (new_size > old_size) {
      beginInsertRows(QModelIndex{}, old_max_index, new_max_index);
      vaild_size_ = size;
      endInsertRows();
    }
  }

  template<typename _Data>
  auto DataListModel<_Data>::isEmpty() const -> bool {
    return datas_.empty();
  }

  template<typename _Data>
  auto DataListModel<_Data>::getSize() const -> size_t {
    return datas_.size();
  }

  template<typename _Data>
  auto DataListModel<_Data>::find(const QString& uid) const -> bool {
    for (const auto& data : datas_) {
      if (data.uid != uid) {
        continue;
      }

      return true;
    }

    return false;
  }

  template<typename _Data>
  auto DataListModel<_Data>::load(const QString& uid) const -> Data {
    for (const auto& data : datas_) {
      if (data.uid != uid) {
        continue;
      }

      return data;
    }

    return {};
  }

  template<typename _Data>
  auto DataListModel<_Data>::pushFront(const Data& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    beginInsertRows(QModelIndex{}, 0, 0);
    datas_.emplace_front(data);
    endInsertRows();

    return true;
  }

  template<typename _Data>
  auto DataListModel<_Data>::pushFront(Data&& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    beginInsertRows(QModelIndex{}, 0, 0);
    datas_.emplace_front(std::move(data));
    endInsertRows();

    return true;
  }

  template<typename _Data>
  auto DataListModel<_Data>::pushFront(const DataContainer& datas) -> size_t {
    if (datas.empty()) {
      return 0;
    }

    DataContainer vaild_datas;
    auto left_size = max_size_ - datas_.size();
    for (const auto& data : datas) {
      if (find(data.uid)) { continue; }
      vaild_datas.emplace_back(data);
      if (--left_size == 0) { break; }
    }

    if (vaild_datas.empty()) {
      return 0;
    }

    const auto begin_index = 0;
    const auto end_index = begin_index + vaild_datas.size() - 1;
    beginInsertRows(QModelIndex{}, begin_index, end_index);
    datas_.insert(datas_.begin(), vaild_datas.begin(), vaild_datas.end());
    endInsertRows();

    return vaild_datas.size();
  }

  template<typename _Data>
  auto DataListModel<_Data>::pushBack(const Data& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    const auto index = std::distance(datas_.cbegin(), datas_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    datas_.emplace_back(data);
    endInsertRows();

    return true;
  }

  template<typename _Data>
  auto DataListModel<_Data>::pushBack(Data&& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    const auto index = std::distance(datas_.cbegin(), datas_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    datas_.emplace_back(std::move(data));
    endInsertRows();

    return true;
  }

  template<typename _Data>
  auto DataListModel<_Data>::pushBack(const DataContainer& datas) -> size_t {
    if (datas.empty()) {
      return 0;
    }

    DataContainer vaild_datas;
    auto left_size = max_size_ - datas_.size();
    for (const auto& data : datas) {
      if (find(data.uid)) { continue; }
      vaild_datas.emplace_back(data);
      if (--left_size == 0) { break; }
    }

    if (vaild_datas.empty()) {
      return 0;
    }

    const auto begin_index = std::distance(datas_.cbegin(), datas_.cend());
    const auto end_index = begin_index + vaild_datas.size() - 1;
    beginInsertRows(QModelIndex{}, begin_index, end_index);
    datas_.insert(datas_.end(), vaild_datas.begin(), vaild_datas.end());
    endInsertRows();

    return vaild_datas.size();
  }

  template<typename _Data>
  auto DataListModel<_Data>::update(const Data& data) -> bool {
    for (auto iter = datas_.begin(); iter != datas_.end(); ++iter) {
      if (iter->uid != data.uid) {
        continue;
      }

      *iter = data;

      const auto index = std::distance(datas_.begin(), iter);
      const auto model_index = createIndex(index, 0);
      Q_EMIT dataChanged(model_index, model_index);

      return true;
    }

    return false;
  }

  template<typename _Data>
  auto DataListModel<_Data>::update(Data&& data) -> bool {
    for (auto iter = datas_.begin(); iter != datas_.end(); ++iter) {
      if (iter->uid != data.uid) {
        continue;
      }

      *iter = std::move(data);

      const auto index = std::distance(datas_.begin(), iter);
      const auto model_index = createIndex(index, 0);
      Q_EMIT dataChanged(model_index, model_index);

      return true;
    }

    return false;
  }

  template<typename _Data>
  auto DataListModel<_Data>::update(const DataContainer& datas) -> size_t {
    static const QModelIndex EMPTY_INDEX;
    size_t count = 0;

    auto begin_index = EMPTY_INDEX;
    auto end_index = EMPTY_INDEX;
    for (const auto& data : datas) {
      for (auto iter = datas_.begin(); iter != datas_.end(); ++iter) {
        if (iter->uid != data.uid) {
          if (begin_index == EMPTY_INDEX) {
            continue;
          }

          Q_EMIT dataChanged(begin_index, end_index);
          begin_index = EMPTY_INDEX;
          end_index = EMPTY_INDEX;
        }

        *iter = data;
        ++count;

        const auto index = std::distance(datas_.begin(), iter);
        const auto model_index = createIndex(index, 0);
        end_index = model_index;

        if (iter == --(datas_.end())) {
          Q_EMIT dataChanged(begin_index, end_index);
        }
      }
    }

    return count;
  }

  template<typename _Data>
  auto DataListModel<_Data>::emplaceFront(const Data& data) -> void {
    if (find(data.uid)) {
      update(data);
    } else {
      pushFront(data);
    }
  }

  template<typename _Data>
  auto DataListModel<_Data>::emplaceFront(Data&& data) -> void {
    if (find(data.uid)) {
      update(std::move(data));
    } else {
      pushFront(std::move(data));
    }
  }

  template<typename _Data>
  auto DataListModel<_Data>::emplaceFront(const DataContainer& datas) -> void {
    DataContainer update_datas;
    DataContainer append_datas;

    for (const auto& data : datas) {
      if (find(data.uid)) {
        update_datas.emplace_back(data);
      } else {
        append_datas.emplace_back(data);
      }
    }

    update(update_datas);
    pushFront(append_datas);
  }

  template<typename _Data>
  auto DataListModel<_Data>::emplaceBack(const Data& data) -> void {
    if (find(data.uid)) {
      update(data);
    } else {
      pushBack(data);
    }
  }

  template<typename _Data>
  auto DataListModel<_Data>::emplaceBack(Data&& data) -> void {
    if (find(data.uid)) {
      update(std::move(data));
    } else {
      pushBack(std::move(data));
    }
  }

  template<typename _Data>
  auto DataListModel<_Data>::emplaceBack(const DataContainer& datas) -> void {
    DataContainer update_datas;
    DataContainer append_datas;

    for (const auto& data : datas) {
      if (find(data.uid)) {
        update_datas.emplace_back(data);
      } else {
        append_datas.emplace_back(data);
      }
    }

    update(update_datas);
    pushBack(append_datas);
  }

  template<typename _Data>
  auto DataListModel<_Data>::erase(const QString& uid) -> bool {
    for (auto iter = datas_.cbegin(); iter != datas_.cend(); ++iter) {
      if (iter->uid != uid) {
        continue;
      }

      const auto index = std::distance(datas_.cbegin(), iter);
      beginRemoveRows(QModelIndex{}, index, index);
      datas_.erase(iter);
      endRemoveRows();

      return true;
    }

    return false;
  }

  template<typename _Data>
  auto DataListModel<_Data>::clear() -> void {
    beginResetModel();
    datas_.clear();
    endResetModel();
  }

  template<typename _Data>
  auto DataListModel<_Data>::rawData() const -> const DataContainer& {
    return datas_;
  }

  template<typename _Data>
  auto DataListModel<_Data>::rawData() -> DataContainer& {
    return datas_;
  }

  template<typename _Data>
  auto DataListModel<_Data>::syncRawData(size_t begin_index, size_t end_index) -> void {
    Q_EMIT dataChanged(createIndex(begin_index, 0), createIndex(end_index, 0));
  }

  template<typename _Data>
  auto DataListModel<_Data>::syncRawData(size_t begin_index,
                                         size_t end_index,
                                         const std::vector<int>& roles) -> void {
    Q_EMIT dataChanged(createIndex(begin_index, 0),
                       createIndex(end_index, 0),
                       QVector<int>{ roles.cbegin(), roles.cend() });
  }

  template<typename _Data>
  auto DataListModel<_Data>::rowCount(const QModelIndex& parent) const -> int {
    std::ignore = parent;
    return static_cast<int>(getVaildSize());
  }

}  // namespace qtuser_qml

#endif
