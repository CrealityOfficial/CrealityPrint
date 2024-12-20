#include "cxcloud/model/download_model.h"

#include "cxcloud/tool/function.h"

namespace cxcloud {

  auto DownloadItemListModel::getReadyCount() const -> int {
    int count{ 0 };

    for (const auto& item : rawData()) {
      if (item.state == Data::State::READY) {
        ++count;
      }
    }

    return count;
  }

  auto DownloadItemListModel::getDownloadingCount() const -> int {
    int count{ 0 };

    for (const auto& item : rawData()) {
      if (item.state == Data::State::DOWNLOADING) {
        ++count;
      }
    }

    return count;
  }

  auto DownloadItemListModel::getFinishedCount() const -> int {
    int count{ 0 };

    for (const auto& item : rawData()) {
      if (item.state == Data::State::FINISHED) {
        ++count;
      }
    }

    return count;
  }

  auto DownloadItemListModel::pushFront(const Data& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    auto signal_sender = std::function<void()>{ nullptr };

    switch (data.state) {
      case Data::State::READY: {
        signal_sender = [this] { readyCountChanged(); };
        break;
      }
      case Data::State::DOWNLOADING: {
        signal_sender = [this] { downloadingCountChanged(); };
        break;
      }
      case Data::State::FINISHED: {
        signal_sender = [this] { finishedCountChanged(); };
        break;
      }
      default: {
        break;
      }
    }

    beginInsertRows({}, 0, 0);
    rawData().emplace_front(data);
    endInsertRows();

    if (signal_sender) {
      signal_sender();
    }

    return true;
  }

  auto DownloadItemListModel::pushFront(Data&& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    auto signal_sender = std::function<void()>{ nullptr };

    switch (data.state) {
      case Data::State::READY: {
        signal_sender = [this] { readyCountChanged(); };
        break;
      }
      case Data::State::DOWNLOADING: {
        signal_sender = [this] { downloadingCountChanged(); };
        break;
      }
      case Data::State::FINISHED: {
        signal_sender = [this] { finishedCountChanged(); };
        break;
      }
      default: {
        break;
      }
    }

    beginInsertRows({}, 0, 0);
    rawData().emplace_front(std::move(data));
    endInsertRows();

    if (signal_sender) {
      signal_sender();
    }

    return true;
  }

  auto DownloadItemListModel::pushBack(const Data& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    auto signal_sender = std::function<void()>{ nullptr };

    switch (data.state) {
      case Data::State::READY: {
        signal_sender = [this] { readyCountChanged(); };
        break;
      }
      case Data::State::DOWNLOADING: {
        signal_sender = [this] { downloadingCountChanged(); };
        break;
      }
      case Data::State::FINISHED: {
        signal_sender = [this] { finishedCountChanged(); };
        break;
      }
      default: {
        break;
      }
    }

    const auto index = std::distance(rawData().cbegin(), rawData().cend());
    beginInsertRows(QModelIndex{}, index, index);
    rawData().emplace_back(data);
    endInsertRows();

    if (signal_sender) {
      signal_sender();
    }

    return true;
  }

  auto DownloadItemListModel::pushBack(Data&& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    auto signal_sender = std::function<void()>{ nullptr };

    switch (data.state) {
      case Data::State::READY: {
        signal_sender = [this] { readyCountChanged(); };
        break;
      }
      case Data::State::DOWNLOADING: {
        signal_sender = [this] { downloadingCountChanged(); };
        break;
      }
      case Data::State::FINISHED: {
        signal_sender = [this] { finishedCountChanged(); };
        break;
      }
      default: {
        break;
      }
    }

    const auto index = std::distance(rawData().cbegin(), rawData().cend());
    beginInsertRows(QModelIndex{}, index, index);
    rawData().emplace_back(std::move(data));
    endInsertRows();

    if (signal_sender) {
      signal_sender();
    }

    return true;
  }

