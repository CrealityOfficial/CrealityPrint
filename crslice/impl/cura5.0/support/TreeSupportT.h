//Copyright (c) 2021 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef TREESUPPORTT_H
#define TREESUPPORTT_H

#include "TreeModelVolumesT.h"
#include "TreeSupportBaseCircleT.h"
#include "TreeSupportElementT.h"
#include "TreeSupportEnums.h"
#include "TreeSupportSettingsT.h"
#include "boost/functional/hash.hpp" // For combining hashes
#include "polyclipping/clipper.hpp"
#include "types/EnumSettings.h"
#include "communication/sliceDataStorage.h"
#include "utils/Coord_t.h"
#include "utils/polygon.h"

namespace cura54
{

    // The various stages of the process can be weighted differently in the progress bar.
    // These weights are obtained experimentally using a small sample size. Sensible weights can differ drastically based on the assumed default settings and model.
    constexpr auto TREE_PROGRESS_TOTAL = 10000;
    constexpr auto TREE_PROGRESS_PRECALC_COLL = TREE_PROGRESS_TOTAL * 0.05;
    constexpr auto TREE_PROGRESS_PRECALC_AVO = TREE_PROGRESS_TOTAL * 0.1;
    constexpr auto TREE_PROGRESS_GENERATE_NODES = TREE_PROGRESS_TOTAL * 0.05;
    constexpr auto TREE_PROGRESS_AREA_CALC = TREE_PROGRESS_TOTAL * 0.7;
    constexpr auto TREE_PROGRESS_DRAW_AREAS = TREE_PROGRESS_TOTAL * 0.1;

    constexpr auto TREE_PROGRESS_GENERATE_BRANCH_AREAS = TREE_PROGRESS_DRAW_AREAS / 3;
    constexpr auto TREE_PROGRESS_SMOOTH_BRANCH_AREAS = TREE_PROGRESS_DRAW_AREAS / 3;
    constexpr auto TREE_PROGRESS_FINALIZE_BRANCH_AREAS = TREE_PROGRESS_DRAW_AREAS / 3;

    constexpr auto SUPPORT_TREE_MINIMUM_FAKE_ROOF_AREA = 100;
    constexpr auto SUPPORT_TREE_MINIMUM_FAKE_ROOF_LAYERS = 1;
    constexpr auto SUPPORT_TREE_MINIMUM_ROOF_AREA_HARD_LIMIT = false;
    constexpr auto SUPPORT_TREE_ONLY_GRACIOUS_TO_MODEL = false;
    constexpr auto SUPPORT_TREE_AVOID_SUPPORT_BLOCKER = true;
    constexpr coord_t SUPPORT_TREE_EXPONENTIAL_THRESHOLD = 1000;
    constexpr auto SUPPORT_TREE_EXPONENTIAL_FACTOR = 1.5;
    constexpr size_t SUPPORT_TREE_PRE_EXPONENTIAL_STEPS = 1;
    constexpr coord_t SUPPORT_TREE_COLLISION_RESOLUTION = 500; // Only has an effect if SUPPORT_TREE_USE_EXPONENTIAL_COLLISION_RESOLUTION is false

    using PropertyAreasUnordered = std::unordered_map<TreeSupportElementT, cura52::Polygons>;
    using PropertyAreas = std::map<TreeSupportElementT, cura52::Polygons>;

    /*!
     * \brief Generates a tree structure to support your models.
     */
    class TreeSupportT
    {
    public:
        /*!
         * \brief Creates an instance of the tree support generator.
         *
         * \param storage The data storage to get global settings from.
         */
        TreeSupportT(const cura52::SliceDataStorage& storage);

        /*!
         * \brief Create the areas that need support.
         *
         * These areas are stored inside the given cura52::SliceDataStorage object.
         * \param storage The data storage where the mesh data is gotten from and
         * where the resulting support areas are stored.
         */
        void generateSupportAreas(cura52::SliceDataStorage& storage);


    private:


        /*!
         * \brief Precalculates all avoidances, that could be required.
         *
         * \param storage[in] Background storage to access meshes.
         * \param currently_processing_meshes[in] Indexes of all meshes that are processed in this iteration
         */
        void precalculate(const cura52::SliceDataStorage& storage, std::vector<size_t> currently_processing_meshes);


