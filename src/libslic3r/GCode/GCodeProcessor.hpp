#ifndef slic3r_GCodeProcessor_hpp_
#define slic3r_GCodeProcessor_hpp_

#include "libslic3r/GCodeReader.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/CustomGCode.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <mutex>
#include <string>
#include <string_view>
#include <optional>
#include <functional>
#ifdef SLIC3R_ENABLE_GCODE_IMPORT_PROFILE_OVERLAY_FOR_TEST
#include <utility>
#endif // SLIC3R_ENABLE_GCODE_IMPORT_PROFILE_OVERLAY_FOR_TEST

namespace Slic3r {

class Print;

// slice warnings enum strings
#define NOZZLE_HRC_CHECKER                                          "the_actual_nozzle_hrc_smaller_than_the_required_nozzle_hrc"
#define BED_TEMP_TOO_HIGH_THAN_FILAMENT                             "bed_temperature_too_high_than_filament"
#define NOT_SUPPORT_TRADITIONAL_TIMELAPSE                           "not_support_traditional_timelapse"
#define NOT_GENERATE_TIMELAPSE                                      "not_generate_timelapse"
#define LONG_RETRACTION_WHEN_CUT                                    "activate_long_retraction_when_cut"

    enum class EMoveType : unsigned char
    {
        Noop,
        Retract,
        Unretract,
        Seam,
        Tool_change,
        Color_change,
        Pause_Print,
        Custom_GCode,
        Travel,
        Wipe,
        Extrude,
        Extrude_Alter,  //Used to mean the toolpaths that be filtered for the gcode preview lite mode
        Count
    };

    struct PrintEstimatedStatistics
    {
        enum class ETimeMode : unsigned char
        {
            Normal,
            Stealth,
            Count
        };

        struct Mode
        {
            double time;
            float prepare_time;
            float flush_time;
            std::vector<std::pair<CustomGCode::Type, std::pair<float, float>>> custom_gcode_times;
            std::vector<std::pair<EMoveType, float>> moves_times;
            std::vector<std::pair<ExtrusionRole, float>> roles_times;
            std::vector<double> layers_times;
            double init_duration{ 0.0 };
            #ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
            std::vector<std::vector<std::pair<EMoveType, float>>> layer_moves_times;
            std::vector<std::vector<std::pair<ExtrusionRole, float>>> layer_roles_times;
            #endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

            void reset() {
                time = 0.0;
                prepare_time = 0.0f;
                flush_time = 0.0f;
                custom_gcode_times.clear();
                custom_gcode_times.shrink_to_fit();
                moves_times.clear();
                moves_times.shrink_to_fit();
                roles_times.clear();
                roles_times.shrink_to_fit();
                layers_times.clear();
                layers_times.shrink_to_fit();
                init_duration = 0.0;

                #ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
                layer_moves_times.clear();
                layer_moves_times.shrink_to_fit();
                layer_roles_times.clear();
                layer_roles_times.shrink_to_fit();
                #endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
            }

            // Shared "model printing time" definition for UI display and
            // Creality Cloud reporting entrypoints. Keep those interfaces
            // on model_time_s() so Klipper init_duration is handled consistently.
            static double time_excluding_init_duration(double time, double init_duration) {
                return std::max(0.0, time - init_duration);
            }

            double model_time_s() const {
                return time_excluding_init_duration(time, init_duration);
            }

            // Shared layer-time display view for the same UI semantics as
            // model_time_s(). Klipper init_duration is scalar, so subtract
            // it from the first layer bucket only.
            std::vector<double> model_layers_times() const {
                std::vector<double> ret = layers_times;
                if (!ret.empty())
                    ret.front() = time_excluding_init_duration(ret.front(), init_duration);
                return ret;
            }
        };

        // Creality
        double                                              total_filament_cost{0.0};
        double                                              total_estimated_time{0.0};

        std::vector<double>                                 volumes_per_color_change;
        std::map<size_t, double>                            model_volumes_per_extruder;
        std::map<size_t, double>                            wipe_tower_volumes_per_extruder;
        std::map<size_t, double>                            support_volumes_per_extruder;
        std::map<size_t, double>                            total_volumes_per_extruder;
        //BBS: the flush amount of every filament
        std::map<size_t, double>                            flush_per_filament;
        std::map<ExtrusionRole, std::pair<double, double>>  used_filaments_per_role;

        std::array<Mode, static_cast<size_t>(ETimeMode::Count)> modes;
        unsigned int                                        total_filamentchanges;

        PrintEstimatedStatistics() { reset(); }

        void reset() {
            for (auto &m : modes) {
                m.reset();
            }
            volumes_per_color_change.clear();
            volumes_per_color_change.shrink_to_fit();
            wipe_tower_volumes_per_extruder.clear();
            model_volumes_per_extruder.clear();
            support_volumes_per_extruder.clear();
            total_volumes_per_extruder.clear();
            flush_per_filament.clear();
            used_filaments_per_role.clear();

            total_filamentchanges = 0;
        }
    };

    struct ConflictResult
    {
        std::string        _objName1;
        std::string        _objName2;
        double             _height;
        const void *_obj1; // nullptr means wipe tower
        const void *_obj2;
        int                layer = -1;
        ConflictResult(const std::string &objName1, const std::string &objName2, double height, const void *obj1, const void *obj2)
            : _objName1(objName1), _objName2(objName2), _height(height), _obj1(obj1), _obj2(obj2)
        {}
        ConflictResult() = default;
    };

    struct ToolpathOutsideResult
    {
        std::string _objName;
        const void* _obj;
        ToolpathOutsideResult(const std::string& objName, const void* obj) : _objName(objName), _obj(obj) {}
        ToolpathOutsideResult() = default;
    };

    struct BedMatchResult
    {
        bool match;
        std::string bed_type_name;
        int extruder_id;
        BedMatchResult():match(true),bed_type_name(""),extruder_id(-1) {}
        BedMatchResult(bool _match,const std::string& _bed_type_name="",int _extruder_id=-1)
            :match(_match),bed_type_name(_bed_type_name),extruder_id(_extruder_id)
        {}
    };

    using ConflictResultOpt = std::optional<ConflictResult>;
    using ToolpathOutsideResultOpt = std::optional<ToolpathOutsideResult>;

    struct GCodeProcessorResult
    {
        ConflictResultOpt conflict_result;
        ToolpathOutsideResultOpt toolpath_outside_result;
        BedMatchResult  bed_match_result;

        struct SettingsIds
        {
            std::string print;
            std::vector<std::string> filament;
            std::string printer;

            void reset() {
                print.clear();
                filament.clear();
                printer.clear();
            }
        };