  auto DownloadItemListModel::update(const Data& data) -> bool {
    auto& datas = rawData();
    for (auto iter = datas.begin(); iter != datas.end(); ++iter) {
      if (iter->uid != data.uid) {
        continue;
      }

      auto signal_sender_list = std::list<std::function<void()>>{
        [this, index = std::distance(datas.begin(), iter)]() {
          const auto model_index = createIndex(index, index);
          dataChanged(model_index, model_index);
        },
      };

      constexpr auto READY{ Data::State::READY };
      if ((iter->state == READY && data.state != READY) ||
          (iter->state != READY && data.state == READY)) {
        signal_sender_list.emplace_back([this] { readyCountChanged(); });
      }

      constexpr auto DOWNLOADING{ Data::State::DOWNLOADING };
      if ((iter->state == DOWNLOADING && data.state != DOWNLOADING) ||
          (iter->state != DOWNLOADING && data.state == DOWNLOADING)) {
        signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
      }

      constexpr auto FINISHED{ Data::State::FINISHED };
      if ((iter->state == FINISHED && data.state != FINISHED) ||
          (iter->state != FINISHED && data.state == FINISHED)) {
        signal_sender_list.emplace_back([this] { finishedCountChanged(); });
      }

      *iter = data;

      for (const auto& signal_sender : signal_sender_list) {
        signal_sender();
      }

      return true;
    }

    return false;
  }

  auto DownloadItemListModel::update(Data&& data) -> bool {
    auto& datas = rawData();
    for (auto iter = datas.begin(); iter != datas.end(); ++iter) {
      if (iter->uid != data.uid) {
        continue;
      }

      auto signal_sender_list = std::list<std::function<void()>>{
        [this, index = std::distance(datas.begin(), iter)]() {
          const auto model_index = createIndex(index, index);
          dataChanged(model_index, model_index);
        },
      };

      constexpr auto READY{ Data::State::READY };
      if ((iter->state == READY && data.state != READY) ||
          (iter->state != READY && data.state == READY)) {
        signal_sender_list.emplace_back([this] { readyCountChanged(); });
      }

      constexpr auto DOWNLOADING{ Data::State::DOWNLOADING };
      if ((iter->state == DOWNLOADING && data.state != DOWNLOADING) ||
          (iter->state != DOWNLOADING && data.state == DOWNLOADING)) {
        signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
      }

      constexpr auto FINISHED{ Data::State::FINISHED };
      if ((iter->state == FINISHED && data.state != FINISHED) ||
          (iter->state != FINISHED && data.state == FINISHED)) {
        signal_sender_list.emplace_back([this] { finishedCountChanged(); });
      }

      *iter = std::move(data);

      for (const auto& signal_sender : signal_sender_list) {
        signal_sender();
      }

      return true;
    }

    return false;
  }

  auto DownloadItemListModel::erase(const QString& uid) -> bool {
    auto& datas = rawData();
    for (auto iter = datas.cbegin(); iter != datas.cend(); ++iter) {
      if (iter->uid != uid) {
        continue;
      }

      auto signal_sender = std::function<void()>{ nullptr };

      switch (iter->state) {
        case Data::State::READY: {
          signal_sender = [this] { readyCountChanged(); };
          break;
        }
        case Data::State::DOWNLOADING: {
          signal_sender = [this] { downloadingCountChanged(); };
          break;
        }
        case Data::State::FINISHED: {
          signal_sender = [this] { finishedCountChanged(); };
          break;
        }
        default: {
          break;
        }
      }

      const auto index = std::distance(datas.cbegin(), iter);
      beginRemoveRows(QModelIndex{}, index, index);
      datas.erase(iter);
      endRemoveRows();

      if (signal_sender) {
        signal_sender();
      }

      return true;
    }

    return false;
  }

