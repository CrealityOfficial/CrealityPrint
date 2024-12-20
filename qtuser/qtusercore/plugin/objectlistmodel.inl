#pragma once

#include "qtusercore/plugin/objectlistmodel.h"

namespace qtuser_qml {

  template <typename _Object>
  ObjectListModel<_Object>::ObjectListModel(QObject* parent) : QAbstractListModel{ parent } {}

  template <typename _Object>
  ObjectListModel<_Object>::ObjectListModel(const ObjectPointerContainer& objects, QObject* parent)
      : QAbstractListModel{ parent }, objects_{ objects } {}

  template <typename _Object>
  ObjectListModel<_Object>::ObjectListModel(ObjectPointerContainer&& objects, QObject* parent)
      : QAbstractListModel{ parent }, objects_{ std::move(objects) } {}

  template <typename _Object>
  auto ObjectListModel<_Object>::getSize() const -> size_t {
    return objects_.size();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::isEmpty() const -> bool {
    return getSize() == 0;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getFront() const -> ObjectPointer {
    return objects_.front();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::pushFront(ObjectPointer object) -> void {
    beginInsertRows(QModelIndex{}, 0, 0);
    // objects_.push_front(object);
    objects_.insert(objects_.cbegin(), object);
    endInsertRows();
    __sendCountChangdIfNeed();
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplaceFront(_Types&&... values) -> void {
    const auto index = std::distance(objects_.cbegin(), objects_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    // objects_.emplace_front(std::forward<_Types>(values)...);
    objects_.emplace(objects_.cbegin(), std::forward<_Types>(values)...);
    endInsertRows();
    __sendCountChangdIfNeed();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::popFront() -> ObjectPointer {
    auto front = objects_.front();
    beginRemoveRows(QModelIndex{}, 0, 0);
    // objects_.pop_front();
    objects_.erase(objects_.cbegin());
    endRemoveRows();
    __sendCountChangdIfNeed();
    return front;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getBack() const -> ObjectPointer {
    return objects_.back();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::pushBack(ObjectPointer object) -> void {
    const auto index = std::distance(objects_.cbegin(), objects_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    objects_.push_back(object);
    endInsertRows();
    __sendCountChangdIfNeed();
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplaceBack(_Types&&... values) -> void {
    const auto index = std::distance(objects_.cbegin(), objects_.cend());
    beginInsertRows(QModelIndex{}, index, index);
    objects_.emplace_back(std::forward<_Types>(values)...);
    endInsertRows();
    __sendCountChangdIfNeed();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::popBack() -> ObjectPointer {
    auto       back  = objects_.back();
    const auto index = std::distance(objects_.cbegin(), objects_.cend());
    beginRemoveRows(QModelIndex{}, index, index);
    objects_.pop_back();
    endRemoveRows();
    __sendCountChangdIfNeed();
    return back;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getAt(size_t index) const -> ObjectPointer {
    return objects_.at(index);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insertAt(size_t index, ObjectPointer object) -> ConstIterator {
    beginInsertRows(QModelIndex{}, index, index);
    auto result = objects_.insert(objects_.cbegin() + index, object);
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insertAt(size_t                        index,
                                          const ObjectPointerContainer& objects) -> ConstIterator {
    const auto last_index = index + objects.size() - 1;
    objects_.reserve(objects_.size() + objects.size());
    beginInsertRows(QModelIndex{}, index, last_index);
    auto result = objects_.insert(objects_.cbegin() + index, objects.cbegin(), objects.cend());
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insertAt(size_t                   index,
                                          ObjectPointerContainer&& objects) -> ConstIterator {
    const auto last_index = index + objects.size() - 1;
    objects_.reserve(objects_.size() + objects.size());
    beginInsertRows(QModelIndex{}, index, last_index);
    auto result = objects_.insert(objects_.cbegin() + index,
                                  std::make_move_iterator(objects.begin()),
                                  std::make_move_iterator(objects.end()));
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insertAt(ConstIterator iter,
                                          ObjectPointer object) -> ConstIterator {
    const auto index = std::distance(objects_.cbegin(), iter);
    beginInsertRows(QModelIndex{}, index, index);
    auto result = objects_.insert(iter, object);
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insertAt(ConstIterator                 iter,
                                          const ObjectPointerContainer& objects) -> ConstIterator {
    const auto first_index = std::distance(objects_.cbegin(), iter);
    const auto last_index  = first_index + objects.size() - 1;
    objects_.reserve(objects_.size() + objects.size());
    beginInsertRows(QModelIndex{}, first_index, last_index);
    auto result = objects_.insert(iter, objects.cbegin(), objects.cend());
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insertAt(ConstIterator            iter,
                                          ObjectPointerContainer&& objects) -> ConstIterator {
    const auto first_index = std::distance(objects_.cbegin(), iter);
    const auto last_index  = first_index + objects.size() - 1;
    objects_.reserve(objects_.size() + objects.size());
    beginInsertRows(QModelIndex{}, first_index, last_index);
    auto result = objects_.insert(iter,
                                  std::make_move_iterator(objects.begin()),
                                  std::make_move_iterator(objects.end()));
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplaceAt(size_t index, _Types&&... values) -> ConstIterator {
    beginInsertRows(QModelIndex{}, index, index);
    auto result = objects_.emplace(objects_.cbegin() + index, std::forward<_Types>(values)...);
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplaceAt(ConstIterator iter,
                                           _Types&&...   values) -> ConstIterator {
    const auto index = std::distance(objects_.cbegin(), iter);
    beginInsertRows(QModelIndex{}, index, index);
    auto result = objects_.emplace(iter, std::forward<_Types>(values)...);
    endInsertRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::eraseAt(size_t index) -> ConstIterator {
    beginRemoveRows(QModelIndex{}, index, index);
    auto result = objects_.erase(objects_.cbegin() + index);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::eraseAt(size_t first_index, size_t last_index) -> ConstIterator {
    const auto cbegin     = objects_.cbegin();
    const auto first_iter = cbegin + first_index;
    const auto last_iter  = cbegin + (last_index + 1);
    beginRemoveRows(QModelIndex{}, first_index, last_index);
    auto result = objects_.erase(first_iter, last_iter);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::eraseAt(ConstIterator iter) -> ConstIterator {
    const auto index = std::distance(objects_.cbegin(), iter);
    beginRemoveRows(QModelIndex{}, index, index);
    auto result = objects_.erase(iter);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::eraseAt(ConstIterator first_iter,
                                         ConstIterator last_iter) -> ConstIterator {
    const auto cbegin      = objects_.cbegin();
    const auto first_index = std::distance(cbegin, first_iter);
    const auto last_index  = std::distance(cbegin, last_iter);
    beginRemoveRows(QModelIndex{}, first_index, last_index);
    auto result = objects_.erase(first_iter, last_iter);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::takeAt(size_t index) -> ObjectPointer {
    beginRemoveRows(QModelIndex{}, index, index);
    auto result = std::move(objects_.at(index));
    objects_.erase(objects_.cbegin() + index);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::takeAt(size_t first_index,
                                        size_t last_index) -> ObjectPointerContainer {
    const auto cbegin     = objects_.cbegin();
    const auto first_iter = cbegin + first_index;
    const auto last_iter  = cbegin + (last_index + 1);
    beginRemoveRows(QModelIndex{}, first_index, last_index);
    auto result = ObjectPointerContainer{ std::make_move_iterator(first_iter),
                                          std::make_move_iterator(last_iter) };
    objects_.erase(first_iter, last_iter);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::takeAt(ConstIterator iter) -> ObjectPointer {
    const auto index  = std::distance(getConstBegin(), iter);
    beginRemoveRows(QModelIndex{}, index, index);
    auto result = std::move(objects_.at(index));
    objects_.erase(iter);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::takeAt(ConstIterator first_iter,
                                        ConstIterator last_iter) -> ObjectPointerContainer {
    const auto cbegin      = objects_.cbegin();
    const auto first_index = std::distance(cbegin, first_iter);
    const auto last_index  = std::distance(cbegin, last_iter);
    beginRemoveRows(QModelIndex{}, first_index, last_index);
    auto result = ObjectPointerContainer{ std::make_move_iterator(first_iter),
                                          std::make_move_iterator(last_iter) };
    objects_.erase(first_iter, last_iter);
    endRemoveRows();
    __sendCountChangdIfNeed();
    return result;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getConstBegin() const -> ConstIterator {
    return objects_.cbegin();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getConstEnd() const -> ConstIterator {
    return objects_.cend();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getConstReverseBegin() const -> ConstReverseIterator {
    return objects_.crbegin();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getConstReverseEnd() const -> ConstReverseIterator {
    return objects_.crend();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getRoleIndex() const -> int {
    return Qt::ItemDataRole::UserRole;
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::getRoleName() const -> QByteArray {
    return QByteArrayLiteral("object");
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::roleNames() const -> QHash<int, QByteArray> {
    return {
      { getRoleIndex(), getRoleName() },
    };
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::rowCount(const QModelIndex& parent) const -> int {
    return getSize();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::data(const QModelIndex& index, int role) const -> QVariant {
    const auto object_index = index.row();
    if (object_index < 0 || object_index >= getSize()) {
      return { QVariant::Type::Invalid };
    }

    if (role != getRoleIndex()) {
      return { QVariant::Type::Invalid };
    }

    auto object = objects_.at(object_index);
    auto* object_ptr = object.data();
    auto object_var = QVariant::fromValue<QObject*>(object_ptr);
    return QVariant::fromValue<QObject*>(objects_.at(object_index).data());
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::__sendCountChangdIfNeed() const -> void {}





  // stl style

  template <typename _Object>
  auto ObjectListModel<_Object>::size() const -> size_t {
    return getSize();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::empty() const -> bool {
    return isEmpty();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::front() const -> ObjectPointer {
    return getFront();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::push_front(ObjectPointer object) -> void {
    pushFront(object);
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplace_front(_Types&&... values) -> void {
    emplaceFront(std::forward<_Types>(values)...);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::pop_front() -> void {
    popFront();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::back() const -> ObjectPointer {
    return getBack();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::push_back(ObjectPointer object) -> void {
    pushBack(object);
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplace_back(_Types&&... values) -> void {
    emplaceBack(std::forward<_Types>(values)...);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::pop_back() -> void {
    popBack();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::at(size_t index) const -> ObjectPointer {
    return getAt(index);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insert(const_iterator iter,
                                        ObjectPointer  object) -> const_iterator {
    return insertAt(iter, object);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insert(const_iterator           iter,
                                        ObjectPointerContainer&& objects) -> const_iterator {
    return insertAt(iter, std::move(objects));
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::insert(const_iterator                iter,
                                        const ObjectPointerContainer& objects) -> const_iterator {
    return insertAt(iter, objects);
  }

  template <typename _Object>
  template <typename... _Types>
  auto ObjectListModel<_Object>::emplace(const_iterator iter,
                                         _Types&&...    values) -> const_iterator {
    return emplaceAt(iter, std::forward<_Types>(values)...);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::erase(const_iterator iter) -> const_iterator {
    return eraseAt(iter);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::erase(const_iterator first_iter,
                                       const_iterator last_iter) -> const_iterator {
    return eraseAt(first_iter, last_iter);
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::cbegin() const -> const_iterator {
    return getConstBegin();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::cend() const -> const_iterator {
    return getConstEnd();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::crbegin() const -> const_reverse_iterator {
    return getConstReverseBegin();
  }

  template <typename _Object>
  auto ObjectListModel<_Object>::crend() const -> const_reverse_iterator {
    return getConstReverseEnd();
  }

}  // namespace qtuser_qml
