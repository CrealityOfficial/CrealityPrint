#ifndef slic3r_Flow_hpp_
#define slic3r_Flow_hpp_

#include "Config.hpp"
#include "Exception.hpp"
#include "ExtrusionEntity.hpp"

namespace Slic3r {

class PrintObject;
class PrintRegion;
class Print;

// Extra spacing of bridge threads, in mm.
#define BRIDGE_EXTRA_SPACING 0.05

enum FlowRole {
    frExternalPerimeter,
    frPerimeter,
    frInfill,
    frSolidInfill,
    frTopSolidInfill,
    frSupportMaterial,
    frSupportMaterialInterface,
    frSupportTransition,  // BBS
    frSupportIroning,  // ironing
};

class FlowError : public Slic3r::InvalidArgument
{
public:
	FlowError(const std::string& what_arg) : Slic3r::InvalidArgument(what_arg) {}
	FlowError(const char* what_arg) : Slic3r::InvalidArgument(what_arg) {}
};

class FlowErrorNegativeSpacing : public FlowError
{
public:
    FlowErrorNegativeSpacing();
};

class FlowErrorNegativeFlow : public FlowError
{
public:
    FlowErrorNegativeFlow();
};

class FlowErrorMissingVariable : public FlowError
{
public:
    FlowErrorMissingVariable(const std::string& what_arg) : FlowError(what_arg) {}
};

class Flow
{
public:
    Flow() = default;
    Flow(float width, float height, float nozzle_diameter) :
        Flow(width, height, rounded_rectangle_extrusion_spacing(width, height), nozzle_diameter, false) {}

    // Non bridging flow: Maximum width of an extrusion with semicircles at the ends.
    // Bridging flow: Bridge thread diameter.
    float   width()           const { return m_width; }
    coord_t scaled_width()    const { return coord_t(scale_(m_width)); }
    // Non bridging flow: Layer height.
    // Bridging flow: Bridge thread diameter = layer height.
    float   height()          const { return m_height; }
    // Spacing between the extrusion centerlines.
    float   spacing()         const { return m_spacing; }
    void    set_spacing(float spacing) { m_spacing = spacing; }
    coord_t scaled_spacing()  const { return coord_t(scale_(m_spacing)); }
    // Nozzle diameter. 
    float   nozzle_diameter() const { return m_nozzle_diameter; }
    // Is it a bridge?
    bool    bridge()          const { return m_bridge; }
    // Cross section area of the extrusion.
    double  mm3_per_mm()      const;

    // Elephant foot compensation spacing to be used to detect narrow parts, where the elephant foot compensation cannot be applied.
    // To be used on frExternalPerimeter only.
    // Enable some perimeter squish (see INSET_OVERLAP_TOLERANCE).
    // Here an overlap of 0.2x external perimeter spacing is allowed for by the elephant foot compensation.
    coord_t scaled_elephant_foot_spacing() const { return coord_t(0.5f * float(this->scaled_width() + 0.6f * this->scaled_spacing())); }

    bool operator==(const Flow &rhs) const { return m_width == rhs.m_width && m_height == rhs.m_height && m_nozzle_diameter == rhs.m_nozzle_diameter && m_bridge == rhs.m_bridge; }
    bool operator!=(const Flow& rhs) const
    {
        return m_width != rhs.m_width || m_height != rhs.m_height || m_nozzle_diameter != rhs.m_nozzle_diameter || m_bridge != rhs.m_bridge;
    }

    bool operator<(const Flow& rhs) const { return this->mm3_per_mm() < rhs.mm3_per_mm(); }
    Flow        with_width (float width)  const { 
        assert(! m_bridge); 
        return Flow(width, m_height, rounded_rectangle_extrusion_spacing(width, m_height), m_nozzle_diameter, m_bridge, m_adaptive_width);
    }
    Flow        with_height(float height) const { 
        assert(! m_bridge); 
        return Flow(m_width, height, rounded_rectangle_extrusion_spacing(m_width, height), m_nozzle_diameter, m_bridge, m_adaptive_width);
    }
    // Adjust extrusion flow for new extrusion line spacing, maintaining the old spacing between extrusions.
    Flow        with_spacing(float spacing) const;
    // Adjust the width / height of a rounded extrusion model to reach the prescribed cross section area while maintaining extrusion spacing.
    Flow        with_cross_section(float area) const;
    Flow        with_flow_ratio(double ratio) const { return this->with_cross_section(this->mm3_per_mm() * ratio); }