  auto DownloadItemListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = static_cast<size_t>(index.row());
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    static const auto n2s = [](auto number) { return QString::number(number, 10, 2); };

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }
      case DataRole::IMAGE: {
        return QVariant::fromValue(data.image);
      }
      case DataRole::FORMAT: {
        return QVariant::fromValue(data.format);
      }
      case DataRole::SIZE: {
        return QVariant::fromValue(data.size);
      }
      case DataRole::SIZE_WITH_UNIT: {
        auto static const FORMAT = QStringLiteral("%1 %2");

        const auto size_b = static_cast<double>(data.size);
        auto value = size_b < 1_kb ? FORMAT.arg(n2s(size_b       )).arg(QStringLiteral("B" ))
                   : size_b < 1_mb ? FORMAT.arg(n2s(size_b / 1_kb)).arg(QStringLiteral("KB"))
                   : size_b < 1_gb ? FORMAT.arg(n2s(size_b / 1_mb)).arg(QStringLiteral("MB"))
                                   : FORMAT.arg(n2s(size_b / 1_gb)).arg(QStringLiteral("GB"));

        return QVariant::fromValue(std::move(value));
      }
      case DataRole::DATE: {
        return QVariant::fromValue(data.date);
      }
      case DataRole::SPEED: {
        return QVariant::fromValue(data.speed);
      }
      case DataRole::SPEED_WITH_UNIT: {
        auto static const FORMAT = QStringLiteral("%1 %2/s");

        const auto speed_b = data.speed * 1024.0;
        auto value = speed_b < 1_kb ? FORMAT.arg(n2s(speed_b       )).arg(QStringLiteral("B" ))
                   : speed_b < 1_mb ? FORMAT.arg(n2s(speed_b / 1_kb)).arg(QStringLiteral("KB"))
                   : speed_b < 1_gb ? FORMAT.arg(n2s(speed_b / 1_mb)).arg(QStringLiteral("MB"))
                                    : FORMAT.arg(n2s(speed_b / 1_gb)).arg(QStringLiteral("GB"));

        return QVariant::fromValue(std::move(value));
      }
      case DataRole::STATE: {
        return QVariant::fromValue(static_cast<int>(data.state));
      }
      case DataRole::PROGRESS: {
        return QVariant::fromValue(data.progress);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto DownloadItemListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID            ), QByteArrayLiteral("model_uid"            ) },
      { static_cast<int>(DataRole::NAME           ), QByteArrayLiteral("model_name"           ) },
      { static_cast<int>(DataRole::FORMAT         ), QByteArrayLiteral("model_format"         ) },
      { static_cast<int>(DataRole::IMAGE          ), QByteArrayLiteral("model_image"          ) },
      { static_cast<int>(DataRole::SIZE           ), QByteArrayLiteral("model_size"           ) },
      { static_cast<int>(DataRole::SIZE_WITH_UNIT ), QByteArrayLiteral("model_size_with_unit" ) },
      { static_cast<int>(DataRole::DATE           ), QByteArrayLiteral("model_date"           ) },
      { static_cast<int>(DataRole::SPEED          ), QByteArrayLiteral("model_speed"          ) },
      { static_cast<int>(DataRole::SPEED_WITH_UNIT), QByteArrayLiteral("model_speed_with_unit") },
      { static_cast<int>(DataRole::STATE          ), QByteArrayLiteral("model_state"          ) },
      { static_cast<int>(DataRole::PROGRESS       ), QByteArrayLiteral("model_progress"       ) },
    };

    return ROLE_NAMES;
  }





  auto operator<<(DownloadGroupInfo& info, const ModelGroupInfoModel& model) -> DownloadGroupInfo& {
    info.uid = model.getUid();
    info.name = model.getName();
    if (!model.models().expired()) {
      info << *model.models().lock();
    }
    return info;
  }

  auto operator<<(DownloadGroupInfo& info, const ModelInfoListModel& model) -> DownloadGroupInfo& {
    if (info.items) {
      for (const auto& model_item : model.rawData()) {
        auto item = info.items->load(model_item.uid);
        item.uid = model_item.uid;
        item.name = model_item.name;
        item.format = model_item.format;
        item.size = model_item.size;
        item.image = model_item.image;
        if (model_item.broken) {
          item.state = DownloadItemInfo::State::BROKEN;
        }
        info.items->emplaceBack(std::move(item));
      }
    }
    return info;
  }





  auto DownloadGroupListModel::getReadyCount() const -> int {
    int count{ 0 };

    for (const auto& group : rawData()) {
      count += group.items->getReadyCount();
    }

    return count;
  }

  auto DownloadGroupListModel::getDownloadingCount() const -> int {
    int count{ 0 };

    for (const auto& group : rawData()) {
      count += group.items->getDownloadingCount();
    }

    return count;
  }

  auto DownloadGroupListModel::getFinishedCount() const -> int {
    int count{ 0 };

    for (const auto& group : rawData()) {
      count += group.items->getFinishedCount();
    }

    return count;
  }

  auto DownloadGroupListModel::pushFront(const Data& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    connect(data.items.get(), &DownloadItemListModel::readyCountChanged,
            this, &DownloadGroupListModel::readyCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::downloadingCountChanged,
            this, &DownloadGroupListModel::downloadingCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::finishedCountChanged,
            this, &DownloadGroupListModel::finishedCountChanged,
            Qt::ConnectionType::UniqueConnection);

    auto signal_sender_list{ std::list<std::function<void()>>{} };

    if (data.items->getReadyCount() != 0) {
      signal_sender_list.emplace_back([this] { readyCountChanged(); });
    }

    if (data.items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
    }

    if (data.items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this] { finishedCountChanged(); });
    }

    beginInsertRows(QModelIndex{}, 0, 0);
    rawData().emplace_front(data);
    endInsertRows();

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  auto DownloadGroupListModel::pushFront(Data&& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    connect(data.items.get(), &DownloadItemListModel::readyCountChanged,
            this, &DownloadGroupListModel::readyCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::downloadingCountChanged,
            this, &DownloadGroupListModel::downloadingCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::finishedCountChanged,
            this, &DownloadGroupListModel::finishedCountChanged,
            Qt::ConnectionType::UniqueConnection);

    auto signal_sender_list = std::list<std::function<void()>>{};

    if (data.items->getReadyCount() != 0) {
      signal_sender_list.emplace_back([this] { readyCountChanged(); });
    }

    if (data.items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
    }

    if (data.items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this] { finishedCountChanged(); });
    }

    beginInsertRows(QModelIndex{}, 0, 0);
    rawData().emplace_front(std::move(data));
    endInsertRows();

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  auto DownloadGroupListModel::pushBack(const Data& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    connect(data.items.get(), &DownloadItemListModel::readyCountChanged,
            this, &DownloadGroupListModel::readyCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::downloadingCountChanged,
            this, &DownloadGroupListModel::downloadingCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::finishedCountChanged,
            this, &DownloadGroupListModel::finishedCountChanged,
            Qt::ConnectionType::UniqueConnection);

    auto signal_sender_list = std::list<std::function<void()>>{};

    if (data.items->getReadyCount() != 0) {
      signal_sender_list.emplace_back([this] { readyCountChanged(); });
    }

    if (data.items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
    }

    if (data.items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this] { finishedCountChanged(); });
    }

    const auto index = std::distance(rawData().cbegin(), rawData().cend());
    beginInsertRows(QModelIndex{}, index, index);
    rawData().emplace_back(data);
    endInsertRows();

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  auto DownloadGroupListModel::pushBack(Data&& data) -> bool {
    if (find(data.uid)) {
      return false;
    }

    connect(data.items.get(), &DownloadItemListModel::readyCountChanged,
            this, &DownloadGroupListModel::readyCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::downloadingCountChanged,
            this, &DownloadGroupListModel::downloadingCountChanged,
            Qt::ConnectionType::UniqueConnection);

    connect(data.items.get(), &DownloadItemListModel::finishedCountChanged,
            this, &DownloadGroupListModel::finishedCountChanged,
            Qt::ConnectionType::UniqueConnection);

    auto signal_sender_list = std::list<std::function<void()>>{};

    if (data.items->getReadyCount() != 0) {
      signal_sender_list.emplace_back([this] { readyCountChanged(); });
    }

    if (data.items->getDownloadingCount() != 0) {
      signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
    }

    if (data.items->getFinishedCount() != 0) {
      signal_sender_list.emplace_back([this] { finishedCountChanged(); });
    }

    const auto index = std::distance(rawData().cbegin(), rawData().cend());
    beginInsertRows(QModelIndex{}, index, index);
    rawData().emplace_back(std::move(data));
    endInsertRows();

    for (const auto& signal_sender : signal_sender_list) {
      signal_sender();
    }

    return true;
  }

  auto DownloadGroupListModel::update(const Data& data) -> bool {
    auto& datas = rawData();
    for (auto iter = datas.begin(); iter != datas.end(); ++iter) {
      if (iter->uid != data.uid) {
        continue;
      }

      auto signal_sender_list = std::list<std::function<void()>>{
        [this, index = std::distance(datas.begin(), iter)]()->void {
          const auto model_index = createIndex(index, index);
          dataChanged(model_index, model_index);
        }
      };

      if (iter->items->getReadyCount() != 0) {
        signal_sender_list.emplace_back([this] { readyCountChanged(); });
      }

      if (iter->items->getDownloadingCount() != 0) {
        signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
      }

      if (iter->items->getFinishedCount() != 0) {
        signal_sender_list.emplace_back([this] { finishedCountChanged(); });
      }

      *iter = data;

      for (const auto& signal_sender : signal_sender_list) {
        signal_sender();
      }

      return true;
    }

    return false;
  }

  auto DownloadGroupListModel::update(Data&& data) -> bool {
    auto& datas = rawData();
    for (auto iter = datas.begin(); iter != datas.end(); ++iter) {
      if (iter->uid != data.uid) {
        continue;
      }

      auto signal_sender_list = std::list<std::function<void()>>{
        [this, index = std::distance(datas.begin(), iter)]()->void {
          const auto model_index = createIndex(index, index);
          dataChanged(model_index, model_index);
        }
      };

      if (iter->items->getReadyCount() != 0) {
        signal_sender_list.emplace_back([this] { readyCountChanged(); });
      }

      if (iter->items->getDownloadingCount() != 0) {
        signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
      }

      if (iter->items->getFinishedCount() != 0) {
        signal_sender_list.emplace_back([this] { finishedCountChanged(); });
      }

      *iter = std::move(data);

      for (const auto& signal_sender : signal_sender_list) {
        signal_sender();
      }

      return true;
    }

    return false;
  }

  auto DownloadGroupListModel::erase(const QString& uid) -> bool {
    auto& datas = rawData();
    for (auto iter = datas.cbegin(); iter != datas.cend(); ++iter) {
      if (iter->uid != uid) {
        continue;
      }

      iter->items->disconnect(this);

      auto signal_sender_list = std::list<std::function<void()>>{};

      if (iter->items->getDownloadingCount() != 0) {
        signal_sender_list.emplace_back([this] { downloadingCountChanged(); });
      }

      if (iter->items->getFinishedCount() != 0) {
        signal_sender_list.emplace_back([this] { finishedCountChanged(); });
      }

      const auto index = std::distance(datas.cbegin(), iter);
      beginRemoveRows(QModelIndex{}, index, index);
      datas.erase(iter);
      endRemoveRows();

      for (const auto& signal_sender : signal_sender_list) {
        signal_sender();
      }

      return true;
    }

    return false;
  }

  auto DownloadGroupListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = static_cast<size_t>(index.row());
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }

      case DataRole::ITEMS: {
        return QVariant::fromValue(data.items.get());
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto DownloadGroupListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID   ), QByteArrayLiteral("model_uid"   ) },
      { static_cast<int>(DataRole::NAME  ), QByteArrayLiteral("model_name"  ) },
      { static_cast<int>(DataRole::ITEMS ), QByteArrayLiteral("model_items" ) },
    };

    return ROLE_NAMES;
  }

} // namespace cxcloud
