#pragma once

#ifndef CXCLOUD_MODEL_BASE_MODEL_H_
#define CXCLOUD_MODEL_BASE_MODEL_H_

#include <deque>
#include <limits>

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariantMap>

#include "cxcloud/define.hpp"
#include "cxcloud/interface.hpp"
#include "cxcloud/net/http_request.h"

namespace cxcloud {

/// @brief help to control QAbstractListModel's raw data
/// @note must derived this class
/// @tparam _Data struct of data item info, must have a QString member `uid` as item's unique id
template<typename _Data>
class CXCLOUD_API DataListModel : public QAbstractListModel {
public:
  using Data = _Data;

  template<typename _T>
  using Container = std::deque<_T>;

  using DataContainer = Container<Data>;

protected:
  using SuperType = DataListModel<_Data>;

public:
  explicit DataListModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

  explicit DataListModel(const DataContainer& data, QObject* parent = nullptr)
      : QAbstractListModel(parent), datas_(data) {}

  explicit DataListModel(DataContainer&& data, QObject* parent = nullptr)
      : QAbstractListModel(parent), datas_(std::move(data)) {}

  virtual ~DataListModel() = default;

public:
  /// @brief used to get data manually in some special situations, it usually invoke from qml
  /// @param index index of list
  /// @return map of role to data
  Q_INVOKABLE virtual QVariantMap get(int index) const { return {}; }

public:  // limit data size
  virtual auto getMaxSize() const -> size_t { return max_size_; }

  virtual auto setMaxSize(size_t size) -> void {
    max_size_ = size;
    if (datas_.size() > max_size_) {
      beginRemoveRows(QModelIndex{}, max_size_, datas_.size() - 1);
      datas_.erase(datas_.cbegin() + datas_.size(), datas_.cend());
      endRemoveRows();
    }
  }

public:  // visible data size for qml
  virtual auto getVaildSize() const -> size_t {
    return std::min<size_t>(vaild_size_, datas_.size());
  };

  virtual auto setVaildSize(size_t size) -> void {
    auto const old_size = std::min<size_t>(vaild_size_, datas_.size());
    auto const new_size = std::min<size_t>(size, datas_.size());

    auto const old_max_index = std::max<size_t>(0, old_size - 1);
    auto const new_max_index = std::max<size_t>(0, new_size - 1);

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

public:  // real data size
  virtual auto isEmpty() const -> bool { return datas_.empty(); }

  virtual auto getSize() const -> size_t { return datas_.size(); }

public:  // model data control
  virtual auto find(const QString& uid) const -> bool {
    for (const auto& data : datas_) {
      if (data.uid != uid) {
        continue;
      }

      return true;
    }

    return false;
  }

  virtual auto load(const QString& uid) const -> Data {
    for (const auto& data : datas_) {
      if (data.uid != uid) {
        continue;
      }

      return data;
    }

    return {};
  }

  virtual auto pushFront(const Data& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    beginInsertRows(QModelIndex{}, 0, 0);
    datas_.emplace_front(data);
    endInsertRows();

    return true;
  }

  virtual auto pushFront(Data&& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    beginInsertRows(QModelIndex{}, 0, 0);
    datas_.emplace_front(std::move(data));
    endInsertRows();

    return true;
  }

  /// @brief pushFront each data in list, and notify the qml engine of all changes in one operation
  /// @return pushFront successed data count
  virtual auto pushFront(const DataContainer& datas) -> size_t {
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

  virtual auto pushBack(const Data& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    const auto index = std::distance(datas_.cbegin(), datas_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    datas_.emplace_back(data);
    endInsertRows();

    return true;
  }

  virtual auto pushBack(Data&& data) -> bool {
    if (find(data.uid) || datas_.size() >= max_size_) {
      return false;
    }

    const auto index = std::distance(datas_.cbegin(), datas_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    datas_.emplace_back(std::move(data));
    endInsertRows();

    return true;
  }

  /// @brief pushBack each data in list, and notify the qml engine of all changes in one operation
  /// @return pushBack successed data count
  virtual auto pushBack(const DataContainer& datas) -> size_t {
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

  virtual auto update(const Data& data) -> bool {
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

  virtual auto update(Data&& data) -> bool {
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

  /// @brief update each data in container,
  ///        and notify the qml engine in one operation for each consecutive data group
  /// @return update successed data count
  virtual auto update(const DataContainer& datas) -> size_t {
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

  /// @brief try pushFront if uid not exists, otherwise try update
  virtual auto emplaceFront(const Data& data) -> void {
    if (find(data.uid)) {
      update(data);
    } else {
      pushFront(data);
    }
  }

  /// @brief try pushFront if uid not exists, otherwise try update
  virtual auto emplaceFront(Data&& data) -> void {
    if (find(data.uid)) {
      update(std::move(data));
    } else {
      pushFront(std::move(data));
    }
  }

  /// @brief update datas that has exist uid in list together, and pushFront rest of them together
  /// @see auto update(const DataContainer& datas) -> size_t;
  /// @see auto pushFront(const DataContainer& datas) -> size_t;
  virtual auto emplaceFront(const DataContainer& datas) -> void {
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

  /// @brief try pushBack if uid not exists, otherwise try update
  virtual auto emplaceBack(const Data& data) -> void {
    if (find(data.uid)) {
      update(data);
    } else {
      pushBack(data);
    }
  }

  /// @brief try pushBack if uid not exists, otherwise try update
  virtual auto emplaceBack(Data&& data) -> void {
    if (find(data.uid)) {
      update(std::move(data));
    } else {
      pushBack(std::move(data));
    }
  }

  /// @brief update datas that has exist uid in list together, and pushBack rest of them together
  /// @see auto update(const DataContainer& datas) -> size_t;
  /// @see auto pushBack(const DataContainer& datas) -> size_t;
  virtual auto emplaceBack(const DataContainer& datas) -> void {
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

  virtual auto erase(const QString& uid) -> bool {
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

  virtual auto clear() -> void {
    beginResetModel();
    datas_.clear();
    endResetModel();
  }

public:  // raw data control
  virtual auto rawData() const -> const DataContainer& {
    return datas_;
  }

  virtual auto rawData() -> DataContainer& {
    return datas_;
  }

  virtual auto syncRawData(size_t begin_index, size_t end_index) -> void {
    Q_EMIT dataChanged(createIndex(begin_index, 0), createIndex(end_index, 0));
  }

protected:
  int rowCount(const QModelIndex& parent = QModelIndex{}) const override {
    std::ignore = parent;
    return static_cast<int>(getVaildSize());
  }

private:
  size_t max_size_{ (std::numeric_limits<size_t>::max)() };
  size_t vaild_size_{ max_size_ };
  DataContainer datas_{};
};





struct ServerInfo {
  QString uid{};  // string of ServerType
  QString name{};
};

class CXCLOUD_API ServerListModel : public DataListModel<ServerInfo> {
  Q_OBJECT;

public:
  using SuperType::SuperType;

  virtual ~ServerListModel() = default;

public:
  Q_INVOKABLE QVariantMap get(int index) const override;

protected:
  QVariant data(const QModelIndex& index, int role = Qt::ItemDataRole::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  enum class DataRole : int {
    UID = Qt::ItemDataRole::UserRole + 1,
    INDEX,
    NAME,
  };
};

}  // namespace cxcloud

#endif  // CXCLOUD_MODEL_BASE_MODEL_H_
