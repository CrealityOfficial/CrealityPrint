#include "cxcloud/model/download_model.h"

#include <tuple>

#include "cxcloud/tool/function.h"

namespace cxcloud {

int DownloadItemListModel::getReadyCount() const {
  int count{ 0 };

  for (const auto& item : rawData()) {
    if (item.state == Data::State::READY) {
      ++count;
    }
  }

  return count;
}

int DownloadItemListModel::getDownloadingCount() const {
  int count{ 0 };

  for (const auto& item : rawData()) {
    if (item.state == Data::State::DOWNLOADING) {
      ++count;
    }
  }

  return count;
}

int DownloadItemListModel::getFinishedCount() const {
  int count{ 0 };

  for (const auto& item : rawData()) {
    if (item.state == Data::State::FINISHED) {
      ++count;
    }
  }

  return count;
}

auto DownloadItemListModel::pushFront(const Data& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  std::function<void()> signal_sender = []()->void {};

  switch (info.state) {
    case Data::State::READY:
      signal_sender = [this]() {
        Q_EMIT readyCountChanged();
      };
      break;
    case Data::State::DOWNLOADING:
      signal_sender = [this]() {
        Q_EMIT downloadingCountChanged();
      };
      break;
    case Data::State::FINISHED:
      signal_sender = [this]() {
        Q_EMIT finishedCountChanged();
      };
      break;
    default:
      break;
  }

  beginInsertRows(QModelIndex{}, 0, 0);
  rawData().emplace_front(info);
  endInsertRows();

  signal_sender();

  return true;
}

auto DownloadItemListModel::pushFront(Data&& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  std::function<void()> signal_sender = []()->void {};

  switch (info.state) {
    case Data::State::READY:
      signal_sender = [this]() {
        Q_EMIT readyCountChanged();
      };
      break;
    case Data::State::DOWNLOADING:
      signal_sender = [this]() {
        Q_EMIT downloadingCountChanged();
      };
      break;
    case Data::State::FINISHED:
      signal_sender = [this]() {
        Q_EMIT finishedCountChanged();
      };
      break;
    default:
      break;
  }

  beginInsertRows(QModelIndex{}, 0, 0);
  rawData().emplace_front(std::move(info));
  endInsertRows();

  signal_sender();

  return true;
}

auto DownloadItemListModel::pushBack(const Data& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  std::function<void()> signal_sender = []()->void {};

  switch (info.state) {
    case Data::State::READY:
      signal_sender = [this]() {
        Q_EMIT readyCountChanged();
      };
      break;
    case Data::State::DOWNLOADING:
      signal_sender = [this]() {
        Q_EMIT downloadingCountChanged();
      };
      break;
    case Data::State::FINISHED:
      signal_sender = [this]() {
        Q_EMIT finishedCountChanged();
      };
      break;
    default:
      break;
  }

  const auto index = std::distance(rawData().cbegin(), rawData().cend());
  beginInsertRows(QModelIndex{}, index, index);
  rawData().emplace_back(info);
  endInsertRows();

  signal_sender();

  return true;
}

auto DownloadItemListModel::pushBack(Data&& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  std::function<void()> signal_sender = []()->void {};

  switch (info.state) {
    case Data::State::READY:
      signal_sender = [this]() {
        Q_EMIT readyCountChanged();
      };
      break;
    case Data::State::DOWNLOADING:
      signal_sender = [this]() {
        Q_EMIT downloadingCountChanged();
      };
      break;
    case Data::State::FINISHED:
      signal_sender = [this]() {
        Q_EMIT finishedCountChanged();
      };
      break;
    default:
      break;
  }

  const auto index = std::distance(rawData().cbegin(), rawData().cend());
  beginInsertRows(QModelIndex{}, index, index);
  rawData().emplace_back(std::move(info));
  endInsertRows();

  signal_sender();

  return true;
}

auto DownloadItemListModel::update(const Data& info) -> bool {
  for (auto iter = rawData().begin(); iter != rawData().end(); ++iter) {
    if (iter->uid != info.uid) {
      continue;
    }

    auto signal_sender_list{ std::list<std::function<void()>>{
        [this, index = std::distance(rawData().begin(), iter)]() {
          const auto model_index = createIndex(index, index);
          Q_EMIT dataChanged(model_index, model_index);
        },
      }
    };

    constexpr auto READY{ Data::State::READY };
    if ((iter->state == READY && info.state != READY) ||
        (iter->state != READY && info.state == READY)) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT readyCountChanged();
      });
    }

    constexpr auto DOWNLOADING{ Data::State::DOWNLOADING };
    if ((iter->state == DOWNLOADING && info.state != DOWNLOADING) ||
        (iter->state != DOWNLOADING && info.state == DOWNLOADING)) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT downloadingCountChanged();
      });
    }

    constexpr auto FINISHED{ Data::State::FINISHED };
    if ((iter->state == FINISHED && info.state != FINISHED) ||
        (iter->state != FINISHED && info.state == FINISHED)) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT finishedCountChanged();
      });
    }

    *iter = info;

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  return false;
}

