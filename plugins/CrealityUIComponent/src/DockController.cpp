#include "DockController.h"

#include <qqml.h>

/// @brief QML 属性定义
namespace property {

constexpr auto ADSORBED            { "adsorbed"           };
constexpr auto VISIBLE             { "visible"            };

constexpr auto X                   { "x"                  };
constexpr auto Y                   { "y"                  };
constexpr auto WIDTH               { "width"              };
constexpr auto HEIGHT              { "height"             };

constexpr auto ADSORBED_ITEM_X     { "adsorbedItemX"      };
constexpr auto ADSORBED_ITEM_Y     { "adsorbedItemY"      };
constexpr auto ADSORBED_ITEM_WIDTH { "adsorbedItemWidth"  };
constexpr auto ADSORBED_ITEM_HEIGHT{ "adsorbedItemHeight" };

constexpr auto TITLE               { "title"              };

}

/// @brief QML 方法定义
namespace methord {

constexpr auto RAISE{ "raise" };

}





ItemData::ItemData(QPointer<QObject> item) : item_(item) {}

QPointer<QObject> ItemData::getItem() const { return item_; }

void ItemData::setItem(QPointer<QObject> item) { item_ = item; }

bool ItemData::isVaild() const { return item_ != nullptr; }

ItemData::operator bool() const { return isVaild(); }

bool ItemData::isEqual(const ItemData& that) const { return this->item_ == that.item_; }

bool ItemData::operator==(const ItemData& that) const { return isEqual(that); }

bool ItemData::isAdsorbed() const {
  if (!isVaild()) { return false; }
  return item_->property(property::ADSORBED).toBool();
}

void ItemData::setAdsorbed(bool adsorbed) {
  if (!isVaild()) { return; }
  item_->setProperty(property::ADSORBED, adsorbed);
}

bool ItemData::isVisible() const {
  if (!isVaild()) { return false; }
  return item_->property(property::VISIBLE).toBool();
}

void ItemData::setVisible(bool visible) {
  if (!isVaild()) { return; }
  item_->setProperty(property::VISIBLE, visible);
}

int ItemData::getX() const {
  if (!isVaild()) { return -1; }
  return item_->property(property::X).toInt();
}

void ItemData::setX(int x) {
  if (!isVaild()) { return; }
  item_->setProperty(property::X, x);
}

int ItemData::getY() const {
  if (!isVaild()) { return -1; }
  return item_->property(property::Y).toInt();
}

void ItemData::setY(int y) {
  if (!isVaild()) { return; }
  item_->setProperty(property::Y, y);
}

int ItemData::getWidth() const {
  if (!isVaild()) { return -1; }
  return item_->property(property::WIDTH).toInt();
}

void ItemData::setWidth(int width) {
  if (!isVaild()) { return; }
  item_->setProperty(property::WIDTH, width);
}

int ItemData::getHeight() const {
  if (!isVaild()) { return -1; }
  return item_->property(property::HEIGHT).toInt();
}

void ItemData::setHeight(int height) {
  if (!isVaild()) { return; }
  item_->setProperty(property::HEIGHT, height);
}

QByteArray ItemData::getTtile() const {
  if (!isVaild()) { return {}; }
  return item_->property(property::TITLE).toByteArray();
}

void ItemData::setTitle(const QByteArray& title) {
  if (!isVaild()) { return; }
  item_->setProperty(property::TITLE, title);
}

void ItemData::raise() {
  if (!isVaild()) { return; }
  QMetaObject::invokeMethod(item_, methord::RAISE);
}





ContextData::ContextData(QPointer<QObject> context) : context_(context) {}

QPointer<QObject> ContextData::getContext() const { return context_; }

void ContextData::setContext(QPointer<QObject> context) { context_ = context; }

bool ContextData::isVaild() const { return context_ != nullptr; }

ContextData::operator bool() const { return isVaild(); }

int ContextData::getAdsorbedItemX() const {
  if (!isVaild()) { return -1; }
  return context_->property(property::ADSORBED_ITEM_X).toInt();
}

int ContextData::getAdsorbedItemY() const {
  if (!isVaild()) { return -1; }
  return context_->property(property::ADSORBED_ITEM_Y).toInt();
}

int ContextData::getAdsorbedItemWidth() const {
  if (!isVaild()) { return -1; }
  return context_->property(property::ADSORBED_ITEM_WIDTH).toInt();
}

int ContextData::getAdsorbedItemHeight() const {
  if (!isVaild()) { return -1; }
  return context_->property(property::ADSORBED_ITEM_HEIGHT).toInt();
}





