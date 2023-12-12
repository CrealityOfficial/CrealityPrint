//Copyright (c) 2021 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef TREEMODELVOLUMES_H
#define TREEMODELVOLUMES_H

#include <unordered_map>
#include "TreeSupportSettings.h"
#include "types/EnumSettings.h" //To store whether X/Y or Z distance gets priority.
#include "types/LayerIndex.h" //Part of the RadiusLayerPair.
#include "utils/polygon.h" //For polygon parameters.
#include <unordered_set>
#include <mutex>
#include "utils/Simplify.h"


namespace cura52
{
    class SliceDataStorage;
    class Settings;

    /*!
     * \brief Lazily generates tree guidance volumes.
     *
     * \warning This class is not currently thread-safe and should not be accessed in OpenMP blocks
     */
    class TreeModelVolumes
    {
    public:
        TreeModelVolumes() = default;
        /*!
         * \brief Construct the TreeModelVolumes object
         *
         * \param storage The slice data storage object to extract the model
         * contours from.
         * \param settings The settings object to get relevant settings from.
         * \param xy_distance The required clearance between the model and the
         * tree branches.
         * \param max_move The maximum allowable movement between nodes on
         * adjacent layers
         * \param radius_sample_resolution Sample size used to round requested node radii.
         */
        TreeModelVolumes(const SliceDataStorage& storage, const Settings& settings);

        TreeModelVolumes(TreeModelVolumes&& original) = default;
        TreeModelVolumes& operator=(TreeModelVolumes&& original) = default;

        TreeModelVolumes(const TreeModelVolumes& original) = delete;
        TreeModelVolumes& operator=(const TreeModelVolumes& original) = delete;

        /*!
         * \brief Creates the areas that have to be avoided by the tree's branches.
         *
         * The result is a 2D area that would cause nodes of radius \p radius to
         * collide with the model.
         *
         * \param radius The radius of the node of interest
         * \param layer The layer of interest
         * \return cura52::Polygons object
         */
        const cura52::Polygons& getCollision(cura52::coord_t radius, cura52::LayerIndex layer_idx) const;

        /*!
         * \brief Creates the areas that have to be avoided by the tree's branches
         * in order to reach the build plate.
         *
         * The result is a 2D area that would cause nodes of radius \p radius to
         * collide with the model or be unable to reach the build platform.
         *
         * The input collision areas are inset by the maximum move distance and
         * propagated upwards.
         *
         * \param radius The radius of the node of interest
         * \param layer The layer of interest
         * \return cura52::Polygons object
         */
        const cura52::Polygons& getAvoidance(cura52::coord_t radius, cura52::LayerIndex layer_idx) const;

        /*!
         * \brief Generates the area of a given layer that must be avoided if the
         * branches wish to go towards the model
         *
         * The area represents the areas that do not collide with the model but
         * are unable to reach the build platform
         *
         * \param radius The radius of the node of interest
         * \param layer The layer of interest
         * \return cura52::Polygons object
         */
        const cura52::Polygons& getInternalModel(cura52::coord_t radius, cura52::LayerIndex layer_idx) const;

    private:
        /*!
         * \brief Convenience typedef for the keys to the caches
         */
        using RadiusLayerPair = std::pair<cura52::coord_t, cura52::LayerIndex>;

        /*!
         * \brief Round \p radius upwards to a multiple of radius_sample_resolution_
         *
         * \param radius The radius of the node of interest
         */
        cura52::coord_t ceilRadius(cura52::coord_t radius) const;

        /*!
         * \brief Calculate the collision areas at the radius and layer indicated
         * by \p key.
         *
         * \param key The radius and layer of the node of interest
         */
        const cura52::Polygons& calculateCollision(const RadiusLayerPair& key) const;

        /*!
         * \brief Calculate the avoidance areas at the radius and layer indicated
         * by \p key.
         *
         * \param key The radius and layer of the node of interest
         */
        const cura52::Polygons& calculateAvoidance(const RadiusLayerPair& key) const;

        /*!
         * \brief Calculate the internal model areas at the radius and layer
         * indicated by \p key.
         *
         * \param key The radius and layer of the node of interest
         */
        const cura52::Polygons& calculateInternalModel(const RadiusLayerPair& key) const;

        /*!
         * \brief Calculate the collision area around the printable area of the machine.
         *
         * \param a cura52::Polygons object representing the non-printable areas on and around the build platform
         */
        static cura52::Polygons calculateMachineBorderCollision(Polygon machine_border);

        /*!
         * \brief cura52::Polygons representing the limits of the printable area of the
         * machine
         */
        cura52::Polygons machine_border_;

        /*!
         * \brief The required clearance between the model and the tree branches
         */
        cura52::coord_t xy_distance_;

        /*!
         * The minimum X/Y distance between the model and the tree branches.
         *
         * Used only if the Z distance overrides the X/Y distance and in places that
         * are near the surface where the Z distance applies.
         */
        cura52::coord_t xy_distance_overhang;

        /*!
         * The number of layers of spacing to hold as Z distance.
         *
         * This determines where the overhang X/Y distance is used, if the Z
         * distance overrides the X/Y distance.
         */
        int z_distance_layers;

        /*!
         * The priority of X/Y distance over Z distance.
         */
        SupportDistPriority distance_priority;

        /*!
         * \brief The maximum distance that the centrepoint of a tree branch may
         * move in consequtive layers
         */
        cura52::coord_t max_move_;

        /*!
         * \brief Sample resolution for radius values.
         *
         * The radius will be rounded (upwards) to multiples of this value before
         * calculations are done when collision, avoidance and internal model
         * cura52::Polygons are requested.
         */
        cura52::coord_t radius_sample_resolution_;

        /*!
         * \brief Storage for layer outlines of the meshes.
         */
        std::vector<cura52::Polygons> layer_outlines_;

        /*!
         * \brief Caches for the collision, avoidance and internal model polygons
         * at given radius and layer indices.
         *
         * These are mutable to allow modification from const function. This is
         * generally considered OK as the functions are still logically const
         * (ie there is no difference in behaviour for the user betweeen
         * calculating the values each time vs caching the results).
         */
        mutable std::unordered_map<RadiusLayerPair, cura52::Polygons> collision_cache_;
        mutable std::unordered_map<RadiusLayerPair, cura52::Polygons> avoidance_cache_;
        mutable std::unordered_map<RadiusLayerPair, cura52::Polygons> internal_model_cache_;
    };

}

#endif //TREEMODELVOLUMES_H
