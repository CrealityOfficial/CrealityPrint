// Ordering of the tools to minimize tool switches.

#ifndef slic3r_ToolOrdering_hpp_
#define slic3r_ToolOrdering_hpp_

#include "../libslic3r.h"
#include "../MixedFilament.hpp"

#include <utility>
#include <cstdint>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace Slic3r {

class ExtrusionEntity;
class ExtrusionEntityCollection;
class LayerTools;
class Print;
class PrintObject;
class PrintConfig;
class PrintRegion;
namespace CustomGCode { struct Item; }

float flush_volume_from_matrix(const PrintConfig& config,
                               unsigned int old_extruder, unsigned int new_extruder);
float skeleton_wipe_line_width(const Print& print, const PrintRegion& region, unsigned int extruder);
double skeleton_wipe_volume(const ExtrusionEntity& entity, float wipe_line_width);
float skeleton_wipe_volume(const Print& print, const PrintRegion& region, unsigned int extruder, const ExtrusionEntity& entity);

// Object of this class holds information about whether an extrusion is printed immediately
// after a toolchange (as part of infill/perimeter wiping) or not. One extrusion can be a part
// of several copies - this has to be taken into account.
class WipingExtrusions
{
public:
    bool is_anything_overridden() const {   // if there are no overrides, all the agenda can be skipped - this function can tell us if that's the case
        return something_overridden;
    }

    // When allocating extruder overrides of an object's ExtrusionEntity, overrides for maximum 3 copies are allocated in place.
    typedef boost::container::small_vector<int32_t, 3> ExtruderPerCopy;

    // This is called from GCode::process_layer - see implementation for further comments:
    const ExtruderPerCopy* get_extruder_overrides(const ExtrusionEntity* entity, const PrintObject* object, int correct_extruder_id, size_t num_of_copies);
    ExtruderPerCopy resolved_extruder_overrides(const ExtrusionEntity* entity, const PrintObject& object, int correct_extruder_id, size_t num_of_copies) const;
    int get_support_extruder_overrides(const PrintObject* object);
    int get_support_interface_extruder_overrides(const PrintObject* object);
    float skeleton_flush_volume(unsigned int old_extruder, unsigned int new_extruder) const;
    bool is_skeleton_flush_target(const PrintObject* object, unsigned int copy, unsigned int old_extruder, unsigned int new_extruder) const;
    bool is_skeleton_flush_entity_target(const ExtrusionEntity* entity, const PrintObject* object, unsigned int copy,
                                         unsigned int old_extruder, unsigned int new_extruder) const;

    // This function goes through all infill entities, decides which ones will be used for wiping and
    // marks them by the extruder id. Returns volume that remains to be wiped on the wipe tower:
    float mark_wiping_extrusions(const Print& print, unsigned int old_extruder, unsigned int new_extruder, float volume_to_wipe,
                                 float skeleton_volume_to_wipe = -1.f, bool skeleton_only = false,
                                 bool solid_skeleton_target_old = false, bool solid_skeleton_entry_prime = false,
                                 bool require_full_skeleton_wipe = false, bool allow_global_skeleton_wipe = false);

    void ensure_perimeters_infills_order(const Print& print);

    bool is_overriddable(const ExtrusionEntityCollection& ee, const PrintConfig& print_config, const PrintObject& object, const PrintRegion& region) const;
    bool is_overriddable_and_mark(const ExtrusionEntityCollection& ee, const PrintConfig& print_config, const PrintObject& object, const PrintRegion& region) {
    	bool out = this->is_overriddable(ee, print_config, object, region);
    	this->something_overridable |= out;
    	return out;
    }
    // For ByObject mode there is no PrintConfig here, but the object may still enable flushing overrides.
    bool is_obj_overriddable(const ExtrusionEntityCollection &ee, const PrintObject &object) const;
    bool is_obj_overriddable_and_mark(const ExtrusionEntityCollection &ee, const PrintObject &object)
    {
        bool out = this->is_obj_overriddable(ee, object);
        this->something_overridable |= out;
        return out;
    }

    // BBS
    bool is_support_overriddable(const ExtrusionRole role, const PrintObject& object) const;
    bool is_support_overriddable_and_mark(const ExtrusionRole role, const PrintObject& object) {
        bool out = this->is_support_overriddable(role, object);
        this->something_overridable |= out;
        return out;
    }

