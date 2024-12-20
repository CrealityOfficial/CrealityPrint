#ifndef DOCKCONTROLLER_H
#define DOCKCONTROLLER_H

#include <list>
#include <memory>

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QStringListModel>

/// @brief 组件对象工具类
class ItemData {
public:
  explicit ItemData(QPointer<QObject> item = nullptr);

public:
  QPointer<QObject> getItem() const;
  void setItem(QPointer<QObject> item);

  bool isVaild() const;
  operator bool() const;

  bool isEqual(const ItemData& that) const;
  bool operator==(const ItemData& that) const;

public:
  bool isAdsorbed() const;
  void setAdsorbed(bool adsorbed);

  bool isVisible() const;
  void setVisible(bool visible);

  int getX() const;
  void setX(int x);

  int getY() const;
  void setY(int y);

  int getWidth() const;
  void setWidth(int width);

  int getHeight() const;
  void setHeight(int height);

  QByteArray getTtile() const;
  void setTitle(const QByteArray& title);

public:
  void raise();

private:
  QPointer<QObject> item_;
};





/// @brief 上下文对象工具类
class ContextData {
public:
  explicit ContextData(QPointer<QObject> context = nullptr);

public:
  QPointer<QObject> getContext() const;
  void setContext(QPointer<QObject> context);

  bool isVaild() const;
  operator bool() const;

public:
  int getAdsorbedItemX() const;
  int getAdsorbedItemY() const;

  int getAdsorbedItemWidth() const;
  int getAdsorbedItemHeight() const;

private:
  QPointer<QObject> context_;
};





/// @brief Dock 组件控制器, 由 DockContext 创建
/// @see DockContext.qml
/// @see DockItem.qml
class DockController : public QObject {
  Q_OBJECT;

public:
  explicit DockController(QObject* parent = nullptr);

  static void RegisterQmlComponent();

public: 
  // 当前上下文
  Q_PROPERTY(QObject* context READ getContext WRITE setContext NOTIFY sigContextChanged);
  Q_INVOKABLE QObject* getContext() const;
  Q_SLOT void setContext(QObject* context);
  Q_SIGNAL void sigContextChanged(QObject* context);

  // 当前上下文的 tabbar 组件选项列表 (即当前吸附且显示的组件标题)
  Q_PROPERTY(QAbstractListModel* contextTitleList READ getContextTitleList NOTIFY sigContextTitleListChanged);
  Q_INVOKABLE QAbstractListModel* getContextTitleList() const;
  Q_SIGNAL void sigContextTitleListChanged(QAbstractListModel* context_title_list);

  // 当前上下文是否可见
  Q_PROPERTY(bool contextVisible READ isContextVisible NOTIFY sigContextVisibleChanged);
  Q_INVOKABLE bool isContextVisible() const;
  Q_SIGNAL void sigContextVisibleChanged(bool visible);

  // 同步上下文状态
  Q_INVOKABLE void syncContext();

public:
  // 当前绑定到上下文的组件列表 (包括没有显示的没有吸附的)
  Q_PROPERTY(QObjectList itemList READ getItemList NOTIFY sigItemListChanged);
  Q_INVOKABLE QObjectList getItemList() const;
  Q_SIGNAL void sigItemListChanged(const QObjectList& item_list);

  Q_INVOKABLE bool isItemExist(QObject* item) const;
  Q_INVOKABLE void appendItem(QObject* item);
  Q_INVOKABLE void removeItem(QObject* item);
  Q_INVOKABLE void focusItem(QObject* item);
  Q_INVOKABLE int countItem() const;

  /// @brief 关闭所有组件(不解除与上下文的绑定)
  Q_INVOKABLE void closeAllItem();

  /// @brief 根据标题找到组件
  /// @param title 标题名/组件选项名
  /// @return 组件对象, nullptr 代表没找到
  Q_INVOKABLE QObject* findItemByTitle(const QString& title);

private:
  Q_SLOT void onContextDestroyed();
  Q_SLOT void onItemDestroyed(QObject* item);
  Q_SLOT void onItemAdsorbedChanged(bool adsorbed);

private:
  std::unique_ptr<ContextData> context_data_;
  std::list<std::shared_ptr<ItemData>> item_data_list_;
  // std::unique_ptr<QStringListModel> model_;
};

#endif // DOCKCONTROLLER_H