        float x_offset{0};
        float y_offset{0};
        float belt_z_offset{0};
        bool machine_is_belt{false};
        struct MoveVertex
        {
            unsigned int gcode_id{ 0 };
            EMoveType type{ EMoveType::Noop };
            ExtrusionRole extrusion_role{ erNone };
            unsigned char extruder_id{ 0 };
            unsigned char cp_color_id{ 0 };
            Vec3f position{ Vec3f::Zero() }; // mm
            float delta_extruder{ 0.0f }; // mm
            float feedrate{ 0.0f }; // mm/s
            float width{ 0.0f }; // mm
            float height{ 0.0f }; // mm
            float mm3_per_mm{ 0.0f };
            float fan_speed{ 0.0f }; // percentage
            float temperature{ 0.0f }; // Celsius degrees
            float time{ 0.0f }; // s
            float layer_duration{ 0.0f }; // s (layer id before finalize)
            float acceleration{ 0.0f };  //mm/s2

            //BBS: arc move related data
            EMovePathType move_path_type{ EMovePathType::Noop_move };
            Vec3f arc_center_position{ Vec3f::Zero() };      // mm
            std::vector<Vec3f> interpolation_points;     // interpolation points of arc for drawing

            float volumetric_rate() const { return feedrate * mm3_per_mm; }
            //BBS: new function to support arc move
            bool is_arc_move_with_interpolation_points() const {
                return (move_path_type == EMovePathType::Arc_move_ccw || move_path_type == EMovePathType::Arc_move_cw) && interpolation_points.size();
            }
            bool is_arc_move() const {
                return move_path_type == EMovePathType::Arc_move_ccw || move_path_type == EMovePathType::Arc_move_cw;
            }
        };

        struct SliceWarning {
            int         level;                  // 0: normal tips, 1: warning; 2: error
            std::string msg;                    // enum string
            std::string error_code;             // error code for studio
            std::vector<std::string> params;    // extra msg info
        };

        std::string filename;
        unsigned int id;
        std::vector<MoveVertex> moves;
        // Cached "interest region" classification per MoveVertex (indexed by move_id).
        // Filled during slicing (3mf workflow) and consumed by GUI preview; empty when loading a standalone .gcode.
        std::vector<unsigned char> custom_interest_by_move_id;
        // Label object id parsed from "; OBJECT_ID: <id>" markers (indexed by move_id).
        // Filled during slicing and can be used to apply per-object logic in post-processing / ROI detection.
        std::vector<int> object_id_by_move_id;
        // Positions of ends of lines of the final G-code this->filename after TimeProcessor::post_process() finalizes the G-code.
        std::vector<size_t> lines_ends;
        Pointfs printable_area;
        //BBS: add bed exclude area
        Pointfs bed_exclude_area;
        //BBS: add toolpath_outside
        bool toolpath_outside;
        //BBS: add object_label_enabled
        bool label_object_enabled;
        //BBS : extra retraction when change filament,experiment func
        bool long_retraction_when_cut {0};
        int timelapse_warning_code {0};
        bool support_traditional_timelapse{true};
        float printable_height;
        SettingsIds settings_ids;
        size_t extruders_count;
        bool backtrace_enabled;
        std::vector<std::string> extruder_colors;
        std::vector<float> filament_diameters;
        std::vector<int>   required_nozzle_HRC;
        std::vector<float> filament_densities;
        std::vector<float> filament_costs;
        std::vector<float> filament_flow_ratios;
        std::vector<int> filament_vitrification_temperature;
        PrintEstimatedStatistics print_statistics;
        std::vector<CustomGCode::Item> custom_gcode_per_print_z;
        std::vector<std::pair<float, std::pair<size_t, size_t>>> spiral_vase_layers;

        // creality
        std::vector<std::string>        creality_extruder_colors;
        std::vector<std::string>        creality_complete_extruder_colors;
        std::vector<std::string>        creality_default_extruder_colors;
        std::vector<std::string>        creality_extruder_types;
        std::array<bool, 256>           rendered_extruder_used{};
        float                           creality_flush_time{0};
        std::vector<std::vector<int>>   tool_change_path;        // record tool change
        std::vector<std::vector<float>> tool_change_volumes_map; // t1 -> t2 used volumne: tool_change_volumes_map[1][2]
        float                           flush_multiplier{1.0f};
        float                           defaultAcc;
        std::string                     printer_model;
        std::string                     printer_settings_id;
        std::string                     gcode_uuid;
        float                           nozzle_diameter;
        bool                            all_surface_with_shell{false};   //false as default value, preview old version gcode file cannot enable auto lite mode
        std::vector<int>                wipe_tower_tool_changes_layers;
		bool							should_enable_preview_lod;
		int                             max_printer_bed_temp{0};
        int                             max_printer_nozzle_temp{0};
        bool                            multicolor_method{0};
        std::vector<std::pair<std::array<int, 2>, std::vector<unsigned char>>> image_data;

        //BBS
        std::vector<SliceWarning> warnings;
        int nozzle_hrc;
        NozzleType nozzle_type;
        BedType bed_type = BedType::btCount;
#if ENABLE_GCODE_VIEWER_STATISTICS
        int64_t time{ 0 };
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        void reset();

        //BBS: add mutex for protection of gcode result
        mutable std::mutex result_mutex;
        GCodeProcessorResult& operator=(const GCodeProcessorResult &other)
        {
            filename = other.filename;
            id = other.id;
            moves = other.moves;
            custom_interest_by_move_id = other.custom_interest_by_move_id;
            object_id_by_move_id = other.object_id_by_move_id;
            lines_ends = other.lines_ends;
            printable_area = other.printable_area;
            bed_exclude_area = other.bed_exclude_area;
            toolpath_outside = other.toolpath_outside;
            label_object_enabled = other.label_object_enabled;
            long_retraction_when_cut = other.long_retraction_when_cut;
            timelapse_warning_code = other.timelapse_warning_code;
            printable_height = other.printable_height;
            settings_ids = other.settings_ids;
            extruders_count = other.extruders_count;
            extruder_colors = other.extruder_colors;
            filament_diameters = other.filament_diameters;
            filament_densities = other.filament_densities;
            filament_costs = other.filament_costs;
            filament_flow_ratios = other.filament_flow_ratios;
            print_statistics = other.print_statistics;
            custom_gcode_per_print_z = other.custom_gcode_per_print_z;
            spiral_vase_layers = other.spiral_vase_layers;
            warnings = other.warnings;
            bed_type = other.bed_type;
            bed_match_result = other.bed_match_result;
            // creality
            creality_extruder_colors = other.creality_extruder_colors;
            creality_complete_extruder_colors = other.creality_complete_extruder_colors;
            creality_default_extruder_colors = other.creality_default_extruder_colors;
            creality_extruder_types = other.creality_extruder_types;
            rendered_extruder_used = other.rendered_extruder_used;
            creality_flush_time = other.creality_flush_time;
            tool_change_path = other.tool_change_path;
            tool_change_volumes_map = other.tool_change_volumes_map;
            flush_multiplier                  = other.flush_multiplier;
            defaultAcc = other.defaultAcc;
            printer_model = other.printer_model;
            printer_settings_id = other.printer_settings_id;
            gcode_uuid = other.gcode_uuid;
            nozzle_diameter = other.nozzle_diameter;
            all_surface_with_shell = other.all_surface_with_shell;
            wipe_tower_tool_changes_layers = other.wipe_tower_tool_changes_layers;
            should_enable_preview_lod = other.should_enable_preview_lod;
            max_printer_bed_temp = other.max_printer_bed_temp;
            max_printer_nozzle_temp = other.max_printer_nozzle_temp;
            image_data = other.image_data;
            x_offset =other.x_offset;
            y_offset = other.y_offset;
#if ENABLE_GCODE_VIEWER_STATISTICS
            time = other.time;
#endif
            return *this;
        }