    bool is_support_overridden(const PrintObject* object) const {
        return support_map.find(object) != support_map.end();
    }

    bool is_support_interface_overridden(const PrintObject* object) const {
        return support_intf_map.find(object) != support_intf_map.end();
    }

    void set_layer_tools_ptr(const LayerTools* lt) { m_layer_tools = lt; }

private:
    int first_nonsoluble_extruder_on_layer(const PrintConfig& print_config) const;
    int last_nonsoluble_extruder_on_layer(const PrintConfig& print_config) const;

    // This function is called from mark_wiping_extrusions and sets extruder that it should be printed with (-1 .. as usual)
    void set_extruder_override(const ExtrusionEntity* entity, const PrintObject* object, size_t copy_id, int extruder, size_t num_of_copies);
    // BBS
    void set_support_extruder_override(const PrintObject* object, size_t copy_id, int extruder, size_t num_of_copies);
    void set_support_interface_extruder_override(const PrintObject* object, size_t copy_id, int extruder, size_t num_of_copies);

    // Returns true in case that entity is not printed with its usual extruder for a given copy:
    bool is_entity_overridden(const ExtrusionEntity* entity, const PrintObject *object, size_t copy_id) const {
        auto it = entity_map.find(std::make_tuple(entity, object));
        return it == entity_map.end() ? false : it->second[copy_id] != -1;
    }

    std::map<std::tuple<const ExtrusionEntity*, const PrintObject *>, ExtruderPerCopy> entity_map;  // to keep track of who prints what
    // BBS
    std::map<const PrintObject*, int> support_map;
    std::map<const PrintObject*, int> support_intf_map;
    std::map<std::pair<unsigned int, unsigned int>, float> skeleton_flush_volume_map;
    std::set<std::tuple<const PrintObject*, unsigned int, unsigned int, unsigned int>> skeleton_flush_targets;
    std::set<std::tuple<const ExtrusionEntity*, const PrintObject*, unsigned int, unsigned int, unsigned int>>
        skeleton_flush_entity_targets;
    bool something_overridable = false;
    bool something_overridden = false;
    const LayerTools* m_layer_tools = nullptr;    // so we know which LayerTools object this belongs to
};

class LayerTools
{
public:
    LayerTools(const coordf_t z) : print_z(z) {}

    // Changing these operators to epsilon version can make a problem in cases where support and object layers get close to each other.
    // In case someone tries to do it, make sure you know what you're doing and test it properly (slice multiple objects at once with supports).
    bool operator< (const LayerTools &rhs) const { return print_z < rhs.print_z; }
    bool operator==(const LayerTools &rhs) const { return print_z == rhs.print_z; }

    bool is_extruder_order(unsigned int a, unsigned int b) const;
    bool has_extruder(unsigned int extruder) const { return std::find(this->extruders.begin(), this->extruders.end(), extruder) != this->extruders.end(); }

    // Return a zero based extruder from the region, or extruder_override if overriden.
    unsigned int wall_filament(const PrintRegion &region) const;
    unsigned int sparse_infill_filament(const PrintRegion &region) const;
    unsigned int solid_infill_filament(const PrintRegion &region) const;
    // Convert a 1-based support/interface filament to the zero-based physical tool collected for this layer.
    // Config value 0 returns a placeholder 0; the caller must still resolve automatic selection separately.
    unsigned int support_filament(unsigned int filament_id_1based) const;
	// Returns a zero based extruder this eec should be printed with, according to PrintRegion config or extruder_override if overriden.
	unsigned int extruder(const ExtrusionEntityCollection &extrusions, const PrintRegion &region) const;

    coordf_t 					print_z	= 0.;
    bool 						has_object = false;
    bool						has_support = false;
    // Zero based extruder IDs, ordered to minimize tool switches.
    std::vector<unsigned int> 	extruders;
    bool                        preserve_extruder_order = false;
    // If per layer extruder switches are inserted by the G-code preview slider, this value contains the new (1 based) extruder, with which the whole object layer is being printed with.
    // If not overriden, it is set to 0.
    unsigned int 				extruder_override = 0;
    // Sequential layer index (0-based), used by mixed-filament resolution.
    int                         layer_index = 0;
    // Actual layer height for this print_z where available.
    coordf_t                    layer_height = 0.;
    // Should a skirt be printed at this layer?
    // Layers are marked for infinite skirt aka draft shield. Not all the layers have to be printed.
    bool                        has_skirt = false;
    // Will there be anything extruded on this layer for the wipe tower?
    // Due to the support layers possibly interleaving the object layers,
    // wipe tower will be disabled for some support only layers.
    bool 						has_wipe_tower = false;
    // Number of wipe tower partitions to support the required number of tool switches
    // and to support the wipe tower partitions above this one.
    size_t                      wipe_tower_partitions = 0;
    coordf_t 					wipe_tower_layer_height = 0.;
    // Custom G-code (color change, extruder switch, pause) to be performed before this layer starts to print.
    const CustomGCode::Item    *custom_gcode = nullptr;

