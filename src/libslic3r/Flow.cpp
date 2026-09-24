#include "Flow.hpp"
#include "I18N.hpp"
#include "Print.hpp"
#include <cmath>
#include <assert.h>

#include <boost/algorithm/string/predicate.hpp>

// Mark string for localization and translate.
#define L(s) Slic3r::I18N::translate(s)

namespace Slic3r {

FlowErrorNegativeSpacing::FlowErrorNegativeSpacing() : 
	FlowError("Flow::spacing() produced negative spacing. Did you set some extrusion width too small?") {}

FlowErrorNegativeFlow::FlowErrorNegativeFlow() :
    FlowError("Flow::mm3_per_mm() produced negative flow. Did you set some extrusion width too small?") {}

// This static method returns a sane extrusion width default.
float Flow::auto_extrusion_width(FlowRole role, float nozzle_diameter)
{
    switch (role) {
    case frSupportMaterial:
    case frSupportMaterialInterface:
    case frSupportTransition:
    case frTopSolidInfill:
        return nozzle_diameter;
    default:
    case frExternalPerimeter:
    case frPerimeter:
    case frSolidInfill:
    case frInfill:
        return 1.125f * nozzle_diameter;
    }
}

// Used by the Flow::extrusion_width() funtion to provide hints to the user on default extrusion width values,
// and to provide reasonable values to the PlaceholderParser.
static inline FlowRole opt_key_to_flow_role(const std::string &opt_key)
{
 	if (opt_key == "inner_wall_line_width" || 
 		// or all the defaults:
 		opt_key == "line_width" || opt_key == "initial_layer_line_width")
        return frPerimeter;
    else if (opt_key == "outer_wall_line_width")
        return frExternalPerimeter;
    else if (opt_key == "sparse_infill_line_width")
        return frInfill;
    else if (opt_key == "internal_solid_infill_line_width")
        return frSolidInfill;
	else if (opt_key == "top_surface_line_width")
		return frTopSolidInfill;
	else if (opt_key == "support_line_width")
    	return frSupportMaterial;
    else 
    	throw Slic3r::RuntimeError("opt_key_to_flow_role: invalid argument");
};

static inline void throw_on_missing_variable(const std::string &opt_key, const char *dependent_opt_key) 
{
	throw FlowErrorMissingVariable((boost::format(L("Failed to calculate line width of %1%. Can not get value of \"%2%\" ")) % opt_key % dependent_opt_key).str());
}

// Used to provide hints to the user on default extrusion width values, and to provide reasonable values to the PlaceholderParser.
double Flow::extrusion_width(const std::string& opt_key, const ConfigOptionFloatsOrPercentsNullable* opt, const ConfigOptionResolver& config, const unsigned int first_printing_extruder)
{
	assert(opt != nullptr);
    ConfigOptionFloatOrPercent width = nozzle_variant_option(*opt, first_printing_extruder);

#if 0
// This is the logic used for skit / brim, but not for the rest of the 1st layer.
	if (opt->value == 0. && first_layer) {
		// The "initial_layer_line_width" was set to zero, try a substitute.
		opt = config.option<ConfigOptionFloatOrPercent>("inner_wall_line_width");
		if (opt == nullptr)
    		throw_on_missing_variable(opt_key, "inner_wall_line_width");
	}
#endif

	if (width.value == 0.) {
		// The role specific extrusion width value was set to zero, try the role non-specific extrusion width.
		const auto *default_width = config.option<ConfigOptionFloatsOrPercentsNullable>("line_width");
		if (default_width == nullptr)
    		throw_on_missing_variable(opt_key, "line_width");
        width = nozzle_variant_option(*default_width, first_printing_extruder);
	}

    auto opt_nozzle_diameters = config.option<ConfigOptionFloats>("nozzle_diameter");
    if (opt_nozzle_diameters == nullptr)
        throw_on_missing_variable(opt_key, "nozzle_diameter");

    if (width.percent) {
		return width.get_abs_value(float(opt_nozzle_diameters->get_at(first_printing_extruder)));
	}

	if (width.value == 0.) {
        // If user left option to 0, calculate a sane default width.
        return auto_extrusion_width(opt_key_to_flow_role(opt_key), float(opt_nozzle_diameters->get_at(first_printing_extruder)));
    }

	return width.value;
}

// Used to provide hints to the user on default extrusion width values, and to provide reasonable values to the PlaceholderParser.
double Flow::extrusion_width(const std::string& opt_key, const ConfigOptionResolver &config, const unsigned int first_printing_extruder)
{
    return extrusion_width(opt_key, config.option<ConfigOptionFloatsOrPercentsNullable>(opt_key), config, first_printing_extruder);
}

// This constructor builds a Flow object from an extrusion width config setting
// and other context properties.
Flow Flow::new_from_config_width(FlowRole role, const ConfigOptionFloatOrPercent& width, float nozzle_diameter, float height, float adaptive_width)
{
    if (height <= 0)
        throw Slic3r::InvalidArgument("Invalid flow height supplied to new_from_config_width()");

    float w;
    if (!width.percent  && width.value <= 0.) {
        // If user left option to 0, calculate a sane default width.
        w = auto_extrusion_width(role, nozzle_diameter);
    } else {
        // If user set a manual value, use it.
        w = float(width.get_abs_value(nozzle_diameter));
        if (role == frExternalPerimeter && (adaptive_width > EPSILON))
            w = adaptive_width;
    }
    
    return Flow(w, height, rounded_rectangle_extrusion_spacing(w, height), nozzle_diameter, false, role == FlowRole::frExternalPerimeter);
}

// Adjust extrusion flow for new extrusion line spacing, maintaining the old spacing between extrusions.
Flow Flow::with_spacing(float new_spacing) const
{
    Flow out = *this;
    if (m_bridge) {
        // Diameter of the rounded extrusion.
        assert(m_width == m_height);
        float gap          = m_spacing - m_width;
        auto  new_diameter = new_spacing - gap;
        out.m_width        = out.m_height = new_diameter;
    } else {
        assert(m_width >= m_height);
        out.m_width += new_spacing - m_spacing;
        if (out.m_width < out.m_height)
            throw Slic3r::InvalidArgument(L("Invalid spacing supplied to Flow::with_spacing(), check your layer height and extrusion width"));
    }
    out.m_spacing = new_spacing;
    return out;
}

// Adjust the width / height of a rounded extrusion model to reach the prescribed cross section area while maintaining extrusion spacing.
Flow Flow::with_cross_section(float area_new) const
{
    assert(! m_bridge);
    assert(m_width >= m_height);

    // Adjust for bridge_flow, maintain the extrusion spacing.
    float area = this->mm3_per_mm();
    if (area_new > area + EPSILON) {
        // Increasing the flow rate.
        float new_full_spacing = area_new / m_height;
        if (new_full_spacing > m_spacing) {
            // Filling up the spacing without an air gap. Grow the extrusion in height.
            float height = area_new / m_spacing;
            return Flow(rounded_rectangle_extrusion_width_from_spacing(m_spacing, height), height, m_spacing, m_nozzle_diameter, false);
        } else {
            return this->with_width(rounded_rectangle_extrusion_width_from_spacing(area / m_height, m_height));
        }
    } else if (area_new < area - EPSILON) {
        // Decreasing the flow rate.
        float width_new = m_width - (area - area_new) / m_height;
        assert(width_new > 0);
        if (width_new > m_height) {
            // Shrink the extrusion width.
            return this->with_width(width_new);
        } else {
            // Create a rounded extrusion.
            auto dmr = float(sqrt(area_new / M_PI));
            return Flow(dmr, dmr, m_spacing, m_nozzle_diameter, false);
        }
    } else
        return *this;
}

float Flow::rounded_rectangle_extrusion_spacing(float width, float height)
{
    auto out = width - height * float(1. - 0.25 * PI);
    if (out <= 0.f)
        throw FlowErrorNegativeSpacing();
    return out;
}

float Flow::rounded_rectangle_extrusion_width_from_spacing(float spacing, float height)
{
    return float(spacing + height * (1. - 0.25 * PI));
}

float Flow::bridge_extrusion_spacing(float dmr)
{
    return dmr + BRIDGE_EXTRA_SPACING;
}

// This method returns extrusion volume per head move unit.
double Flow::mm3_per_mm() const
{
    float res = m_bridge ?
        // Area of a circle with dmr of this->width.
        float((m_width * m_width) * 0.25 * PI) :
        // Rectangle with semicircles at the ends. ~ h (w - 0.215 h)
        float(m_height * (m_width - m_height * (1. - 0.25 * PI)));
    //assert(res > 0.);
	if (res <= 0.)
		throw FlowErrorNegativeFlow();
    return res;
}

Flow FlowWidthConfig::flow(float height, float adaptive_width) const
{
    return Flow::new_from_config_width(role, width, float(nozzle_diameter), height, adaptive_width);
}

FlowWidthError FlowWidthConfig::validate(double height) const
{
    const double value = width.get_abs_value(nozzle_diameter);
    if (value == 0.)
        return FlowWidthError::None;
    if (value <= height)
        return FlowWidthError::TooSmall;
    if (value > 5. * nozzle_diameter)
        return FlowWidthError::TooLarge;
    return FlowWidthError::None;
}

size_t resolve_flow_nozzle_index(const Print &print, unsigned int filament_id)
{
    const PrintConfig &config = print.config();
    const size_t filament_count = std::max(config.filament_colour.size(), config.filament_diameter.size());
    if (filament_id > filament_count) {
        const ExpandedFilamentUsage usage = print.mixed_filament_manager().expand_filament_usage({filament_id}, filament_count);
        if (!usage.valid() || usage.physical_filament_ids.empty())
            throw SlicingError(L("Unable to resolve a mixed filament used by the current plate."));
        filament_id = usage.physical_filament_ids.front();
    }
    if (filament_id == 0 || config.nozzle_diameter.values.empty())
        throw SlicingError(L("Filament nozzle mapping is incomplete."));
    if (config.support_filament_nozzle_mapping.value &&
        (filament_id > config.filament_map.size() || config.filament_map.values[filament_id - 1] <= 0))
        throw SlicingError(L("Filament nozzle mapping is incomplete."));
    const size_t index = get_physical_nozzle_index(config, filament_id - 1);
    if (index < config.nozzle_diameter.size())
        return index;
    // Legacy single-nozzle multi-material printers have no physical-nozzle map.
    if (config.nozzle_diameter.size() == 1 && !config.support_filament_nozzle_mapping.value)
        return 0;
    throw SlicingError(L("Filament nozzle mapping points to an unavailable nozzle."));
}

static size_t support_flow_nozzle_index(const PrintObject &object, bool is_interface)
{
    const Print &print = *object.print();
    const PrintConfig &config = print.config();
    const int filament = is_interface ? object.config().support_interface_filament.value : object.config().support_filament.value;
    if (filament > 0)
        return resolve_flow_nozzle_index(print, unsigned(filament));

    // Automatic support has one geometry Flow before per-layer tool scheduling.
    // Choose a stable participating model filament, preferring non-soluble ones,
    // instead of accidentally reading an idle physical nozzle through -1.
    std::vector<unsigned int> model_filaments;
    for (const PrintObject *candidate : print.objects())
        for (const PrintRegion &region : candidate->all_regions())
            region.collect_object_printing_extruders(print, model_filaments);
    for (unsigned int &id : model_filaments)
        ++id;
    const size_t filament_count = std::max(config.filament_colour.size(), config.filament_diameter.size());
    const ExpandedFilamentUsage usage = print.mixed_filament_manager().expand_filament_usage(model_filaments, filament_count);
    if (!usage.valid() || usage.physical_filament_ids.empty())
        throw SlicingError(L("Unable to resolve a mixed filament used by the current plate."));
    for (unsigned int id : usage.physical_filament_ids)
        if (!config.filament_soluble.get_at(id - 1))
            return resolve_flow_nozzle_index(print, id);
    return resolve_flow_nozzle_index(print, usage.physical_filament_ids.front());
}

FlowWidthConfig resolve_model_flow_width(const PrintObject &object, const PrintRegion &region,
                                        FlowRole role, bool first_layer)
{
    const PrintConfig &print_config = object.print()->config();
    const PrintRegionConfig &config = region.config();
    const size_t nozzle = resolve_flow_nozzle_index(*object.print(), region.extruder(role));
    const ConfigOptionFloatsOrPercentsNullable *option = nullptr;
    const char *key = nullptr;
    switch (role) {
    case frExternalPerimeter: option = &config.outer_wall_line_width; key = "outer_wall_line_width"; break;
    case frPerimeter: option = &config.inner_wall_line_width; key = "inner_wall_line_width"; break;
    case frInfill: option = &config.sparse_infill_line_width; key = "sparse_infill_line_width"; break;
    case frSolidInfill: option = &config.internal_solid_infill_line_width; key = "internal_solid_infill_line_width"; break;
    case frTopSolidInfill: option = &config.top_surface_line_width; key = "top_surface_line_width"; break;
    default: throw InvalidArgument("Unknown model flow role");
    }
    ConfigOptionFloatOrPercent width = nozzle_variant_option(*option, nozzle);
    const ConfigOptionFloatOrPercent initial_width = nozzle_variant_option(print_config.initial_layer_line_width, nozzle);
    if (first_layer && initial_width.value > 0.) {
        width = initial_width;
        key = "initial_layer_line_width";
    }
    if (width.value == 0.) {
        width = nozzle_variant_option(object.config().line_width, nozzle);
        key = "line_width";
    }
    return {role, width, print_config.nozzle_diameter.get_at(nozzle), key};
}

FlowWidthConfig resolve_support_flow_width(const PrintObject &object, bool is_interface, bool first_layer)
{
    // Match SupportParameters' effective interface Flow when interfaces are off.
    if (is_interface && object.config().support_interface_top_layers.value == 0 && !object.has_raft())
        is_interface = false;
    const PrintConfig &config = object.print()->config();
    const size_t nozzle = support_flow_nozzle_index(object, is_interface);
    ConfigOptionFloatOrPercent width = nozzle_variant_option(object.config().support_line_width, nozzle);
    const char *key = "support_line_width";
    const ConfigOptionFloatOrPercent initial_width = nozzle_variant_option(config.initial_layer_line_width, nozzle);
    if (first_layer && initial_width.value > 0.) {
        width = initial_width;
        key = "initial_layer_line_width";
    }
    if (width.value <= 0.) {
        width = nozzle_variant_option(object.config().line_width, nozzle);
        key = "line_width";
    }
    return {is_interface ? frSupportMaterialInterface : frSupportMaterial, width, config.nozzle_diameter.get_at(nozzle), key};
}

FlowWidthConfig resolve_infill_detail_flow_width(const PrintObject &object, const PrintRegion &region,
                                                FlowRole role, bool skin)
{
    const size_t nozzle = resolve_flow_nozzle_index(*object.print(), region.extruder(role));
    const auto &option = skin ? region.config().skin_infill_line_width : region.config().skeleton_infill_line_width;
    // Locked infill passes zero directly to Flow's automatic width calculation.
    return {role, nozzle_variant_option(option, nozzle), object.print()->config().nozzle_diameter.get_at(nozzle),
            skin ? "skin_infill_line_width" : "skeleton_infill_line_width"};
}

FlowWidthConfig resolve_skirt_flow_width(const Print &print)
{
    const PrintConfig &config = print.config();
    const size_t nozzle = print.objects().empty() ? 0 : support_flow_nozzle_index(*print.objects().front(), false);
    ConfigOptionFloatOrPercent width = nozzle_variant_option(config.initial_layer_line_width, nozzle);
    const char *key = "initial_layer_line_width";
    if (width.value <= 0. && !print.objects().empty()) {
        width = nozzle_variant_option(print.objects().front()->config().line_width, nozzle);
        key = "line_width";
    }
    return {frPerimeter, width, config.nozzle_diameter.get_at(nozzle), key};
}

FlowWidthConfig resolve_brim_flow_width(const Print &print)
{
    const PrintRegionConfig &region = print.get_print_region(0).config();
    const size_t nozzle = resolve_flow_nozzle_index(print, region.wall_filament.value);
    ConfigOptionFloatOrPercent width = nozzle_variant_option(print.config().initial_layer_line_width, nozzle);
    const char *key = "initial_layer_line_width";
    if (width.value <= 0.) {
        width = nozzle_variant_option(region.inner_wall_line_width, nozzle);
        key = "inner_wall_line_width";
    }
    if (width.value <= 0.) {
        width = nozzle_variant_option(print.objects().front()->config().line_width, nozzle);
        key = "line_width";
    }
    return {frPerimeter, width, print.config().nozzle_diameter.get_at(nozzle), key};
}

Flow support_material_flow(const PrintObject *object, float layer_height)
{
    return resolve_support_flow_width(*object, false).flow(
        layer_height > 0.f ? layer_height : float(object->config().layer_height.value));
}

Flow support_transition_flow(const PrintObject *object)
{
    const float diameter = float(resolve_support_flow_width(*object, false).nozzle_diameter);
    return Flow::bridging_flow(diameter, diameter);
}

Flow support_material_1st_layer_flow(const PrintObject *object, float layer_height)
{
    return resolve_support_flow_width(*object, false, true).flow(
        layer_height > 0.f ? layer_height : float(object->print()->config().initial_layer_print_height.value));
}

Flow support_material_interface_flow(const PrintObject *object, float layer_height)
{
    return resolve_support_flow_width(*object, true).flow(
        layer_height > 0.f ? layer_height : float(object->config().layer_height.value));
}

    Flow support_material_ironing_flow(const PrintObject* object, float layer_height, float ironing_width)
    {
        ConfigOptionFloatOrPercent _ironing_width =  ConfigOptionFloatOrPercent(ironing_width, false);
        const PrintConfig &print_config = object->print()->config();
        const size_t nozzle_index = support_flow_nozzle_index(*object, true);
        const ConfigOptionFloatOrPercent default_width = nozzle_variant_option(object->config().line_width, nozzle_index);
        return Flow::new_from_config_width(
            frSupportIroning,
            // The width parameter accepted by new_from_config_width is of type ConfigOptionFloatOrPercent, the Flow class takes care of the percent to value substitution.
            (ironing_width > 0) ? _ironing_width : default_width,
            float(print_config.nozzle_diameter.get_at(nozzle_index)),
            (layer_height > 0.f) ? layer_height : float(object->config().layer_height.value));
    }
}