        void  take(const GCodeProcessorResult& other) {
            filename                 = other.filename;
            id                       = other.id;

            moves                    = std::move( other.moves );
            custom_interest_by_move_id = other.custom_interest_by_move_id;
            object_id_by_move_id = other.object_id_by_move_id;
            lines_ends               = std::move( other.lines_ends );

            printable_area           = other.printable_area;
            bed_exclude_area         = other.bed_exclude_area;
            toolpath_outside         = other.toolpath_outside;
            label_object_enabled     = other.label_object_enabled;
            long_retraction_when_cut = other.long_retraction_when_cut;
            timelapse_warning_code   = other.timelapse_warning_code;
            printable_height         = other.printable_height;
            settings_ids             = other.settings_ids;
            extruders_count          = other.extruders_count;
            extruder_colors          = other.extruder_colors;
            filament_diameters       = other.filament_diameters;
            filament_densities       = other.filament_densities;
            filament_costs           = other.filament_costs;
            filament_flow_ratios     = other.filament_flow_ratios;
            print_statistics         = other.print_statistics;
            custom_gcode_per_print_z = other.custom_gcode_per_print_z;
            spiral_vase_layers       = other.spiral_vase_layers;
            warnings                 = other.warnings;
            bed_type                 = other.bed_type;
            bed_match_result         = other.bed_match_result;
            // creality
            creality_extruder_colors          = other.creality_extruder_colors;
            creality_complete_extruder_colors = other.creality_complete_extruder_colors;
            creality_default_extruder_colors  = other.creality_default_extruder_colors;
            creality_extruder_types           = other.creality_extruder_types;
            rendered_extruder_used            = other.rendered_extruder_used;
            creality_flush_time               = other.creality_flush_time;
            tool_change_path                  = other.tool_change_path;
            tool_change_volumes_map           = other.tool_change_volumes_map;
            flush_multiplier                  = other.flush_multiplier;
            defaultAcc                        = other.defaultAcc;
            printer_model                     = other.printer_model;
            printer_settings_id                = other.printer_settings_id;
            gcode_uuid                        = other.gcode_uuid;
            nozzle_diameter                   = other.nozzle_diameter;
            all_surface_with_shell            = other.all_surface_with_shell;
            wipe_tower_tool_changes_layers    = other.wipe_tower_tool_changes_layers;
            should_enable_preview_lod         = other.should_enable_preview_lod;
            max_printer_bed_temp              = other.max_printer_bed_temp;
            max_printer_nozzle_temp           = other.max_printer_nozzle_temp;
            image_data                        = other.image_data;
            x_offset                          = other.x_offset;
            y_offset                          = other.y_offset;
            multicolor_method                 = other.multicolor_method;
            belt_z_offset                     = other.belt_z_offset;
            machine_is_belt                   = other.machine_is_belt;
#if ENABLE_GCODE_VIEWER_STATISTICS
            time = other.time;
#endif
        }
        void  lock() const { result_mutex.lock(); }
        void  unlock() const { result_mutex.unlock(); }
    };

    class GCodeProcessor
    {
        static const std::vector<std::string> Reserved_Tags;
        static const std::vector<std::string> Reserved_Tags_compatible;
        static const std::string Flush_Start_Tag;
        static const std::string Flush_End_Tag;
    public:
        enum class ETags : unsigned char
        {
            Role,
            Wipe_Start,
            Wipe_End,
            Height,
            Width,
            Layer_Change,
            Color_Change,
            Pause_Print,
            Custom_Code,
            First_Line_M73_Placeholder,
            Last_Line_M73_Placeholder,
            Estimated_Printing_Time_Placeholder,
            Time_Filament_Used,
            Total_Layer_Number_Placeholder,
            Manual_Tool_Change,
            During_Print_Exhaust_Fan,
            Wipe_Tower_Start,
            Wipe_Tower_End,
            Skeleton_Flush_Preview_Start,
            Skeleton_Flush_Preview_End,
        };

        static const std::string& reserved_tag(ETags tag) { return s_IsBBLPrinter ? Reserved_Tags[static_cast<unsigned char>(tag)] : Reserved_Tags_compatible[static_cast<unsigned char>(tag)]; }
        static const std::string& choose_reserved_tag(ETags tag, bool isBBLPrinter)
        {
            return isBBLPrinter ?
            Reserved_Tags[static_cast<unsigned char>(tag)] :
             Reserved_Tags_compatible[static_cast<unsigned char>(tag)];
        }
        // checks the given gcode for reserved tags and returns true when finding the 1st (which is returned into found_tag)
        static bool contains_reserved_tag(const std::string& gcode, std::string& found_tag);
        // checks the given gcode for reserved tags and returns true when finding any
        // (the first max_count found tags are returned into found_tag)
        static bool contains_reserved_tags(const std::string& gcode, unsigned int max_count, std::vector<std::string>& found_tag);

        static int get_gcode_last_filament(const std::string &gcode_str);
        static bool get_last_z_from_gcode(const std::string& gcode_str, double& z);
        static bool get_last_position_from_gcode(const std::string& gcode_str, Vec3f& pos);

        static const float Wipe_Width;
        static const float Wipe_Height;

        static bool s_IsBBLPrinter;
        static bool s_IsCFSPrinter;
        static float s_creality_flush_time;

#if ENABLE_GCODE_VIEWER_DATA_CHECKING
        static const std::string Mm3_Per_Mm_Tag;
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    private:
        using AxisCoords = std::array<double, 4>;
        using ExtruderColors = std::vector<unsigned char>;
        using ExtruderTemps = std::vector<float>;

        enum class EUnits : unsigned char
        {
            Millimeters,
            Inches
        };

        enum class EPositioningType : unsigned char
        {
            Absolute,
            Relative
        };

        struct CachedPosition
        {
            AxisCoords position; // mm
            float feedrate; // mm/s

            void reset();
        };

        struct CpColor
        {
            unsigned char counter;
            unsigned char current;

            void reset();
        };

    public:
        struct FeedrateProfile
        {
            float entry{ 0.0f }; // mm/s
            float cruise{ 0.0f }; // mm/s
            float exit{ 0.0f }; // mm/s
        };

        struct Trapezoid
        {
            float accelerate_until{ 0.0f }; // mm
            float decelerate_after{ 0.0f }; // mm
            float cruise_feedrate{ 0.0f }; // mm/sec
            float elapsed_time{ 0.0f }; // sec