    WipingExtrusions& wiping_extrusions() {
        m_wiping_extrusions.set_layer_tools_ptr(this);
        return m_wiping_extrusions;
    }

    const WipingExtrusions& wiping_extrusions() const {
        const_cast<WipingExtrusions&>(m_wiping_extrusions).set_layer_tools_ptr(this);
        return m_wiping_extrusions;
    }

    // Mixed-filament resolution context (set by ToolOrdering during collect_extruders).
    const MixedFilamentManager *mixed_mgr    = nullptr;
    size_t                      num_physical = 0;
    bool                        has_mixed_filaments = false;
    // Optional mixed-layer cadence override from print settings.
    float                       mixed_layer_height_a    = 0.f;
    float                       mixed_layer_height_b    = 0.f;
    float                       mixed_base_layer_height = 0.2f;
    bool                        enable_mixed_color_sublayer = false;
    // Total number of layers in the print, used for run-based gradient calculation.
    size_t                      total_layer_count = 0;

    // ----- Mixed sub-layer group (phase A: minimal port from Bambu MixedSubLayerGroup) -----
    // When enable_mixed_color_sublayer is true and a MixedFilament row is enabled,
    // resolve_mixed_sublayers() populates one entry per mixed row. Each entry
    // describes how the corresponding layer is split into a stack of sub-layers,
    // one per component, with `sub_heights[i]` holding the height of the i-th
    // sub-layer and `components_0based[i]` its physical extruder. The whole
    // stack is the same thickness as a regular layer (`layer_height`).
    //
    // When the feature is disabled, `mixed_sub_layer_groups` is empty and the
    // existing resolve_mixed_with_layer_heights() path stays in charge.
    struct MixedSubLayerGroup {
        // 0-based virtual filament index (matches lt.extruders after reorder).
        unsigned int              mixed_slot_0based = 0;
        // Physical extruders (0-based) that this mixed slot's sub-layer stack
        // is composed of.  Order is significant: sorted ascending.
        std::vector<unsigned int> components_0based;
        // Height occupied by each sub-layer.  `sub_heights` sums to
        // `layer_height`; `sub_heights[i]` > 0 always.
        std::vector<double>       sub_heights;
        // Whether this group uses gradient mode (ratio changes per layer).
        bool                      is_gradient = false;

        // ----- Per-object gradient (Bambu-aligned port) -----
        // When gradient_enabled AND per_part_gradient are true for this slot,
        // each PrintObject that uses the slot gets its own gradient ramp
        // independent of other objects' heights.  Keyed by PrintObject*.
        struct ObjectGradient {
            size_t        total_layers = 0;   // Total gradient layers for this object in current run
            size_t        current_idx  = 0;   // Current layer index within the run (consumed by GCode.cpp)
            double        gradient_start = 0.10;
            double        gradient_end   = 0.90;
            GradientCurve curve;              // Empty -> linear fallback (start, end); non-empty wins
        };
        std::map<const PrintObject*, ObjectGradient> per_object_gradient;

        // Index of "first" config component after sorting, used by GCode.cpp
        // to map r1 (first component ratio) to the correct sorted position.
        int                       gradient_first_sorted_idx = 0;
    };
    std::vector<MixedSubLayerGroup> mixed_sub_layer_groups;

    // Per-layer resolution used when a mixed slot is not split into sublayers
    // (sublayer disabled, or the first layer). Maps a 0-based virtual slot to
    // the selected 0-based physical filament.
    std::map<unsigned int, unsigned int> mixed_filament_resolution;

    unsigned int resolve_mixed_slot(unsigned int filament_0based) const
    {
        auto it = mixed_filament_resolution.find(filament_0based);
        return it == mixed_filament_resolution.end() ? filament_0based : it->second;
    }

