#include "gcodespeedlistmodel.h"

namespace cxgcode {

GcodeSpeedListModel::GcodeSpeedListModel(QObject* parent)
    : GcodePreviewListModel(gcode::GCodeVisualType::gvt_speed, parent)
    , min_speed_(0.0f)
    , max_speed_(1.0f) {}

void GcodeSpeedListModel::setDataList(const QList<GcodeSpeedData>& data_list) {
  beginResetModel();
  data_list_ = data_list;
  endResetModel();
}

void GcodeSpeedListModel::setColors(const QList<QColor>& colors) {
  colors_ = colors;
  resetData();
}

void GcodeSpeedListModel::setRange(double min_speed, double max_speed) {
  assert(min_speed <= max_speed && "min_speed is bigger than max_speed");
  min_speed_ = min_speed;
  max_speed_ = max_speed;
  resetData();
}

int GcodeSpeedListModel::rowCount(const QModelIndex& parent) const {
  return data_list_.size();
}

QVariant GcodeSpeedListModel::data(const QModelIndex& index, int role) const {
  QVariant result{ QVariant::Type::Invalid };
  if (index.row() < 0 || index.row() >= rowCount() || rowCount() < 2) {
    return result;
  }

  auto const& data = data_list_[index.row()];
  switch (static_cast<DataRole>(role)) {
    case DataRole::COLOR:
      result = QVariant::fromValue(data.color);
      break;
    case DataRole::SPEED:
      result = QVariant::fromValue(data.speed);
      break;
    default:
      break;
  }

  return result;
}

QHash<int, QByteArray> GcodeSpeedListModel::roleNames() const {
  return {
    { static_cast<int>(DataRole::COLOR), QByteArrayLiteral("model_color") },
    { static_cast<int>(DataRole::SPEED), QByteArrayLiteral("model_speed") },
  };
}

void GcodeSpeedListModel::resetData() {
  QList<GcodeSpeedData> data_list;

  double const speed_diff = (max_speed_ - min_speed_) / (colors_.size() - 1);
  for (size_t index = 0ull; index < colors_.size(); ++index) {
    double speed = (min_speed_ + speed_diff * index) / 60.0f;
    data_list << GcodeSpeedData{ colors_[index], speed };
  }

  std::reverse(data_list.begin(), data_list.end());

  beginResetModel();
  data_list_ = std::move(data_list);
  endResetModel();
}

}  // namespace cxgcode