            float acceleration_time(float entry_feedrate, float acceleration) const;
            float cruise_time() const;
            float deceleration_time(float distance, float acceleration) const;
            float cruise_distance() const;
        };

        struct TimeBlock
        {
            struct Flags
            {
                bool recalculate{ false };
                bool nominal_length{ false };
                bool prepare_stage{ false };
                bool flush_stage{ false };
                bool flush_related_stage{ false };
            };

            // Move properties used by Klipper ACCEL_TO_DECEL lookahead flush.
            struct Common
            {
                EMoveType      move_type{ EMoveType::Noop };
                ExtrusionRole  role{ erNone };
                unsigned int   g1_line_id{ 0 };
                unsigned int   layer_id{ 0 };
                float          distance{ 0.0f }; // mm
                float          acceleration{ 0.0f }; // mm/s^2
                float          max_entry_speed{ 0.0f }; // mm/s
                float          safe_feedrate{ 0.0f }; // mm/s
                float          extruder_e{ 0.0f };
                float          extruder_r{ 0.0f }; // Normalized extruder ratio (E per move distance).
                float          nominal_rate{ 0.0f };
                float          max_start_v2{ 0.0f };
                float          max_cruise_v2{ 0.0f };
                float          delta_v2{ 0.0f };
                float          min_move_t{ 0.0f };
                float          deceleration{ 0.0f }; // mm/s^2
                float          junction_deviation{ 0.05f };
                Vec3f          axes_r{ 1.0f, 0.0f, 0.0f }; // 归一化运动方向向量
                bool           is_kinematic_move{ true };
                float          instant_corner_v{ 10.0f };
            };

            // Start-speed budget propagated by the AccelToDecel solver.
            struct AccelToDecelSolver
            {
                float max_start_v2{ 0.0f };
                float delta_v2{ 0.0f };
            };

            #if 0 // new solver but is not used now beacause of firmware using accel_to_decel solver
            // Start-speed budget propagated by the MinimumCruiseRatio solver.
            struct MinimumCruiseRatioSolver
            {
                float max_start_v2{ 0.0f };
                float delta_v2{ 0.0f };
            };
            #endif // end of new solver

            // Final start / cruise / end v^2 chosen by the active solver.
            struct Solution
            {
                float start_v2{ 0.0f };
                float cruise_v2{ 0.0f };
                float end_v2{ 0.0f };
            };

            Flags flags;
            FeedrateProfile feedrate_profile;
            Trapezoid trapezoid;
            Common common;
            AccelToDecelSolver accel_to_decel;
            Solution solution;

            // Legacy cache fields formerly used by the inactive
            // process_G2_G3_klipper() path.
            // TODO: Delete these commented-out members once the current
            // Klipper time-estimation structure is considered stable.
            // float accel;
            // float move_d;
            // Calculates this block's trapezoid

            void calculate_trapezoid();
            void  prepare();

            void set_junction(float s_v2, float c_v2, float e_v2);

            void calc_junction(const TimeBlock& prev);
            float extruder_calc_juntion(const TimeBlock& prev);
            float calc_move_time() const;
            // float move_t;
            float time() const;
        };


    private:
        struct TimeMachine
        {
            // Legacy Marlin planner scratch state for the current/previous move being
            // processed. This is transient runtime data used by older planning paths,
            // not canonical machine-state.
            struct State
            {
                float feedrate; // mm/s
                float safe_feedrate; // mm/s
                //BBS: feedrate of X-Y-Z-E axis. But when the move is G2 and G3, X-Y will be
                //same value which means feedrate in X-Y plane.
                AxisCoords axis_feedrate; // mm/s
                AxisCoords abs_axis_feedrate; // mm/s

                //BBS: unit vector of enter speed and exit speed in x-y-z space.
                //For line move, there are same. For arc move, there are different.
                Vec3f enter_direction;
                Vec3f exit_direction;
                float delta_v2=99999;
                float max_start_v2 = 0;

                float junction_deviation;
                float max_cruise_v2 = 9999;
                float next_junction_v2 = 9999;

                float max_accel_to_decel_start_v2 = 0;
                float accel_to_decel_delta_v2 = 0;
                float acc=1000.0f; // Acceleration, unit mm/s^2

                void reset();
            };

            struct CustomGCodeTime
            {
                bool needed;
                float cache;
                std::vector<std::pair<CustomGCode::Type, float>> times;

                void reset();
            };

            struct G1LinesCacheItem
            {
                unsigned int id;
                double elapsed_time;
            };

            struct StopTime
            {
                unsigned int g1_line_id;
                double elapsed_time;
            };

            // Per-time-mode runtime context: accumulated time, queued/frozen blocks,
            // custom G-code timing caches, legacy planner scratch state, and
            // Klipper-only display-time alignment state. This block does not
            // own canonical Marlin/Klipper motion knobs.
            struct RuntimeContext
            {
                // Dedicated to the active Klipper estimation chain. This
                // mirrors the firmware print_stats init_duration boundary
                // without changing the raw time accounting buckets.
                struct PrintStartTracker
                {
                    double filament_used{ 0.0 };
                    bool started{ false };
                    double init_duration{ 0.0 };

                    void reset()
                    {
                        filament_used = 0.0;
                        started = false;
                        init_duration = 0.0;
                            }
                };

                // Whether this estimation mode's machine (Normal / Stealth) is active.
                // Normal is always enabled; Stealth only when the silent-mode estimator
                // is on (Marlin dual-mode). Per-mode loops and all time functions
                // (simulate_st_synchronize / flush_time_klipper / finalize_time_klipper /
                // calculate_time / account_klipper_blocks ...) skip the machine when false.
                bool enabled{ false };
                
                double time{ 0.0 };
                float prepare_time{ 0.0f };
                float additional_time{ 0.0f };
                float flushing_time{ 0.0f };
                float additional_flush_time{ 0.0f };
                PrintStartTracker print_start;
                std::vector<StopTime> stop_times;
                std::string line_m73_main_mask;
                std::string line_m73_stop_mask;
                CustomGCodeTime gcode_time;
                // blocks is legacy planner storage; it is not the active Klipper lookahead queue.
                std::vector<TimeBlock> blocks;
                std::vector<G1LinesCacheItem> g1_times_cache;
                std::array<float, static_cast<size_t>(EMoveType::Count)> moves_time;
                std::array<float, static_cast<size_t>(ExtrusionRole::erCount)> roles_time;
                std::vector<double> layers_time;

                // Legacy planner scratch only. The active Klipper lookahead path builds
                // and flushes TimeBlock objects directly and does not use curr/prev.
                State curr;
                State prev;

                // Runtime extrusion override written by M221. Legacy motion-limit
                // code and the Klipper print-start tracker both consult this.
                float extrude_factor_override_percentage{ 1.0f };

                #ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
                std::vector<std::array<float, static_cast<size_t>(EMoveType::Count)>> layer_moves_time;
                std::vector<std::array<float, static_cast<size_t>(ExtrusionRole::erCount)>> layer_roles_time;
                #endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
            };