    // Raw virtual mixed-filament slot IDs (0-based) that were configured
    // for this layer BEFORE resolve_mixed() converted them to physical IDs.
    // Used by resolve_mixed_sublayers() to identify which slots need sub-layer splitting.
    std::vector<unsigned int> raw_mixed_filament_ids_0based;

    // True when `slot_id_0based` is a mixed virtual slot (>= num_physical and
    // has a valid MixedFilament entry in mixed_mgr).  Pure query, no side effects.
    bool is_mixed_slot(unsigned int slot_id_0based) const;
    // Return the MixedSubLayerGroup for the given 0-based virtual slot, or
    // nullptr if none.  Lifetime is tied to this LayerTools instance.
    const MixedSubLayerGroup* mixed_group_by_slot(unsigned int slot_id_0based) const;

private:
    // Resolve a 1-based filament ID through the mixed-filament manager for this layer.
    unsigned int resolve_mixed_1based(unsigned int filament_id) const;
    // This object holds list of extrusion that will be used for extruder wiping
    WipingExtrusions m_wiping_extrusions;
};

class ToolOrdering
{
public:
    ToolOrdering() = default;

    // For the use case when each object is printed separately
    // (print->config().print_sequence == PrintSequence::ByObject is true).
    ToolOrdering(const PrintObject &object, unsigned int first_extruder, bool prime_multi_material = false);

    // For the use case when all objects are printed at once.
    // (print->config().print_sequence == PrintSequence::ByObject is false).
    ToolOrdering(const Print& print, unsigned int first_extruder, bool prime_multi_material = false);

    void 				clear() {
        m_layer_tools.clear(); m_tool_order_cache.clear();
        m_mixed_object_layers.clear();
        m_object_all_layer_indices.clear();
    }

    // Only valid for non-sequential print:
	// Assign a pointer to a custom G-code to the respective ToolOrdering::LayerTools.
	// Ignore color changes, which are performed on a layer and for such an extruder, that the extruder will not be printing above that layer.
	// If multiple events are planned over a span of a single layer, use the last one.
	void 				assign_custom_gcodes(const Print &print);

    // Get the first extruder printing, including the extruder priming areas, returns -1 if there is no layer printed.
    unsigned int   		first_extruder() const { return m_first_printing_extruder; }

    // Get the first extruder printing the layer_tools, returns -1 if there is no layer printed.
    unsigned int   		last_extruder() const { return m_last_printing_extruder; }

    // For a multi-material print, the printing extruders are ordered in the order they shall be primed.
    const std::vector<unsigned int>& all_extruders() const { return m_all_printing_extruders; }

    // Keep initial_extruder when it is non-support. Otherwise return the first non-support
    // filament in print order, falling back when the task only contains support filaments.
    unsigned int first_non_support_extruder(const PrintConfig& config, unsigned int initial_extruder) const;

    // Find LayerTools with the closest print_z.
    const LayerTools&	tools_for_layer(coordf_t print_z) const;
    LayerTools&			tools_for_layer(coordf_t print_z) { return const_cast<LayerTools&>(std::as_const(*this).tools_for_layer(print_z)); }

    const LayerTools&   front()       const { return m_layer_tools.front(); }
    const LayerTools&   back()        const { return m_layer_tools.back(); }
    std::vector<LayerTools>::const_iterator begin() const { return m_layer_tools.begin(); }
    std::vector<LayerTools>::const_iterator end()   const { return m_layer_tools.end(); }
    bool 				empty()       const { return m_layer_tools.empty(); }
    std::vector<LayerTools>& layer_tools() { return m_layer_tools; }
    bool 				has_wipe_tower() const { return ! m_layer_tools.empty() && m_first_printing_extruder != (unsigned int)-1 && m_layer_tools.front().has_wipe_tower; }

private:
    void				initialize_layers(std::vector<coordf_t> &zs);
    void                      initialize_mixed_context();
    void 				collect_extruders(const PrintObject &object, const std::vector<std::pair<double, unsigned int>> &per_layer_extruder_switches);
    void				reorder_extruders(unsigned int last_extruder_id);
    // BBS
    void                reorder_extruders(std::vector<unsigned int> tool_order_layer0);
    void 				fill_wipe_tower_partitions(const PrintConfig &config, coordf_t object_bottom_z, coordf_t max_layer_height);
    bool                insert_wipe_tower_filament(const Print &print);
    void                count_wipe_tower_partitions();
    void                mark_skirt_layers(const PrintConfig &config, coordf_t max_layer_height);
    void 				collect_extruder_statistics(bool prime_multi_material);
    void                reorder_extruders_for_minimum_flush_volume();

