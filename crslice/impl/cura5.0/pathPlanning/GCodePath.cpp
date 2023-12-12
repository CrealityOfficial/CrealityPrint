//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "pathPlanning/GCodePath.h"
#include "settings/GCodePathConfig.h"

namespace cura52
{
    GCodePath::GCodePath(const GCodePathConfig& config, std::string mesh_id, const SpaceFillType space_fill_type, const Ratio flow, const Ratio width_factor, const bool spiralize, const Ratio speed_factor) :
        config(&config),
        mesh_id(mesh_id),
        space_fill_type(space_fill_type),
        flow(flow),
        width_factor(width_factor),
        speed_factor(speed_factor),
        speed_back_pressure_factor(1.0),
        retract(false),
        unretract_before_last_travel_move(false),
        perform_z_hop(false),
        perform_prime(false),
        skip_agressive_merge_hint(false),
        points(std::vector<Point>()),
        done(false),
        spiralize(spiralize),
        fan_speed(GCodePathConfig::FAN_SPEED_DEFAULT),
		retract_move_icount(0),
        estimates(TimeMaterialEstimates())
    {
        speedSlowDownPath = 0.0;
        entireLayerSlowdown = false;
    }

    bool GCodePath::isTravelPath() const
    {
        return config->isTravelPath();
    }

    double GCodePath::getExtrusionMM3perMM() const
    {
        return flow * width_factor * config->getExtrusionMM3perMM();
    }

    coord_t GCodePath::getLineWidthForLayerView() const
    {
        return flow * width_factor * config->getLineWidth() * config->getFlowRatio();
    }

    void GCodePath::setFanSpeed(double fan_speed)
    {
        this->fan_speed = fan_speed;
    }

    double GCodePath::getFanSpeed() const
    {
        return (fan_speed >= 0 && fan_speed <= 100) ? fan_speed : config->getFanSpeed();
    }

    bool GCodePath::needSlowdown(int level) const
    {
        switch (level)
        {
        case 1:
        {
            return config->type == PrintFeatureType::Infill || config->type == PrintFeatureType::Skin;
        }
        case 2:
        {
            return config->type == PrintFeatureType::Infill || config->type == PrintFeatureType::Skin || config->type == PrintFeatureType::InnerWall;
        }
        default:
            return !isTravelPath();
        }
    }

}