            // Canonical Marlin planner state. Patch A cuts all Marlin getters/setters over to this block.
            struct MarlinMotionState
            {
                float acceleration{ 0.0f };
                float deceleration{ 0.0f };
                float max_acceleration{ 0.0f };
                float retract_acceleration{ 0.0f };
                float max_retract_acceleration{ 0.0f };
                float travel_acceleration{ 0.0f };
                float max_travel_acceleration{ 0.0f };
            };

            // Canonical Klipper ToolHead state. Klipper active estimation should only read from this block.
            struct KlipperToolheadState
            {
                float max_velocity{ 0.0f };
                float max_accel{ 0.0f };
                float requested_accel_to_decel{ 0.0f };
                float max_accel_to_decel{ 0.0f };
                float square_corner_velocity{ 5.0f };
                float junction_deviation{ 0.0f };
            };

            // Canonical Klipper Z special-case motion limits.
            struct KlipperZMotionState
            {
                float max_velocity{ 0.0f };
                float max_accel{ 0.0f };
            };

            // Canonical Klipper extruder-only / retract limits.
            struct KlipperExtruderState
            {
                float instant_corner_velocity{ 1.0f };
                float max_velocity{ 0.0f };
                float max_accel{ 0.0f };
            };

            RuntimeContext runtime;
            MarlinMotionState marlin;
            KlipperToolheadState klipper_toolhead;
            KlipperZMotionState klipper_z;
            KlipperExtruderState klipper_extruder;
            void reset();
            void initialize_klipper_toolhead_defaults(float base_velocity, float base_acceleration);
            void initialize_klipper_special_motion_limits(float base_z_velocity,
                                                          float base_z_accel,
                                                          float base_e_velocity,
                                                          float base_e_accel);
            // Simulates firmware st_synchronize() for legacy planner-backed firmware.
            void simulate_st_synchronize_legacy(float additional_time = 0.0f, bool count_as_flush = false);
            // Simulates firmware st_synchronize() for Klipper queue-backed timing.
            void simulate_st_synchronize_klipper(float additional_time = 0.0f, bool count_as_flush = false);
            void calculate_time(size_t keep_last_n_blocks = 0);
            // Active Klipper path: solve queue_ and account the flushed prefix without copying it to runtime.blocks.
            void flush_time_klipper(bool lazy);
            // Final Klipper drain. Klipper motion must be fully owned by queue_, never runtime.blocks.
            void finalize_time_klipper();

            bool add_move(TimeBlock&& move);
            bool should_flush() const;
            bool                  empty() const;
        private:
            void accumulate_print_start_init_time(double block_time);
            void update_print_start_tracker(double block_time, float extruder_e);
            // Handles pending additional time when there is no motion block to attach it to.
            void calculate_time_klipper_additional_time();
            // Shared accumulator for finalized Klipper blocks from queue_.
            void account_klipper_blocks(std::vector<TimeBlock>::const_iterator begin,
                                        std::vector<TimeBlock>::const_iterator end);
            // Runs ACCEL_TO_DECEL lookahead and returns the stable queue_ prefix length.
            size_t flush_accel_to_decel(bool lazy);

            // Current active canonical motion lookahead queue of Klipper.
            std::vector<TimeBlock> queue_;
            // Klipper: LOOKAHEAD_FLUSH_TIME = 0.250
            float       junction_flush_time_ = 0.25f;
            const float default_flush_time_  = 0.25f;
        };

        struct TimeProcessor
        {
            struct Planner
            {
                // Size of the firmware planner queue. The old 8-bit Marlins usually just managed 16 trapezoidal blocks.
                // Let's be conservative and plan for newer boards with more memory.
                static constexpr size_t queue_size = 64;
                // The firmware recalculates last planner_queue_size trapezoidal blocks each time a new block is added.
                // We are not simulating the firmware exactly, we calculate a sequence of blocks once a reasonable number of blocks accumulate.
                static constexpr size_t refresh_threshold = queue_size * 4;
            };

            // extruder_id is currently used to correctly calculate filament load / unload times into the total print time.
            // This is currently only really used by the MK3 MMU2:
            // extruder_unloaded = true means no filament is loaded yet, all the filaments are parked in the MK3 MMU2 unit.
            bool extruder_unloaded;
            // allow to skip the lines M201/M203/M204/M205 generated by GCode::print_machine_envelope() for non-Normal time estimate mode
            bool machine_envelope_processing_enabled;
            MachineEnvelopeConfig machine_limits;
            // Additional load / unload times for a filament exchange sequence.
            std::vector<float> filament_load_times;
            std::vector<float> filament_unload_times;
            float machine_tool_change_time;
            bool  disable_m73;

            std::array<TimeMachine, static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Count)> machines;

            void reset();

            // post process the file with the given filename to add remaining time lines M73
            // and updates moves' gcode ids accordingly
            void post_process(const std::string& filename, std::vector<GCodeProcessorResult::MoveVertex>& moves, std::vector<size_t>& lines_ends, size_t total_layer_num, float filament_used = 0.0f,float flush_times = 0.0f);
        };

        struct UsedFilaments  // filaments per ColorChange
        {
            double color_change_cache;
            std::vector<double> volumes_per_color_change;

            double model_extrude_cache;
            std::map<size_t, double> model_volumes_per_extruder;

            double wipe_tower_cache;
            std::map<size_t, double>wipe_tower_volumes_per_extruder;

            double support_volume_cache;
            std::map<size_t, double>support_volumes_per_extruder;

            //BBS: the flush amount of every filament
            std::map<size_t, double> flush_per_filament;

            double total_volume_cache;
            std::map<size_t, double>total_volumes_per_extruder;

            double role_cache;
            std::map<ExtrusionRole, std::pair<double, double>> filaments_per_role;

            void reset();

            void increase_support_caches(double extruded_volume);
            void increase_model_caches(double extruded_volume);
            void increase_wipe_tower_caches(double extruded_volume);

            void process_color_change_cache();
            void process_model_cache(GCodeProcessor* processor);
            void process_wipe_tower_cache(GCodeProcessor* processor);
            void process_support_cache(GCodeProcessor* processor);
            void process_total_volume_cache(GCodeProcessor* processor);

            void update_flush_per_filament(size_t extrude_id, float flush_length);
            void process_role_cache(GCodeProcessor* processor);
            void process_caches(GCodeProcessor* processor);

            friend class GCodeProcessor;
        };

    public:
        class SeamsDetector
        {
            bool m_active{ false };
            std::optional<Vec3f> m_first_vertex;

        public:
            void activate(bool active) {
                if (m_active != active) {
                    m_active = active;
                    if (m_active)
                        m_first_vertex.reset();
                }
            }

            std::optional<Vec3f> get_first_vertex() const { return m_first_vertex; }
            void set_first_vertex(const Vec3f& vertex) { m_first_vertex = vertex; }

            bool is_active() const { return m_active; }
            bool has_first_vertex() const { return m_first_vertex.has_value(); }
        };