auto DownloadItemListModel::update(Data&& info) -> bool {
  for (auto iter = rawData().begin(); iter != rawData().end(); ++iter) {
    if (iter->uid != info.uid) {
      continue;
    }

    auto signal_sender_list{ std::list<std::function<void()>>{
        [this, index = std::distance(rawData().begin(), iter)]() {
          const auto model_index = createIndex(index, index);
          Q_EMIT dataChanged(model_index, model_index);
        },
      }
    };

    constexpr auto READY{ Data::State::READY };
    if ((iter->state == READY && info.state != READY) ||
        (iter->state != READY && info.state == READY)) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT readyCountChanged();
      });
    }

    constexpr auto DOWNLOADING{ Data::State::DOWNLOADING };
    if ((iter->state == DOWNLOADING && info.state != DOWNLOADING) ||
        (iter->state != DOWNLOADING && info.state == DOWNLOADING)) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT downloadingCountChanged();
      });
    }

    constexpr auto FINISHED{ Data::State::FINISHED };
    if ((iter->state == FINISHED && info.state != FINISHED) ||
        (iter->state != FINISHED && info.state == FINISHED)) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT finishedCountChanged();
      });
    }

    *iter = std::move(info);

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  return false;
}

auto DownloadItemListModel::erase(const QString& uid) -> bool {
  for (auto iter = rawData().cbegin(); iter != rawData().cend(); ++iter) {
    if (iter->uid != uid) {
      continue;
    }

    std::function<void()> signal_sender = []()->void {};

    switch (iter->state) {
      case Data::State::READY:
        signal_sender = [this]() {
          Q_EMIT readyCountChanged();
        };
        break;
      case Data::State::DOWNLOADING:
        signal_sender = [this]() {
          Q_EMIT downloadingCountChanged();
        };
        break;
      case Data::State::FINISHED:
        signal_sender = [this]() {
          Q_EMIT finishedCountChanged();
        };
        break;
      default:
        break;
    }

    const auto index = std::distance(rawData().cbegin(), iter);
    beginRemoveRows(QModelIndex{}, index, index);
    rawData().erase(iter);
    endRemoveRows();

    signal_sender();
    return true;
  }

  return false;
}

QVariant DownloadItemListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  static const auto n2s = [](auto number) { return QString::number(number, 10, 2); };

  QVariant result;
  const auto& item = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(item.uid);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(item.name);
      break;
    case DataRole::IMAGE:
      result = QVariant::fromValue(item.image);
      break;
    case DataRole::SIZE:
      result = QVariant::fromValue(item.size);
      break;
    case DataRole::SIZE_WITH_UNIT: {
      auto static const FORMAT = QStringLiteral("%1 %2");

      auto const size_b = static_cast<double>(item.size);
      auto value = size_b < 1_kb ? FORMAT.arg(n2s(size_b       )).arg(QStringLiteral("B" ))
                 : size_b < 1_mb ? FORMAT.arg(n2s(size_b / 1_kb)).arg(QStringLiteral("KB"))
                 : size_b < 1_gb ? FORMAT.arg(n2s(size_b / 1_mb)).arg(QStringLiteral("MB"))
                                 : FORMAT.arg(n2s(size_b / 1_gb)).arg(QStringLiteral("GB"));

      result = QVariant::fromValue(std::move(value));
      break;
    }
    case DataRole::DATE:
      result = QVariant::fromValue(item.date);
      break;
    case DataRole::SPEED:
      result = QVariant::fromValue(item.speed);
      break;
    case DataRole::SPEED_WITH_UNIT: {
      auto static const FORMAT = QStringLiteral("%1 %2/s");

      auto const speed_b = item.speed * 1024.0;
      auto value = speed_b < 1_kb ? FORMAT.arg(n2s(speed_b       )).arg(QStringLiteral("B" ))
                 : speed_b < 1_mb ? FORMAT.arg(n2s(speed_b / 1_kb)).arg(QStringLiteral("KB"))
                 : speed_b < 1_gb ? FORMAT.arg(n2s(speed_b / 1_mb)).arg(QStringLiteral("MB"))
                                  : FORMAT.arg(n2s(speed_b / 1_gb)).arg(QStringLiteral("GB"));

      result = QVariant::fromValue(std::move(value));
      break;
    }
    case DataRole::STATE:
      result = QVariant::fromValue(static_cast<int>(item.state));
      break;
    case DataRole::PROGRESS:
      result = QVariant::fromValue(item.progress);
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> DownloadItemListModel::roleNames() const {
  static const QHash<int, QByteArray> role_names{
    { static_cast<int>(DataRole::UID            ), QByteArrayLiteral("model_uid"            ) },
    { static_cast<int>(DataRole::NAME           ), QByteArrayLiteral("model_name"           ) },
    { static_cast<int>(DataRole::IMAGE          ), QByteArrayLiteral("model_image"          ) },
    { static_cast<int>(DataRole::SIZE           ), QByteArrayLiteral("model_size"           ) },
    { static_cast<int>(DataRole::SIZE_WITH_UNIT ), QByteArrayLiteral("model_size_with_unit" ) },
    { static_cast<int>(DataRole::DATE           ), QByteArrayLiteral("model_date"           ) },
    { static_cast<int>(DataRole::SPEED          ), QByteArrayLiteral("model_speed"          ) },
    { static_cast<int>(DataRole::SPEED_WITH_UNIT), QByteArrayLiteral("model_speed_with_unit") },
    { static_cast<int>(DataRole::STATE          ), QByteArrayLiteral("model_state"          ) },
    { static_cast<int>(DataRole::PROGRESS       ), QByteArrayLiteral("model_progress"       ) },
  };

  return role_names;
}





