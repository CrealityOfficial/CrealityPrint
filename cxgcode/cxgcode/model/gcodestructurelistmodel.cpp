#include "gcodestructurelistmodel.h"
#include <qsettings.h>
namespace cxgcode {

GcodeStructureListModel::GcodeStructureListModel(QObject* parent)
    : GcodePreviewListModel(gcode::GCodeVisualType::gvt_structure, parent) {}

void GcodeStructureListModel::checkItem(int type, bool checked) {
    QSettings setting;
  for (auto const& data : data_list_) {
    if (data.type == type) {
        setting.beginGroup(QStringLiteral("GCodePreviw_Struct"));
        setting.setValue(data.name, checked);
        setting.endGroup();
      Q_EMIT itemCheckedChanged(type, checked);
    }
  }
}

void GcodeStructureListModel::setDataList(const QList<GcodeStructureData>& data_list) {
  beginResetModel();
  data_list_ = data_list;
  endResetModel();
}

void GcodeStructureListModel::setTimeParts(const gcode::TimeParts& time_parts) {
  auto const outer_wall_time      = static_cast<int>(time_parts.OuterWall);
  auto const inner_wall_time      = static_cast<int>(time_parts.InnerWall);
  auto const skin_time            = static_cast<int>(time_parts.Skin);
  auto const support_time         = static_cast<int>(time_parts.Support);
  auto const skirt_brim_time      = static_cast<int>(time_parts.SkirtBrim);
  auto const infill_time          = static_cast<int>(time_parts.Infill);
  auto const support_infill_time  = static_cast<int>(time_parts.SupportInfill);
  auto const move_combing_time    = static_cast<int>(time_parts.MoveCombing);
  auto const move_retraction_time = static_cast<int>(time_parts.MoveRetraction);
  auto const prime_tower_time     = static_cast<int>(time_parts.PrimeTower);

  auto total_time = time_parts.OuterWall
                  + time_parts.InnerWall
                  + time_parts.Skin
                  + time_parts.Support
                  + time_parts.SkirtBrim
                  + time_parts.Infill
                  + time_parts.SupportInfill
                  + time_parts.MoveCombing
                  + time_parts.MoveRetraction
                  + time_parts.PrimeTower;

  if (total_time == 0) {
    total_time = 1;
  }

  auto const outer_wall_percent      = time_parts.OuterWall      / total_time * 100.0;
  auto const inner_wall_percent      = time_parts.InnerWall      / total_time * 100.0;
  auto const skin_percent            = time_parts.Skin           / total_time * 100.0;
  auto const support_percent         = time_parts.Support        / total_time * 100.0;
  auto const skirt_brim_percent      = time_parts.SkirtBrim      / total_time * 100.0;
  auto const infill_percent          = time_parts.Infill         / total_time * 100.0;
  auto const support_infill_percent  = time_parts.SupportInfill  / total_time * 100.0;
  auto const move_combing_percent    = time_parts.MoveCombing    / total_time * 100.0;
  auto const move_retraction_percent = time_parts.MoveRetraction / total_time * 100.0;
  auto const prime_tower_percent     = time_parts.PrimeTower     / total_time * 100.0;

  static const auto sec2str = [](unsigned int secs) {
    unsigned int const mins = secs / 60;
    unsigned int const hors = mins / 60;

    unsigned int const sec = secs % 60 % 60;
    unsigned int const min = mins % 60;
    unsigned int const hor = hors;

    if (hor != 0) {
      return QStringLiteral("%1h%2m%3s").arg(hor).arg(min).arg(sec);
    } else if (min != 0) {
      return QStringLiteral("%1m%2s").arg(min).arg(sec);
    } else {
      return QStringLiteral("%1s").arg(sec);
    }
  };


  QList<GcodeStructureData> data_list{
    { QColor{ QStringLiteral("#772D28") }, QStringLiteral("Outer Perimeter") , sec2str(outer_wall_time)        , outer_wall_percent     , 1 , true  },
    { QColor{ QStringLiteral("#028C05") }, QStringLiteral("Inner Perimeter") , sec2str(inner_wall_time)        , inner_wall_percent     , 2 , true  },
    { QColor{ QStringLiteral("#FFB27F") }, QStringLiteral("Skin")            , sec2str(skin_time)              , skin_percent           , 3 , true  },
    { QColor{ QStringLiteral("#058C8C") }, QStringLiteral("Support")         , sec2str(support_time)           , support_percent        , 4 , true  },
    { QColor{ QStringLiteral("#511E54") }, QStringLiteral("SkirtBrim")       , sec2str(skirt_brim_time)        , skirt_brim_percent     , 5 , true  },
    { QColor{ QStringLiteral("#E5DB33") }, QStringLiteral("Infill")          , sec2str(infill_time)            , infill_percent         , 6 , true  },
    { QColor{ QStringLiteral("#B5BC38") }, QStringLiteral("SupportInfill")   , sec2str(support_infill_time)    , support_infill_percent , 7 , true  },
    { QColor{ QStringLiteral("#339919") }, QStringLiteral("PrimeTower")      , sec2str(prime_tower_time)       , prime_tower_percent    , 11, true  },
    { QColor{ QStringLiteral("#60595F") }, QStringLiteral("Travel")          , sec2str(move_combing_time)      , move_combing_percent   , 13, false },
    { QColor{ QStringLiteral("#FFFFFF") }, QStringLiteral("Zseam")           , sec2str(0)                      , 0.0                    , 17, true  },
    { QColor{ QStringLiteral("#FF00FF") }, QStringLiteral("Retraction")      , sec2str(move_retraction_time), move_retraction_percent, 18, false },
  };
  QSettings setting;
  for (auto &data : data_list) {
      
      setting.beginGroup(QStringLiteral("GCodePreviw_Struct"));
      auto checked = setting.value(data.name, data.checked).toBool();
      setting.endGroup();
      data.checked = checked;
      Q_EMIT itemCheckedChanged(data.type, checked);
  }
  beginResetModel();
  data_list_ = std::move(data_list);
  endResetModel();
}

int GcodeStructureListModel::rowCount(const QModelIndex& parent) const {
  return data_list_.size();
}

QHash<int, QByteArray> GcodeStructureListModel::roleNames() const {
  return {
    { static_cast<int>(DataRole::COLOR)  , QByteArrayLiteral("model_color")   },
    { static_cast<int>(DataRole::NAME)   , QByteArrayLiteral("model_name")    },
    { static_cast<int>(DataRole::TIME)   , QByteArrayLiteral("model_time")    },
    { static_cast<int>(DataRole::PERCENT), QByteArrayLiteral("model_percent") },
    { static_cast<int>(DataRole::TYPE)   , QByteArrayLiteral("model_type")    },
    { static_cast<int>(DataRole::CHECKED), QByteArrayLiteral("model_checked") },
  };
}

QVariant GcodeStructureListModel::data(const QModelIndex& index, int role) const {
  QVariant result{ QVariant::Type::Invalid };
  if (index.row() < 0 || index.row() >= rowCount()) {
    return result;
  }

  auto const& data = data_list_[index.row()];
  switch (static_cast<DataRole>(role)) {
    case DataRole::COLOR:
      result = QVariant::fromValue(data.color);
      break;
    case DataRole::NAME:
      result = QVariant::fromValue(data.name);
      break;
    case DataRole::TIME:
      result = QVariant::fromValue(data.time);
      break;
    case DataRole::PERCENT:
      result = QVariant::fromValue(data.percent);
      break;
    case DataRole::TYPE:
      result = QVariant::fromValue(data.type);
      break;
    case DataRole::CHECKED:
      result = QVariant::fromValue(data.checked);
      break;
    default:
      break;
  }

  return result;
}

}  // namespace cxgcode