        // Helper class used to fix the z for color change, pause print and
        // custom gcode markes
        class OptionsZCorrector
        {
            GCodeProcessorResult& m_result;
            std::optional<size_t> m_move_id;
            std::optional<size_t> m_custom_gcode_per_print_z_id;

        public:
            explicit OptionsZCorrector(GCodeProcessorResult& result) : m_result(result) {
            }

            void set() {
                m_move_id = m_result.moves.size() - 1;
                m_custom_gcode_per_print_z_id = m_result.custom_gcode_per_print_z.size() - 1;
            }

            void update(float height) {
                if (!m_move_id.has_value() || !m_custom_gcode_per_print_z_id.has_value())
                    return;

                const Vec3f position = m_result.moves.back().position;

                GCodeProcessorResult::MoveVertex& move = m_result.moves.emplace_back(m_result.moves[*m_move_id]);
                move.position = position;
                move.height = height;
                m_result.moves.erase(m_result.moves.begin() + *m_move_id);
                m_result.custom_gcode_per_print_z[*m_custom_gcode_per_print_z_id].print_z = position.z();
                reset();
            }

            void reset() {
                m_move_id.reset();
                m_custom_gcode_per_print_z_id.reset();
            }
        };

#if ENABLE_GCODE_VIEWER_DATA_CHECKING
        struct DataChecker
        {
            struct Error
            {
                float value;
                float tag_value;
                ExtrusionRole role;
            };

            std::string type;
            float threshold{ 0.01f };
            float last_tag_value{ 0.0f };
            unsigned int count{ 0 };
            std::vector<Error> errors;

            DataChecker(const std::string& type, float threshold)
                : type(type), threshold(threshold)
            {}

            void update(float value, ExtrusionRole role) {
                if (role != erCustom) {
                    ++count;
                    if (last_tag_value != 0.0f) {
                        if (std::abs(value - last_tag_value) / last_tag_value > threshold)
                            errors.push_back({ value, last_tag_value, role });
                    }
                }
            }

            void reset() { last_tag_value = 0.0f; errors.clear(); count = 0; }

            std::pair<float, float> get_min() const {
                float delta_min = FLT_MAX;
                float perc_min = 0.0f;
                for (const Error& e : errors) {
                    if (delta_min > e.value - e.tag_value) {
                        delta_min = e.value - e.tag_value;
                        perc_min = 100.0f * delta_min / e.tag_value;
                    }
                }
                return { delta_min, perc_min };
            }

            std::pair<float, float> get_max() const {
                float delta_max = -FLT_MAX;
                float perc_max = 0.0f;
                for (const Error& e : errors) {
                    if (delta_max < e.value - e.tag_value) {
                        delta_max = e.value - e.tag_value;
                        perc_max = 100.0f * delta_max / e.tag_value;
                    }
                }
                return { delta_max, perc_max };
            }

            void output() const {
                if (!errors.empty()) {
                    std::cout << type << ":\n";
                    std::cout << "Errors: " << errors.size() << " (" << 100.0f * float(errors.size()) / float(count) << "%)\n";
                    auto [min, perc_min] = get_min();
                    auto [max, perc_max] = get_max();
                    std::cout << "min: " << min << "(" << perc_min << "%) - max: " << max << "(" << perc_max << "%)\n";
                }
            }
        };
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    private:
        GCodeReader m_parser;
        EUnits m_units;
        EPositioningType m_global_positioning_type;
        EPositioningType m_e_local_positioning_type;
        std::vector<Vec3f> m_extruder_offsets;
        GCodeFlavor m_flavor;

        float       m_nozzle_volume;
        AxisCoords m_start_position; // mm
        AxisCoords m_end_position; // mm
        AxisCoords m_origin; // mm
        CachedPosition m_cached_position;

        bool m_wiping;
        // True while processing the explicit ; FLUSH_START ... ; FLUSH_END section.
        // Motion blocks generated in this window are counted as flush time.
        bool               m_flushing = false;

        // True while processing toolchange/wipe-tower/CFS stages that are not always
        // inside the explicit flush window, but should still be attributed to flush time.
        bool               m_flush_related_stage = false;
        bool               m_cfs_change_stage = false;
        bool m_wipe_tower;
        bool m_has_extruded = false;
        float m_remaining_volume;
        bool m_manual_filament_change;

        // creality
        float m_currentAcc{ 0 };
        bool m_reading_image{ false };
        std::vector<std::pair<std::array<int, 2>, std::string>> image_data_cache;

        //BBS: x, y offset for gcode generated
        double          m_x_offset{ 0 };
        double          m_y_offset{ 0 };
        //BBS: arc move related data
        EMovePathType m_move_path_type{ EMovePathType::Noop_move };
        Vec3f m_arc_center{ Vec3f::Zero() };    // mm
        std::vector<Vec3f> m_interpolation_points;

        unsigned int m_line_id;
        unsigned int m_last_line_id;
        float m_feedrate; // mm/s
        float m_width; // mm
        float m_height; // mm
        float m_forced_width; // mm
        float m_forced_height; // mm
        float m_mm3_per_mm;
        float m_fan_speed; // percentage
        float m_z_offset; // mm
        ExtrusionRole m_extrusion_role;
        int m_object_id{ -1 }; // Current label object id from "; OBJECT_ID:" comment markers.
        unsigned char m_extruder_id;
        unsigned char m_last_extruder_id;
        int m_skeleton_flush_preview_extruder_id;
        ExtruderColors m_extruder_colors;
        ExtruderTemps m_extruder_temps;
        ExtruderTemps m_extruder_temps_config;
        ExtruderTemps m_extruder_temps_first_layer_config;
        bool  m_is_XL_printer = false;
        int m_highest_bed_temp;
        float m_extruded_last_z;
        float m_first_layer_height; // mm
        float m_zero_layer_height; // mm
        bool m_processing_start_custom_gcode;
        unsigned int m_g1_line_id;
        unsigned int m_layer_id;
        CpColor m_cp_color;
        SeamsDetector m_seams_detector;
        OptionsZCorrector m_options_z_corrector;
        size_t m_last_default_color_id;
        bool m_detect_layer_based_on_tag {false};
        int m_seams_count;
        bool m_single_extruder_multi_material;
        float m_preheat_time;
        int m_preheat_steps;
        float             m_flush_time = 2.0;
#if ENABLE_GCODE_VIEWER_STATISTICS
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

        enum class EProducer
        {
            Unknown,
            CrealityPrint,
            Slic3rPE,
            Slic3r,
            SuperSlicer,
            Cura,
            Simplify3D,
            CraftWare,
            ideaMaker,
            KissSlicer
        };

        static const std::vector<std::pair<GCodeProcessor::EProducer, std::string>> Producers;
        EProducer m_producer;

        TimeProcessor m_time_processor;
        UsedFilaments m_used_filaments;

        Print* m_print{ nullptr };

