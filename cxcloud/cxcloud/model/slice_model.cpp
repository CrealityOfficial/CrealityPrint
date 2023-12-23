#include "cxcloud/model/slice_model.h"

namespace cxcloud {

SliceListModel::SliceListModel(const QStringList& vaild_suffix_list, QObject* parent)
    : SuperType(parent), vaild_suffix_list_(vaild_suffix_list) {}

SliceListModel::SliceListModel(QStringList&& vaild_suffix_list, QObject* parent)
    : SuperType(parent), vaild_suffix_list_(std::move(vaild_suffix_list)) {}

QStringList SliceListModel::getVaildSuffixList() const {
  return vaild_suffix_list_;
}

void SliceListModel::setVaildSuffixList(const QStringList& vaild_suffix_list) {
  vaild_suffix_list_ = vaild_suffix_list;
}

QVariantMap SliceListModel::get(int index) const {
  if (rawData().size() <= index) {
    return {};
  }

  const auto& data = rawData().at(index);
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

QVariant SliceListModel::data(const QModelIndex& index, int role) const {
  if (index.row() < 0 || index.row() >= rawData().size()) {
    return {};
  }

  QVariant result;
  const auto& data = rawData().at(index.row());
  switch (static_cast<DataRole>(role)) {
    case DataRole::UID:
      result = QVariant::fromValue(data.uid);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(data.name);
      break;
    case DataRole::IMAGE:
      result = QVariant::fromValue(data.image);
      break;
    case DataRole::SUFFIX:
      result = QVariant::fromValue(data.suffix);
      break;
    case DataRole::URL:
      result = QVariant::fromValue(data.url);
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> SliceListModel::roleNames() const {
  static const QHash<int, QByteArray> ROLE_NAMES{
    { static_cast<int>(DataRole::UID   ), QByteArrayLiteral("model_uid"   ) },
    { static_cast<int>(DataRole::NAME  ), QByteArrayLiteral("model_name"  ) },
    { static_cast<int>(DataRole::IMAGE ), QByteArrayLiteral("model_image" ) },
    { static_cast<int>(DataRole::SUFFIX), QByteArrayLiteral("model_suffix") },
    { static_cast<int>(DataRole::URL   ), QByteArrayLiteral("model_url"   ) },
  };

  return ROLE_NAMES;
}

}  // namespace cxcloud