        /*!
         * \brief Creates the initial influence areas (that can later be propagated down) by placing them below the overhang.
         *
         * Generates Points where the Model should be supported and creates the areas where these points have to be placed.
         *
         * \param mesh[in] The mesh that is currently processed.
         * \param move_bounds[out] Storage for the influence areas.
         * \param storage[in] Background storage, required for adding roofs.
         */
        void generateInitialAreas(const cura52::SliceMeshStorage& mesh, std::vector<std::set<TreeSupportElementT*>>& move_bounds, cura52::SliceDataStorage& storage);


        /*!
         * \brief Merges Influence Areas if possible.
         *
         * Branches which do overlap have to be merged. This helper merges all elements in input with the elements into reduced_new_layer.
         * Elements in input_aabb are merged together if possible, while elements reduced_new_layer_aabb are not checked against each other.
         *
         * \param reduced_aabb[in,out] The already processed elements.
         * \param input_aabb[in] Not yet processed elements
         * \param to_bp_areas[in] The Elements of the current Layer that will reach the buildplate. Value is the influence area where the center of a circle of support may be placed.
         * \param to_model_areas[in] The Elements of the current Layer that do not have to reach the buildplate. Also contains main as every element that can reach the buildplate is not forced to.
         *     Value is the influence area where the center of a circle of support may be placed.
         * \param influence_areas[in] The influence areas without avoidance removed.
         * \param insert_bp_areas[out] Elements to be inserted into the main dictionary after the Helper terminates.
         * \param insert_model_areas[out] Elements to be inserted into the secondary dictionary after the Helper terminates.
         * \param insert_influence[out] Elements to be inserted into the dictionary containing the largest possibly valid influence area (ignoring if the area may not be there because of avoidance)
         * \param erase[out] Elements that should be deleted from the above dictionaries.
         * \param layer_idx[in] The Index of the current Layer.
         */
        void mergeHelper
        (
            std::map<TreeSupportElementT, cura52::AABB>& reduced_aabb,
            std::map<TreeSupportElementT, cura52::AABB>& input_aabb,
            const PropertyAreasUnordered& to_bp_areas,
            const PropertyAreas& to_model_areas,
            const PropertyAreas& influence_areas,
            PropertyAreasUnordered& insert_bp_areas,
            PropertyAreasUnordered& insert_model_areas,
            PropertyAreasUnordered& insert_influence,
            std::vector<TreeSupportElementT>& erase,
            const cura52::LayerIndex layer_idx
        );

        /*!
         * \brief Merges Influence Areas if possible.
         *
         * Branches which do overlap have to be merged. This manages the helper and uses a divide and conquer approach to parallelize this problem.
         * This parallelization can at most accelerate the merging by a factor of 2.
         *
         * \param to_bp_areas[in] The Elements of the current Layer that will reach the buildplate.
         *  Value is the influence area where the center of a circle of support may be placed.
         * \param to_model_areas[in] The Elements of the current Layer that do not have to reach the buildplate. Also contains main as every element that can reach the buildplate is not forced to.
         *  Value is the influence area where the center of a circle of support may be placed.
         * \param influence_areas[in] The Elements of the current Layer without avoidances removed. This is the largest possible influence area for this layer.
         *  Value is the influence area where the center of a circle of support may be placed.
         * \param layer_idx[in] The current layer.
         */
        void mergeInfluenceAreas
        (
            PropertyAreasUnordered& to_bp_areas,
            PropertyAreas& to_model_areas,
            PropertyAreas& influence_areas,
            cura52::LayerIndex layer_idx
        );