        GCodeProcessorResult m_result;
        static unsigned int s_result_id;

#if ENABLE_GCODE_VIEWER_DATA_CHECKING
        DataChecker m_mm3_per_mm_compare{ "mm3_per_mm", 0.01f };
        DataChecker m_height_compare{ "height", 0.01f };
        DataChecker m_width_compare{ "width", 0.01f };
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

    public:
        GCodeProcessor();

        void apply_config(const PrintConfig& config);
        void set_print(Print* print) { m_print = print; }
        void enable_stealth_time_estimator(bool enabled);
        bool is_stealth_time_estimator_enabled() const {
            return m_time_processor.machines[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Stealth)].runtime.enabled;
        }
        void enable_machine_envelope_processing(bool enabled) { m_time_processor.machine_envelope_processing_enabled = enabled; }
        void reset();

        const GCodeProcessorResult& get_result() const { return m_result; }
        GCodeProcessorResult& result() { return m_result; }
        GCodeProcessorResult&& extract_result() { return std::move(m_result); }

        // Load a G-code into a stand-alone G-code viewer.
        // throws CanceledException through print->throw_if_canceled() (sent by the caller as callback).
        void process_file(const std::string& filename, std::function<void()> cancel_callback = nullptr);

#ifdef SLIC3R_ENABLE_GCODE_IMPORT_PROFILE_OVERLAY_FOR_TEST
        struct GCodeImportProfileOverlay {
            DynamicPrintConfig config;
            std::string profile_name;
            std::string profile_file;
            std::string profile_source;
            std::string profile_setting_id;
            std::string match_rule;
            std::string gcode_printer_settings_id;
            std::string gcode_printer_model;
            std::string gcode_nozzle_diameter;
            std::string gcode_candidate_name;
        };

        using GCodeImportProfileOverlayResolver = std::function<std::optional<GCodeImportProfileOverlay>(const DynamicPrintConfig&, const std::string&)>;
        void set_gcode_import_profile_overlay_resolver(GCodeImportProfileOverlayResolver resolver)
        {
            m_gcode_import_profile_overlay_resolver = std::move(resolver);
        }
#endif // SLIC3R_ENABLE_GCODE_IMPORT_PROFILE_OVERLAY_FOR_TEST

        // Streaming interface, for processing G-codes just generated by PrusaSlicer in a pipelined fashion.
        void initialize(const std::string& filename);
        void process_buffer(const std::string& buffer);
        void  finalize(bool post_process, float filament_used = 0.0f, float flush_time = 0.0f, bool is_multicolor_method = false);
        float layer_time();
        float layer_flow();

        double get_time(PrintEstimatedStatistics::ETimeMode mode) const;
        float get_prepare_time(PrintEstimatedStatistics::ETimeMode mode) const;
        float get_flush_time(PrintEstimatedStatistics::ETimeMode mode) const;
        std::string get_time_dhm(PrintEstimatedStatistics::ETimeMode mode) const;
        std::vector<std::pair<CustomGCode::Type, std::pair<float, float>>> get_custom_gcode_times(PrintEstimatedStatistics::ETimeMode mode, bool include_remaining) const;

        std::vector<std::pair<EMoveType, float>> get_moves_time(PrintEstimatedStatistics::ETimeMode mode) const;
        std::vector<std::pair<ExtrusionRole, float>> get_roles_time(PrintEstimatedStatistics::ETimeMode mode) const;
        std::vector<double> get_layers_time(PrintEstimatedStatistics::ETimeMode mode) const;

        #ifdef SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT
        std::vector<std::vector<std::pair<EMoveType, float>>> get_layer_moves_time(PrintEstimatedStatistics::ETimeMode mode) const;
        std::vector<std::vector<std::pair<ExtrusionRole, float>>> get_layer_roles_time(PrintEstimatedStatistics::ETimeMode mode) const;
        #endif // SLIC3R_ENABLE_TIME_ANALYTICS_EXPORT

        //BBS: set offset for gcode writer
        void set_xy_offset(double x, double y) {
            m_x_offset = x;
            m_y_offset = y;
            m_result.x_offset = m_x_offset;
            m_result.y_offset = m_y_offset;
        }

        // Orca: if true, only change new layer if ETags::Layer_Change occurs
        // otherwise when we got a lift of z during extrusion, a new layer will be added
        void detect_layer_based_on_tag(bool enabled) { m_detect_layer_based_on_tag = enabled; }

    private:
        void apply_config(const DynamicPrintConfig& config);
#ifdef SLIC3R_ENABLE_GCODE_IMPORT_PROFILE_OVERLAY_FOR_TEST
        GCodeImportProfileOverlayResolver m_gcode_import_profile_overlay_resolver;
        void apply_gcode_import_profile_overlay_for_test(const std::string& filename, DynamicPrintConfig& config);
