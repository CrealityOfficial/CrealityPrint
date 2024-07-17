#include "crslice2/gcodeprocessor.h"
#include "libslic3r/GCode/GCodeProcessor.hpp"
namespace crslice2
{
    enum class SliceLineType
    {
        NoneType = 0, // used to mark unspecified jumps in polygons. libArcus depends on it
                                  erPerimeter = 20,
                                  erExternalPerimeter = 1,
                                  erOverhangPerimeter = 22,
                                  erInternalInfill,
                                  erSolidInfill,
                                  erTopSolidInfill,
                                  erBottomSurface,
                                  erIroning,
                                  erBridgeInfill,
                                  erGapFill,
                                  erSkirt,
                                  erBrim,
                                  erSupportMaterial,
                                  erSupportMaterialInterface,
                                  erSupportTransition,
                                  erWipeTower,
                                  erCustom,
                                  erMixed,
                                  Noop = 40,
                                  Retract = 14,
                                  Unretract = 42,
                                  Seam,
                                  Tool_change,
                                  Color_change,
                                  Pause_Print,
                                  Custom_GCode,
                                  //Travel = 13,
                                  Wipe = 49,
                                  Extrude,
                                  erInternalBridgeInfill,
                                  erCount


    };
    void deal_roles_times(const std::vector<std::pair<Slic3r::ExtrusionRole, float>>& moves_times, std::vector<std::vector<std::pair<int, float>>>& times)
    {
        for (auto& custom_gcode_time : moves_times)
        {
            auto funPushTimes = [](int role,float time, std::vector<std::vector<std::pair<int, float>>>& times) {
                std::pair<int, float> _pair;
                _pair.first = role;
                _pair.second = time;
                times.back().push_back(_pair);
            };
            int role = 1;
            switch (custom_gcode_time.first)
            {
            case Slic3r::ExtrusionRole::erPerimeter:
                role = static_cast<int>(SliceLineType::erPerimeter);
                funPushTimes(role,custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erExternalPerimeter:
                role = static_cast<int>(SliceLineType::erExternalPerimeter);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erOverhangPerimeter:
                role = static_cast<int>(SliceLineType::erOverhangPerimeter);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erInternalInfill:
                role = static_cast<int>(SliceLineType::erInternalInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erSolidInfill:
                role = static_cast<int>(SliceLineType::erSolidInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erTopSolidInfill:
                role = static_cast<int>(SliceLineType::erTopSolidInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erBottomSurface:
                role = static_cast<int>(SliceLineType::erBottomSurface);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erIroning:
                role = static_cast<int>(SliceLineType::erIroning);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erBridgeInfill:
                role = static_cast<int>(SliceLineType::erBridgeInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erInternalBridgeInfill:
                role = static_cast<int>(SliceLineType::erInternalBridgeInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erGapFill:
                role = static_cast<int>(SliceLineType::erGapFill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erSkirt:
                role = static_cast<int>(SliceLineType::erSkirt);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erBrim:
                role = static_cast<int>(SliceLineType::erBrim);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erSupportMaterial:
                role = static_cast<int>(SliceLineType::erSupportMaterial);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erSupportMaterialInterface:
                role = static_cast<int>(SliceLineType::erSupportMaterialInterface);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erSupportTransition:
                role = static_cast<int>(SliceLineType::erSupportTransition);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erWipeTower:
                role = static_cast<int>(SliceLineType::erWipeTower);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erCustom:
                role = static_cast<int>(SliceLineType::erCustom);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erMixed:
                role = static_cast<int>(SliceLineType::erMixed);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case Slic3r::ExtrusionRole::erCount:
                role = static_cast<int>(SliceLineType::erCount);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            default:
                break;
            }
            //if(_pair.first == 2)
            //    _pair.first = 1;
            //else if (_pair.first >0 )
            //    _pair.first += 19;

            
            
        }
    }

    void deal_moves_times(const std::vector<std::pair<Slic3r::EMoveType, float>>& roles_times, std::vector<std::vector<std::pair<int, float>>>& times)
    {
        for (auto& custom_gcode_time : roles_times)
        {
            std::pair<int, float> _pair;
            _pair.first = (int)custom_gcode_time.first;

            if (_pair.first == 1)
            {
                _pair.first =14;
            }
            else if (_pair.first == 8)
            {
                _pair.first = 13;
            }
            else
            {
                _pair.first += 40;
            }
            _pair.second = custom_gcode_time.second;
            times.back().push_back(_pair);
        }
    }

    void process_file(const std::string& file, std::vector<std::vector<std::pair<int, float>>>& times)
    {
        // process gcode
        Slic3r::GCodeProcessor processor;
        try
        {
            processor.process_file(file);

            Slic3r::PrintEstimatedStatistics::ETimeMode mode = Slic3r::PrintEstimatedStatistics::ETimeMode::Normal;
            float time = processor.get_time(mode);
            float prepare_time = processor.get_prepare_time(mode);
            const std::vector<std::pair<Slic3r::CustomGCode::Type, std::pair<float, float>>>& custom_gcode_times = processor.get_custom_gcode_times(mode, true);
            const std::vector<std::pair<Slic3r::EMoveType, float>>& moves_times = processor.get_moves_time(mode);
            const std::vector<std::pair<Slic3r::ExtrusionRole, float>>& roles_times = processor.get_roles_time(mode);
            const std::vector<float>& layers_times = processor.get_layers_time(mode);

            times.push_back(std::vector<std::pair<int, float>>());
            deal_moves_times(moves_times, times);
            deal_roles_times(roles_times, times);

            times.push_back(std::vector<std::pair<int, float>>());
            for (int i = 0; i < layers_times.size();i++)
            {
                std::pair<int, float> _pair;
                _pair.first = i-1;
                _pair.second = layers_times[i];
                times.back().push_back(_pair);
            }

            times.push_back(std::vector<std::pair<int, float>>());
            times.back().push_back(std::pair<int, float>(0, time));
        }
        catch (const std::exception& ex)
        {
            //show_error(this, ex.what());
            return;
        }
    }


}