        /*!
         * \brief Checks if an influence area contains a valid subsection and returns the corresponding metadata and the new Influence area.
         *
         * Calculates an influence areas of the layer below, based on the influence area of one element on the current layer.
         * Increases every influence area by maximum_move_distance_slow. If this is not enough, as in we would change our gracious or to_buildplate status the influence areas are instead increased by maximum_move_distance_slow.
         * Also ensures that increasing the radius of a branch, does not cause it to change its status (like to_buildplate ). If this were the case, the radius is not increased instead.
         *
         * Warning: The used format inside this is different as the SupportElement does not have a valid area member. Instead this area is saved as value of the dictionary. This was done to avoid not needed heap allocations.
         *
         * \param settings[in] Which settings have to be used to check validity.
         * \param layer_idx[in] Number of the current layer.
         * \param parent[in] The metadata of the parents influence area.
         * \param relevant_offset[in] The maximal possible influence area. No guarantee regarding validity with current layer collision required, as it is ensured in-function!
         * \param to_bp_data[out] The part of the Influence area that can reach the buildplate.
         * \param to_model_data[out] The part of the Influence area that do not have to reach the buildplate. This has overlap with new_layer_data.
         * \param increased[out]  Area than can reach all further up support points. No assurance is made that the buildplate or the model can be reached in accordance to the user-supplied settings.
         * \param overspeed[in] How much should the already offset area be offset again. Usually this is 0.
         * \param mergelayer[in] Will the merge method be called on this layer. This information is required as some calculation can be avoided if they are not required for merging.
         * \return A valid support element for the next layer regarding the calculated influence areas. Empty if no influence are can be created using the supplied influence area and settings.
         */
        std::optional<TreeSupportElementT> increaseSingleArea
        (
            AreaIncreaseSettings settings,
            cura52::LayerIndex layer_idx,
            TreeSupportElementT* parent,
            const cura52::Polygons& relevant_offset,
            cura52::Polygons& to_bp_data,
            cura52::Polygons& to_model_data,
            cura52::Polygons& increased,
            const coord_t overspeed,
            const bool mergelayer
        );

        /*!
         * \brief Increases influence areas as far as required.
         *
         * Calculates influence areas of the layer below, based on the influence areas of the current layer.
         * Increases every influence area by maximum_move_distance_slow. If this is not enough, as in it would change the gracious or to_buildplate status, the influence areas are instead increased by maximum_move_distance.
         * Also ensures that increasing the radius of a branch, does not cause it to change its status (like to_buildplate ). If this were the case, the radius is not increased instead.
         *
         * Warning: The used format inside this is different as the SupportElement does not have a valid area member. Instead this area is saved as value of the dictionary. This was done to avoid not needed heap allocations.
         *
         * \param to_bp_areas[out] Influence areas that can reach the buildplate
         * \param to_model_areas[out] Influence areas that do not have to reach the buildplate. This has overlap with new_layer_data, as areas that can reach the buildplate are also considered valid areas to the model.
         *     This redundancy is required if a to_buildplate influence area is allowed to merge with a to model influence area.
         * \param influence_areas[out] Area than can reach all further up support points. No assurance is made that the buildplate or the model can be reached in accordance to the user-supplied settings.
         * \param bypass_merge_areas[out] Influence areas ready to be added to the layer below that do not need merging.
         * \param last_layer[in] Influence areas of the current layer.
         * \param layer_idx[in] Number of the current layer.
         * \param mergelayer[in] Will the merge method be called on this layer. This information is required as some calculation can be avoided if they are not required for merging.
         */
        void increaseAreas
        (
            PropertyAreasUnordered& to_bp_areas,
            PropertyAreas& to_model_areas,
            PropertyAreas& influence_areas,
            std::vector<TreeSupportElementT*>& bypass_merge_areas,
            const std::vector<TreeSupportElementT*>& last_layer,
            const cura52::LayerIndex layer_idx,
            const bool mergelayer
        );

        /*!
         * \brief Propagates influence downwards, and merges overlapping ones.
         *
         * \param move_bounds[in,out] All currently existing influence areas
         */
        void createLayerPathing(std::vector<std::set<TreeSupportElementT*>>& move_bounds);


        /*!
         * \brief Sets the result_on_layer for all parents based on the SupportElement supplied.
         *
         * \param elem[in] The SupportElements, which parent's position should be determined.
         */
        void setPointsOnAreas(const TreeSupportElementT* elem);

        /*!
         * \brief Get the best point to connect to the model and set the result_on_layer of the relevant SupportElement accordingly.
         *
         * \param move_bounds[in,out] All currently existing influence areas
         * \param first_elem[in,out] SupportElement that did not have its result_on_layer set meaning that it does not have a child element.
         * \param layer_idx[in] The current layer.
         * \return Should elem be deleted.
         */
        bool setToModelContact(std::vector<std::set<TreeSupportElementT*>>& move_bounds, TreeSupportElementT* first_elem, const cura52::LayerIndex layer_idx);

        /*!
         * \brief Set the result_on_layer point for all influence areas
         *
         * \param move_bounds[in,out] All currently existing influence areas
         */
        void createNodesFromArea(std::vector<std::set<TreeSupportElementT*>>& move_bounds);

