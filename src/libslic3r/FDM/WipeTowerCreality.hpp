#ifndef WipeTowerCreality_
#define WipeTowerCreality_

#include "libslic3r/Point.hpp"
#include "libslic3r/GCode/WipeTower.hpp"
#include "libslic3r/GCode/WipeTower2.hpp"
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r
{

class WipeTowerWriterCreality;
class PrintRegionConfig;

class WipeTowerCreality
{
public:
    // Construct ToolChangeResult from current state of WipeTowerCreality and WipeTowerWriterCreality.
    // WipeTowerWriterCreality is moved from !
    WipeTower::ToolChangeResult construct_tcr(WipeTowerWriterCreality& writer,
                                   size_t old_tool,
								   bool is_finish) const;

	// x			-- x coordinates of wipe tower in mm ( left bottom corner )
	// y			-- y coordinates of wipe tower in mm ( left bottom corner )
	// width		-- width of wipe tower in mm ( default 60 mm - leave as it is )
	// wipe_area	-- space available for one toolchange in mm
    WipeTowerCreality(const PrintConfig& config,
	          const PrintRegionConfig& default_region_config,
			  int plate_idx, Vec3d plate_origin,
			  const std::vector<std::vector<float>>& wiping_matrix,
			  size_t initial_tool);


	// Set the extruder properties.
    void set_extruder(size_t idx, const PrintConfig& config);

	// Appends into internal structure m_plan containing info about the future wipe tower
	// to be used before building begins. The entries must be added ordered in z.
    void plan_toolchange(float z_par, float layer_height_par, unsigned int old_tool, unsigned int new_tool, float wipe_volume = 0.f);

	// Iterates through prepared m_plan, generates ToolChangeResults and appends them to "result"
	void generate(std::vector<std::vector<WipeTower::ToolChangeResult>> &result);

    float get_depth() const { return m_wipe_tower_depth; }
	std::vector<std::pair<float, float>> get_z_and_depth_pairs() const;
    float get_brim_width() const { return m_wipe_tower_brim_width_real; }
	float get_wipe_tower_height() const { return m_wipe_tower_height; }





	// Switch to a next layer.
	void set_layer(
		// Print height of this layer.
		float print_z,
		// Layer height, used to calculate extrusion the rate.
		float layer_height,
		// Maximum number of tool changes on this layer or the layers below.
		size_t max_tool_changes,
		// Is this the first layer of the print? In that case print the brim first. (OBSOLETE)
		bool /*is_first_layer*/,
		// Is this the last layer of the waste tower?
		bool is_last_layer)
	{
		m_z_pos 				= print_z;
		m_layer_height			= layer_height;
		m_depth_traversed  = 0.f;
        m_current_layer_finished = false;

		
        // Advance m_layer_info iterator, making sure we got it right
		while (!m_plan.empty() && m_layer_info->z < print_z - WT_EPSILON && m_layer_info+1 != m_plan.end())
			++m_layer_info;

		m_current_shape = (! this->is_first_layer() && m_current_shape == SHAPE_NORMAL) ? SHAPE_REVERSED : SHAPE_NORMAL;
		if (this->is_first_layer()) {
            m_num_layer_changes = 0;
            m_num_tool_changes 	= 0;
        } else
            ++ m_num_layer_changes;
		
		// Calculate extrusion flow from desired line width, nozzle diameter, filament diameter and layer_height:
		m_extrusion_flow = extrusion_flow(layer_height);
	}

	// Return the wipe tower position.
	const Vec2f& 		 position() const { return m_wipe_tower_pos; }
	// Return the wipe tower width.
	float     		 width()    const { return m_wipe_tower_width; }
	// The wipe tower is finished, there should be no more tool changes or wipe tower prints.
	bool 	  		 finished() const { return m_max_color_changes == 0; }

	// Returns gcode for a toolchange and a final print head position.
	// On the first layer, extrude a brim around the future wipe tower first.
    WipeTower::ToolChangeResult tool_change(size_t new_tool);

	// Fill the unfilled space with a sparse infill.
	// Call this method only if layer_finished() is false.
	WipeTower::ToolChangeResult finish_layer();

	// Is the current layer finished?
	bool 			 layer_finished() const {
        return m_current_layer_finished;
	}

    std::vector<float> get_used_filament() const { return m_used_filament_length; }
    int get_number_of_toolchanges() const { return m_num_tool_changes; }

    struct FilamentParameters {
        std::string 	    material = "PLA";
        bool                is_soluble = false;
        int  			    temperature = 0;
        int  			    first_layer_temperature = 0;
        float               ramming_line_width_multiplicator = 1.f;
        float               ramming_step_multiplicator = 1.f;
        float               max_e_speed = std::numeric_limits<float>::max();
        std::vector<float>  ramming_speed;
        float               nozzle_diameter;
        float               filament_area;
    };

private:
	enum wipe_shape // A fill-in direction
	{
		SHAPE_NORMAL = 1,
		SHAPE_REVERSED = -1
	};

    const float Width_To_Nozzle_Ratio = 1.25f; // desired line width (oval) in multiples of nozzle diameter - may not be actually neccessary to adjust
    const float WT_EPSILON            = 1e-3f;
    float filament_area() const {
        return m_filpar[0].filament_area; // all extruders are assumed to have the same filament diameter at this point
    }

    Vec2f  m_wipe_tower_pos; 			// Left front corner of the wipe tower in mm.
	float  m_wipe_tower_width; 			// Width of the wipe tower.
	float  m_wipe_tower_depth 	= 0.f; 	// Depth of the wipe tower
	float  m_wipe_tower_height  = 0.f;
	float  m_wipe_tower_cone_angle = 0.f;
    float  m_wipe_tower_brim_width      = 0.f; 	// Width of brim (mm) from config
    float  m_wipe_tower_brim_width_real = 0.f; 	// Width of brim (mm) after generation
	float  m_wipe_tower_rotation_angle = 0.f; // Wipe tower rotation angle in degrees (with respect to x axis)
    float  m_internal_rotation  = 0.f;
	float  m_y_shift			= 0.f;  // y shift passed to writer
	float  m_z_pos 				= 0.f;  // Current Z position.
	float  m_layer_height 		= 0.f; 	// Current layer height.
	size_t m_max_color_changes 	= 0; 	// Maximum number of color changes per layer.
    int    m_old_temperature    = -1;   // To keep track of what was the last temp that we set (so we don't issue the command when not neccessary)
    float  m_travel_speed       = 0.f;
	float  m_infill_speed       = 0.f;
    float  m_wipe_tower_max_purge_speed   = 90.f;
	float  m_perimeter_speed    = 0.f;
    float  m_first_layer_speed  = 0.f;
    size_t m_first_layer_idx    = size_t(-1);

	// G-code generator parameters.
    float           m_bridging                  = 0.f;
    bool            m_no_sparse_layers          = false;
    bool            m_adhesion                  = true;
    GCodeFlavor     m_gcode_flavor;

	PrimeTowerEnhanceType m_prime_tower_enhance_type = PrimeTowerEnhanceType::pteCone;
    // Bed properties
    enum {
        RectangularBed,
        CircularBed,
        CustomBed
    } m_bed_shape;
    float m_bed_width; // width of the bed bounding box
    Vec2f m_bed_bottom_left; // bottom-left corner coordinates (for rectangular beds)

	float m_perimeter_width = 0.4f * Width_To_Nozzle_Ratio; // Width of an extrusion line, also a perimeter spacing for 100% infill.
	float m_extrusion_flow = 0.038f; //0.029f;// Extrusion flow is derived from m_perimeter_width, layer height and filament diameter.

	// Extruder specific parameters.
    std::vector<FilamentParameters> m_filpar;

	// State of the wipe tower generator.
	unsigned int m_num_layer_changes = 0; // Layer change counter for the output statistics.
	unsigned int m_num_tool_changes  = 0; // Tool change change counter for the output statistics.
	///unsigned int 	m_idx_tool_change_in_layer = 0; // Layer change counter in this layer. Counting up to m_max_color_changes.
	bool m_print_brim = true;
	// A fill-in direction (positive Y, negative Y) alternates with each layer.
	wipe_shape   	m_current_shape = SHAPE_NORMAL;
    size_t 	m_current_tool  = 0;
    const std::vector<std::vector<float>> wipe_volumes;

	float           m_depth_traversed = 0.f; // Current y position at the wipe tower.
    bool            m_current_layer_finished = false;
	bool 			m_left_to_right   = true;
	float			m_extra_spacing   = 1.f;

    bool is_first_layer() const { return size_t(m_layer_info - m_plan.begin()) == m_first_layer_idx; }

	// Calculates extrusion flow needed to produce required line width for given layer height
	float extrusion_flow(float layer_height = -1.f) const	// negative layer_height - return current m_extrusion_flow
	{
		if ( layer_height < 0 )
			return m_extrusion_flow;
		return layer_height * ( m_perimeter_width - layer_height * (1.f-float(M_PI)/4.f)) / filament_area();
	}

	// Calculates length of extrusion line to extrude given volume
	float volume_to_length(float volume, float line_width, float layer_height) const {
		return std::max(0.f, volume / (layer_height * (line_width - layer_height * (1.f - float(M_PI) / 4.f))));
	}

	// Calculates depth for all layers and propagates them downwards
	void plan_tower();

    // to store information about tool changes for a given layer
	struct WipeTowerInfo{
		struct ToolChange {
            size_t old_tool;
            size_t new_tool;
			float required_depth;
            float ramming_depth;
            float first_wipe_line;
            float wipe_volume;
            ToolChange(size_t old, size_t newtool, float depth=0.f, float ramming_depth=0.f, float fwl=0.f, float wv=0.f)
            : old_tool{old}, new_tool{newtool}, required_depth{depth}, ramming_depth{ramming_depth}, first_wipe_line{fwl}, wipe_volume{wv} {}
		};
		float z;		// z position of the layer
		float height;	// layer height
		float depth;	// depth of the layer based on all layers above
		float extra_spacing;
		float toolchanges_depth() const { float sum = 0.f; for (const auto &a : tool_changes) sum += a.required_depth; return sum; }

		std::vector<ToolChange> tool_changes;

		WipeTowerInfo(float z_par, float layer_height_par)
			: z{z_par}, height{layer_height_par}, depth{0}, extra_spacing{1.f} {}
	};

	std::vector<WipeTowerInfo> m_plan; 	// Stores information about all layers and toolchanges for the future wipe tower (filled by plan_toolchange(...))
	std::vector<WipeTowerInfo>::iterator m_layer_info = m_plan.end();

	// This sums height of all extruded layers, not counting the layers which
	// will be later removed when the "no_sparse_layers" is used.
	float m_current_height = 0.f;

    // Stores information about used filament length per extruder:
    std::vector<float> m_used_filament_length;

    // Return index of first toolchange that switches to non-soluble extruder
    // ot -1 if there is no such toolchange.
    int first_toolchange_to_nonsoluble(
            const std::vector<WipeTowerInfo::ToolChange>& tool_changes) const;

	void toolchange_Unload(
		WipeTowerWriterCreality &writer,
		const WipeTower::box_coordinates  &cleaning_box, 
		const std::string&	 	current_material,
		const int 				new_temperature);

	void toolchange_Change(
		WipeTowerWriterCreality &writer,
        const size_t		new_tool,
		const std::string& 		new_material);
	
	void toolchange_Wipe(
		WipeTowerWriterCreality &writer,
		const WipeTower::box_coordinates  &cleaning_box,
		float wipe_volume);
};

} // namespace Slic3r

#endif // WipeTowerCreality_ 