DownloadGroupInfo& operator<<(DownloadGroupInfo& info, const ModelGroupInfoModel& model) {
  info.uid = model.getUid();
  info.name = model.getName();
  if (model.models().use_count() > 0) {
    info << *model.models().lock();
  }
  return info;
}

DownloadGroupInfo& operator<<(DownloadGroupInfo& info, const ModelInfoListModel& model) {
  if (info.items) {
    for (const auto& model_item : model.rawData()) {
      auto item = info.items->load(model_item.uid);
      item.uid = model_item.uid;
      item.name = model_item.name;
      item.size = model_item.size;
      item.image = model_item.image;
      info.items->emplaceBack(std::move(item));
    }
  }
  return info;
}





int DownloadGroupListModel::getReadyCount() const {
  int count{ 0 };

  for (const auto& group : rawData()) {
    count += group.items->getReadyCount();
  }

  return count;
}

int DownloadGroupListModel::getDownloadingCount() const {
  int count{ 0 };

  for (const auto& group : rawData()) {
    count += group.items->getDownloadingCount();
  }

  return count;
}

int DownloadGroupListModel::getFinishedCount() const {
  int count{ 0 };

  for (const auto& group : rawData()) {
    count += group.items->getFinishedCount();
  }

  return count;
}

auto DownloadGroupListModel::pushFront(const Data& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  connect(info.items.get(), &DownloadItemListModel::readyCountChanged,
          this, &DownloadGroupListModel::readyCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::downloadingCountChanged,
          this, &DownloadGroupListModel::downloadingCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::finishedCountChanged,
          this, &DownloadGroupListModel::finishedCountChanged,
          Qt::ConnectionType::UniqueConnection);

  auto signal_sender_list{ std::list<std::function<void()>>{} };

  if (info.items->getReadyCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT readyCountChanged();
    });
  }

  if (info.items->getDownloadingCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT downloadingCountChanged();
    });
  }

  if (info.items->getFinishedCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT finishedCountChanged();
    });
  }

  beginInsertRows(QModelIndex{}, 0, 0);
  rawData().emplace_front(info);
  endInsertRows();

  for (const auto& signal_sender : signal_sender_list) {
    signal_sender();
  }

  return true;
}

auto DownloadGroupListModel::pushFront(Data&& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  connect(info.items.get(), &DownloadItemListModel::readyCountChanged,
          this, &DownloadGroupListModel::readyCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::downloadingCountChanged,
          this, &DownloadGroupListModel::downloadingCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::finishedCountChanged,
          this, &DownloadGroupListModel::finishedCountChanged,
          Qt::ConnectionType::UniqueConnection);

  auto signal_sender_list{ std::list<std::function<void()>>{} };

  if (info.items->getReadyCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT readyCountChanged();
    });
  }

  if (info.items->getDownloadingCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT downloadingCountChanged();
    });
  }

  if (info.items->getFinishedCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT finishedCountChanged();
    });
  }

  beginInsertRows(QModelIndex{}, 0, 0);
  rawData().emplace_front(std::move(info));
  endInsertRows();

  for (const auto& signal_sender : signal_sender_list) {
    signal_sender();
  }

  return true;
}

