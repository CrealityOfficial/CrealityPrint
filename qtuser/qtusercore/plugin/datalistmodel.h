#pragma once

#ifndef QTUSERCORE_PLIGIN_DATALISTMODEL_H_
#define QTUSERCORE_PLIGIN_DATALISTMODEL_H_

#include <deque>
#include <limits>
#include <type_traits>
#include <vector>

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariantMap>

namespace qtuser_qml {

  /// @brief help to control QAbstractListModel's raw data
  /// @note must derived this class
  /// @tparam _Data struct of data item info, must have a *QString* member *uid* as item's unique id
  /// @example cxcloud/model/server_list_model.h
  template<typename _Data>
  class DataListModel : public QAbstractListModel {
    static_assert(std::is_same<decltype(_Data::uid), QString>::value,
                  "*_Data* must have a *QString* member *uid*");

   public:
    using Data = _Data;

    template<typename _T>
    using Container = std::deque<_T>;

    using DataContainer = Container<Data>;

   protected:
    using SuperType = DataListModel<_Data>;

   public:
    explicit DataListModel(QObject* parent = nullptr);
    explicit DataListModel(const DataContainer& data, QObject* parent = nullptr);
    explicit DataListModel(DataContainer&& data, QObject* parent = nullptr);
    virtual ~DataListModel() = default;

   public:
    /// @brief used to get data manually in some special situations, it usually invoke from qml
    /// @param index index of list
    /// @return map of role to data
    Q_INVOKABLE virtual QVariantMap get(int index) const;

   public:  // limit data size
    virtual auto getMaxSize() const -> size_t;
    virtual auto setMaxSize(size_t size) -> void;

   public:  // visible data size for qml
    virtual auto getVaildSize() const -> size_t;
    virtual auto setVaildSize(size_t size) -> void;

   public:  // real data size
    virtual auto isEmpty() const -> bool;
    virtual auto getSize() const -> size_t;

   public:  // model data control
    virtual auto find(const QString& uid) const -> bool;
    virtual auto load(const QString& uid) const -> Data;

    virtual auto pushFront(const Data& data) -> bool;
    virtual auto pushFront(Data&& data) -> bool;

    /// @brief pushFront each data in list,
    ///        and notify the qml engine of all changes in one operation
    /// @return pushFront successed data count
    virtual auto pushFront(const DataContainer& datas) -> size_t;

    virtual auto pushBack(const Data& data) -> bool;
    virtual auto pushBack(Data&& data) -> bool;

    /// @brief pushBack each data in list, and notify the qml engine of all changes in one operation
    /// @return pushBack successed data count
    virtual auto pushBack(const DataContainer& datas) -> size_t;

    virtual auto update(const Data& data) -> bool;
    virtual auto update(Data&& data) -> bool;

    /// @brief update each data in container,
    ///        and notify the qml engine in one operation for each consecutive data group
    /// @return update successed data count
    virtual auto update(const DataContainer& datas) -> size_t;

    /// @brief try pushFront if uid not exists, otherwise try update
    virtual auto emplaceFront(const Data& data) -> void;

    /// @brief try pushFront if uid not exists, otherwise try update
    virtual auto emplaceFront(Data&& data) -> void;

    /// @brief update datas that has exist uid in list together, and pushFront rest of them together
    /// @see auto update(const DataContainer& datas) -> size_t;
    /// @see auto pushFront(const DataContainer& datas) -> size_t;
    virtual auto emplaceFront(const DataContainer& datas) -> void;

    /// @brief try pushBack if uid not exists, otherwise try update
    virtual auto emplaceBack(const Data& data) -> void;

    /// @brief try pushBack if uid not exists, otherwise try update
    virtual auto emplaceBack(Data&& data) -> void;

    /// @brief update datas that has exist uid in list together, and pushBack rest of them together
    /// @see auto update(const DataContainer& datas) -> size_t;
    /// @see auto pushBack(const DataContainer& datas) -> size_t;
    virtual auto emplaceBack(const DataContainer& datas) -> void;

    virtual auto erase(const QString& uid) -> bool;
    virtual auto clear() -> void;

   public:  // raw data control
    virtual auto rawData() const -> const DataContainer&;
    virtual auto rawData() -> DataContainer&;

    virtual auto syncRawData(size_t begin_index, size_t end_index) -> void;
    virtual auto syncRawData(size_t begin_index,
                             size_t end_index,
                             const std::vector<int>& roles) -> void;

   protected:
    auto rowCount(const QModelIndex& parent = QModelIndex{}) const -> int override;

   private:
    size_t max_size_{ (std::numeric_limits<size_t>::max)() };
    size_t vaild_size_{ max_size_ };
    DataContainer datas_{};
  };

}  // namespace qtuser_qml

#endif

#include "qtusercore/plugin/datalistmodel.inl"
