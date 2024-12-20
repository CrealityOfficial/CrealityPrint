#pragma once

#include <type_traits>
#include <vector>

#include <QtCore/QAbstractListModel>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>

/// If you want to add some js/qml features to your derived class of qtuser_qml::ObjectListModel
/// to make it easier to use in qml, add this marco to the definition of the derived class
/// just like the Q_OBJECT macro.
/// It provides the following features:
/// ```qml
///   function toArray(): Array<var>
///   function get(index: number): var
///   readonly property int count
/// ```
#define QTUSER_QML_OBJECT_LIST_MODEL\
   public:\
    Q_INVOKABLE QVariantList toArray() const {\
      QVariantList array{};\
      array.reserve(size());\
      std::transform(cbegin(), cend(), array.begin(), [](auto&& item) {\
        return QVariant::fromValue<QObject*>(item.data());\
      });\
      return array;\
    }\
    Q_INVOKABLE QVariant get(int index) const {\
      if (index < 0 || static_cast<size_t>(index) >= size()) {\
        return QVariant::fromValue<QObject*>(nullptr);\
      }\
      return QVariant::fromValue<QObject*>(at(static_cast<size_t>(index)).data());\
    }\
    auto getCount() const -> int { return size(); }\
    Q_SIGNAL void countChanged() const;\
    Q_PROPERTY(int count READ getCount NOTIFY countChanged);\
   protected:\
    auto __sendCountChangdIfNeed() const -> void override { countChanged(); }\
   private:

namespace qtuser_qml {

  /// @brief Qt object pointer ListModel, which can work like a Qt object pointer list.
  /// @note Derived this class if you want to use any functionality in the Qt MetaObject system.
  /// @tparam _Data type of Qt object, must be QObject or derived from it.
  template <typename _Object>
  class ObjectListModel : public QAbstractListModel {
    static_assert(std::is_same<QObject, _Object>::value || std::is_base_of<QObject, _Object>::value,
                  "*_Objcet* must be a *QObject*");

    ObjectListModel(ObjectListModel&&)                         = delete;
    ObjectListModel(const ObjectListModel&)                    = delete;
    auto operator=(ObjectListModel&&) -> ObjectListModel&      = delete;
    auto operator=(const ObjectListModel&) -> ObjectListModel& = delete;

   public:
    using Object        = _Object;
    using ObjectPointer = QPointer<Object>;

    template <typename _T>
    using Container              = std::vector<_T>;
    using ObjectPointerContainer = Container<ObjectPointer>;

    using ConstIterator        = typename ObjectPointerContainer::const_iterator;
    using ConstReverseIterator = typename ObjectPointerContainer::const_reverse_iterator;

   public:  // stl style
    using const_iterator         = ConstIterator;
    using const_reverse_iterator = ConstReverseIterator;

   public:
    explicit ObjectListModel(QObject* parent = nullptr);
    explicit ObjectListModel(ObjectPointerContainer&& objects, QObject* parent = nullptr);
    explicit ObjectListModel(const ObjectPointerContainer& objects, QObject* parent = nullptr);
    virtual ~ObjectListModel() = default;

   public:
    auto getSize() const -> size_t;
    auto isEmpty() const -> bool;

    auto getFront() const -> ObjectPointer;
    auto pushFront(ObjectPointer object) -> void;
    template <typename... _Types>
    auto emplaceFront(_Types&&... values) -> void;
    auto popFront() -> ObjectPointer;

    auto getBack() const -> ObjectPointer;
    auto pushBack(ObjectPointer object) -> void;
    template <typename... _Types>
    auto emplaceBack(_Types&&... values) -> void;
    auto popBack() -> ObjectPointer;

    auto getAt(size_t index) const -> ObjectPointer;
    auto insertAt(size_t index, ObjectPointer object) -> ConstIterator;
    auto insertAt(size_t index, const ObjectPointerContainer& objects) -> ConstIterator;
    auto insertAt(size_t index, ObjectPointerContainer&& objects) -> ConstIterator;
    auto insertAt(ConstIterator iter, ObjectPointer object) -> ConstIterator;
    auto insertAt(ConstIterator iter, const ObjectPointerContainer& objects) -> ConstIterator;
    auto insertAt(ConstIterator iter, ObjectPointerContainer&& objects) -> ConstIterator;
    template <typename... _Types>
    auto emplaceAt(size_t index, _Types&&... values) -> ConstIterator;
    template <typename... _Types>
    auto emplaceAt(ConstIterator iter, _Types&&... values) -> ConstIterator;
    auto eraseAt(size_t index) -> ConstIterator;
    auto eraseAt(size_t first_index, size_t last_index) -> ConstIterator;
    auto eraseAt(ConstIterator iter) -> ConstIterator;
    auto eraseAt(ConstIterator first_iter, ConstIterator last_iter) -> ConstIterator;
    auto takeAt(size_t index) -> ObjectPointer;
    auto takeAt(size_t first_index, size_t last_index) -> ObjectPointerContainer;
    auto takeAt(ConstIterator iter) -> ObjectPointer;
    auto takeAt(ConstIterator first_iter, ConstIterator last_iter) -> ObjectPointerContainer;

    auto getConstBegin() const -> ConstIterator;
    auto getConstEnd() const -> ConstIterator;
    auto getConstReverseBegin() const -> ConstReverseIterator;
    auto getConstReverseEnd() const -> ConstReverseIterator;

   public:  // stl style
    auto size() const -> size_t;
    auto empty() const -> bool;

    auto front() const -> ObjectPointer;
    auto push_front(ObjectPointer object) -> void;
    template <typename... _Types>
    auto emplace_front(_Types&&... values) -> void;
    auto pop_front() -> void;

    auto back() const -> ObjectPointer;
    auto push_back(ObjectPointer object) -> void;
    template <typename... _Types>
    auto emplace_back(_Types&&... values) -> void;
    auto pop_back() -> void;

    auto at(size_t index) const -> ObjectPointer;
    auto insert(const_iterator iter, ObjectPointer object) -> const_iterator;
    auto insert(const_iterator iter, const ObjectPointerContainer& objects) -> const_iterator;
    auto insert(const_iterator iter, ObjectPointerContainer&& objects) -> const_iterator;
    template <typename... _Types>
    auto emplace(const_iterator iter, _Types&&... values) -> const_iterator;
    auto erase(const_iterator iter) -> const_iterator;
    auto erase(const_iterator first_iter, const_iterator last_iter) -> const_iterator;

    auto cbegin() const -> const_iterator;
    auto cend() const -> const_iterator;
    auto crbegin() const -> const_reverse_iterator;
    auto crend() const -> const_reverse_iterator;

   protected:
    virtual auto getRoleIndex() const -> int;
    virtual auto getRoleName() const -> QByteArray;

   protected:
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto rowCount(const QModelIndex& parent = QModelIndex{}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

   protected:
    virtual auto __sendCountChangdIfNeed() const -> void;

   private:
    ObjectPointerContainer objects_{};
  };

}  // namespace qtuser_qml

#include "qtusercore/plugin/objectlistmodel.inl"