    // BBS
    std::vector<unsigned int> generate_first_layer_tool_order(const Print& print);
    std::vector<unsigned int> generate_first_layer_tool_order(const PrintObject& object);
    void                      update_mixed_layer_height_settings();

    // ----- Mixed sub-layer group resolution (phase A) -----
    // For each LayerTools in m_layer_tools, scan lt.extruders for virtual
    // mixed-slot IDs (>= m_num_physical), build MixedSubLayerGroup entries,
    // and append the physical component IDs to lt.extruders so the GCode
    // main loop visits them.  Must run AFTER reorder_extruders().
    void                      resolve_mixed_sublayers(const PrintConfig &config);
    // Topological sort of lt.extruders to satisfy the ordering constraints
    // imposed by mixed_sub_layer_groups.  Uses Kahn's algorithm.
    void                      enforce_mixed_component_order();

    // Resolve a 1-based filament ID through the mixed-filament manager.
    // Returns the resolved physical extruder (1-based).  If the ID is not a
    // mixed filament or no manager is set, returns the input unchanged.
    unsigned int resolve_mixed(unsigned int filament_id_1based,
                               int          layer_index,
                               float        layer_print_z = 0.f,
                               float        layer_height  = 0.f) const;


    std::vector<LayerTools>    m_layer_tools;
    // First printing extruder, including the multi-material priming sequence.
    unsigned int               m_first_printing_extruder = (unsigned int)-1;
    // Final printing extruder.
    unsigned int               m_last_printing_extruder  = (unsigned int)-1;
    // All extruders, which extrude some material over m_layer_tools.
    std::vector<unsigned int>  m_all_printing_extruders;
    std::map<std::pair<std::vector<unsigned int>, std::optional<unsigned int>>, std::vector<uint8_t>> m_tool_order_cache;
    const DynamicPrintConfig*  m_print_full_config = nullptr;
    const PrintConfig*         m_print_config_ptr = nullptr;
    const PrintObject*         m_print_object_ptr = nullptr;
    bool                       m_is_BBL_printer = false;
    // Mixed filament support: pointer to manager (owned by Print) and
    // number of physical extruders.
    const MixedFilamentManager* m_mixed_mgr    = nullptr;
    size_t                      m_num_physical  = 0;
    bool                        m_has_mixed_filaments = false;
    float                       m_mixed_layer_height_a    = 0.f;
    float                       m_mixed_layer_height_b    = 0.f;
    float                       m_mixed_base_layer_height = 0.2f;
    // Cached result of `enable_mixed_color_sublayer` in the print full
    // config.  Refreshed by read_enable_mixed_color_sublayer() right after
    // m_print_full_config is assigned in the constructors.  Defaults to
    // false to keep behavior bit-identical when the option is absent
    // (e.g. older preset bundles that pre-date the Quality > Layer height
    // > Mixed color sublayer checkbox in PrintConfigDef).
    bool                        m_enable_mixed_color_sublayer = false;
    // Read the `enable_mixed_color_sublayer` key from m_print_full_config
    // without throwing when the key is unknown.  Returns false when the
    // config is null, the key is missing, or the value is false.
    bool                        read_enable_mixed_color_sublayer() const;

    // Per-object gradient tracking (Bambu-aligned port):
    //   slot(0-based) -> PrintObject* -> list of layer indices where that
    //   object uses the slot.  Populated by collect_extruders(), consumed
    //   by resolve_mixed_sublayers() to build per_object_gradient.
    std::map<unsigned int, std::map<const PrintObject*, std::vector<size_t>>> m_mixed_object_layers;

    // All layer indices (in m_layer_tools) where each object has any layer.
    // Used by gradient run detection to distinguish real gaps (object has
    // a layer that doesn't use the slot) from spurious gaps (another
    // object's layer).
    std::map<const PrintObject*, std::vector<size_t>> m_object_all_layer_indices;
};

} // namespace SLic3r

#endif /* slic3r_ToolOrdering_hpp_ */
