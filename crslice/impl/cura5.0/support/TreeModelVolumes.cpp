//Copyright (c) 2021 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "communication/sliceDataStorage.h"
#include "TreeModelVolumes.h"
#include "TreeSupportT.h"


namespace cura52
{

    TreeModelVolumes::TreeModelVolumes(const cura52::SliceDataStorage& storage, const Settings& settings)
        : machine_border_(calculateMachineBorderCollision(storage.getMachineBorder()))
        , xy_distance_(settings.get<cura52::coord_t>("support_xy_distance"))
        , xy_distance_overhang(settings.get<cura52::coord_t>("support_xy_distance_overhang"))
        , distance_priority(settings.get<SupportDistPriority>("support_xy_overrides_z"))
        , radius_sample_resolution_(settings.get<cura52::coord_t>("support_tree_collision_resolution"))
    {
        const cura52::coord_t layer_height = settings.get<cura52::coord_t>("layer_height");
        const AngleRadians angle = settings.get<AngleRadians>("support_tree_angle");
        max_move_ = (angle < TAU / 4) ? (cura52::coord_t)(tan(angle) * layer_height) : std::numeric_limits<cura52::coord_t>::max();
        z_distance_layers = round_up_divide(settings.get<cura52::coord_t>("support_top_distance"), layer_height) + 1;
        for (std::size_t layer_idx = 0; layer_idx < storage.support.supportLayers.size(); ++layer_idx)
        {
            constexpr bool include_support = false;
            constexpr bool include_prime_tower = true;
            layer_outlines_.push_back(storage.getLayerOutlines(layer_idx, include_support, include_prime_tower));
        }
    }

    const cura52::Polygons& TreeModelVolumes::getCollision(cura52::coord_t radius, LayerIndex layer_idx) const
    {
        radius = ceilRadius(radius);
        RadiusLayerPair key{ radius, layer_idx };
        const auto it = collision_cache_.find(key);
        if (it != collision_cache_.end())
        {
            return it->second;
        }
        else
        {
            return calculateCollision(key);
        }
    }

    const cura52::Polygons& TreeModelVolumes::getAvoidance(cura52::coord_t radius, LayerIndex layer_idx) const
    {
        radius = ceilRadius(radius);
        RadiusLayerPair key{ radius, layer_idx };
        const auto it = avoidance_cache_.find(key);
        if (it != avoidance_cache_.end())
        {
            return it->second;
        }
        else
        {
            return calculateAvoidance(key);
        }
    }

    const cura52::Polygons& TreeModelVolumes::getInternalModel(cura52::coord_t radius, LayerIndex layer_idx) const
    {
        radius = ceilRadius(radius);
        RadiusLayerPair key{ radius, layer_idx };
        const auto it = internal_model_cache_.find(key);
        if (it != internal_model_cache_.end())
        {
            return it->second;
        }
        else
        {
            return calculateInternalModel(key);
        }
    }

    cura52::coord_t TreeModelVolumes::ceilRadius(cura52::coord_t radius) const
    {
        const auto remainder = radius % radius_sample_resolution_;
        const auto delta = remainder != 0 ? radius_sample_resolution_ - remainder : 0;
        return radius + delta;
    }

    const cura52::Polygons& TreeModelVolumes::calculateCollision(const RadiusLayerPair& key) const
    {
        const cura52::coord_t radius = key.first;
        const LayerIndex layer_idx = key.second;

        cura52::Polygons collision_areas = machine_border_;
        if (layer_idx < static_cast<LayerIndex>(layer_outlines_.size()))
        {
            if (distance_priority == SupportDistPriority::XY_OVERRIDES_Z)
            {
                //If X/Y overrides Z, simply use the X/Y distance as distance to keep away from the model.
                collision_areas = collision_areas.unionPolygons(layer_outlines_[layer_idx].offset(xy_distance_ + radius));
            }
            else if (layer_idx + z_distance_layers < static_cast<LayerIndex>(layer_outlines_.size()))
            {
                //If Z overrides X/Y, use X/Y distance if the surface is vertical, or Min X/Y distance if not.
                cura52::Polygons z_influenced = layer_outlines_[layer_idx + z_distance_layers].difference(layer_outlines_[layer_idx]).offset(xy_distance_); //In-between layers are ignored for performance.
                cura52::Polygons collision_not_overhang = layer_outlines_[layer_idx].offset(xy_distance_); //In places where there is no overhang nearby, use the normal X/Y distance.
                if (!z_influenced.empty())
                {
                    collision_not_overhang = collision_not_overhang.difference(z_influenced);
                }
                cura52::Polygons collision_model = collision_not_overhang.unionPolygons(layer_outlines_[layer_idx].offset(xy_distance_overhang)); //Apply the minimum distance everywhere else.
                collision_areas = collision_areas.unionPolygons(collision_model.offset(radius));
            }
        }
        const auto ret = collision_cache_.insert({ key, std::move(collision_areas) });
        assert(ret.second);
        return ret.first->second;
    }

    const cura52::Polygons& TreeModelVolumes::calculateAvoidance(const RadiusLayerPair& key) const
    {
        const auto& radius = key.first;
        const auto& layer_idx = key.second;

        if (layer_idx == 0)
        {
            avoidance_cache_[key] = getCollision(radius, 0);
            return avoidance_cache_[key];
        }

        // Avoidance for a given layer depends on all layers beneath it so could have very deep recursion depths if
        // called at high layer heights. We can limit the reqursion depth to N by checking if the if the layer N
        // below the current one exists and if not, forcing the calculation of that layer. This may cause another recursion
        // if the layer at 2N below the current one but we won't exceed our limit unless there are N*N uncalculated layers
        // below our current one.
        constexpr auto max_recursion_depth = 100;
        // Check if we would exceed the recursion limit by trying to process this layer
        if (layer_idx >= max_recursion_depth
            && avoidance_cache_.find({ radius, layer_idx - max_recursion_depth }) == avoidance_cache_.end())
        {
            // Force the calculation of the layer `max_recursion_depth` below our current one, ignoring the result.
            getAvoidance(radius, layer_idx - max_recursion_depth);
        }
        auto avoidance_areas = getAvoidance(radius, layer_idx - 1).offset(-max_move_).smooth(5);
        avoidance_areas = avoidance_areas.unionPolygons(getCollision(radius, layer_idx));
        const auto ret = avoidance_cache_.insert({ key, std::move(avoidance_areas) });
        assert(ret.second);
        return ret.first->second;
    }

    const cura52::Polygons& TreeModelVolumes::calculateInternalModel(const RadiusLayerPair& key) const
    {
        const auto& radius = key.first;
        const auto& layer_idx = key.second;

        const auto& internal_areas = getAvoidance(radius, layer_idx).difference(getCollision(radius, layer_idx));
        const auto ret = internal_model_cache_.insert({ key, internal_areas });
        assert(ret.second);
        return ret.first->second;
    }

    cura52::Polygons TreeModelVolumes::calculateMachineBorderCollision(Polygon machine_border)
    {
        cura52::Polygons machine_volume_border;
        machine_volume_border.add(machine_border.offset(MM2INT(1000))); //Put a border of 1m around the print volume so that we don't collide.
        machine_border.reverse(); //Makes the polygon negative so that we subtract the actual volume from the collision area.
        machine_volume_border.add(machine_border);
        return machine_volume_border;
    }

}