#endif // SLIC3R_ENABLE_GCODE_IMPORT_PROFILE_OVERLAY_FOR_TEST
        void apply_config_simplify3d(const std::string& filename);
        void apply_config_superslicer(const std::string& filename);
        void apply_config_cura(const std::string& filename);
        void process_gcode_line(const GCodeReader::GCodeLine& line, bool producers_enabled);

        // Process tags embedded into comments
        void process_tags(const std::string_view comment, bool producers_enabled);
        bool process_producers_tags(const std::string_view comment);

        //Creality
        void prepare_process_creality_image();
        void begin_process_creality_image(const std::string& comment);
        bool process_creality_image(const std::string& comment);
        void end_process_creality_image();
        bool process_creality_tags(const std::string_view comment);

        bool process_bambuslicer_tags(const std::string_view comment);
        bool process_cura_tags(const std::string_view comment);
        bool process_simplify3d_tags(const std::string_view comment);
        bool process_craftware_tags(const std::string_view comment);
        bool process_ideamaker_tags(const std::string_view comment);
        bool process_kissslicer_tags(const std::string_view comment);

        bool detect_producer(const std::string_view comment);

        // Legacy experimental helper kept for reference only; currently unused.
        // void flush_time(std::vector<TimeBlock>& queue, bool lazy = false);

        // Move
        void process_G0(const GCodeReader::GCodeLine& line);
        void process_G1(const GCodeReader::GCodeLine& line);
        void process_G2_G3(const GCodeReader::GCodeLine& line);
        void process_G1_klipper(const GCodeReader::GCodeLine& line);
        // Legacy experimental path, currently unused.
        // void process_G1_klipper_new(const GCodeReader::GCodeLine& line);
        void process_G2_G3_new_klipper(const GCodeReader::GCodeLine& line);
        // Legacy experimental path, currently unused.
        // void process_G2_G3_klipper_new(const GCodeReader::GCodeLine& line);

        // BBS: handle delay command
        void process_G4(const GCodeReader::GCodeLine& line);

        // Retract
        void process_G10(const GCodeReader::GCodeLine& line);

        // Unretract
        void process_G11(const GCodeReader::GCodeLine& line);

        // Set Units to Inches
        void process_G20(const GCodeReader::GCodeLine& line);

        // Set Units to Millimeters
        void process_G21(const GCodeReader::GCodeLine& line);

        // Firmware controlled Retract
        void process_G22(const GCodeReader::GCodeLine& line);

        // Firmware controlled Unretract
        void process_G23(const GCodeReader::GCodeLine& line);

        // Move to origin
        void process_G28(const GCodeReader::GCodeLine& line);

        // BBS
        void process_G29(const GCodeReader::GCodeLine& line);

        // Set to Absolute Positioning
        void process_G90(const GCodeReader::GCodeLine& line);

        // Set to Relative Positioning
        void process_G91(const GCodeReader::GCodeLine& line);

        // Set Position
        void process_G92(const GCodeReader::GCodeLine& line);

        // Sleep or Conditional stop
        void process_M1(const GCodeReader::GCodeLine& line);

        // Set extruder to absolute mode
        void process_M82(const GCodeReader::GCodeLine& line);

        // Set extruder to relative mode
        void process_M83(const GCodeReader::GCodeLine& line);

        // Set extruder temperature
        void process_M104(const GCodeReader::GCodeLine& line);

        // Set fan speed
        void process_M106(const GCodeReader::GCodeLine& line);

        // Disable fan
        void process_M107(const GCodeReader::GCodeLine& line);

        // Set tool (Sailfish)
        void process_M108(const GCodeReader::GCodeLine& line);

        // Set extruder temperature and wait
        void process_M109(const GCodeReader::GCodeLine& line);

        // Recall stored home offsets
        void process_M132(const GCodeReader::GCodeLine& line);

        // Set tool (MakerWare)
        void process_M135(const GCodeReader::GCodeLine& line);

        //BBS: Set bed temperature
        void process_M140(const GCodeReader::GCodeLine& line);

        //BBS: wait bed temperature
        void process_M190(const GCodeReader::GCodeLine& line);

        //BBS: wait chamber temperature
        void process_M191(const GCodeReader::GCodeLine& line);

        // Set max printing acceleration
        void process_M201(const GCodeReader::GCodeLine& line);

        // Set maximum feedrate
        void process_M203(const GCodeReader::GCodeLine& line);

        // Set default acceleration
        void process_M204(const GCodeReader::GCodeLine& line);

        // Advanced settings
        void process_M205(const GCodeReader::GCodeLine& line);

        // Klipper SET_VELOCITY_LIMIT
        void process_SET_VELOCITY_LIMIT(const GCodeReader::GCodeLine& line);

        //SET Define TEMP
        void process_define_TEMP(const GCodeReader::GCodeLine& line);

        // Set extrude factor override percentage
        void process_M221(const GCodeReader::GCodeLine& line);

        // BBS: handle delay command. M400 is defined by BBL only
        void process_M400(const GCodeReader::GCodeLine& line);

        // Repetier: Store x, y and z position
        void process_M401(const GCodeReader::GCodeLine& line);

        // Repetier: Go to stored position
        void process_M402(const GCodeReader::GCodeLine& line);

        // Set allowable instantaneous speed change
        void process_M566(const GCodeReader::GCodeLine& line);

        // Unload the current filament into the MK3 MMU2 unit at the end of print.
        void process_M702(const GCodeReader::GCodeLine& line);

        // Processes T line (Select Tool)
        void process_T(const GCodeReader::GCodeLine& line);
        void process_T(const std::string_view command);

        void process_M8200(const GCodeReader::GCodeLine& line);
        void process_M8200P(const GCodeReader::GCodeLine& line);
        void process_M8200R(const GCodeReader::GCodeLine& line);
        void process_M8200C(const GCodeReader::GCodeLine& line);
        void process_M8200L(const GCodeReader::GCodeLine& line);
        void process_M8200O(const GCodeReader::GCodeLine& line);

        // post process the file with the given filename to:
        // 1) add remaining time lines M73 and update moves' gcode ids accordingly
        // 2) update used filament data
        void run_post_process();

        //BBS: different path_type is only used for arc move
        void store_move_vertex(EMoveType type, EMovePathType path_type = EMovePathType::Noop_move);

        void set_extrusion_role(ExtrusionRole role);

        float minimum_feedrate(PrintEstimatedStatistics::ETimeMode mode, float feedrate) const;
        float minimum_travel_feedrate(PrintEstimatedStatistics::ETimeMode mode, float feedrate) const;
        float get_axis_max_feedrate(PrintEstimatedStatistics::ETimeMode mode, Axis axis) const;
        float get_axis_max_acceleration(PrintEstimatedStatistics::ETimeMode mode, Axis axis) const;
        float get_axis_max_jerk(PrintEstimatedStatistics::ETimeMode mode, Axis axis) const;
        Vec3f get_xyz_max_jerk(PrintEstimatedStatistics::ETimeMode mode) const;
        float get_retract_acceleration(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_retract_acceleration(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_acceleration(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_acceleration(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_requested_accel_to_decel(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_requested_accel_to_decel(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_max_accel_to_decel(PrintEstimatedStatistics::ETimeMode mode) const;
        void  sync_max_accel_to_decel(PrintEstimatedStatistics::ETimeMode mode);
        void  set_deceleration(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_deceleration(PrintEstimatedStatistics::ETimeMode mode) const;
        float get_square_corner_velocity(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_square_corner_velocity(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_extruder_instant_corner_velocity(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_extruder_instant_corner_velocity(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_klipper_junction_deviation(PrintEstimatedStatistics::ETimeMode mode) const;
        void  sync_klipper_junction_deviation(PrintEstimatedStatistics::ETimeMode mode);
        float get_klipper_max_velocity(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_klipper_max_velocity(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_klipper_max_z_velocity(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_klipper_max_z_velocity(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_klipper_max_z_accel(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_klipper_max_z_accel(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_klipper_max_e_velocity(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_klipper_max_e_velocity(PrintEstimatedStatistics::ETimeMode mode, float value);
        float get_klipper_max_e_accel(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_klipper_max_e_accel(PrintEstimatedStatistics::ETimeMode mode, float value);
		float get_travel_acceleration(PrintEstimatedStatistics::ETimeMode mode) const;
        void  set_travel_acceleration(PrintEstimatedStatistics::ETimeMode mode, float value);
        TimeBlock build_klipper_time_block(const TimeMachine& machine,
                                           EMoveType type,
                                           const AxisCoords& delta_pos,
                                           float distance,
                                           bool extrusion_only_move) const;
        float get_filament_load_time(size_t extruder_id);
        float get_filament_unload_time(size_t extruder_id);
        float get_tool_change_time() const;
        int   get_filament_vitrification_temperature(size_t extrude_id);
        void process_custom_gcode_time(CustomGCode::Type code);
        void process_filaments(CustomGCode::Type code);

        // Simulates firmware st_synchronize() call
        void simulate_st_synchronize(float additional_time = 0.0f, bool force_flush_time = false);

        void update_estimated_times_stats();
        //BBS:
        void update_slice_warnings();

        void calculateVolume();
   };

} /* namespace Slic3r */

#endif /* slic3r_GCodeProcessor_hpp_ */