DockController::DockController(QObject* parent)
    : QObject(parent)
    , context_data_(std::make_unique<ContextData>(nullptr)) {}

void DockController::RegisterQmlComponent() {
  qmlRegisterType<DockController>("DockController", 1, 0, "DockController");
}

QObject* DockController::getContext() const {
  return context_data_->getContext().data();
}

void DockController::setContext(QObject* context) {
  QPointer<QObject> context_pointer{ context };
  if (context_data_->getContext() == context_pointer) {
    return;
  }

  if (context_data_->isVaild()) {
    context_data_->getContext()->disconnect(this);
    this->disconnect(context_data_->getContext());
    item_data_list_.clear();
  }

  connect(context, &QObject::destroyed, this, &DockController::onContextDestroyed);
  context_data_->setContext(context_pointer);

  Q_EMIT sigContextChanged(context_pointer.data());
  Q_EMIT sigItemListChanged(getItemList());
}

QAbstractListModel* DockController::getContextTitleList() const {
  QStringList result;

  for (const auto& item_data : item_data_list_) {
    if (!item_data->isVisible() || !item_data->isAdsorbed()) { continue; }
    result.push_front(item_data->getTtile());
  }

  return new QStringListModel{ result };
}

bool DockController::isContextVisible() const {
  for (const auto& item_data : item_data_list_) {
    if (item_data->isVisible() && item_data->isAdsorbed()) {
      return true;
    }
  }

  return false;
}

void DockController::syncContext() {
  Q_EMIT sigContextTitleListChanged(getContextTitleList());
  Q_EMIT sigContextVisibleChanged(isContextVisible());

  for (auto& item_data : item_data_list_) {
    if (!item_data->isVisible()) { continue; }
    item_data->raise();
  }

  for (auto& item_data : item_data_list_) {
    if (!item_data->isVisible() || item_data->isAdsorbed()) { continue; }
    item_data->raise();
  }
}

QObjectList DockController::getItemList() const {
  QObjectList result;
  for (const auto& item_data : item_data_list_) {
    result.push_back(item_data->getItem().data());
  }
  return result;
}

bool DockController::isItemExist(QObject* item) const {
  ItemData item_data{ item };
  for (const auto& list_data : item_data_list_) {
    if (*list_data == item_data) {
      return true;
    }
  }

  return false;
}

void DockController::appendItem(QObject* item) {
  if (!item) { return; }
  if (isItemExist(item)) { return; }

  item_data_list_.emplace_back(std::make_shared<ItemData>(item));

  connect(item, &QObject::destroyed, this, &DockController::onItemDestroyed);
  connect(item, SIGNAL(itemAdsorbedChanged(bool)), this, SLOT(onItemAdsorbedChanged(bool)));

  syncContext();
}

void DockController::removeItem(QObject* item) {
  if (!item) { return; }
  if (!isItemExist(item)) { return; }

  ItemData data{ item };

  data.setAdsorbed(false);
  item_data_list_.remove_if([&data](const auto& item_data) {
    return *item_data == data;
  });

  item->disconnect(this);
  this->disconnect(item);

  syncContext();
}

void DockController::focusItem(QObject* item) {
  if (!item) { return; }
  if (!isItemExist(item)) { return; }

  ItemData data{ item };
  data.setVisible(true);
  data.setX(context_data_->getAdsorbedItemX());
  data.setY(context_data_->getAdsorbedItemY());
  data.setWidth(context_data_->getAdsorbedItemWidth());
  data.setHeight(context_data_->getAdsorbedItemHeight());
  data.raise();

  if (data.isAdsorbed()) {

  }
}

int DockController::countItem() const { return static_cast<int>(item_data_list_.size()); }

void DockController::closeAllItem() {
  for (const auto& item_data : item_data_list_) {
    item_data->setVisible(false);
  }
}

QObject* DockController::findItemByTitle(const QString& title) {
  for (const auto& item_data : item_data_list_) {
    if (item_data->getTtile() == title) {
      return item_data->getItem().data();
    }
  }

  return nullptr;
}

void DockController::onContextDestroyed() {
  for (const auto& item_data : item_data_list_) {
    removeItem(item_data->getItem());
  }
}

void DockController::onItemDestroyed(QObject* item) {
  removeItem(item);
}

void DockController::onItemAdsorbedChanged(bool adsorbed) {
  // QPointer<QObject> sender_data{ sender() };

  // for (const auto& item_data : item_data_list_) {
  //   if (item_data->getItem() != sender_data) {
  //     continue;
  //   }

  //   if (adsorbed) {
    
  //   } else {

  //   }

  //   break;
  // }
}