        /*!
         * \brief Draws circles around result_on_layer points of the influence areas
         *
         * \param linear_data[in] All currently existing influence areas with the layer they are on
         * \param layer_tree_cura52::Polygons[out] Resulting branch areas with the cura52::LayerIndex they appear on.
         *    layer_tree_cura52::Polygons.size() has to be at least linear_data.size() as each Influence area in linear_data will save have at least one (that's why it's a vector<vector>) corresponding branch area in layer_tree_cura52::Polygons.
         * \param inverse_tree_order[in] A mapping that returns the child of every influence area.
         */
        void generateBranchAreas
        (
            std::vector<std::pair<cura52::LayerIndex, TreeSupportElementT*>>& linear_data,
            std::vector<std::unordered_map<TreeSupportElementT*, cura52::Polygons>>& layer_tree_polygons,
            const std::map<TreeSupportElementT*, TreeSupportElementT*>& inverse_tree_order
        );

        /*!
         * \brief Applies some smoothing to the outer wall, intended to smooth out sudden jumps as they can happen when a branch moves though a hole.
         *
         * \param layer_tree_cura52::Polygons[in,out] Resulting branch areas with the cura52::LayerIndex they appear on.
         */
        void smoothBranchAreas(std::vector<std::unordered_map<TreeSupportElementT*, cura52::Polygons>>& layer_tree_polygons);

        /*!
         * \brief Drop down areas that do rest non-gracefully on the model to ensure the branch actually rests on something.
         *
         * \param layer_tree_cura52::Polygons[in] Resulting branch areas with the cura52::LayerIndex they appear on.
         * \param linear_data[in] All currently existing influence areas with the layer they are on
         * \param dropped_down_areas[out] Areas that have to be added to support all non-graceful areas.
         * \param inverse_tree_order[in] A mapping that returns the child of every influence area.
         */
        void dropNonGraciousAreas
        (
            std::vector<std::unordered_map<TreeSupportElementT*, cura52::Polygons>>& layer_tree_polygons,
            const std::vector<std::pair<cura52::LayerIndex, TreeSupportElementT*>>& linear_data,
            std::vector<std::vector<std::pair<cura52::LayerIndex, cura52::Polygons>>>& dropped_down_areas,
            const std::map<TreeSupportElementT*, TreeSupportElementT*>& inverse_tree_order
        );


        void filterFloatingLines(std::vector<cura52::Polygons>& support_layer_storage);

        /*!
         * \brief Generates Support Floor, ensures Support Roof can not cut of branches, and saves the branches as support to storage
         *
         * \param support_layer_storage[in] Areas where support should be generated.
         * \param support_roof_storage[in] Areas where support was replaced with roof.
         * \param storage[in,out] The storage where the support should be stored.
         */
        void finalizeInterfaceAndSupportAreas(std::vector<cura52::Polygons>& support_layer_storage, std::vector<cura52::Polygons>& support_roof_storage, cura52::SliceDataStorage& storage);

        /*!
         * \brief Draws circles around result_on_layer points of the influence areas and applies some post processing.
         *
         * \param move_bounds[in] All currently existing influence areas
         * \param storage[in,out] The storage where the support should be stored.
         */
        void drawAreas(std::vector<std::set<TreeSupportElementT*>>& move_bounds, cura52::SliceDataStorage& storage);

        /*!
         * \brief Settings with the indexes of meshes that use these settings.
         */
        std::vector<std::pair<TreeSupportSettingsT, std::vector<size_t>>> grouped_meshes;

        /*!
         * \brief Areas that should have been support roof, but where the roof settings would not allow any lines to be generated.
         */
        std::vector<cura52::Polygons> additional_required_support_area;

        /*!
         * \brief A representation of already placed lines. Required for subtracting from new support areas.
         */
        std::vector<cura52::Polygons> placed_support_lines_support_areas;

        /*!
         * \brief Generator for model collision, avoidance and internal guide volumes.
         *
         */
        TreeModelVolumesT volumes_;

        /*!
         * \brief Contains config settings to avoid loading them in every function. This was done to improve readability of the code.
         */
        TreeSupportSettingsT config;

        /*!
         * \brief The progress multiplier of all values added progress bar.
         * Required for the progress bar the behave as expected when areas have to be calculated multiple times
         */
        double progress_multiplier = 1;

        /*!
         * \brief The progress offset added to all values communicated to the progress bar.
         * Required for the progress bar the behave as expected when areas have to be calculated multiple times
         */
        double progress_offset = 0;
        cura52::SliceContext* application = nullptr;
    };
} // namespace cura


#endif /* TREESUPPORT_H */
