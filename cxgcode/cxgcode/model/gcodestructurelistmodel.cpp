#include "gcodestructurelistmodel.h"
#include "gcode/gcode/sliceline.h"
namespace cxgcode {

GcodeStructureListModel::GcodeStructureListModel(QObject* parent)
    : GcodePreviewListModel(gcode::GCodeVisualType::gvt_structure, parent) {}

void GcodeStructureListModel::checkItem(int type, bool checked) {
  for (auto const& data : data_list_) {
    if (data.type == type) {
      Q_EMIT itemCheckedChanged(type, checked);
    }
  }
}

void GcodeStructureListModel::setDataList(const QList<GcodeStructureData>& data_list) {
  beginResetModel();
  data_list_ = data_list;
  endResetModel();
}
static const auto sec2str = [](unsigned int secs) {
    unsigned int const mins = secs / 60;
    unsigned int const hors = mins / 60;

    unsigned int const sec = secs % 60 % 60;
    unsigned int const min = mins % 60;
    unsigned int const hor = hors;

    if (hor != 0) {
        return QStringLiteral("%1h%2m%3s").arg(hor).arg(min).arg(sec);
    }
    else if (min != 0) {
        return QStringLiteral("%1m%2s").arg(min).arg(sec);
    }
    else {
        return QStringLiteral("%1s").arg(sec);
    }
};
void GcodeStructureListModel::setOrcaTimeParts(std::vector<std::pair<int, float>> time_parts,int total_time)
{
    float outer_wall_time = 0;
    float inner_wall_time = 0;
    float overhang_time = 0;
    float solid_infill_time = 0;
    float internal_infill_time = 0;
    float top_solid_infill_time = 0;
    float bottom_solid_infill_time = 0;
    float support_time = 0;
    float skirt_time = 0;
    float brim_time = 0;
    float infill_time = 0;
    float bridge_infill = 0;
    float support_infill_time = 0;
    float support_material = 0;
    float support_material_interface = 0;
    float support_transition = 0;
    float wipe_tower = 0;
    float custom = 0;
    float noop = 0;
    float retract = 0;
    float unretract = 0;
    float seam = 0;
    float tool_change = 0;
    float color_change = 0;
    float pause_Print = 0;
    float custom_gcode = 0;
    float travel = 0;
    float wipe = 0;
    float extrude = 0;
    float ironing = 0;
    float internalBridge=0;
    float gapfill_time=0;
    QList<GcodeStructureData> data_list;
    for (int i = 0; i < time_parts.size(); i++)
    {
        std::pair<int, float> pair = time_parts[i];
        
        switch (static_cast<SliceLineType>(pair.first))
        {
           case SliceLineType::erPerimeter:
               inner_wall_time = static_cast<float>(pair.second);
               data_list.append({QColor{ QStringLiteral("#FFE64D") }, QStringLiteral("Inner Perimeter") , sec2str(inner_wall_time)        , inner_wall_time/total_time * 100.0     , static_cast<int>(SliceLineType::erPerimeter) , true  });
               break;
           case SliceLineType::erExternalPerimeter:
               outer_wall_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#FF7D38") }, QStringLiteral("Outer Perimeter") , sec2str(outer_wall_time)        , outer_wall_time/total_time * 100.0     , static_cast<int>(SliceLineType::erExternalPerimeter) , true  });
               break;
           case SliceLineType::erOverhangPerimeter:
               overhang_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#0000FF") }, QStringLiteral("Overhang Wall")            , sec2str(overhang_time)              , overhang_time/total_time * 100.0         , static_cast<int>(SliceLineType::erOverhangPerimeter) , true });
               break;
           case SliceLineType::erInternalInfill:
               internal_infill_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#B03029") }, QStringLiteral("Sparse infill")            , sec2str(internal_infill_time)              , internal_infill_time/total_time * 100.0         , static_cast<int>(SliceLineType::erInternalInfill) , true });
               break;
           case SliceLineType::erSolidInfill:
               solid_infill_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#9654CC") }, QStringLiteral("Internal solid infill")         , sec2str(solid_infill_time)           , solid_infill_time/ total_time * 100.0       , static_cast<int>(SliceLineType::erSolidInfill) , true });
               break;
           case SliceLineType::erTopSolidInfill:
               top_solid_infill_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#F04040") }, QStringLiteral("Top surface"), sec2str(top_solid_infill_time), top_solid_infill_time / total_time * 100.0, static_cast<int>(SliceLineType::erTopSolidInfill), true  });
               break;
           case SliceLineType::erBottomSurface:
               bottom_solid_infill_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#665CC7") }, QStringLiteral("Bottom surface"), sec2str(bottom_solid_infill_time), bottom_solid_infill_time/ total_time * 100.0, static_cast<int>(SliceLineType::erBottomSurface), true });
               break;
           case SliceLineType::erIroning:
               ironing = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#FF8C69") }, QStringLiteral("Ironing"), sec2str(ironing), ironing / total_time * 100.0, static_cast<int>(SliceLineType::erIroning), true });
               break;
          case SliceLineType::erBridgeInfill:
               bridge_infill = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#4D66A1") }, QStringLiteral("Bridge"), sec2str(bridge_infill), bridge_infill / total_time * 100.0, static_cast<int>(SliceLineType::erBridgeInfill), true });
               break;
           case SliceLineType::erSkirt:
               skirt_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#00876E") }, QStringLiteral("Skirt"), sec2str(skirt_time), skirt_time / total_time * 100.0, static_cast<int>(SliceLineType::erSkirt), true });
               
               break;
           case SliceLineType::erBrim:
               brim_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#003B6E") }, QStringLiteral("Brim"), sec2str(brim_time), brim_time / total_time * 100.0, static_cast<int>(SliceLineType::erBrim), true });

               break;
           case SliceLineType::erGapFill:
               gapfill_time = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#FFFFFF") }, QStringLiteral("Gap Fill"), sec2str(brim_time), brim_time / total_time * 100.0, static_cast<int>(SliceLineType::erGapFill), true });

               break;
           case SliceLineType::erSupportMaterial:
               support_material = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#00FF00") }, QStringLiteral("Support"), sec2str(support_material), support_material / total_time * 100.0, static_cast<int>(SliceLineType::erSupportMaterial), true });
               break;
           case SliceLineType::erSupportMaterialInterface:
               support_material_interface = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#008000") }, QStringLiteral("Support interface"), sec2str(support_material_interface), support_material_interface / total_time * 100.0, static_cast<int>(SliceLineType::erSupportMaterialInterface), true });
               break;
           case SliceLineType::erSupportTransition:
               support_transition = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#004000") }, QStringLiteral("Support transition"), sec2str(support_transition), support_transition / total_time * 100.0, static_cast<int>(SliceLineType::erSupportTransition), true });
               break;
           case SliceLineType::erWipeTower:
               wipe_tower = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#B3E3AB") }, QStringLiteral("Prime tower"), sec2str(wipe_tower), wipe_tower / total_time * 100.0, static_cast<int>(SliceLineType::erWipeTower), true });
               break;
           case SliceLineType::erCustom:
               custom = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#5ED194") }, QStringLiteral("Custom"), sec2str(custom), custom / total_time * 100.0, static_cast<int>(SliceLineType::erCustom), true });
               break;
           case SliceLineType::erInternalBridgeInfill:
               internalBridge = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#4D80BA") }, QStringLiteral("Internal Bridge"), sec2str(internalBridge), internalBridge / total_time * 100.0, static_cast<int>(SliceLineType::erInternalBridgeInfill), true });
               break;
           case SliceLineType::Noop:
               noop = static_cast<float>(pair.second);
               break;
           case SliceLineType::Retract:
               retract = static_cast<float>(pair.second);
               
               break;
           case SliceLineType::Unretract:
               unretract = static_cast<float>(pair.second);
               
               break;
           case SliceLineType::Seam:
               seam = static_cast<float>(pair.second);
               
               break;
           case SliceLineType::Tool_change:
               tool_change = static_cast<float>(pair.second);
               break;
           case SliceLineType::Color_change:
               color_change = static_cast<float>(pair.second);
               break;
           case SliceLineType::Pause_Print:
               pause_Print = static_cast<float>(pair.second);
               break;
           case SliceLineType::Custom_GCode:
               custom_gcode = static_cast<float>(pair.second);
               break;
           case SliceLineType::Travel:
               travel = static_cast<float>(pair.second);
               data_list.append({ QColor{ QStringLiteral("#38489B") }, QStringLiteral("Travel"), sec2str(travel), travel/ total_time * 100.0, static_cast<int>(SliceLineType::Travel), false });
               break;
           case SliceLineType::Wipe:
               wipe = static_cast<float>(pair.second);
               
               break;
           case SliceLineType::Extrude:
               extrude = static_cast<float>(pair.second);
               break;
           default:
               break;
        }
    }
    data_list.append({ QColor{ QStringLiteral("#CD22D6") }, QStringLiteral("Retract"), 0, 0, static_cast<int>(SliceLineType::Retract), false });
    data_list.append({ QColor{ QStringLiteral("#49ADCF") }, QStringLiteral("Unretract"), 0, 0, static_cast<int>(SliceLineType::Unretract), false });
    data_list.append({ QColor{ QStringLiteral("#FFFF00") }, QStringLiteral("Wipe"), 0, 0, static_cast<int>(SliceLineType::Wipe), false });
    data_list.append({ QColor{ QStringLiteral("#FFFFFF") }, QStringLiteral("Seams"), 0, 0, static_cast<int>(SliceLineType::Seam), true });

    beginResetModel();
    data_list_ = std::move(data_list);
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