    static Flow bridging_flow(float dmr, float nozzle_diameter) { return Flow { dmr, dmr, bridge_extrusion_spacing(dmr), nozzle_diameter, true, 0.0f}; }

    static Flow new_from_config_width(
        FlowRole role, const ConfigOptionFloatOrPercent& width, float nozzle_diameter, float height, float adaptive_width = 0.0f);

    // Spacing of extrusions with rounded extrusion model.
    static float rounded_rectangle_extrusion_spacing(float width, float height);
    // Width of extrusions with rounded extrusion model.
    static float rounded_rectangle_extrusion_width_from_spacing(float spacing, float height);
    // Spacing of round thread extrusions.
    static float bridge_extrusion_spacing(float dmr);

    // Sane extrusion width defautl based on nozzle diameter.
    // The defaults were derived from manual Prusa MK3 profiles.
    static float auto_extrusion_width(FlowRole role, float nozzle_diameter);

    // Extrusion width from full config, taking into account the defaults (when set to zero) and ratios (percentages).
    // Precise value depends on layer index (1st layer vs. other layers vs. variable layer height),
    // on active extruder etc. Therefore the value calculated by this function shall be used as a hint only.
	static double extrusion_width(const std::string &opt_key, const ConfigOptionFloatsOrPercentsNullable *opt, const ConfigOptionResolver &config, const unsigned int first_printing_extruder = 0);
	static double extrusion_width(const std::string &opt_key, const ConfigOptionResolver &config, const unsigned int first_printing_extruder = 0);

private:
    Flow(float width, float height, float spacing, float nozzle_diameter, bool bridge, float adaptive_width = 0.0f)
        : 
        m_width(width)
        , m_height(height)
        , m_spacing(spacing)
        , m_nozzle_diameter(nozzle_diameter)
        , m_bridge(bridge)
        , m_adaptive_width(adaptive_width)
        { 
            // Gap fill violates this condition.
            //assert(width >= height); 
        }

    float       m_width { 0 };
    float       m_height { 0 };
    float       m_spacing { 0 };
    float       m_nozzle_diameter { 0 };
    bool        m_bridge { false };
    float       m_adaptive_width {0};
};

// Effective input shared by validation and path generation. Keep the source key
// so a fallback error points to the parameter that actually supplied the width.
enum class FlowWidthError { None, TooSmall, TooLarge };
struct FlowWidthConfig
{
    FlowRole role;
    ConfigOptionFloatOrPercent width;
    double nozzle_diameter;
    const char *source_key;

    Flow flow(float height, float adaptive_width = 0.f) const;
    FlowWidthError validate(double height) const;
};

// Filament IDs are 1-based. Mixed filaments use the same representative member
// as resolve_filament_mapping(), including before its runtime proxy is installed.
size_t resolve_flow_nozzle_index(const Print &print, unsigned int filament_id);
FlowWidthConfig resolve_model_flow_width(const PrintObject &object, const PrintRegion &region,
                                        FlowRole role, bool first_layer = false);
FlowWidthConfig resolve_support_flow_width(const PrintObject &object, bool is_interface, bool first_layer = false);
FlowWidthConfig resolve_infill_detail_flow_width(const PrintObject &object, const PrintRegion &region,
                                                FlowRole role, bool skin);

FlowWidthConfig resolve_skirt_flow_width(const Print &print);
FlowWidthConfig resolve_brim_flow_width(const Print &print);

extern Flow support_material_flow(const PrintObject* object, float layer_height = 0.f);
extern Flow support_transition_flow(const PrintObject *object); //BBS
extern Flow support_material_1st_layer_flow(const PrintObject *object, float layer_height = 0.f);
extern Flow support_material_interface_flow(const PrintObject *object, float layer_height = 0.f);
extern Flow support_material_ironing_flow(const PrintObject* object, float layer_height = 0.f, float ironing_width = 0.f);
}

#endif
