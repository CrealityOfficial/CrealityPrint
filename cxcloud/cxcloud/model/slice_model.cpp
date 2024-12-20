#include "cxcloud/model/slice_model.h"

namespace cxcloud {

  SliceListModel::SliceListModel(const QStringList& vaild_suffix_list, QObject* parent)
      : SuperType{ parent }, vaild_suffix_list_{ vaild_suffix_list } {}

  SliceListModel::SliceListModel(QStringList&& vaild_suffix_list, QObject* parent)
      : SuperType{ parent }, vaild_suffix_list_{ std::move(vaild_suffix_list) } {}

  auto SliceListModel::getVaildSuffixList() const -> QStringList {
    return vaild_suffix_list_;
  }

  auto SliceListModel::setVaildSuffixList(const QStringList& vaild_suffix_list) -> void {
    vaild_suffix_list_ = vaild_suffix_list;
  }

  auto SliceListModel::get(int index) const -> QVariantMap {
    const auto& datas = rawData();
    if (datas.size() <= index) {
      return {};
    }

    const auto& data = datas[index];

    return {
      { QByteArrayLiteral("model_uid"   ), QVariant::fromValue(data.uid   ) },
      { QByteArrayLiteral("model_name"  ), QVariant::fromValue(data.name  ) },
      { QByteArrayLiteral("model_image" ), QVariant::fromValue(data.image ) },
      { QByteArrayLiteral("model_suffix"), QVariant::fromValue(data.suffix) },
      { QByteArrayLiteral("model_url"   ), QVariant::fromValue(data.url   ) },
    };
  }

  auto SliceListModel::pushBack(const Data& data) -> bool {
    if (vaild_suffix_list_.empty() || vaild_suffix_list_.contains(data.suffix)) {
      return SuperType::pushBack(data);
    }

    return false;
  }

  auto SliceListModel::pushBack(Data&& data) -> bool {
    if (vaild_suffix_list_.empty() || vaild_suffix_list_.contains(data.suffix)) {
      return SuperType::pushBack(std::move(data));
    }

    return false;
  }

  auto SliceListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = index.row();
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
      case DataRole::IMAGE: {
        return QVariant::fromValue(data.image);
      }
      case DataRole::SUFFIX: {
        return QVariant::fromValue(data.suffix);
      }
      case DataRole::URL: {
        return QVariant::fromValue(data.url);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto SliceListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID   ), QByteArrayLiteral("model_uid"   ) },
      { static_cast<int>(DataRole::NAME  ), QByteArrayLiteral("model_name"  ) },
      { static_cast<int>(DataRole::IMAGE ), QByteArrayLiteral("model_image" ) },
      { static_cast<int>(DataRole::SUFFIX), QByteArrayLiteral("model_suffix") },
      { static_cast<int>(DataRole::URL   ), QByteArrayLiteral("model_url"   ) },
    };

    return ROLE_NAMES;
  }

}  // namespace cxcloud