auto DownloadGroupListModel::pushBack(const Data& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  connect(info.items.get(), &DownloadItemListModel::readyCountChanged,
          this, &DownloadGroupListModel::readyCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::downloadingCountChanged,
          this, &DownloadGroupListModel::downloadingCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::finishedCountChanged,
          this, &DownloadGroupListModel::finishedCountChanged,
          Qt::ConnectionType::UniqueConnection);

  auto signal_sender_list{ std::list<std::function<void()>>{} };

  if (info.items->getReadyCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT readyCountChanged();
    });
  }

  if (info.items->getDownloadingCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT downloadingCountChanged();
    });
  }

  if (info.items->getFinishedCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT finishedCountChanged();
    });
  }

  const auto index = std::distance(rawData().cbegin(), rawData().cend());
  beginInsertRows(QModelIndex{}, index, index);
  rawData().emplace_back(info);
  endInsertRows();

  for (const auto& signal_sender : signal_sender_list) {
    signal_sender();
  }

  return true;
}

auto DownloadGroupListModel::pushBack(Data&& info) -> bool {
  if (find(info.uid)) {
    return false;
  }

  connect(info.items.get(), &DownloadItemListModel::readyCountChanged,
          this, &DownloadGroupListModel::readyCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::downloadingCountChanged,
          this, &DownloadGroupListModel::downloadingCountChanged,
          Qt::ConnectionType::UniqueConnection);

  connect(info.items.get(), &DownloadItemListModel::finishedCountChanged,
          this, &DownloadGroupListModel::finishedCountChanged,
          Qt::ConnectionType::UniqueConnection);

  auto signal_sender_list{ std::list<std::function<void()>>{} };

  if (info.items->getReadyCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT readyCountChanged();
    });
  }

  if (info.items->getDownloadingCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT downloadingCountChanged();
    });
  }

  if (info.items->getFinishedCount() != 0) {
    signal_sender_list.emplace_back([this]() {
      Q_EMIT finishedCountChanged();
    });
  }

  const auto index = std::distance(rawData().cbegin(), rawData().cend());
  beginInsertRows(QModelIndex{}, index, index);
  rawData().emplace_back(std::move(info));
  endInsertRows();

  for (const auto& signal_sender : signal_sender_list) {
    signal_sender();
  }

  return true;
}

auto DownloadGroupListModel::update(const Data& info) -> bool {
  for (auto iter = rawData().begin(); iter != rawData().end(); ++iter) {
    if (iter->uid != info.uid) {
      continue;
    }

    auto signal_sender_list{ std::list<std::function<void()>>{
        [this, index = std::distance(rawData().begin(), iter)]()->void {
          const auto model_index = createIndex(index, index);
          Q_EMIT dataChanged(model_index, model_index);
        }
      }
    };

    if (iter->items->getReadyCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT readyCountChanged();
      });
    }

    if (iter->items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT downloadingCountChanged();
      });
    }

    if (iter->items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT finishedCountChanged();
      });
    }

    *iter = info;

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  return false;
}

auto DownloadGroupListModel::update(Data&& info) -> bool {
  for (auto iter = rawData().begin(); iter != rawData().end(); ++iter) {
    if (iter->uid != info.uid) {
      continue;
    }

    auto signal_sender_list{ std::list<std::function<void()>>{
        [this, index = std::distance(rawData().begin(), iter)]()->void {
          const auto model_index = createIndex(index, index);
          Q_EMIT dataChanged(model_index, model_index);
        }
      }
    };

    if (iter->items->getReadyCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT readyCountChanged();
      });
    }

    if (iter->items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT downloadingCountChanged();
      });
    }

    if (iter->items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT finishedCountChanged();
      });
    }

    *iter = std::move(info);

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  return false;
}

auto DownloadGroupListModel::erase(const QString& uid) -> bool {
  for (auto iter = rawData().cbegin(); iter != rawData().cend(); ++iter) {
    if (iter->uid != uid) {
      continue;
    }

    iter->items->disconnect(this);

    auto signal_sender_list{ std::list<std::function<void()>>{} };

    if (iter->items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT downloadingCountChanged();
      });
    }

    if (iter->items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this]() {
        Q_EMIT finishedCountChanged();
      });
    }

    const auto index = std::distance(rawData().cbegin(), iter);
    beginRemoveRows(QModelIndex{}, index, index);
    rawData().erase(iter);
    endRemoveRows();

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  return false;
}

QVariant DownloadGroupListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  QVariant result;
  const auto& group = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(group.uid);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(group.name);
      break;
    case DataRole::ITEMS:
      result = QVariant::fromValue(group.items.get());
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> DownloadGroupListModel::roleNames() const {
  static const QHash<int, QByteArray> role_names{
    { static_cast<int>(DataRole::UID         ), QByteArrayLiteral("model_uid"         ) },
    { static_cast<int>(DataRole::NAME        ), QByteArrayLiteral("model_name"        ) },
    { static_cast<int>(DataRole::ITEMS       ), QByteArrayLiteral("model_items"       ) },
  };
  return role_names;
}

} // namespace cxcloud
