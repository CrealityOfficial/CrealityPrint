#include <sstream>
#include <fstream>
#include <memory>
#include <optional>
#include "crslice/gcode/parasegcode.h"
#include "gcodeprocesslib/gcode_position.h"
#include "gcodeprocesslib/gcode_parser.h"
#include "crslice/gcode/thumbnail.h"
#include "gcode/sliceline.h"
#include "ccglobal/platform.h"

#include "boost/nowide/cstdio.hpp"

#define DEFAULT_GCODE_BUFFER_SIZE 50
#define DEFAULT_G90_G91_INFLUENCES_EXTRUDER false

#define sqr(x) x*x

namespace gcode
{
    enum GCodeType
    {
        ColorChange,
        PausePrint,
        ToolChange,
        Template,
        Custom,
        Unknown,
    };

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
        Count
    };

    struct UsedFilaments  // filaments per ColorChange
    {
        double color_change_cache{0.0f};
        std::vector<double> volumes_per_color_change;

        double Wipe_cache{ 0.0f };
        std::map<int, double> volumes_per_tower;

        double tool_change_cache{ 0.0f };
        std::map<int, double> volumes_per_extruder;

        //BBS: the flush amount of every filament
        std::map<int, double> flush_per_filament;
        std::map<size_t, int> flush_icount_per_filament;

        //double role_cache;
        //std::map<ExtrusionRole, std::pair<double, double>> filaments_per_role;
        void process_color_change_cache()
        {
            if (color_change_cache != 0.0f) {
                volumes_per_color_change.push_back(color_change_cache);
                color_change_cache = 0.0f;
            }
        }

        void process_extruder_cache(int active_extruder_id)
        {
            //size_t active_extruder_id = processor->m_extruder_id;
            if (tool_change_cache >= 0.0f) {
                if (volumes_per_extruder.find(active_extruder_id) != volumes_per_extruder.end())
                    volumes_per_extruder[active_extruder_id] += tool_change_cache;
                else
                    volumes_per_extruder[active_extruder_id] = tool_change_cache;
                tool_change_cache = 0.0f;
            }

            if (Wipe_cache > 0.0f) {
                if (volumes_per_tower.find(active_extruder_id) != volumes_per_tower.end())
                    volumes_per_tower[active_extruder_id] += Wipe_cache;
                else
                    volumes_per_tower[active_extruder_id] = Wipe_cache;
                Wipe_cache = 0.0f;
            }
        }

        void update_flush_per_filament(size_t extrude_id, float flush_volume)
		{
			if (flush_per_filament.find(extrude_id) != flush_per_filament.end())
				flush_per_filament[extrude_id] += flush_volume;
			else
				flush_per_filament[extrude_id] = flush_volume;
		}

        void increase_caches(double extruded_volume)
        {
            color_change_cache += extruded_volume;
            tool_change_cache += extruded_volume;
            //role_cache += extruded_volume;
        }

        void increase_color_change_caches(double extruded_volume)
        {
            Wipe_cache += extruded_volume;
        }

        void reset()
        {
            color_change_cache = 0.0f;
            Wipe_cache = 0.0f;
            volumes_per_color_change = std::vector<double>();

            tool_change_cache = 0.0f;
            volumes_per_extruder.clear();
            volumes_per_tower.clear();
            flush_per_filament.clear();

            //role_cache = 0.0f;
            //filaments_per_role.clear();
        }
    };

    class SeamsDetector
    {
        bool m_active{ false };
        std::optional<trimesh::vec3> m_first_vertex;

        public:
        void activate(bool active) {
            if (m_active != active) {
                m_active = active;
                if (m_active)
                    m_first_vertex.reset();
            }
        }

        std::optional<trimesh::vec3> get_first_vertex() const { return m_first_vertex; }
        void set_first_vertex(const trimesh::vec3& vertex) { m_first_vertex = vertex; }

        bool is_active() const { return m_active; }
        bool has_first_vertex() const { return m_first_vertex.has_value(); }
    };

    struct GCodeProcessor {

        SliceCompany sliceCompany = SliceCompany::none;
        std::unordered_map<std::string, std::string> kvs;

        GCodeParseInfo gcodeParaseInfo;
        UsedFilaments m_used_filaments;


        SliceLineType curType = SliceLineType::NoneType;
        SliceLineType lastType = SliceLineType::NoneType;
        EMoveType eMoveType{ EMoveType::Noop};
        bool haveStartCmd{ false };
        bool have_start_print{ false };

        int m_extruder_id{0};
        int m_last_extruder_id;
        int extruders_count;
        trimesh::box3 box;
        float m_nozzle_volume;
        float m_remaining_volume;

        bool m_flushing{false};
        bool m_Firmware_flushing{ false };
        bool m_wiping{ false };

        bool isFirstLayerHeight{ false };

        double current_e;
        trimesh::vec3 current_v;

        std::set <int> extruders;

        std::vector<std::pair<trimesh::ivec2, std::string>> images;

        bool writeImage{false};

        //seam
        SeamsDetector m_seams_detector;

        std::vector<float> filament_diameters;
        std::vector<float> filament_costs;
        std::vector<float> material_densitys;
        GCodeProcessor()
        {
            m_extruder_id = 0;
            m_nozzle_volume = 0.0f;
            extruders_count = 0;
            m_remaining_volume = 0.0f;
            filament_diameters.clear();
            material_densitys.clear();
            filament_costs.clear();
        }
    };

    gcode_position_args get_args_(bool g90_g91_influences_extruder, int buffer_size)
    {
        gcode_position_args args;
        // Configure gcode_position_args
        args.g90_influences_extruder = g90_g91_influences_extruder;
        args.position_buffer_size = buffer_size;
        args.autodetect_position = true;
        args.home_x = 0;
        args.home_x_none = true;
        args.home_y = 0;
        args.home_y_none = true;
        args.home_z = 0;
        args.home_z_none = true;
        args.shared_extruder = true;
        args.zero_based_extruder = true;


        args.default_extruder = 0;
        args.xyz_axis_default_mode = "absolute";
        args.e_axis_default_mode = "absolute";
        args.units_default = "millimeters";
        args.location_detection_commands = std::vector<std::string>();
        args.is_bound_ = false;
        args.is_circular_bed = false;
        args.x_min = -9999;
        args.x_max = 9999;
        args.y_min = -9999;
        args.y_max = 9999;
        args.z_min = -9999;
        args.z_max = 9999;
        return args;
    }

    void removeSpace(std::string& str)
    {
        str.erase(0, str.find_first_not_of(" "));
        str.erase(str.find_last_not_of(" ") + 1);
    }
    
    void Stringsplit(std::string str, const char split, std::vector<std::string>& res)
    {
        std::istringstream iss(str);	// 输入流
        std::string token;			// 接收缓冲区
        while (getline(iss, token, split))	// 以split为分隔符
        {
            removeSpace(token);
            res.push_back(token);
        }
    }

    void changeKey(const std::string& cource, const std::string& dest, std::unordered_map<std::string, std::string>& kvs)
    {
        auto iter = kvs.find(cource);
        if (iter != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ';', _kvs);
            if (_kvs.size() > 1) {
                iter->second = _kvs[0];
            }
            kvs.insert(std::make_pair(dest, iter->second));
            kvs.erase(iter);
        }
    }

    void getKvs(const std::string& comment, const SliceCompany& sliceCompany, std::unordered_map<std::string, std::string>& kvs)
    {
        //收集所有的参数信息
        std::vector<std::string> _kvs;
        switch (sliceCompany)
        {
        case SliceCompany::prusa:
            Stringsplit(comment, ':', _kvs);
            if (_kvs.size() > 1) {
                kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
            }
            else { //end of gcode
                _kvs.clear();
                Stringsplit(comment, '=', _kvs);
                if (_kvs.size() > 1) {
                    kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
                }
                else {
                    std::string str = comment;
                    removeSpace(str);
                    kvs.insert(std::make_pair(str, ""));
                }
            }
            break;
        case SliceCompany::bambu:
        case SliceCompany::superslicer:
            Stringsplit(comment, '=', _kvs);
            if (_kvs.size() > 1) {
                kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
            }
            else { //end of gcode
                _kvs.clear();
                Stringsplit(comment, ':', _kvs);
                if (_kvs.size() > 1) {
                    kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
                }
                else {
                    Stringsplit(comment, '#', _kvs);
                    if (_kvs.size() > 1) {
                        kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
                    }
                    else
                    {
                        std::string str = comment;
                        removeSpace(str);
                        kvs.insert(std::make_pair(str, ""));
                    }
                }
            }
            break;
        case SliceCompany::simplify:
            Stringsplit(comment, ',', _kvs);
            if (_kvs.size() > 1) {
                kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
                //detect   ; layer 3, Z = 0.580
                if (_kvs[0].find("layer ") != std::string::npos && _kvs[1].find("Z ") != std::string::npos)
                {
                    std::vector<std::string> __kvs;
                    Stringsplit(_kvs[0], ' ', __kvs);
                    if (__kvs.size() > 1) {
                        kvs.insert(std::make_pair(__kvs[0], __kvs[1]));
                    }
                    __kvs.clear();
                    Stringsplit(_kvs[1], '=', __kvs);
                    if (__kvs.size() > 1) {
                        kvs.insert(std::make_pair(__kvs[0], __kvs[1]));
                    }
                }
            }
            else { //end of gcode
                _kvs.clear();
                Stringsplit(comment, ':', _kvs);
                if (_kvs.size() > 1) {
                    kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
                }
                else {
                    std::string str = comment;
                    int pos = str.find("feature");
                    if (pos >= 0 && pos < str.length())
                    {
                        str = str.substr(pos + 7, str.length());
                    }

                    removeSpace(str);
                    kvs.insert(std::make_pair(str, ""));
                }
            }
            break;
        case SliceCompany::craftware:
            Stringsplit(comment, ':', _kvs);
            if (_kvs.size() > 1) {
                kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
            }
            else { //end of gcode
                std::string str = comment;
                if (str.find("AreaBegin") != std::string::npos)
                {
                    //; @AreaBegin "Raft" Z-0.650 H0.3
                    int pos = str.find("\"");
                    if (pos != std::string::npos)
                    {
                        std::string _comment = str.substr(pos + 1, str.length());
                        pos = _comment.find("\"");
                        if (pos != std::string::npos)
                        {
                            std::string type = _comment.substr(0, pos);
                            kvs.insert(std::make_pair("TYPE", type));

                            pos = str.find("H");
                            if (pos != std::string::npos)
                            {
                                std::string h = str.substr(pos + 1, str.length());
                                kvs.insert(std::make_pair("HEIGHT", h));
                            }
                        }
                    }
                }

                //LayerBegin N1 Z - 0.400 H0.3
                if (str.find("LayerBegin") != std::string::npos)
                {
                    kvs.insert(std::make_pair("LAYER", ""));
                    int pos = str.find("H");
                    if (pos != std::string::npos)
                    {
                        std::string h = str.substr(pos + 1, str.length());
                        kvs.insert(std::make_pair("HEIGHT", h));
                    }
                }

                removeSpace(str);
                kvs.insert(std::make_pair(str, ""));
            }
                break;
        default:
            Stringsplit(comment, ':', _kvs);
            if (_kvs.size() > 1) {
                kvs.insert(std::make_pair(_kvs[0], _kvs[1]));
            }

            break;
        };
    }

    void _paraseGcode(SliceCompany& sliceCompany,const std::string& gCodeFile, trimesh::box3& box, std::vector<std::vector<SliceLine3D>>& m_sliceLayers, std::unordered_map<std::string, std::string>& kvs)
    {
        //SliceCompany sliceCompany = SliceCompany::creality;
        //trimesh::box3 box;
        std::vector<SliceLine3D > m_sliceLines;
        const char* path = gCodeFile.data();
        //sphere
        gcode_parser parser_;
        std::ifstream gcode_file;
        gcode_file.open(path);

        std::string line;
        int lines_with_no_commands = 0;
        gcode_file.sync_with_stdio(false);

        gcode_parser parser;
        int gcodes_processed = 0;
        int num_arc_commands_ = 0;
        int lines_processed_ = 0;
        bool is_shell = true;
        gcode_position* p_source_position_ = new gcode_position(get_args_(DEFAULT_G90_G91_INFLUENCES_EXTRUDER, DEFAULT_GCODE_BUFFER_SIZE));

        if (gcode_file.is_open())
        {
            parsed_command cmd;
            bool is_get_company = false;
            while (std::getline(gcode_file, line))
            {
                lines_processed_++;
                bool isLayer = false;
                cmd.clear();
                parser.try_parse_gcode(line.c_str(), cmd);
                bool has_gcode = false;
                if (cmd.gcode.length() > 0)
                {
                    has_gcode = true;
                    gcodes_processed++;
                }
                else
                {
                    lines_with_no_commands++;
                }
                p_source_position_->update(cmd, lines_processed_, gcodes_processed, -1);
                gcode_comment_processor* gcode_comment_processor = p_source_position_->get_gcode_comment_processor();

                getKvs(cmd.comment, sliceCompany, kvs);

                if (!is_get_company)
                {
                    std::vector<std::string> res;
                    Stringsplit(cmd.comment, ' ', res);

                    if(!res.empty())
                        is_get_company = true;
                    for (std::string t : res)
                    {

                        if (t == "Creality")
                        {
                            sliceCompany = SliceCompany::creality;
                            break;
                        }
                        else if (t == "PrusaSlicer")
                        {
                            sliceCompany = SliceCompany::prusa;
                            break;
                        }
                        else if (t == "BambuStudio")
                        {
                            sliceCompany = SliceCompany::bambu;
                            break;
                        }
                        else if (t == "Cura_SteamEngine")
                        {
                            sliceCompany = SliceCompany::cura;
                            break;
                        }
                        else if (t == "SuperSlicer")
                        {
                            sliceCompany = SliceCompany::superslicer;
                            break;
                        }
                        else if (t == "ideaMaker")
                        {
                            sliceCompany = SliceCompany::ideamaker;
                            break;
                        }
                        else if (t == "ffslicer")
                        {
                            sliceCompany = SliceCompany::ffslicer;
                            break;
                        }
                        else if (t == "Simplify3D(R)")
                        {
                            sliceCompany = SliceCompany::simplify;
                            break;
                        }
                        else
                            is_get_company = false;
                    }
                }


                //if (sliceCompany == SliceCompany::creality && (cmd.comment == "TYPE:SKIRT" || cmd.comment == "TYPE:SUPPORT-INTERFACE" || cmd.comment == "TYPE:SUPPORT" ||
                //    cmd.comment == "TYPE:FILL" || cmd.comment == "TYPE:WALL-INNER"))
                //{
                //    is_shell = false; continue;
                //}
                //else if (sliceCompany == SliceCompany::creality && (cmd.comment == "TYPE:SKIN" || cmd.comment == "TYPE:WALL-OUTER"))
                //    is_shell = true;

                //if (sliceCompany == SliceCompany::prusa && (cmd.comment == "TYPE:Solid infill" || cmd.comment == "TYPE:External perimeter"))
                //    is_shell = true;
                //else if (sliceCompany == SliceCompany::prusa && (cmd.comment == "WIPE_START" || cmd.comment == "WIPE_END"))
                //{
                //    is_shell = false;
                //    continue;
                //}
                //if (sliceCompany == SliceCompany::bambu && (cmd.comment == " FEATURE: Internal solid infill" || cmd.comment == " FEATURE: Outer wall"))
                //    is_shell = true;
                //else if (sliceCompany == SliceCompany::bambu && (cmd.comment == " WIPE_START" || cmd.comment == " WIPE_END"))
                //{
                //    is_shell = false;
                //    continue;
                //}

                //if (sliceCompany == SliceCompany::simplify && (cmd.comment == " inner perimeter" ||
                //    cmd.comment == " infill" || cmd.comment == " skirt"))
                //{
                //    is_shell = false; continue;
                //}
                //else
                //    is_shell = true;


                if ((sliceCompany == SliceCompany::creality || sliceCompany == SliceCompany::cura || sliceCompany == SliceCompany::ideamaker) && cmd.comment.find("LAYER:") != std::string::npos)
                {
                    isLayer = true;
                }
                else if (sliceCompany == SliceCompany::ffslicer && cmd.comment.find("layer:") != std::string::npos)
                {
                    isLayer = true;
                }
                else if ((sliceCompany == SliceCompany::prusa || sliceCompany == SliceCompany::superslicer) && cmd.comment.find("AFTER_LAYER_CHANGE") != std::string::npos)
                {
                    isLayer = true;
                }
                else if (sliceCompany == SliceCompany::bambu && cmd.comment.find(" CHANGE_LAYER") != std::string::npos)
                {
                    isLayer = true;
                }
                else if (sliceCompany == SliceCompany::simplify && cmd.comment.find("layer") != std::string::npos)
                {
                    isLayer = true;
                }

                if ((sliceCompany == SliceCompany::creality || sliceCompany == SliceCompany::cura || sliceCompany == SliceCompany::ideamaker) && gcodes_processed < 26)
                    continue;
                else if ((sliceCompany == SliceCompany::prusa || sliceCompany == SliceCompany::superslicer) && gcodes_processed < 32)
                    continue;
                else if (sliceCompany == SliceCompany::bambu && gcodes_processed < 38)
                    continue;
                else if (sliceCompany == SliceCompany::ffslicer && gcodes_processed < 14)
                    continue;

                //if (sliceCompany == SliceCompany::prusa && cmd.comment == "END gcode for filament")
                //    break;
                //if (sliceCompany == SliceCompany::prusa && cmd.comment == " filament end gcode")
                //    break;
                //if (sliceCompany == SliceCompany::bambu && cmd.comment == " filament end gcode ")
                //    break;
                //if (sliceCompany == SliceCompany::superslicer && cmd.comment == " Filament-specific end gcode ")
                //    break;
                if (sliceCompany == SliceCompany::ffslicer && cmd.comment == "end gcode")
                    break;

                if (cmd.command == "G1" && is_shell && !p_source_position_->get_current_position_ptr()->is_travel())
                {
                    // increment the number of arc commands encountered
                    num_arc_commands_++;
                    // Get the current and previous positions


                    trimesh::vec3 v3(p_source_position_->get_previous_position_ptr()->x, p_source_position_->get_previous_position_ptr()->y,
                        p_source_position_->get_previous_position_ptr()->z);
                    trimesh::vec3 v4(p_source_position_->get_current_position_ptr()->x, p_source_position_->get_current_position_ptr()->y,
                        p_source_position_->get_current_position_ptr()->z);
                    //if ((v3.x == 0.0f && v3.y == 0.0f) || (v4.x == 0.0f && v4.y == 0.0f))
                        //continue;
                    //if (sliceCompany == SliceCompany::creality && v3.x == 460.0 && v3.y == 490.0)
                    //    break;
                    SliceLine3D s;
                    s.start.x = v3.x;
                    s.start.y = v3.y;
                    s.start.z = v3.z;
                    s.end.x = v4.x;
                    s.end.y = v4.y;
                    s.end.z = v4.z;

                    box += v3;
                    box += v4;

                    m_sliceLines.emplace_back(s);
                }
                if (isLayer)
                {
                    m_sliceLayers.emplace_back(m_sliceLines);
                    m_sliceLines.clear();
                }
            }
        
            kvs.insert(std::make_pair("box_max_x", std::to_string(box.size().x)));
            kvs.insert(std::make_pair("box_max_y", std::to_string(box.size().y)));
            kvs.insert(std::make_pair("box_max_z", std::to_string(box.size().z)));
            kvs.insert(std::make_pair("layer_count", std::to_string(m_sliceLayers.size())));
            float averageThickness = box.size().z/(float)m_sliceLayers.size();
            kvs.insert(std::make_pair("averageThickness", std::to_string(averageThickness)));
        }
        gcode_file.close();

        //get filesize;
        gcode_file.open(path, std::ios_base::binary);
        gcode_file.seekg(0, std::ios_base::end);
        int nFileLen = gcode_file.tellg();
        kvs.insert(std::make_pair("fileSize", std::to_string(nFileLen)));
        gcode_file.close();
    }
    void _crealityKv(std::unordered_map<std::string, std::string>& kvs, trimesh::box3& box)
    {
        changeKey("Machine Name", "machine_name", kvs);
        changeKey("Filament used", "filament_used", kvs);
        changeKey("Material Diameter", "material_diameter", kvs);
        changeKey("Support Density", "material_density", kvs);
        changeKey("TIME", "print_time", kvs);
        changeKey("Layer height", "layer_height", kvs);
        changeKey("Layer Height", "layer_height", kvs);
        changeKey("Infill Pattern", "infill_pattern", kvs);
        changeKey("Infill Sparse Density", "infill_sparse_density", kvs);
        changeKey("Support Type", "support_structure", kvs);
        changeKey("Support Pattern", "support_pattern", kvs);
        changeKey("Support Enable", "support_enable", kvs);
        changeKey("Support Angle", "support_angle", kvs);
        changeKey("Support Density", "support_infill_rate", kvs);
        changeKey("Adhesion Type", "adhesion_type", kvs);
        changeKey("Z Seam Alignment", "z_seam_type", kvs);
        changeKey("Wall Line Count", "wall_line_count", kvs);	
        changeKey("Top/Bottom Thickness", "top_bottom_thickness", kvs); 	
        changeKey("Out Wall Line Width", "wall_line_width", kvs);
        changeKey("Filament Cost", "filament_cost", kvs);
        changeKey("Filament Colour", "filament_colour", kvs);
        changeKey("Flush Volumes Matrix", "flush_volumes_matrix", kvs);
        
        //changeKey("Print Speed", "speed_print", kvs);
        changeKey("Initial Layer Speed", "speed_layer_0", kvs);
        changeKey("Travel Speed", "speed_travel", kvs);
        //changeKey("Wall Speed", "speed_wall", kvs);
        changeKey("Outer Wall Speed", "speed_wall_0", kvs);
        changeKey("Inner Wall Speed", "speed_wall_x", kvs);
        changeKey("Infill Speed", "speed_infill", kvs);
        changeKey("Support Speed", "speed_support", kvs);
        changeKey("Top/Bottom Speed", "speed_topbottom", kvs);
        changeKey("Retraction Distance", "retraction_amount", kvs);
        changeKey("Retraction Speed", "retraction_speed", kvs);
        changeKey("Print Temperature", "material_print_temperature", kvs);
        changeKey("Bed Temperature", "material_bed_temperature", kvs);

        auto iter = kvs.find("Wall Thickness");
        auto iter1 = kvs.find("wall_line_width");
        if (iter != kvs.end() && iter1 == kvs.end())
        {
            std::string str = iter->second;
            float Wall_Thickness = atof(str.c_str());

            auto iter1 = kvs.find("Wall Line Count");
            if (iter1 != kvs.end())
            {
                std::string str1 = iter->second;
                int Wall_Line_Count = atoi(str1.c_str());

                float Wall_Line_Width = (Wall_Thickness + 0.5f) / Wall_Line_Count;
                kvs.insert(std::make_pair("wall_line_width", std::to_string(Wall_Line_Width)));
            }
        }

        //; Filament used : 20.0176m
        iter1 = kvs.find("Filament used");
        if (iter1 != kvs.end())
        {
            if (iter1->second.length() > 1) {
                iter1->second = iter1->second.substr(0, iter1->second.length() - 1);
            }
        }

        iter1 = kvs.find("MINX");
        if (iter1 != kvs.end())
        {
            box.min.x = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MINY");
        if (iter1 != kvs.end())
        {
            box.min.y = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MINZ");
        if (iter1 != kvs.end())
        {
            box.min.z = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MAXX");
        if (iter1 != kvs.end())
        {
            box.max.x = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MAXY");
        if (iter1 != kvs.end())
        {
            box.max.y = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MAXZ");
        if (iter1 != kvs.end())
        {
            box.max.z = atof(iter1->second.c_str());
        }
    }

    void _curaKv(std::unordered_map<std::string, std::string>& kvs, trimesh::box3& box,bool _layer)
    {
        if (_layer)
        {
            auto iter = kvs.find("LAYER_COUNT");
            if (iter != kvs.end())
            {
                kvs.insert(std::make_pair("layer_count", iter->second));
            }

            iter = kvs.find("Layer height");
            if (iter != kvs.end())
            {
                kvs.insert(std::make_pair("layer_height", iter->second));
            }
        
            return;
        }

        auto iter = kvs.find("Layer height");
        if (iter != kvs.end())
        {
            kvs.insert(std::make_pair("layer_height", iter->second));
        }

        iter = kvs.find("LAYER_COUNT");
        if (iter != kvs.end())
        {
            kvs.insert(std::make_pair("layer_count", iter->second));
        }

        //; Filament used : 20.0176m
        iter = kvs.find("Filament used");
        if (iter != kvs.end())
        {
            if (iter->second.length() > 1) {
                iter->second = iter->second.substr(0, iter->second.length() - 1);
            }
        }

        auto iter1 = kvs.find("MINX");
        if (iter1 != kvs.end())
        {
            box.min.x = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MINY");
        if (iter1 != kvs.end())
        {
            box.min.y = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MINZ");
        if (iter1 != kvs.end())
        {
            box.min.z = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MAXX");
        if (iter1 != kvs.end())
        {
            box.max.x = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MAXY");
        if (iter1 != kvs.end())
        {
            box.max.y = atof(iter1->second.c_str());
        }
        iter1 = kvs.find("MAXZ");
        if (iter1 != kvs.end())
        {
            box.max.z = atof(iter1->second.c_str());
        }

    }

    //orca
    //case erNone: return L("Undefined");
    //case erPerimeter: return L("Inner wall");
    //case erExternalPerimeter: return L("Outer wall");
    //case erOverhangPerimeter: return L("Overhang wall");
    //case erInternalInfill: return L("Sparse infill");
    //case erSolidInfill: return L("Internal solid infill");
    //case erTopSolidInfill: return L("Top surface");
    //case erBottomSurface: return L("Bottom surface");
    //case erIroning: return L("Ironing");
    //case erBridgeInfill: return L("Bridge");
    //case erGapFill: return L("Gap infill");
    //case erSkirt: return ("Skirt");
    //case erBrim: return ("Brim");
    //case erSupportMaterial: return L("Support");
    //case erSupportMaterialInterface: return L("Support interface");
    //case erSupportTransition: return L("Support transition");
    //case erWipeTower: return L("Prime tower");
    //case erCustom: return L("Custom");
    //case erMixed: return L("Multiple");
    void _bambuKv(std::unordered_map<std::string, std::string>& kvs, bool _layer)
    {
        if (_layer)
        {
            //changeKey("AFTER_LAYER_CHANGE", "LAYER", kvs);
            changeKey("LAYER_CHANGE", "LAYER", kvs);
            changeKey("CHANGE_LAYER", "LAYER", kvs);
            changeKey("===== noozle load line ===============================", "LAYER", kvs);
            
            //kvs.insert(std::make_pair("TIME_ELAPSED", "0"));
           
            changeKey("FEATURE", "TYPE", kvs);
            changeKey("Z", "Z_HEIGHT", kvs);

            changeKey("filament_diameter", "material_diameter", kvs);
            changeKey("filament_density", "material_density", kvs);
            
            return;
        }

        changeKey("total_layer_count", "layer_count", kvs);
        changeKey("printer_model", "machine_name", kvs);
        changeKey("model printing time", "print_time", kvs);
        changeKey("estimated printing time (normal mode)", "print_time", kvs);
        changeKey("total estimated time", "print_time", kvs);
        changeKey("filament_diameter", "material_diameter", kvs);
        changeKey("filament used [mm]", "filament_used", kvs);
        changeKey("filament used [g]", "filament_weight", kvs);
        changeKey("filament cost", "gcode_filament_cost", kvs);

        //changeKey("layer_height", "layer_height", kvs);
        changeKey("line_width", "wall_line_width", kvs);
        changeKey("sparse_infill_pattern", "infill_pattern", kvs);
        changeKey("sparse_infill_density", "infill_sparse_density", kvs);
        changeKey("support_on_build_plate_only", "support_structure", kvs);
        changeKey("enable_support", "support_enable", kvs);
        changeKey("support_threshold_angle", "support_angle", kvs);
        changeKey("outer_wall_speed", "speed_wall_0", kvs);
        changeKey("inner_wall_speed", "speed_wall_x", kvs);
        changeKey("initial_layer_speed", "speed_layer_0", kvs);
        changeKey("travel_speed", "speed_travel", kvs);
        changeKey("support_speed", "speed_support", kvs);
        changeKey("internal_solid_infill_speed", "speed_infill", kvs);
        changeKey("top_surface_speed", "speed_topbottom", kvs);
        changeKey("retraction_length", "retraction_amount", kvs);
        changeKey("retraction_speed", "retraction_speed", kvs);
        changeKey("nozzle_temperature", "material_print_temperature", kvs);
        
        //4h 0m 41s -> 4*3600 + 0 + 41
        auto iter = kvs.find("print_time");
        if (iter != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ' ', _kvs);
            if (_kvs.size() == 1)
            {
                int strS = atoi(_kvs[0].substr(0, _kvs[0].length() - 1).c_str());
                int time = strS;
                iter->second = std::to_string(time);
            }
            else if (_kvs.size() == 2) {
                if (_kvs[0].length() > 1 && _kvs[1].length() > 1)
                {
                    int strM = atoi(_kvs[0].substr(0, _kvs[0].length() - 1).c_str());
                    int strS = atoi(_kvs[1].substr(0, _kvs[1].length() - 1).c_str());
                    int time = strM * 60 + strS;
                    iter->second = std::to_string(time);
                }
            }
            else if (_kvs.size() > 2) {
                if (_kvs[0].length() > 1 && _kvs[1].length() > 1 && _kvs[2].length() > 1)
                {
                    int strH = atoi(_kvs[0].substr(0, _kvs[0].length() - 1).c_str());
                    int strM = atoi(_kvs[1].substr(0, _kvs[1].length() - 1).c_str());
                    int strS = atoi(_kvs[2].substr(0, _kvs[2].length() - 1).c_str());
                    int time = strH * 3600 + strM * 60 + strS;
                    iter->second = std::to_string(time);
                }
            }
        }

        //filament used [mm] = 4386.68, 2941.27, 1378.17, 1562.97
        iter = kvs.find("filament_used");
        if (iter != kvs.end())
        {
            float filament_len = 0.0f;
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ',', _kvs);
            for (auto& v : _kvs)
            {
                filament_len += std::atof(v.c_str());
            }
            filament_len /= 1000.f;
            iter->second = std::to_string(filament_len);
        }

        //filament used [g] = 13.08, 8.77, 4.11, 4.66
        iter = kvs.find("filament_weight");
        if (iter != kvs.end())
        {
            float filament_len = 0.0f;
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ',', _kvs);
            for (auto& v : _kvs)
            {
                filament_len += std::atof(v.c_str());
            }
            iter->second = std::to_string(filament_len);
        }

        //filament cost = 0.26, 0.18, 0.08, 0.09;
        iter = kvs.find("gcode_filament_cost");
        if (iter != kvs.end())
        {
            float filament_len = 0.0f;
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ',', _kvs);
            for (auto& v : _kvs)
            {
                filament_len += std::atof(v.c_str());
            }
            iter->second = std::to_string(filament_len);
        }

        //; filament_diameter = 1.75,1.75
        auto iter1 = kvs.find("material_diameter");
        if (iter1 != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter1->second, ',', _kvs);
            if (_kvs.size() > 1) {
                iter1->second = _kvs[0];

                for (int i = 1; i < _kvs.size(); i++)
                {
                    kvs.insert(std::make_pair("material_diameter" + std::to_string(i), _kvs[i]));
                }
            }
        }

        //; nozzle_temperature = 220,220
        iter1 = kvs.find("material_print_temperature");
        if (iter1 != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter1->second, ',', _kvs);
            if (_kvs.size() > 1) {
                iter1->second = _kvs[0];

                for (int i = 1; i < _kvs.size(); i++)
                {
                    kvs.insert(std::make_pair("material_print_temperature" + std::to_string(i), _kvs[i]));
                }
            }
        }

        auto iter2 = kvs.find("support_enable");
        if (iter2 != kvs.end())
        {
            int support = atoi(iter2->second.c_str());
            if (support != 0)
            {
                iter2->second = "true";
            }
            else
                iter2->second = "false";
        }

        //; support_on_build_plate_only = 0
        auto iter3 = kvs.find("support_structure");
        if (iter3 != kvs.end())
        {
            int support = atoi(iter3->second.c_str());
            if (support != 0)
            {
                iter3->second = "everywhere";
            }
            else
                iter3->second = "buildplate";
        }

        //;  = 0
        auto iter4 = kvs.find("skirt_loops");
        if (iter4 != kvs.end())
        {
            int skirt = atoi(iter4->second.c_str());
            if (skirt != 0)
            {
                iter4->second = "brim";
            }
            else
                iter4->second = "skirt";
        }
        changeKey("skirt_loops", "adhesion_type", kvs);
       
        //; layer num/total_layer_count: 45/500
        auto iter5 = kvs.find("layer num/total_layer_count");
        if (iter5 != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter5->second, '/', _kvs);
            if (_kvs.size() > 1) {
                iter5->second = _kvs[1];
            }
        }
        changeKey("layer num/total_layer_count", "wall_line_count", kvs);

        //; cool_plate_temp = 35,35
        iter5 = kvs.find("cool_plate_temp");
        if (iter5 != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter5->second, ',', _kvs);
            if (_kvs.size() > 0) {
                iter5->second = _kvs[0];
            }
        }
        changeKey("cool_plate_temp", "material_bed_temperature", kvs);    

        auto iter6 = kvs.find("top_shell_thickness");
        if (iter6 != kvs.end())
        {
            float top_thickness = atof(iter6->second.c_str());
            float bottom_thickness = 0.0f;
            iter6 = kvs.find("bottom_shell_thickness");
            if (iter6 != kvs.end())
            {
                bottom_thickness = atof(iter6->second.c_str());
            }
            float top_bottom_thickness = std::max(top_thickness, bottom_thickness);
            kvs.insert(std::make_pair("top_bottom_thickness",std::to_string(top_bottom_thickness)));        
        }
           
    }

    void _ffslicerKv(std::unordered_map<std::string, std::string>& kvs, bool _layer)
    {
        if (_layer)
        {
            changeKey("layer_count", "layer_count", kvs);

            kvs.insert(std::make_pair("TIME_ELAPSED", "0"));

            changeKey("layer", "LAYER", kvs);

            auto iter = kvs.find("structure");
            if (iter != kvs.end())
            {
                if (iter->second == "shell-outer")
                {
                    iter->second = "WALL-OUTER";
                }
                else if (iter->second == "shell-inner")
                {
                    iter->second = "WALL-INNER";
                }
                //else if (iter->second == "Bottom surface"
                //    || iter->second == "Bottom surface")
                //{
                //    iter->second = "SKIN";
                //}
                else if (iter->second == "support")
                {
                    iter->second = "SUPPORT";
                }
                else if (iter->second == "raft")
                {
                    iter->second = "SKIRT";
                }
                else if (iter->second == "infill-solid"
                    || iter->second == "infill-sparse")
                {
                    iter->second = "FILL";
                }
            }

            changeKey("structure", "TYPE", kvs);
            changeKey("filament_diameter", "material_diameter", kvs);
            return;
        }


        changeKey("layer_count", "layer_count", kvs);
        changeKey("machine_type", "machine_name", kvs);
        changeKey("filament_diameter0", "material_diameter", kvs);
        //changeKey("layer_height", "layer_height", kvs);
        changeKey("fill_pattern", "infill_pattern", kvs);
        changeKey("fill_density", "infill_sparse_density", kvs);
        changeKey("perimeter_shells", "wall_line_count", kvs);
        changeKey("base_print_speed", "speed_print", kvs);
        
        changeKey("travel_speed", "speed_travel", kvs);
        changeKey("platform_temperature", "material_bed_temperature", kvs);
        
        auto iter = kvs.find("top_solid_layers");
        if (iter != kvs.end())
        {
            float top_thickness = atof(iter->second.c_str());
            float bottom_thickness = 0.0f;
            iter = kvs.find("bottom_solid_layers");
            if (iter != kvs.end())
            {
                bottom_thickness = atof(iter->second.c_str());
            }
            float top_bottom_thickness = std::max(top_thickness, bottom_thickness);
            kvs.insert(std::make_pair("top_bottom_thickness", std::to_string(top_bottom_thickness)));
        }

        iter = kvs.find("right_extruder_temperature");
        if (iter != kvs.end())
        {
            changeKey("right_extruder_temperature", "material_print_temperature", kvs);
        }
        else
        {
            iter = kvs.find("left_extruder_temperature");
            if (iter != kvs.end())
            {
                changeKey("left_extruder_temperature", "material_print_temperature", kvs);
            }
        }
    }

    void _ideamakerKv(std::unordered_map<std::string, std::string>& kvs, trimesh::box3& box, bool _layer)
    {
        if (_layer)
        {
            kvs.insert(std::make_pair("TIME_ELAPSED", "0"));
            return;
        }

        float material1 = 0.0f;
        float material2 = 0.0f;
        auto iter = kvs.find("Material#1 Used");
        if (iter != kvs.end())
        {
            material1 = atof(iter->second.c_str());
        }
        iter = kvs.find("Material#2 Used");
        if (iter != kvs.end())
        {
            material2 = atof(iter->second.c_str());
        }
        kvs.insert(std::make_pair("filament_used", std::to_string(material1 + material2)));

        //; Filament Diameter #1: 1.750
        iter = kvs.find("Filament Diameter #1");
        if (iter != kvs.end())
        {
            changeKey("Filament Diameter #1", "material_diameter", kvs);
        }
        else
        {
            iter = kvs.find("Filament Diameter #2");
            if (iter != kvs.end())
            {
                changeKey("Filament Diameter #2", "material_diameter", kvs);
            }
        }

        changeKey("Print Time", "print_time", kvs);
        
        iter = kvs.find("Bounding");
        if (iter != kvs.end())
        {
            std::vector<std::string> minMax;
            Stringsplit(iter->second, ' ', minMax);
            if(minMax.size() == 6)
            {
                box.min.x = atof(minMax.at(0).c_str());
                box.min.y = atof(minMax.at(1).c_str());
                box.min.z = atof(minMax.at(2).c_str());
                box.max.x = atof(minMax.at(3).c_str());
                box.max.y = atof(minMax.at(4).c_str());
                box.max.z = atof(minMax.at(5).c_str());
            }
        }

    }

    void _prusaKv(std::unordered_map<std::string, std::string>& kvs, bool _layer)
    {
        if (_layer)
        {
            changeKey("AFTER_LAYER_CHANGE", "LAYER", kvs);
            changeKey("LAYER_CHANGE", "LAYER", kvs);
            changeKey("CHANGE_LAYER", "LAYER", kvs);
            changeKey("FEATURE", "TYPE", kvs);

            changeKey("Z", "Z_HEIGHT", kvs);

            auto iter = kvs.find("TYPE");
            if (iter != kvs.end())
            {
                if (iter->second == "External perimeter" || iter->second == "Custom" || iter->second == "Outer wall"
                    || iter->second == "Overhang wall"
                    || iter->second == "Bridge")
                {
                    iter->second = "WALL-OUTER";
                }
                else if (iter->second == "perimeter" || iter->second == "Inner wall")
                {
                    iter->second = "WALL-INNER";
                }
                else if (iter->second == "Top solid infill"
                    || iter->second == "Bottom surface"
                    || iter->second == "Top surface")
                {
                    iter->second = "SKIN";
                }
                else if (iter->second == "Support")
                {
                    iter->second = "SUPPORT";
                }
                else if (iter->second == "Skirt"
                    || iter->second == "Skirt/Brim")
                {
                    iter->second = "SKIRT";
                }
                else if (iter->second == "Solid infill"
                    || iter->second == "Internal infill"
                    || iter->second == "Bridge infill"
                    || iter->second == "Gap infill"
                    || iter->second == "Internal solid infill"
                    || iter->second == "Sparse infill")
                {
                    iter->second = "FILL";
                }
            }

            kvs.insert(std::make_pair("TIME_ELAPSED", "0"));
            changeKey("filament_diameter", "material_diameter", kvs);
            changeKey("filament_density", "material_density", kvs);
            return;
        }


        changeKey("printer_model", "machine_name", kvs);
        changeKey("filament used [mm]", "filament_used", kvs);
        changeKey("filament_diameter", "material_diameter", kvs);

        //4h 0m 41s -> 4*3600 + 0 + 41
        auto iter = kvs.find("estimated printing time (normal mode)");
        if (iter != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ' ', _kvs);
            if (_kvs.size() > 2) {
                if (_kvs[0].length() > 1 && _kvs[1].length() > 1 && _kvs[2].length() > 1)
                {
                    int strH = atoi(_kvs[0].substr(0, _kvs[0].length() - 1).c_str());
                    int strM = atoi(_kvs[1].substr(0, _kvs[1].length() - 1).c_str());
                    int strS = atoi(_kvs[2].substr(0, _kvs[2].length() - 1).c_str());
                    int time = strH * 3600 + strM * 60 + strS;
                    iter->second = std::to_string(time);
                }
            }
        }
        changeKey("estimated printing time (normal mode)", "print_time", kvs);
        //changeKey("layer_height", "layer_height", kvs);
        changeKey("external perimeters extrusion width", "wall_line_width", kvs);
        changeKey("fill_pattern", "infill_pattern", kvs);
        changeKey("fill_density", "infill_sparse_density", kvs);
        changeKey("support_material_style", "support_pattern", kvs);

        //; support_on_build_plate_only = 0
        auto iter3 = kvs.find("support_material_buildplate_only");
        if (iter3 != kvs.end())
        {
            int support = atoi(iter3->second.c_str());
            if (support != 0)
            {
                iter3->second = "everywhere";
            }
            else
                iter3->second = "buildplate";
            changeKey("support_material_buildplate_only", "support_structure", kvs);
        }
        
        changeKey("support_material_threshold", "support_angle", kvs);
        


        //; support_on_build_plate_only = 0
        iter3 = kvs.find("brim_type");
        if (iter3 != kvs.end())
        {
            if (iter3->second != "no_brim")
                iter3->second = "brim";
            else
                iter3->second = "none";

            changeKey("brim_type", "adhesion_type", kvs);
        }
            
        changeKey("perimeter_speed", "speed_print", kvs);
        changeKey("first_layer_speed", "speed_layer_0", kvs);
        changeKey("travel_speed", "speed_travel", kvs);
        changeKey("external_perimeter_speed", "speed_wall_0", kvs);
        changeKey("infill_speed", "speed_infill", kvs);
        changeKey("support_material_speed", "speed_support", kvs);
        changeKey("top_solid_infill_speed", "speed_topbottom", kvs);
        changeKey("retract_length", "retraction_amount", kvs);
        changeKey("retract_speed", "retraction_speed", kvs);
        
        changeKey("temperature", "material_print_temperature", kvs);
        changeKey("bed_temperature", "material_bed_temperature", kvs);
    }

    void _simplifyKv(std::unordered_map<std::string, std::string>& kvs, bool _layer)
    {     
        if (_layer)
        {
            changeKey("Z", "HEIGHT", kvs);
          
            kvs.insert(std::make_pair("TIME_ELAPSED", "0"));
            changeKey("layer", "LAYER", kvs);

            changeKey("layerHeight", "layer_height", kvs);
            
            auto iter = kvs.find("skirt");
            if (iter != kvs.end())
            {
                iter->second = "SKIRT";
                changeKey("skirt", "TYPE", kvs);
            }

            iter = kvs.find("inner perimeter");
            if (iter != kvs.end())
            {
                iter->second = "WALL-INNER";
                changeKey("inner perimeter", "TYPE", kvs);
            }

            iter = kvs.find("outer perimeter");
            if (iter != kvs.end())
            {
                iter->second = "WALL-OUTER";
                changeKey("outer perimeter", "TYPE", kvs);
            }

            //todo
            iter = kvs.find("solid layer");
            if (iter != kvs.end())
            {
                iter->second = "FILL";
                changeKey("solid layer", "TYPE", kvs);
            }

            iter = kvs.find("outer perimeter");
            if (iter != kvs.end())
            {
                iter->second = "WALL-OUTER";
                changeKey("outer perimeter", "TYPE", kvs);
            }

            iter = kvs.find("gap fill");
            if (iter != kvs.end())
            {
                iter->second = "SKIN";
                changeKey("gap fill", "TYPE", kvs);
            }

            iter = kvs.find("gap fill");
            if (iter != kvs.end())
            {
                iter->second = "SKIN";
                changeKey("gap fill", "TYPE", kvs);
            }

            iter = kvs.find("infill");
            if (iter != kvs.end())
            {
                iter->second = "FILL";
                changeKey("infill", "TYPE", kvs);
            }

            iter = kvs.find("support");
            if (iter != kvs.end())
            {
                iter->second = "SUPPORT";
                changeKey("support", "TYPE", kvs);
            }

            changeKey("filamentDiameters", "material_diameter", kvs);
            return;
        }

        changeKey("layerHeight", "layer_height", kvs);

        //Filament length : 172129.4 mm(172.13 m)
        auto iter = kvs.find("Filament length");
        if (iter != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, 'mm', _kvs);
            if (_kvs.size()>0)
            {
                float len = std::atoi(_kvs[0].c_str())/1000.0f;
                iter->second = std::to_string(len);
                changeKey("Filament length", "filament_used", kvs);            
            }
        }

        //filamentDiameters,1.75|1.75|1.75|1.75|1.75|1.75
        iter = kvs.find("material_diameter");
        if (iter != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, '|', _kvs);
            if (_kvs.size() > 0)
            {
                iter->second = _kvs[0];    
            }
        }

        //Build time : 29 hours 56 minutes
        iter = kvs.find("Build time");
        if (iter != kvs.end())
        {
            int pos = iter->second.find("hours");
            std::string h = "";
            std::string m = "";
            if (pos >=1 )
            {
                h = iter->second.substr(0, pos - 1);
                std::string min = iter->second.substr(pos + 5, iter->second.length());
                m = min.substr(0, 3);
            }
            else
            {
                pos = iter->second.find("minutes");
                if (pos >= 1 )
                {
                    m = iter->second.substr(0, pos - 1);
                }
            }
            float s = std::atoi(h.c_str()) * 3600 + std::atoi(m.c_str()) * 60;
            iter->second = std::to_string(s);
            changeKey("Build time", "print_time", kvs);
        }

        changeKey("layerHeight", "layer_height", kvs);
        changeKey("extruderWidth", "wall_line_width", kvs);
        
        changeKey("internalInfillPattern", "infill_pattern", kvs);
        changeKey("externalInfillPattern", "infill_pattern", kvs);
        changeKey("infillPercentage", "infill_sparse_density", kvs);
        
        changeKey("generateSupport", "support_enable", kvs);
        iter = kvs.find("support_enable");
        if (iter != kvs.end())
        {
            int support = atoi(iter->second.c_str());
            if (support != 0)
            {
                iter->second = "true";
            }
            else
                iter->second = "false";
        }

        changeKey("supportType", "support_structure", kvs);
        //; support_on_build_plate_only = 0
        auto iter3 = kvs.find("support_structure");
        if (iter3 != kvs.end())
        {
            int support = atoi(iter3->second.c_str());
            if (support == 0)
            {
                iter3->second = "everywhere";
            }
            else
                iter3->second = "buildplate";
        }

        changeKey("supportAngles", "support_angle", kvs);
        changeKey("supportInfillPercentage", "support_infill_rate", kvs);
        iter3 = kvs.find("support_infill_rate");
        if (iter3 != kvs.end())
        {
            float density = atoi(iter3->second.c_str()) / 100.0f;
            iter3->second = std::to_string(density);
        }

        //Adhesion Type
        //"lace": "Lace",
        //"skirt": "skirt",
        //    "brim" : "brim",
        //    "autobrim" : "autobrim",
       //     "raft" : "raft",
      //      "none" : "none"
        //useRaft, 1
       //useSkirt, 1
       //usePrimePillar, 0
       //useOozeShield, 0


        iter3 = kvs.find("useRaft");
        if (iter3 != kvs.end())
        {
            if (iter3->second == "1")
                kvs.insert(std::make_pair("adhesion_type", "raft"));
            else
            {
                iter3 = kvs.find("useSkirt");
                if (iter3 != kvs.end())
                {
                    if (iter3->second == "1")
                        kvs.insert(std::make_pair("adhesion_type", "skirt"));
                    else
                        kvs.insert(std::make_pair("adhesion_type", "none"));
                }
                else
                    kvs.insert(std::make_pair("adhesion_type", "none"));
            }
        }
      
        iter3 = kvs.find("startPointOption");
        if (iter3 != kvs.end())
        {
            if (iter3->second == "2")
            {
                kvs.insert(std::make_pair("z_seam_type", "true"));
            }
            else
                kvs.insert(std::make_pair("z_seam_type", "false"));
        }
        
        changeKey("perimeterOutlines", "wall_line_count", kvs);
        
        iter3 = kvs.find("topSolidLayers");
        if (iter3 != kvs.end())
        {
            float num = std::atof(iter3->second.c_str());
            iter3 = kvs.find("layer_height");
            if (iter3 != kvs.end())
            {
                float thickness = num * std::atof(iter3->second.c_str());
                kvs.insert(std::make_pair("top_bottom_thickness", std::to_string(thickness)));
            }
        }

        auto speed_print = [&kvs]()->float {
            float defaultSpeed = 0.0f;
            auto iter = kvs.find("speed_print");
            if (iter != kvs.end())
            {
                defaultSpeed = std::atof(iter->second.c_str());
            }
            return defaultSpeed;
        };

        auto checkoutSpeed = [&kvs, &speed_print](const std::string& key) {
            auto iter = kvs.find(key);
            if (iter != kvs.end())
            {
                float _speed = std::atof(iter->second.c_str());
                if (_speed <= 1.0f)
                {
                    float defaultSpeed = speed_print();
                    if (defaultSpeed > 1.0f)
                    {
                        iter->second = std::to_string(_speed * defaultSpeed);
                    }
                }
            }
        };

        iter3 = kvs.find("defaultSpeed");
        if (iter3 != kvs.end())
        {
            float defaultSpeed = std::atof(iter3->second.c_str()) / 60.0f;
            iter3->second = std::to_string(defaultSpeed);
            changeKey("defaultSpeed", "speed_print", kvs);
        }

        //
        changeKey("firstLayerUnderspeed", "speed_layer_0", kvs);
        checkoutSpeed("speed_layer_0");

        iter3 = kvs.find("rapidXYspeed");
        if (iter3 != kvs.end())
        {
            float defaultSpeed = std::atof(iter3->second.c_str()) / 60.0f;
            iter3->second = std::to_string(defaultSpeed);
            changeKey("rapidXYspeed", "speed_travel", kvs);
        }

        //Outer Wall Speed  outlineUnderspeed
        changeKey("outlineUnderspeed", "speed_wall_0", kvs);
        checkoutSpeed("speed_wall_0");

        changeKey("solidInfillUnderspeed", "speed_infill", kvs);
        checkoutSpeed("speed_infill");
        //Support Speed
        changeKey("supportUnderspeed", "speed_support", kvs);
        checkoutSpeed("speed_support");

        //
        changeKey("toolChangeRetractionDistance", "retraction_amount", kvs);
        changeKey("toolChangeRetractionSpeed", "retraction_speed", kvs);
        iter3 = kvs.find("retraction_speed");
        if (iter3 != kvs.end())
        {
            float defaultSpeed = std::atof(iter3->second.c_str()) / 60.0f;
            iter3->second = std::to_string(defaultSpeed);
        }

        //temperatureSetpointTemperatures, 190, 200, 45
        //temperatureNumber,0,1,0
        iter = kvs.find("temperatureNumber");
        auto iter1 = kvs.find("temperatureSetpointTemperatures");
        auto iter2 = kvs.find("temperatureHeatedBed"); 
        if (iter != kvs.end() && iter1 != kvs.end() && iter2 != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ',', _kvs);
            
            std::vector<std::string> _kvs1;
            Stringsplit(iter1->second, ',', _kvs1);

            std::vector<std::string> _kvs2;
            Stringsplit(iter2->second, ',', _kvs2);

            if (_kvs.size() == _kvs1.size() && _kvs.size() == _kvs2.size())
            {
                for (int i = 0; i < _kvs.size(); i++)
                {
                    int num = std::atoi(_kvs[i].c_str());
                    int isBed = std::atoi(_kvs2[i].c_str());
                    if (num == 0 && isBed == 0)
                    {
                        kvs.insert(std::make_pair("material_print_temperature",_kvs1[i]));
                    }
                    else if (num > 0 && isBed == 0)
                    {
                        kvs.insert(std::make_pair("material_print_temperature" + std::to_string(num), _kvs1[i]));
                    }
                    else if (isBed)
                    {
                        kvs.insert(std::make_pair("material_bed_temperature", _kvs1[i]));
                    }
                }
            }
        }     
    }

    void _superslicerKv(std::unordered_map<std::string, std::string>& kvs, bool _layer)
    {
        if (_layer)
        {
            changeKey("AFTER_LAYER_CHANGE", "LAYER", kvs);
            changeKey("LAYER_CHANGE", "LAYER", kvs);
            changeKey("CHANGE_LAYER", "LAYER", kvs);

            auto iter = kvs.find("TYPE");
            if (iter != kvs.end())
            {
                if (iter->second == "External perimeter")
                {
                    iter->second = "WALL-OUTER";
                }
                else if (iter->second == "perimeter")
                {
                    iter->second = "WALL-INNER";
                }
                else if (iter->second == "Top solid infill")
                {
                    iter->second = "SKIN";
                }
                else if (iter->second == "Support")
                {
                    iter->second = "SUPPORT";
                }
                else if (iter->second == "Skirt"
                    || iter->second == "Skirt/Brim")
                {
                    iter->second = "SKIRT";
                }
                else if (iter->second == "Solid infill"
                    || iter->second == "Internal infill"
                    || iter->second == "Bridge infill")
                {
                    iter->second = "FILL";
                }
            }


            kvs.insert(std::make_pair("TIME_ELAPSED", "0"));
            return;
        }


        changeKey("printer_model", "machine_name", kvs);
        changeKey("filament used [mm]", "filament_used", kvs);
        changeKey("filament_diameter", "material_diameter", kvs);

        //4h 0m 41s -> 4*3600 + 0 + 41
        auto iter = kvs.find("estimated printing time (normal mode)");
        if (iter != kvs.end())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter->second, ' ', _kvs);
            if (_kvs.size() > 2) {
                if (_kvs[0].length() > 1 && _kvs[1].length() > 1 && _kvs[2].length() > 1)
                {
                    int strH = atoi(_kvs[0].substr(0, _kvs[0].length() - 1).c_str());
                    int strM = atoi(_kvs[1].substr(0, _kvs[1].length() - 1).c_str());
                    int strS = atoi(_kvs[2].substr(0, _kvs[2].length() - 1).c_str());
                    int time = strH * 3600 + strM * 60 + strS;
                    iter->second = std::to_string(time);
                }
            }
        }
        changeKey("estimated printing time (normal mode)", "print_time", kvs);
        //changeKey("layer_height", "layer_height", kvs);
        changeKey("external perimeters extrusion width", "wall_line_width", kvs);
        changeKey("fill_pattern", "infill_pattern", kvs);
        changeKey("fill_density", "infill_sparse_density", kvs);
        changeKey("support_material_style", "support_pattern", kvs);

        //; support_on_build_plate_only = 0
        auto iter3 = kvs.find("support_material_buildplate_only");
        if (iter3 != kvs.end())
        {
            int support = atoi(iter3->second.c_str());
            if (support != 0)
            {
                iter3->second = "everywhere";
            }
            else
                iter3->second = "buildplate";
            changeKey("support_material_buildplate_only", "support_structure", kvs);
        }

        changeKey("support_material_threshold", "support_angle", kvs);



        //; support_on_build_plate_only = 0
        iter3 = kvs.find("brim_type");
        if (iter3 != kvs.end())
        {
            if (iter3->second != "no_brim")
                iter3->second = "brim";
            else
                iter3->second = "none";

            changeKey("brim_type", "adhesion_type", kvs);
        }

        changeKey("perimeter_speed", "speed_print", kvs);
        changeKey("first_layer_speed", "speed_layer_0", kvs);
        changeKey("travel_speed", "speed_travel", kvs);
        changeKey("external_perimeter_speed", "speed_wall_0", kvs);
        changeKey("infill_speed", "speed_infill", kvs);
        changeKey("support_material_speed", "speed_support", kvs);
        changeKey("top_solid_infill_speed", "speed_topbottom", kvs);
        changeKey("retract_length", "retraction_amount", kvs);
        changeKey("retract_speed", "retraction_speed", kvs);

        changeKey("temperature", "material_print_temperature", kvs);
        changeKey("bed_temperature", "material_bed_temperature", kvs);
    }

    void _craftwareKv(std::unordered_map<std::string, std::string>& kvs, bool _layer)
    {
        if (_layer)
        {
            changeKey("LayerBegin", "LAYER", kvs);

            auto iter = kvs.find("TYPE");
            if (iter != kvs.end())
            {
                if (iter->second == "Shell")
                {
                    iter->second = "Outer wall";
                }
                else if (iter->second == "Perimeter")
                {
                    iter->second = "Inner wall";
                }
                else if (iter->second == "Support")
                {
                    iter->second = "Support";
                }
                else if (iter->second == "SupportInterface")
                {
                    iter->second = "Support interface";
                }
                else if (iter->second == "Bridge")
                {
                    iter->second = "Bridge";
                }
                else if (iter->second == "Raft"
                    || iter->second == "Skirt")
                {
                    iter->second = "Skirt";
                }
                else if (iter->second == "Infill")
                {
                    iter->second = "Internal solid infill";
                }
                else
                    ;// iter->second = "Outer wall";
            }


            kvs.insert(std::make_pair("TIME_ELAPSED", "0"));
            return;
        }
    }

    void _paraseKvs(GCodeProcessor& gcodeProcessor,trimesh::box3& box,bool _layer = false)
    {
        std::unordered_map<std::string, std::string>& kvs = gcodeProcessor.kvs;
        SliceCompany& sliceCompany = gcodeProcessor.sliceCompany;

        switch (sliceCompany)
        {
        case SliceCompany::creality:
            _crealityKv(kvs, box);
            break;
        case SliceCompany::cura:
            _curaKv(kvs, box, _layer);
            break;
        case SliceCompany::bambu:
            _bambuKv(kvs,_layer);
            break;
        case SliceCompany::ffslicer:
            _ffslicerKv(kvs, _layer);
            break;
        case SliceCompany::ideamaker:
            _ideamakerKv(kvs, box, _layer);
            break;
        case SliceCompany::prusa:
            _prusaKv(kvs, _layer);
            break;
        case SliceCompany::simplify:
            _simplifyKv(kvs, _layer);
            break;
        case SliceCompany::superslicer:
            _superslicerKv(kvs, _layer);
            break;
        case SliceCompany::craftware:
            _craftwareKv(kvs, _layer);
            break;
        default:
            break;
        }
      
        auto iter = kvs.find("layer_height");
        if (iter == kvs.end())
        {
            auto iter1 = kvs.find("averageThickness");
            if (iter != kvs.end())
            {
                kvs.insert(std::make_pair("layer_height", iter1->second));
            }
        }

        //; filament_diameter = 1.75,1.75
        auto iter1 = kvs.find("material_diameter");
        if (iter1 != kvs.end() && gcodeProcessor.filament_diameters.empty())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter1->second, ',', _kvs);
            for (auto& v : _kvs)
            {
                gcodeProcessor.filament_diameters.push_back(std::atof(v.c_str()));
            }
        }

        iter1 = kvs.find("material_density");
        if (iter1 != kvs.end() && gcodeProcessor.filament_diameters.empty())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter1->second, ',', _kvs);
            for (auto& v : _kvs)
            {
                gcodeProcessor.material_densitys.push_back(std::atof(v.c_str()));
            }
        } 

        iter1 = kvs.find("filament_cost");
        if (iter1 != kvs.end() && gcodeProcessor.filament_costs.empty())
        {
            std::vector<std::string> _kvs;
            Stringsplit(iter1->second, ',', _kvs);
            for (auto& v : _kvs)
            {
                gcodeProcessor.filament_costs.push_back(std::atof(v.c_str()));
            }
        }

        iter1 = kvs.find("nozzle_volume");
        if (iter1 != kvs.end() && gcodeProcessor.m_nozzle_volume <= 0 )
        {
            gcodeProcessor.m_nozzle_volume = std::atof(iter1->second.c_str());
        }
        

        iter1 = kvs.find("FLUSH_START");
        if (iter1 != kvs.end())
        {
            gcodeProcessor.m_flushing = true;
            kvs.erase(iter1);
        }
        iter1 = kvs.find("FLUSH_END");
        if (iter1 != kvs.end())
        {
            gcodeProcessor.m_flushing = false;
            kvs.erase(iter1);
        }
		iter1 = kvs.find("FIRMWARE FLUSH");
		if (iter1 != kvs.end())
		{
			gcodeProcessor.m_Firmware_flushing = true;
			kvs.erase(iter1);
		}

        iter1 = kvs.find("WIPE_START");
        if (iter1 != kvs.end())
        {
            gcodeProcessor.eMoveType = EMoveType::Wipe;
            gcodeProcessor.m_wiping = true;
            kvs.erase(iter1);
        }
        iter1 = kvs.find("WIPE_END");
        if (iter1 != kvs.end())
        {
            gcodeProcessor.m_wiping = false;
            kvs.erase(iter1);
        }

    }

    void _detect_layer(GCodeProcessor& gcodeProcessor, std::string& gcode_layer_data,bool& startLayer,int& curLayer, GcodeTracer* pathData)
    {
        std::unordered_map<std::string, std::string>& kvs = gcodeProcessor.kvs;

        auto iter = kvs.find("LAYER");
        if (iter != kvs.end())
        {
            gcodeProcessor.isFirstLayerHeight = true;
            startLayer = true;

            int pos = gcode_layer_data.find_last_of("\n");
            if (pos >= 0 && pos < gcode_layer_data.size())
            {
                gcode_layer_data = gcode_layer_data.substr(0, pos);
            }

            if (curLayer <= 0)
            {
                if (!gcodeProcessor.haveStartCmd)
                    gcode_layer_data.clear();
            }
            else
            {
                gcodeProcessor.haveStartCmd = false;

                pathData->set_data_gcodelayer(curLayer - 1, gcode_layer_data);
                gcode_layer_data.clear();


                if (curLayer > 0)
                {
                    pathData->setLayer(curLayer);

                    auto iter1 = kvs.find("TIME_ELAPSED");
                    if (iter1 != kvs.end())
                    {
                        pathData->setTime(std::atoi(iter1->second.c_str()));
                        kvs.erase(iter1);
                    }
                    else
                    {
                        pathData->setTime(0);
                    }
                }
            }
            kvs.erase(iter);
            curLayer++;
        }
    }

    void _detect_height(std::unordered_map<std::string, std::string>& kvs, GCodeProcessor& gCodeProcessor,bool& haveLayerHeight, GcodeTracer* pathData)
    {
        const SliceLineType& curType = gCodeProcessor.curType;
        if (curType == SliceLineType::FlowTravel)
        {
            auto iter = kvs.find("HEIGHT");
            if (iter != kvs.end())
            {
                kvs.erase(iter);
            }
            return;
        }

        auto iter = kvs.find("layer_height");
        if (iter != kvs.end() && !haveLayerHeight)
        {
            pathData->setZ(std::atof(iter->second.c_str()), std::atof(iter->second.c_str()));
            haveLayerHeight = true;
        }

        iter = kvs.find("HEIGHT");
        if (iter != kvs.end())
        {
            pathData->setZ(std::atof(iter->second.c_str()), std::atof(iter->second.c_str()));
            kvs.erase(iter);
        }


        iter = kvs.find("LAYER_HEIGHT");
        if (iter != kvs.end())
        {
            ////orca当每一层有多个层高修改时，只取第一次的层高值， 表现：悬空层预览断层
            //if (gCodeProcessor.sliceCompany == SliceCompany::bambu)
            //{
            //    if (gCodeProcessor.isFirstLayerHeight)
            //    {
            //        pathData->setZ(std::atof(iter->second.c_str()), std::atof(iter->second.c_str()));
            //        gCodeProcessor.isFirstLayerHeight = false;
            //    }
            //}
            //else
                pathData->setZ(std::atof(iter->second.c_str()), std::atof(iter->second.c_str()));

            kvs.erase(iter);
        }       
    }

    void _detect_type(std::unordered_map<std::string, std::string>& kvs, GCodeProcessor& gCodeProcessor)
    {
        SliceLineType& curType = gCodeProcessor.curType;
        SliceLineType& lastType = gCodeProcessor.lastType;

        auto iter = kvs.find("TYPE");
        if (iter != kvs.end())
        {
            if (iter->second == "WALL-OUTER")
            {
                gCodeProcessor.m_seams_detector.activate(true);
                curType = SliceLineType::OuterWall;
                lastType = curType;
            }
            else if (iter->second == "WALL-INNER")
            {
                curType = SliceLineType::InnerWall;
                lastType = curType;
            }
            else if (iter->second == "SKIN")
            {
                curType = SliceLineType::Skin;
                lastType = curType;
            }
            else if (iter->second == "SUPPORT"
                || iter->second == "SUPPORT-INTERFACE")
            {
                curType = SliceLineType::Support;
                lastType = curType;
            }
            else if (iter->second == "SKIRT")
            {
                curType = SliceLineType::SkirtBrim;
                lastType = curType;
            }
            else if (iter->second == "FILL")
            {
                curType = SliceLineType::Infill;
                lastType = curType;
            }
            else if (iter->second == "PRIME-TOWER")
            {
                curType = SliceLineType::PrimeTower;
                lastType = curType;
            }
            else if (iter->second == "Ironing")
            {
                curType = SliceLineType::FlowTravel;
                lastType = curType;
            }
            else
            {
                //curType = SliceLineType::AdvanceTravel;
                //lastType = curType;
            }

            std::string& role = iter->second;
            if (role == ("Inner wall"))
            {
                curType = SliceLineType::erPerimeter;
                lastType = curType;
            }
            else if (role == ("Outer wall"))
            {
                gCodeProcessor.m_seams_detector.activate(true);
                curType = SliceLineType::erExternalPerimeter;
                lastType = curType;
            }
            else if (role == ("Overhang wall"))
            {
                curType = SliceLineType::erOverhangPerimeter;
                lastType = curType;
            }
            else if (role == ("Sparse infill"))
            {
                curType = SliceLineType::erInternalInfill;
                lastType = curType;
            }
            else if (role == ("Internal solid infill"))
            {
                curType = SliceLineType::erSolidInfill;
                lastType = curType;
            }
            else if (role == ("Top surface"))
            {
                curType = SliceLineType::erTopSolidInfill;
                lastType = curType;
            }
            else if (role == ("Bottom surface"))
            {
                curType = SliceLineType::erBottomSurface;
                lastType = curType;
            }
            else if (role == ("Ironing"))
            {
                curType = SliceLineType::erIroning;
                lastType = curType;
            }
            else if (role == ("Bridge"))
            {
                curType = SliceLineType::erBridgeInfill;
                lastType = curType;
            }
            else if (role == ("Gap infill"))
            {
                curType = SliceLineType::erGapFill;
                lastType = curType;
            }
            else if (role == ("Skirt"))
            {
                curType = SliceLineType::erSkirt;
                lastType = curType;
            }
            else if (role == ("Brim"))
            {
                curType = SliceLineType::erBrim;
                lastType = curType;
            }
            else if (role == ("Support"))
            {
                curType = SliceLineType::erSupportMaterial;
                lastType = curType;
            }
            else if (role == ("Support interface"))
            {
                curType = SliceLineType::erSupportMaterialInterface;
                lastType = curType;
            }
            else if (role == ("Support transition"))
            {
                curType = SliceLineType::erSupportTransition;
                lastType = curType;
            }
            else if (role == ("Prime tower"))
            {
                curType = SliceLineType::erWipeTower;
                lastType = curType;
            }
            else if (role == ("Custom"))
            {
                curType = SliceLineType::erCustom;
                lastType = curType;
            }
            else if (role == ("Multiple"))
            {
                curType = SliceLineType::erMixed;
                lastType = curType;
            }
            else if (role == "Internal Bridge")
            {
                curType = SliceLineType::erInternalBridgeInfill;
                lastType = curType;
            }


            //pathData->setLayer(std::atoi(iter->second.c_str()));
            kvs.erase(iter);
        }
    }

    void _detect_gcode_company(bool& is_get_company,const std::string& comment, SliceCompany& sliceCompany)
    {
        if (!is_get_company)
        {
            is_get_company = true;
            if (comment.find("BambuStudio") != std::string::npos || comment.find("OrcaSlicer") != std::string::npos
                ||  comment.find("Creality_Print") != std::string::npos)
            {
                sliceCompany = SliceCompany::bambu;
            }
            else if (comment.find("Creality") != std::string::npos)
            {
                sliceCompany = SliceCompany::creality;
            }
            else if (comment.find("PrusaSlicer") != std::string::npos )
            {
                sliceCompany = SliceCompany::prusa;
            }
            else if (comment.find("Cura_SteamEngine") != std::string::npos)
            {
                sliceCompany = SliceCompany::cura;
            }
            else if (comment.find("SuperSlicer") != std::string::npos)
            {
                sliceCompany = SliceCompany::superslicer;
            }
            else if (comment.find("ideaMaker") != std::string::npos)
            {
                sliceCompany = SliceCompany::ideamaker;
            }
            else if (comment.find("ffslicer") != std::string::npos)
            {
                sliceCompany = SliceCompany::ffslicer;
            }
            else if (comment.find("Simplify3D") != std::string::npos)
            {
                sliceCompany = SliceCompany::simplify;
            }
            else if (comment.find("CraftWare") != std::string::npos)
            {
                sliceCompany = SliceCompany::craftware;
            }
            else
                is_get_company = false;
        }
    }

    void _detect_image(GCodeProcessor& gcodeProcessor, const std::string& comment)
    {
        if (comment.find("png end") != std::string::npos
            || comment.find("jpg end") != std::string::npos
            || comment.find("bmp end") != std::string::npos
            || comment.find("thumbnail end") != std::string::npos
            || comment.find("thumbnail_JPG end") != std::string::npos
            || comment.find("thumbnail_QOI end") != std::string::npos
            || comment.find("thumbnail_QOI end") != std::string::npos
            || comment.find("THUMBNAIL_BLOCK_START") != std::string::npos
            ) {
            gcodeProcessor.writeImage = false;
        }

        if (gcodeProcessor.writeImage)
        {
            //gcodeProcessor.images.push_back(std::pair<trimesh::ivec2, std::string>(imageSize, ""));
            std::string _comment = comment;
            removeSpace(_comment);
            gcodeProcessor.images.back().second += _comment;
        }

        if (comment.find("png begin") != std::string::npos
            || comment.find("jpg begin") != std::string::npos
            || comment.find("bmp begin") != std::string::npos
            || comment.find("thumbnail begin") != std::string::npos
            || comment.find("thumbnail_JPG begin") != std::string::npos
            || comment.find("thumbnail_QOI begin") != std::string::npos
            || comment.find("thumbnail_QOI begin") != std::string::npos
            ) {
            // std::vector<std::pair<trimesh::ivec2, std::string>> images;
            trimesh::ivec2 imageSize(0, 0);

            std::vector<std::string> vs;
            Stringsplit(comment, ' ', vs);
            if (!vs.empty())
            {
                for (auto& v : vs)
                {
                    if (v.find("*") != std::string::npos)
                    {
                        std::vector<std::string> _vs;
                        Stringsplit(v, '*', _vs);
                        if (_vs.size() > 1)
                        {
                            imageSize.x = std::atoi(_vs[0].c_str());
                            imageSize.y = std::atoi(_vs[1].c_str());
                        }

                        break;
                    }
                    if (v.find("x") != std::string::npos)
                    {
                        std::vector<std::string> _vs;
                        Stringsplit(v, '*', _vs);
                        if (_vs.size() > 1)
                        {
                            imageSize.x = std::atoi(_vs[0].c_str());
                            imageSize.y = std::atoi(_vs[1].c_str());
                        }

                        break;
                    }
                }
            }
            gcodeProcessor.images.push_back(std::pair<trimesh::ivec2, std::string>(imageSize, ""));
            gcodeProcessor.writeImage = true;
        }
    }

    bool detectZSeam(GCodeProcessor& gcodeProcessor,trimesh::vec3& v, trimesh::vec3& vp)
    {
        bool detect = false;
        if (gcodeProcessor.m_seams_detector.is_active()) {
            if (gcodeProcessor.eMoveType == EMoveType::Extrude && gcodeProcessor.curType == SliceLineType::OuterWall && !gcodeProcessor.m_seams_detector.has_first_vertex()) {
                gcodeProcessor.m_seams_detector.set_first_vertex(v);
            }
            else if ((gcodeProcessor.eMoveType != EMoveType::Extrude || (gcodeProcessor.curType != SliceLineType::OuterWall && gcodeProcessor.curType != SliceLineType::erOverhangPerimeter)) && gcodeProcessor.m_seams_detector.has_first_vertex()) {
                const trimesh::vec3 curr_pos = v;
                const trimesh::vec3 new_pos = vp;
                const std::optional<trimesh::vec3> first_vertex = gcodeProcessor.m_seams_detector.get_first_vertex();

                double len = sqrt((new_pos - *first_vertex).x + (new_pos - *first_vertex).y);
                if (len < 0.0625f) {
                    detect = false;
                }
                else
                    detect = true;


                gcodeProcessor.m_seams_detector.activate(false);
            }
        }
        else if (gcodeProcessor.eMoveType == EMoveType::Extrude && gcodeProcessor.curType == SliceLineType::OuterWall) {
            gcodeProcessor.m_seams_detector.activate(true);
            gcodeProcessor.m_seams_detector.set_first_vertex(vp);
        }
            return detect;
    }

    void _get_path(std::string& command, std::vector<parsed_command_parameter>& parameters
        , GCodeProcessor& gcodeProcessor
        , bool is_travel
        , bool has_position_changed
        , trimesh::vec3& v
        , trimesh::vec3& vp
        , GcodeTracer* pathData
        , SliceCompany& sliceCompany)
    {
        double& e = gcodeProcessor.current_e;
        SliceLineType curType = gcodeProcessor.curType;
        if (!command.empty())
        {
            if (gcodeProcessor.eMoveType != EMoveType::Extrude && gcodeProcessor.eMoveType != EMoveType::Noop)
            {
                switch (gcodeProcessor.eMoveType)
                {
                case EMoveType::Retract:
                    curType = SliceLineType::Retract;
                    break;
                case EMoveType::Unretract:
                    curType = SliceLineType::Unretract;
                    break;
                case EMoveType::Seam:
                    break;
                case EMoveType::Tool_change:
                    break;
                case EMoveType::Color_change:
                    break;
                case EMoveType::Pause_Print:
                    break;
                case EMoveType::Custom_GCode:
                    break;
                case EMoveType::Travel:
                    curType = SliceLineType::Travel;
                    break;
                case EMoveType::Wipe:
                    curType = SliceLineType::Wipe;
                    break;
                default:
                    break;
                }
            }

            if (command == "G0")
            {
                for (auto& p : parameters)
                {
                    if (p.name == "F" || p.name == "f")
                        pathData->setSpeed(p.double_value);
                }
                if (has_position_changed)
                {
                    pathData->getPathData(v, e, (int)curType, (sliceCompany == SliceCompany::bambu || sliceCompany == SliceCompany::prusa));
                }
                else
                    pathData->getNotPath();
            }
            else if (command == "G1")
            {
                for (auto& p : parameters)
                {
                    if (p.name == "F" || p.name == "f")
                        pathData->setSpeed(p.double_value);
                }
                if (has_position_changed)
                {
                    if (has_position_changed)
                    {
                        if (detectZSeam(gcodeProcessor,v, vp))
                        {
                            pathData->getPathData(v, e, (int)curType, (sliceCompany == SliceCompany::bambu || sliceCompany == SliceCompany::prusa), true);
                        }
                        else
                        {
                            pathData->getPathData(v, e, (int)curType, (sliceCompany == SliceCompany::bambu || sliceCompany == SliceCompany::prusa));
                        }    
                    }
                    else
                        pathData->getNotPath();
                }
                else
                    pathData->getNotPath();
            }
            else if (command == "G2" || command == "G3")
            {
                bool isG2 = true;
                if (command == "G3")
                    isG2 = false;

                float i = 0.0f;
                float j = 0.0f;
                int _p = 0;
                for (auto& p : parameters)
                { 
                    if (p.name == "F" || p.name == "f")
                        pathData->setSpeed(p.double_value);
                    else if (p.name == "I" || p.name == "i")
                        i = p.double_value;
                    else if (p.name == "J" || p.name == "j")
                        j = p.double_value;
                    else if (p.name == "P" || p.name == "p")
                        _p = (int)p.double_value;
                }

                 if (detectZSeam(gcodeProcessor, v, vp))
                {
                    pathData->getPathDataG2G3(v, i, j, e, (int)curType,_p, isG2, (sliceCompany == SliceCompany::bambu || sliceCompany == SliceCompany::prusa),true);
                }
                else
                {
                    pathData->getPathDataG2G3(v, i, j, e, (int)curType,_p, isG2, (sliceCompany == SliceCompany::bambu || sliceCompany == SliceCompany::prusa));
                }

            }
            else
                pathData->getNotPath();
        }
        else
            pathData->getNotPath();
    }

    size_t getFileSize(FILE* file)
    {
        if (!file)
            return 0;

        size_t size = 0;
        fseek(file, 0L, SEEK_END);
        size = _cc_ftelli64(file);
        fseek(file, 0L, SEEK_SET);
        return size;
    }

    void process_M104(parsed_command& cmd, GcodeTracer* pathData)
    {
        //temp
        //if (cmd.command == "M104" || cmd.command == "M109")
        {
            for (auto& p : cmd.parameters)
            {
                if (p.name == "S" || p.name == "s")
                    pathData->setTEMP(p.double_value);
            }
        }
    }

    std::string removeTrailingNewline(std::string str) {
        if (!str.empty() && str[str.size() - 1] == '\n') {
            if (str.size() > 1 && str[str.size() - 2] == '\r') {
                str.erase(str.size() - 2); // 去除"\r\n"
            }
            else {
                str.erase(str.size() - 1); // 只去除"\n"
            }
        }
        return str;
    }

    void process_M204(parsed_command& cmd, GcodeTracer* pathData)
    {
        //ACC
        {
            //M204 S500
            std::vector<std::string> _kvs;
            Stringsplit(cmd.gcode, ' ', _kvs);
            for (size_t i = 0; i < _kvs.size(); i++)
            {
                if (_kvs[i].length() > 1 )
                {
                    if (_kvs[i][0] == 'S' || _kvs[i][0] == 's' || _kvs[i][0] == 'P' || _kvs[i][0] == 'T' || _kvs[i][0] == 'R')
                    {
                        std::string _value = _kvs[i].substr(1, _kvs[i].length());
                        _value = removeTrailingNewline(_value);
                        pathData->setAcc(atof(_value.c_str()));
                    }
                }
            }
            //for (auto& p : cmd.parameters)
            //{
            //    //"S" "P" acceleration ;  "R"retract_acceleration ; "T" travel_acceleration
            //    if (p.name == "S" || p.name == "s" || p.name == "P" || p.name == "T" || p.name == "R")
            //        pathData->setAcc(p.double_value);
            //}
        }
    }

    void process_M106(parsed_command& cmd, GcodeTracer* pathData)
    {
        //BBS: for Bambu machine ,we both use M106 P1 and M106 to indicate the part cooling fan
        //So we must not ignore M106 P1
        //if (cmd.command == "M106")
        {
            for (auto& p : cmd.parameters)
            {
                if (p.name == "S" || p.name == "s")
                    pathData->setFan((100.0f / 255.0f) * p.double_value);
                else if(p.name == "P" && p.double_value ==1.0f)
                    pathData->setFan(100.0f);
            }
        }
    }

    void process_filaments(GCodeProcessor& gcodeProcessor,GCodeType code)
    {
        if (code == GCodeType::ColorChange)
            gcodeProcessor.m_used_filaments.process_color_change_cache();

        if (code == GCodeType::ToolChange) {
            gcodeProcessor.m_used_filaments.process_extruder_cache(gcodeProcessor.m_extruder_id);
            //BBS: reset remaining filament
            gcodeProcessor.m_remaining_volume = gcodeProcessor.m_nozzle_volume;
        }
    }

    void process_T(parsed_command& cmd, GCodeProcessor& gcodeProcessor, GcodeTracer* pathData)
    {
        //BBS: T255, T1000 and T1100 is used as special command for BBL machine and does not cost time. return directly
        if (cmd.command.length() >= 1) {
            int extruder_id = 0;
            for (auto& p : cmd.parameters)
            {
                if (p.name == "T" || p.name == "t")
                    if (p.unsigned_long_value >= 0 && p.unsigned_long_value <= 254)
                    {
                        extruder_id = p.unsigned_long_value;
                        pathData->setExtruder((int)extruder_id);
                        gcodeProcessor.extruders.insert(extruder_id);
                    }
            }
            if (extruder_id >= 0 && extruder_id <= 254
                &&extruder_id != gcodeProcessor.m_extruder_id)
            {
                //if (id >= m_result.extruders_count) {}
                gcodeProcessor.gcodeParaseInfo.total_filamentchanges++;

                if (gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges.size() <= extruder_id)
                {
                    int num = extruder_id - gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges.size() + 1;
                    for (int i= 0; i < num; i++)
                    {
                        gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges.push_back(0);
                    }
                }
                gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges[extruder_id] ++;

                if (gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index.size() <= extruder_id)
                {
                    int num = extruder_id - gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index.size() + 1;
                    for (int i =0; i < num; i++)
                    {
                        gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index.push_back(std::vector<std::pair<int, int>>());
                    }
                }
                gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index[extruder_id].push_back(std::pair(gcodeProcessor.m_extruder_id, extruder_id));

                auto iter = gcodeProcessor.extruders.find(gcodeProcessor.m_extruder_id);
                if (iter == gcodeProcessor.extruders.end() && gcodeProcessor.m_extruder_id == 0)
                {
                    gcodeProcessor.m_extruder_id = extruder_id;
                }

                gcodeProcessor.m_last_extruder_id = gcodeProcessor.m_extruder_id;
                process_filaments(gcodeProcessor, GCodeType::ToolChange);
                gcodeProcessor.m_extruder_id = extruder_id;

                //todo  加载耗材时间
            }
        }
        pathData->getNotPath();
    }

    void process_G1(GCodeProcessor& pathParam)
    {
        float filament_diameter = 1.75f;
        if (!pathParam.filament_diameters.empty()
            && pathParam.m_extruder_id >=0  
            && pathParam.m_extruder_id < pathParam.filament_diameters.size())
        {
            filament_diameter = pathParam.filament_diameters[pathParam.m_extruder_id];
        }

        float filament_radius = 0.5f * filament_diameter;
        float area_filament_cross_section = static_cast<float>(M_PI) * sqr(filament_radius);

        auto move_type = [&]() {
            EMoveType type = EMoveType::Noop;

            if (pathParam.m_wiping) {
                type = EMoveType::Wipe;
                pathParam.eMoveType = type;
            }
            else if (pathParam.current_e < 0.0f)
            {
                type = (pathParam.current_v.x != 0.0f || pathParam.current_v.y != 0.0f || pathParam.current_v.z != 0.0f) ? EMoveType::Travel : EMoveType::Retract;           
                pathParam.eMoveType = type == EMoveType::Travel ? EMoveType::Travel : EMoveType::Retract;
            }
            else if (pathParam.current_e > 0.0f) {
                if (pathParam.current_v.x == 0.0f && pathParam.current_v.y == 0.0f)
                {
                    type = (pathParam.current_v.z == 0.0f) ? EMoveType::Unretract : EMoveType::Travel;
                    pathParam.eMoveType = type == EMoveType::Unretract ? EMoveType::Unretract : EMoveType::Travel;
                }
                else if (pathParam.current_v.x != 0.0f || pathParam.current_v.y != 0.0f)
                {
                    type = EMoveType::Extrude;
                    pathParam.eMoveType = EMoveType::Extrude;
                }
            }
            else if (pathParam.current_v.x != 0.0f || pathParam.current_v.y != 0.0f || pathParam.current_v.z != 0.0f)
            {
                type = EMoveType::Travel;
                pathParam.eMoveType = EMoveType::Travel;
            }

            return type;
        };

        EMoveType type = move_type();
        if (type == EMoveType::Extrude) {
            float volume_extruded_filament = area_filament_cross_section * pathParam.current_e;

            // save extruded volume to the cache
            if(pathParam.curType == SliceLineType::erWipeTower)
                pathParam.m_used_filaments.increase_color_change_caches(volume_extruded_filament);
            else
            {
                pathParam.m_used_filaments.increase_caches(volume_extruded_filament);
            }

        }
        else if (type == EMoveType::Unretract && pathParam.m_flushing) {
            float volume_flushed_filament = area_filament_cross_section * pathParam.current_e;
            if (pathParam.m_remaining_volume > volume_flushed_filament)
            {
                pathParam.m_used_filaments.update_flush_per_filament(pathParam.m_last_extruder_id, volume_flushed_filament);
                pathParam.m_remaining_volume -= volume_flushed_filament;
            }
            else {
                pathParam.m_used_filaments.update_flush_per_filament(pathParam.m_last_extruder_id, pathParam.m_remaining_volume);
                pathParam.m_used_filaments.update_flush_per_filament(pathParam.m_extruder_id, volume_flushed_filament - pathParam.m_remaining_volume);
                pathParam.m_remaining_volume = 0.f;
            }
        }
        else if (type == EMoveType::Unretract && pathParam.m_Firmware_flushing) {
            pathParam.m_Firmware_flushing = false;
			if (pathParam.m_used_filaments.flush_icount_per_filament.find(pathParam.m_extruder_id) != pathParam.m_used_filaments.flush_icount_per_filament.end()) {
				pathParam.m_used_filaments.flush_icount_per_filament[pathParam.m_extruder_id]++;
			}
			else {
				pathParam.m_used_filaments.flush_icount_per_filament[pathParam.m_extruder_id] = 1;
			}
        }

        else  if (type == EMoveType::Wipe)
        {
            float volume_extruded_filament = area_filament_cross_section * pathParam.current_e;
            // save extruded volume to the cache
            //pathParam.m_used_filaments.increase_color_change_caches(volume_extruded_filament);
        }
    }

    void getDefineTEMP(const std::string& comment, GCodeProcessor& pathParam, GcodeTracer* pathData)
    {
        if (comment.find("START_PRINT") != std::string::npos)
        {
            std::vector<std::string> _kvs;
            Stringsplit(comment, ' ', _kvs);
            for (std::string& str : _kvs)
            {
                if (str.find("EXTRUDER_TEMP") != std::string::npos)
                {
                    std::vector<std::string> kvs;
                    Stringsplit(str, '=', kvs);
                    if (kvs.size()>1)
                    {
                        pathData->setTEMP(atoi(kvs[1].c_str()));
                        pathParam.have_start_print = true;
                    }
                    break;
                }
            }
        }
    }

    void getACCEL(const std::string& comment, GCodeProcessor& pathParam, GcodeTracer* pathData)
    {
        if (comment.find("SET_VELOCITY_LIMIT") != std::string::npos)
        {
            std::vector<std::string> _kvs;
            Stringsplit(comment, ' ', _kvs);
            for (std::string& str : _kvs)
            {
                if (str.find("ACCEL") != std::string::npos)
                {
                    std::vector<std::string> kvs;
                    Stringsplit(str, '=', kvs);
                    if (kvs.size() > 1)
                    {
                        pathData->setAcc(atoi(kvs[1].c_str()));
                    }
                    break;
                }
            }
        }
    }

    void parese_manual_tool_change(parsed_command& cmd)
    {
        if (cmd.comment.find("MANUAL_TOOL_CHANGE") != std::string::npos)
        {
            std::vector<std::string> _kvs;
            removeSpace(cmd.comment);
            Stringsplit(cmd.comment, ' ', _kvs);
            if (_kvs.size() > 1)
            {
                cmd.command = _kvs[1];

                std::vector<std::string> __kvs;
                Stringsplit(_kvs[1], 'T', __kvs);
                if (__kvs.size() > 1)
                {
                    cmd.parameters.push_back(parsed_command_parameter("T", atoi(__kvs[1].c_str())));
                }
            }
        }
    }

    void process_gcode_line(parsed_command& cmd, GCodeProcessor& pathParam, SliceLineType& curType, GcodeTracer* pathData)
    {
        //START_PRINT EXTRUDER_TEMP=220 BED_TEMP=60
        if (!cmd.gcode.empty() && !pathParam.have_start_print)
            getDefineTEMP(cmd.gcode, pathParam,pathData);

        if (!cmd.gcode.empty())
            getACCEL(cmd.gcode, pathParam, pathData);

        parese_manual_tool_change(cmd);

        if (cmd.command.length() >=1 ) {
            switch (cmd.command[0])
            {
            case 'g':
            case 'G':
                switch (cmd.command.size()) 
                {
                case 2:
                    switch (cmd.command[1]) {
                    case '0': //{ process_G0(line); break; }  // Move
                    case '1': //{ process_G1(pathParam); break; }  // Move
                    case '2':
                    case '3': //{ process_G2_G3(line); break; }  // Move
                    { process_G1(pathParam); break; }
                    //BBS
                    //case 4: { process_G4(line); break; }  // Delay
                    default: break;
                    }
                    break;
                case 3:
                    switch (cmd.command[1]) {
                    case '1':
                        switch (cmd.command[2]) {
                        //case '0': { process_G10(line); break; } // Retract
                        //case '1': { process_G11(line); break; } // Unretract
                        default: break;
                        }
                        break;
                    case '2':
                        switch (cmd.command[2]) {
                        //case '0': { process_G20(line); break; } // Set Units to Inches
                        //case '1': { process_G21(line); break; } // Set Units to Millimeters
                        //case '2': { process_G22(line); break; } // Firmware controlled retract
                        //case '3': { process_G23(line); break; } // Firmware controlled unretract
                        //case '8': { process_G28(line); break; } // Move to origin
                        //case '9': { process_G29(line); break; }
                        default: break;
                        }
                        break;
                    case '9':
                        switch (cmd.command[2]) {
                        //case '0': { process_G90(line); break; } // Set to Absolute Positioning
                        //case '1': { process_G91(line); break; } // Set to Relative Positioning
                        //case '2': { process_G92(line); break; } // Set Position
                        default: break;
                        }
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            case 'm':
            case 'M':
                switch (cmd.command.size()) {
                case 2:
                    switch (cmd.command[1]) {
                    //case '1': { process_M1(line); break; }   // Sleep or Conditional stop
                    default: break;
                    }
                    break;
                case 3:
                    switch (cmd.command[1]) {
                    case '8':
                        switch (cmd.command[2]) {
                        //case '2': { process_M82(line); break; }  // Set extruder to absolute mode
                        //case '3': { process_M83(line); break; }  // Set extruder to relative mode
                        default: break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case 4:
                    switch (cmd.command[1]) {
                    case '1':
                        switch (cmd.command[2]) {
                        case '0':
                            switch (cmd.command[3]) {
                            case '4': { 
                                process_M104(cmd, pathData); 
                                break; 
                            } // Set extruder temperature
                            case '6': {  process_M106(cmd, pathData); break;  } // Set fan speed
                            //case '7': { process_M107(line); break; } // Disable fan
                            //case '8': { process_M108(line); break; } // Set tool (Sailfish)
                            case '9': {                                 
                                process_M104(cmd, pathData);
                                break;
                            } // Set extruder temperature
                            default: break;
                            }
                            break;
                        case '3':
                            switch (cmd.command[3]) {
                            //case '2': { process_M132(line); break; } // Recall stored home offsets
                            //case '5': { process_M135(line); break; } // Set tool (MakerWare)
                            default: break;
                            }
                            break;
                        case '4':
                            switch (cmd.command[3]) {
                            //case '0': { process_M140(cmd, pathData); break; } // Set bed temperature
                            default: break;
                            }
                        case '9':
                            switch (cmd.command[3]) {
                            //case '0': { process_M190(line); break; } // Wait bed temperature
                            default: break;
                            }
                        default:
                            break;
                        }
                        break;
                    case '2':
                        switch (cmd.command[2]) {
                        case '0':
                            switch (cmd.command[3]) {
                            //case '1': { process_M201(line); break; } // Set max printing acceleration
                            //case '3': { process_M203(line); break; } // Set maximum feedrate
                            case '4': { process_M204(cmd, pathData); break; } // Set default acceleration
                            //case '5': { process_M205(line); break; } // Advanced settings
                            default: break;
                            }
                            break;
                        case '2':
                            switch (cmd.command[3]) {
                            //case '1': { process_M221(line); break; } // Set extrude factor override percentage
                            default: break;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case '4':
                        switch (cmd.command[2]) {
                        case '0':
                            switch (cmd.command[3]) {
                                //BBS
                            //case '0': { process_M400(line); break; } // BBS delay
                            //case '1': { process_M401(line); break; } // Repetier: Store x, y and z position
                            //case '2': { process_M402(line); break; } // Repetier: Go to stored position
                            default: break;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case '5':
                        switch (cmd.command[2]) {
                        case '6':
                            switch (cmd.command[3]) {
                            //case '6': { process_M566(line); break; } // Set allowable instantaneous speed change
                            default: break;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case '7':
                        switch (cmd.command[2]) {
                        case '0':
                            switch (cmd.command[3]) {
                            //case '2': { process_M702(line); break; } // Unload the current filament into the MK3 MMU2 unit at the end of print.
                            default: break;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            case 't':
            case 'T':
                process_T(cmd, pathParam, pathData); // Select Tool
                break;
            default:
                break;
            }      
        }
    }

    void process_tags(GCodeProcessor& gcodeProcessor)
    {
        
    }

    void _paraseGcodeAndPreview(const std::string& gCodeFile
        ,GCodeProcessor& gcodeProcessor
        , GcodeTracer* pathData, ccglobal::Tracer* tracer = nullptr)
    {
        //temp
        std::vector<std::vector<SliceLine3D>> m_sliceLayers;

        std::unordered_map<std::string, std::string>& kvs = gcodeProcessor.kvs;
        SliceCompany& sliceCompany = gcodeProcessor.sliceCompany;

        std::vector<SliceLine3D > m_sliceLines;
        const char* path = gCodeFile.data();
        gcode_parser parser_;

        FILE* gcode_file = boost::nowide::fopen(gCodeFile.c_str(), "rb");

        int lines_with_no_commands = 0;
        gcode_parser parser;
        int gcodes_processed = 0;
        int lines_processed_ = 0;
        bool is_shell = true; 

        std::shared_ptr<gcode_position> p_source_position_(new gcode_position(get_args_(DEFAULT_G90_G91_INFLUENCES_EXTRUDER, DEFAULT_GCODE_BUFFER_SIZE)));

        bool haveLayerHeight = false;
        bool haveLayerCount = false;

        bool loadInitWiDth = false;

        bool bDataValiable = false;

        //SliceLineType curType(SliceLineType::NoneType);
        //SliceLineType lastType(SliceLineType::NoneType);
        if (gcode_file != nullptr)
        {
            int curLayer = 0;
            bool startLayer = false;
            parsed_command cmd;
            bool is_get_company = false;
            std::string gcode_layer_data = "";
            bool isToolChange = false;
            char _line[2048] = { '\0' };

            std::string line;
            bool endLine = true;
            while (!feof(gcode_file))
            {
                fgets(_line, 2048, gcode_file);
                std::string __line(_line);
                if (!__line.empty())
                {
                    if (__line.length() == 2047 && __line.at(__line.length() - 1) != '\n')
                    {
                        line += __line;
                        endLine = false;
                        continue;
                    }
                    else if (!endLine)
                    {
                        line += __line;
                        endLine = true;
                    }
                    else
                        line = __line;
                }
                else
                    line = __line;

                lines_processed_++;
                bool isLayer = false;
                cmd.clear();
                parser.try_parse_gcode(line.c_str(), cmd);
                bool has_gcode = false;
                if (cmd.gcode.length() > 0)
                {
                    has_gcode = true;
                    gcodes_processed++;
                }
                else
                {
                    lines_with_no_commands++;
                }

                gcode_layer_data += "\n";
                gcode_layer_data += line.c_str();

                p_source_position_->update(cmd, lines_processed_, gcodes_processed, -1);
                gcode_comment_processor* gcode_comment_processor = p_source_position_->get_gcode_comment_processor();

                getKvs(cmd.comment, sliceCompany, kvs);
                _paraseKvs(gcodeProcessor, gcodeProcessor.box,true);

                _detect_image(gcodeProcessor,cmd.comment);

                _detect_gcode_company(is_get_company, cmd.comment, sliceCompany);

                if (curLayer <= 0 
                    && (cmd.comment == "===== noozle load line ==============================="
                        || gcodeProcessor.curType != SliceLineType::NoneType))
                {
                    gcodeProcessor.haveStartCmd = true;
                    startLayer = true;

                    if (!bDataValiable)
                    {
                        gcode_layer_data.clear();
                        gcode_layer_data += line.c_str();
                        bDataValiable = true;
                    }
                }

                auto iter = kvs.find("layer_count");
                if (iter != kvs.end() && !haveLayerCount)
                {
                    pathData->setLayers(std::atoi(iter->second.c_str()));
                    haveLayerCount = true;
                }

                iter = kvs.find("filament_colour");
                if (iter != kvs.end())
                {
                    std::string default_filament_colour = "";
                    std::string default_type = "";
                    auto defalut_iter = kvs.find("default_filament_colour");
                    if (defalut_iter != kvs.end())
                        default_filament_colour = defalut_iter->second;
                    auto defalut_type_iter = kvs.find("default_filament_type");
                    if (defalut_type_iter != kvs.end())
                        default_type = defalut_type_iter->second;

                    pathData->setNozzleColorList(iter->second, default_filament_colour, default_type);
                    kvs.erase(iter);
                }   

                //add init width
                if (!loadInitWiDth)
                {
                    iter = kvs.find("external perimeters extrusion width");
                    if (iter != kvs.end())
                    {
                        std::vector<std::string> widths;
                        Stringsplit(iter->second, 'mm', widths);
                        if (!widths.empty())
                        {
                            pathData->setWidth(std::atof(widths[0].c_str()));
                        }
                        loadInitWiDth = true;
                    }
                }

                iter = kvs.find("WIDTH");
                if (iter != kvs.end())
                {
                    pathData->setWidth(std::atof(iter->second.c_str()));
                    kvs.erase(iter);
                }

                iter = kvs.find("LINE_WIDTH");
                if (iter != kvs.end())
                {
                    pathData->setWidth(std::atof(iter->second.c_str()));
                    kvs.erase(iter);
                }

                iter = kvs.find("Z_HEIGHT");
                if (iter != kvs.end())
                {
                    pathData->setLayerHeight(std::atof(iter->second.c_str()));
                    kvs.erase(iter);
                }

                if (curLayer > 0)
                {
                    iter = kvs.find("PAUSE_PRINTING");
                    if (iter != kvs.end())
                    {
                        pathData->setLayerPause(curLayer - 1);
                        kvs.erase(iter);
                    }
                    else
                    {
                        iter = kvs.find("PAUSE_PRINT");
                        if (iter != kvs.end())
                        {
                            pathData->setLayerPause(curLayer - 1);
                            kvs.erase(iter);
                        }
                    }
                }
                //relative extrusion
                if (cmd.command == "M83" || cmd.command == "G91")
                {
                    p_source_position_->get_current_position_ptr()->set_e_axis_mode("relative");
                    kvs.insert(std::make_pair("relative_extrusion", "true"));
                }

                //must have : layer height
                _detect_height(kvs,gcodeProcessor,haveLayerHeight, pathData);

                //must have : per layer
                _detect_layer(gcodeProcessor, gcode_layer_data, startLayer, curLayer, pathData);

                //must have : per path type
                _detect_type(kvs, gcodeProcessor);

                gcodeProcessor.current_e = p_source_position_->get_current_position_ptr()->get_current_extruder().e_relative;

                trimesh::vec3 _v(0.0f, 0.0f, 0.0f);
                for (auto& p : cmd.parameters)
                {
                    if (p.name == "X" || p.name == "x")
                        _v.x = p.double_value;
                    if (p.name == "Y" || p.name == "y")
                        _v.y = p.double_value;
                    if (p.name == "Z" || p.name == "z")
                        _v.z = p.double_value;
                }  
                gcodeProcessor.current_v = _v;
                process_gcode_line(cmd, gcodeProcessor, gcodeProcessor.curType,pathData);

                //path
                if (startLayer)
                {
                    //double e = p_source_position_->get_current_position_ptr()->get_current_extruder().e_relative;
                    bool is_travel =p_source_position_->get_current_position_ptr()->is_travel();
                    bool has_position_changed = p_source_position_->get_current_position_ptr()->has_position_changed;

                    trimesh::vec3 v(p_source_position_->get_current_position_ptr()->x, p_source_position_->get_current_position_ptr()->y,
                        p_source_position_->get_current_position_ptr()->z);

                    trimesh::vec3 vp(p_source_position_->get_previous_position_ptr()->x, p_source_position_->get_previous_position_ptr()->y,
                        p_source_position_->get_previous_position_ptr()->z);

                    _get_path(cmd.command, cmd.parameters
                        , gcodeProcessor
                        , is_travel
                        , has_position_changed
                        , v
                        , vp
                        , pathData
                        ,sliceCompany);
                }

                //get box
                if ((cmd.command == "G1" || cmd.command == "G2" || cmd.command == "G3")
                    && (gcodeProcessor.eMoveType != EMoveType::Retract && gcodeProcessor.eMoveType != EMoveType::Travel))
                {
                    trimesh::vec3 v3(p_source_position_->get_previous_position_ptr()->x, p_source_position_->get_previous_position_ptr()->y,
                        p_source_position_->get_previous_position_ptr()->z);
                    trimesh::vec3 v4(p_source_position_->get_current_position_ptr()->x, p_source_position_->get_current_position_ptr()->y,
                        p_source_position_->get_current_position_ptr()->z);

                    gcodeProcessor.box += v3;
                    gcodeProcessor.box += v4;
                }
            }

            if (!gcode_layer_data.empty())
            {
                pathData->set_data_gcodelayer(curLayer-1, gcode_layer_data);
                pathData->setLayer(curLayer);
            }

            auto iter1 = kvs.find("TIME_ELAPSED");
            if (iter1 != kvs.end())
            {
                pathData->setTime(std::atoi(iter1->second.c_str()));
                kvs.erase(iter1);
            }

            kvs.insert(std::make_pair("box_max_x", std::to_string(gcodeProcessor.box.size().x)));
            kvs.insert(std::make_pair("box_max_y", std::to_string(gcodeProcessor.box.size().y)));
            kvs.insert(std::make_pair("box_max_z", std::to_string(gcodeProcessor.box.size().z)));
            kvs.insert(std::make_pair("layer_count", std::to_string(m_sliceLayers.size())));
            float averageThickness = gcodeProcessor.box.size().z / (float)m_sliceLayers.size();
            kvs.insert(std::make_pair("averageThickness", std::to_string(averageThickness)));

            size_t fileSize = getFileSize(gcode_file);
            kvs.insert(std::make_pair("fileSize", std::to_string(fileSize)));
        }

        gcodeProcessor.m_used_filaments.process_color_change_cache();

        if (!gcodeProcessor.extruders.empty())
        {
            auto iter = gcodeProcessor.extruders.find(gcodeProcessor.m_extruder_id);
            if (iter == gcodeProcessor.extruders.end() && gcodeProcessor.m_extruder_id == 0)
            {
                auto it = gcodeProcessor.extruders.end();
                it--;
                gcodeProcessor.m_extruder_id =  *it;
            }
        }
        gcodeProcessor.m_used_filaments.process_extruder_cache(gcodeProcessor.m_extruder_id);
        //process_role_cache

        if (!gcodeProcessor.images.empty())
        {
            std::vector<std::pair<trimesh::ivec2, std::vector<unsigned char>>> images;

            for (auto& str : gcodeProcessor.images)
            {
                std::vector<std::string> inPrevData;
                inPrevData.push_back(str.second);
                images.push_back(std::pair<trimesh::ivec2, std::vector<unsigned char>>(str.first, std::vector<unsigned char>()));
                gcode::thumbnail_base2image(inPrevData, images.back().second);
            }
            pathData->writeImages(images);
            gcodeProcessor.images.clear();
        }


        if (gcode_file)
            fclose(gcode_file);
    }


    std::string getValue(const std::unordered_map<std::string, std::string>& kvs,const std::string& key)
    {
        auto iter = kvs.find(key);
        if (iter != kvs.end())
        {
            return iter->second;
        }
        return "";
    }

    void _removeOthersKvs(std::unordered_map<std::string, std::string>& kvs)
    {
        std::vector<std::string> _kvs;
        _kvs.push_back("box_max_y");
        _kvs.push_back("box_max_z");
        _kvs.push_back("fileSize");
        _kvs.push_back("box_max_x");
        _kvs.push_back("box_max_y");
        _kvs.push_back("box_max_z");
        _kvs.push_back("layer_count");
        _kvs.push_back("machine_name");
        _kvs.push_back("filament_used");
        _kvs.push_back("material_diameter");
        _kvs.push_back("print_time");
        _kvs.push_back("layer_height");
        _kvs.push_back("wall_line_width");
        _kvs.push_back("infill_pattern");
        _kvs.push_back("infill_sparse_density");
        _kvs.push_back("support_enable");
        _kvs.push_back("support_structure");
        _kvs.push_back("support_pattern");
        _kvs.push_back("support_angle");
        _kvs.push_back("support_infill_rate");
        _kvs.push_back("adhesion_type");
        _kvs.push_back("z_seam_type");
        _kvs.push_back("wall_line_count");
        _kvs.push_back("top_bottom_thickness");
        _kvs.push_back("speed_print");
        _kvs.push_back("speed_layer_0");
        _kvs.push_back("speed_travel");
        _kvs.push_back("speed_wall");
        _kvs.push_back("speed_wall_0");
        _kvs.push_back("speed_wall_x");
        _kvs.push_back("speed_infill");
        _kvs.push_back("speed_support");
        _kvs.push_back("speed_topbottom");
        _kvs.push_back("retraction_amount");
        _kvs.push_back("retraction_speed");
        _kvs.push_back("material_print_temperature");
        _kvs.push_back("material_bed_temperature");

        std::unordered_map<std::string, std::string> Newkvs;
        for (const auto& key : _kvs)
        {
            auto iter = kvs.find(key);
            if (iter != kvs.end())
            {
                Newkvs.insert(std::make_pair(iter->first, iter->second));
            }
        }

        kvs.clear();
        kvs.swap(Newkvs);
    }

    void deal_roles_times(const std::vector<std::pair<int, float>>& moves_times, std::vector<std::pair<int, float>>& times)
    {
        for (auto& custom_gcode_time : moves_times)
        {
            auto funPushTimes = [](int role, float time, std::vector<std::pair<int, float>>& times) {
                std::pair<int, float> _pair;
                _pair.first = role;
                _pair.second = time;
                times.push_back(_pair);
            };
            int role = 1;
            switch (custom_gcode_time.first)
            {
            case 1://Slic3r::ExtrusionRole::erPerimeter:
                role = static_cast<int>(SliceLineType::erPerimeter);
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 2://Slic3r::ExtrusionRole::erExternalPerimeter:
                role = static_cast<int>(SliceLineType::erExternalPerimeter);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 3://Slic3r::ExtrusionRole::erOverhangPerimeter:
                role = static_cast<int>(SliceLineType::erOverhangPerimeter);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 4://Slic3r::ExtrusionRole::erInternalInfill:
                role = static_cast<int>(SliceLineType::erInternalInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 5://Slic3r::ExtrusionRole::erSolidInfill:
                role = static_cast<int>(SliceLineType::erSolidInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 6://Slic3r::ExtrusionRole::erTopSolidInfill:
                role = static_cast<int>(SliceLineType::erTopSolidInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 7://Slic3r::ExtrusionRole::erBottomSurface:
                role = static_cast<int>(SliceLineType::erBottomSurface);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 8://Slic3r::ExtrusionRole::erIroning:
                role = static_cast<int>(SliceLineType::erIroning);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 9://Slic3r::ExtrusionRole::erBridgeInfill:
                role = static_cast<int>(SliceLineType::erBridgeInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 10://Slic3r::ExtrusionRole::erInternalBridgeInfill:
                role = static_cast<int>(SliceLineType::erInternalBridgeInfill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 11://Slic3r::ExtrusionRole::erGapFill:
                role = static_cast<int>(SliceLineType::erGapFill);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 12://Slic3r::ExtrusionRole::erSkirt:
                role = static_cast<int>(SliceLineType::erSkirt);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 13://Slic3r::ExtrusionRole::erBrim:
                role = static_cast<int>(SliceLineType::erBrim);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 14://Slic3r::ExtrusionRole::erSupportMaterial:
                role = static_cast<int>(SliceLineType::erSupportMaterial);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 15://Slic3r::ExtrusionRole::erSupportMaterialInterface:
                role = static_cast<int>(SliceLineType::erSupportMaterialInterface);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 16://Slic3r::ExtrusionRole::erSupportTransition:
                role = static_cast<int>(SliceLineType::erSupportTransition);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 17://Slic3r::ExtrusionRole::erWipeTower:
                role = static_cast<int>(SliceLineType::erWipeTower);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 18://Slic3r::ExtrusionRole::erCustom:
                role = static_cast<int>(SliceLineType::erCustom);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 19://Slic3r::ExtrusionRole::erMixed:
                role = static_cast<int>(SliceLineType::erMixed);;
                funPushTimes(role, custom_gcode_time.second, times);
                break;
            case 20://Slic3r::ExtrusionRole::erCount:
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

    void deal_moves_times(const std::vector<std::pair<int, float>>& roles_times, std::vector<std::pair<int, float>>& times)
    {
        for (auto& custom_gcode_time : roles_times)
        {
            std::pair<int, float> _pair;
            _pair.first = (int)custom_gcode_time.first;

            if (_pair.first == 1)
            {
                _pair.first = 14;
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
            times.push_back(_pair);
        }
    }


	std::vector<std::string> _splitString(const std::string& str, const std::string& delim = ",", bool addEmpty = false)
	{
		std::vector<std::string> elems;
		size_t pos = 0;
		size_t len = str.length();
		size_t delim_len = delim.length();
		if (delim_len == 0)
			return elems;
		while (pos < len)
		{
			int find_pos = str.find(delim, pos);
			if (find_pos < 0)
			{
				std::string t = str.substr(pos, len - pos);
				if (!t.empty())
					elems.push_back(t);
				break;
			}
			std::string t = str.substr(pos, find_pos - pos);
			if (!t.empty()) {
				elems.push_back(t);
			}
			else {
				if (addEmpty) {
					elems.push_back("");
				}
			}
			pos = find_pos + delim_len;
		}
		if (pos == len && addEmpty)
		{
			elems.push_back("");
		}
		return elems;
	}

    //设置参数
    void _setParam(GCodeProcessor& gcodeProcessor, GcodeTracer* pathData)
    {
        std::unordered_map<std::string, std::string>& kvs = gcodeProcessor.kvs;

        GCodeParseInfo& pathParam = gcodeProcessor.gcodeParaseInfo;

        if (gcodeProcessor.sliceCompany == SliceCompany::bambu)
        {
            pathParam.producer = GProducer::OrcaSlicer;
        }
        pathParam.printTime = std::atof(getValue(kvs,"print_time").c_str());

        //; printable_area = 0x0, 220x0, 220x220, 0x220
        //; printable_height = 250
        trimesh::box box;
        box.clear();
        std::string printable_area = getValue(kvs, "printable_area");
        std::string printable_height = getValue(kvs, "printable_height");
        if (!printable_height.empty())
        {
            box.min.z = 0;
            box.max.z = std::atof(printable_height.c_str());
        }
        if (!printable_area.empty())
        {
            std::vector<std::string> printable_areas;
            Stringsplit(printable_area, ',', printable_areas);
            for (size_t i = 0; i < printable_areas.size(); i++)
            {
                std::vector<std::string> values;
                Stringsplit(printable_areas[i], 'x', values);
                if (0 == i)
                {
                    values.size() > 0 ? box.min.x = std::atof(values[0].c_str()) : 0;
                    values.size() > 1 ? box.min.x = std::atof(values[1].c_str()) : 0;
                }
                if (1 == i)
                {
                    values.size() > 0 ? box.max.x = std::atof(values[0].c_str()) : 0;
                }
                if (3 == i)
                {
                    values.size() > 1 ? box.max.y = std::atof(values[1].c_str()) : 0;
                }
            }
        }

        if (!printable_height.empty() && !printable_area.empty())
        {
            pathData->setSceneBox(box);
            pathParam.machine_width = box.max.x - box.min.x;
            pathParam.machine_depth = box.max.y - box.min.y;
            pathParam.machine_height = box.max.z - box.min.z;
        }

        //float machine_height;
        //float machine_width;
        //float machine_depth;
        float materialLenth;
        float materialDensity;//单位面积密度
        float material_diameter = { 1.75f }; //材料直径
        float material_density = { 1.24f };  //材料密度
        //pathParam.materialLenth =  std::atof(getValue(kvs, "filament_used").c_str());
        pathParam.material_diameter = std::atof(getValue(kvs, "material_diameter").c_str());
        pathParam.material_density = std::atof(getValue(kvs, "material_density").c_str()); 
        pathParam.materialDensity = M_PI * (pathParam.material_diameter * 0.5) * (pathParam.material_diameter * 0.5) * pathParam.material_density;//单位面积密度
        pathParam.lineWidth = std::atof(getValue(kvs, "wall_line_width").c_str());
        pathParam.layerHeight = std::atof(getValue(kvs, "layer_height").c_str());
        pathParam.cost = std::atof(getValue(kvs, "gcode_filament_cost").c_str());
        //pathParam.weight = std::atof(getValue(kvs, "filament_weight").c_str());
        float filament_cost = std::atof(getValue(kvs, "gcode_filament_cost").c_str());
        pathParam.unitPrice = pathParam.materialLenth > 0.0f ? filament_cost / pathParam.materialLenth : 0.0f;
        pathParam.relativeExtrude = std::atoi(getValue(kvs, "relative_extrusion").c_str()) == 1 ? true: false;

        pathParam.timeParts.OuterWall = std::atof(getValue(kvs, "OuterWall Time").c_str());
        pathParam.timeParts.InnerWall = std::atof(getValue(kvs, "InnerWall Time").c_str());
        pathParam.timeParts.Skin = std::atof(getValue(kvs, "Skin Time").c_str());
        pathParam.timeParts.Support = std::atof(getValue(kvs, "Support Time").c_str());
        pathParam.timeParts.SkirtBrim = std::atof(getValue(kvs, "SkirtBrim Time").c_str());
        pathParam.timeParts.Infill = std::atof(getValue(kvs, "Infill Time").c_str());
        pathParam.timeParts.SupportInfill = std::atof(getValue(kvs, "Support Infills Time").c_str());
        pathParam.timeParts.MoveCombing = std::atof(getValue(kvs, "Combing Time").c_str());
        pathParam.timeParts.MoveRetraction = std::atof(getValue(kvs, "Retraction Time").c_str());
        pathParam.timeParts.PrimeTower = std::atof(getValue(kvs, "PrimeTower Time").c_str());

        if (gcodeProcessor.filament_diameters.empty())
        {
            gcodeProcessor.filament_diameters.push_back(1.75);
        }
        if (gcodeProcessor.material_densitys.empty())
        {
            gcodeProcessor.material_densitys.push_back(1.24);
        }
        if (gcodeProcessor.filament_costs.empty())
        {
            gcodeProcessor.filament_costs.push_back(20.0);//default per kg
        }      

        pathParam.materialLenth = 0;
        pathParam.weight = 0;
        pathParam.cost = 0;
        for (auto&f :  gcodeProcessor.m_used_filaments.volumes_per_extruder)
        {
            if (f.first < 0 || f.first > 254)
                continue;
            float filament_radius = 0.5f * gcodeProcessor.filament_diameters[f.first % gcodeProcessor.filament_diameters.size()];
            float filament_density = gcodeProcessor.material_densitys[f.first % gcodeProcessor.material_densitys.size()];
            double s = M_PI * sqr(filament_radius) > 0.0f? M_PI * sqr(filament_radius) : 1.0f;
            float used_filament = f.second / s * 0.001;
            float weight = f.second * filament_density * 0.001;
            pathParam.volumes_per_extruder.push_back(std::pair(f.first,used_filament));
            pathParam.volumes_per_extruder.push_back(std::pair(f.first, weight));

            pathParam.weight += weight;
            pathParam.materialLenth += used_filament;
            pathParam.cost += weight * gcodeProcessor.filament_costs[f.first % gcodeProcessor.filament_costs.size()] * 0.001;
        }
        for (auto& f : gcodeProcessor.m_used_filaments.flush_per_filament)
        {
            if (f.first < 0 || f.first > 254)
                continue;
            float filament_radius = 0.5f * gcodeProcessor.filament_diameters[f.first % gcodeProcessor.filament_diameters.size()];
            float filament_density = gcodeProcessor.material_densitys[f.first % gcodeProcessor.material_densitys.size()];
            double s = M_PI * sqr(filament_radius) > 0.0f ? M_PI * sqr(filament_radius) : 1.0f;
            float used_filament = f.second / s * 0.001;
            float weight = f.second * filament_density * 0.001;
            pathParam.flush_per_filament.push_back(std::pair(f.first, used_filament));
            pathParam.flush_per_filament.push_back(std::pair(f.first, weight));

            pathParam.weight += weight;
            pathParam.materialLenth += used_filament;
            pathParam.cost += weight * gcodeProcessor.filament_costs[f.first % gcodeProcessor.filament_costs.size()] * 0.001;
        }
        for (auto& f : gcodeProcessor.m_used_filaments.volumes_per_tower)
        {
            if (f.first < 0 || f.first > 254)
                continue;
            float filament_radius = 0.5f * gcodeProcessor.filament_diameters[f.first % gcodeProcessor.filament_diameters.size()];
            float filament_density = gcodeProcessor.material_densitys[f.first % gcodeProcessor.material_densitys.size()];
            double s = M_PI * sqr(filament_radius) > 0.0f ? M_PI * sqr(filament_radius) : 1.0f;
            float used_filament = f.second / s * 0.001;
            float weight = f.second * filament_density * 0.001;
            pathParam.volumes_per_tower.push_back(std::pair(f.first, used_filament));
            pathParam.volumes_per_tower.push_back(std::pair(f.first, weight));

            pathParam.weight += weight;
            pathParam.materialLenth += used_filament;
            pathParam.cost += weight * gcodeProcessor.filament_costs[f.first % gcodeProcessor.filament_costs.size()] * 0.001;
        }

        //; type_times_1 =  1,5.778656; 2,5.746951; 8,20.648708; 10,399.188141;
        //; type_times_2 =  0,20.648708; 1,150.658997; 2,86.188622; 4,142.359619; 5,16.140944; 7,6.060084; 10,9.247404; 18,1.099889; 
        std::string moves_times = getValue(kvs, "type_times_1");
        std::string roles_times = getValue(kvs, "type_times_2");
        std::string model_time = getValue(kvs, "type_times_3");


        float printTime = 0.0;
        if (!model_time.empty())
        {
            printTime = std::atof(model_time.c_str());
        }
		if (gcodeProcessor.m_used_filaments.flush_per_filament.empty())
		{
			std::string flush_volumes_matrix = getValue(kvs, "flush_volumes_matrix");
			std::string flush_multiplier = getValue(kvs, "flush_multiplier");
			std::vector<std::string> strs = _splitString(flush_volumes_matrix, ",");

            std::vector<float> flushes;
            for (const std::string& str : strs)
                flushes.push_back(std::atof(str.c_str()) * std::atof(flush_multiplier.c_str()));
			const int isize = std::sqrt(flushes.size());
			auto get_flush = [&flushes, &isize](int i, int j)->float {
				int index = i * isize + j;
				if (index >= 0 && index < flushes.size())
					return flushes.at(index);
				return 280.0f;
            };

			for (auto& f : gcodeProcessor.m_used_filaments.flush_icount_per_filament)
			{
				if (f.first < 0 || f.first > 254)
					continue;

                if (f.second > 0)
                {
                    float filament_radius = 0.5f * gcodeProcessor.filament_diameters[f.first % gcodeProcessor.filament_diameters.size()];
                    float filament_density = gcodeProcessor.material_densitys[f.first % gcodeProcessor.material_densitys.size()];
                    double s = M_PI * sqr(filament_radius) > 0.0f ? M_PI * sqr(filament_radius) : 1.0f;
                    float used_filament = f.second * 290 / s * 0.001;
                    float weight = f.second * 290 * filament_density * 0.001;
                    pathParam.flush_per_filament.push_back(std::pair(f.first, used_filament));
                    pathParam.flush_per_filament.push_back(std::pair(f.first, weight));
                    pathParam.weight += weight;
                    printTime += f.second * 50;
                }
			}

            int extruder_size = (int)gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges.size();
            for (int i = 0; i < extruder_size; ++i)
            {
                int times_ = gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges.at(i);
                if (times_ > 0)
                {
                    printTime += times_ * 86.0f;
					if (gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index.size() == extruder_size)
					{
						float filament_radius = 0.5f * gcodeProcessor.filament_diameters[i % gcodeProcessor.filament_diameters.size()];
						float filament_density = gcodeProcessor.material_densitys[i % gcodeProcessor.material_densitys.size()];
						double s = M_PI * sqr(filament_radius) > 0.0f ? M_PI * sqr(filament_radius) : 1.0f;
						float used_filament = 0.0;
						float weight = 0.0;
						for (int n = 0; n < times_; n++)
						{
							int _first = gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index[i][n].first;
							int _second = gcodeProcessor.gcodeParaseInfo.extruder_filamentchanges_index[i][n].second;
							int _flush = get_flush(_first, _second);
							used_filament += _flush / s * 0.001;;
							weight += _flush * filament_density * 0.001;
						}
						pathParam.flush_per_filament.push_back(std::pair(i, used_filament));
						pathParam.flush_per_filament.push_back(std::pair(i, weight));
					}
                    else
                    {
						float filament_radius = 0.5f * gcodeProcessor.filament_diameters[i % gcodeProcessor.filament_diameters.size()];
						float filament_density = gcodeProcessor.material_densitys[i % gcodeProcessor.material_densitys.size()];
						double s = M_PI * sqr(filament_radius) > 0.0f ? M_PI * sqr(filament_radius) : 1.0f;
						float used_filament = times_ * 290.0f / s * 0.001f;
						float weight = times_ * 290.0f * filament_density * 0.001f;
						pathParam.flush_per_filament.push_back(std::pair(i, used_filament));
						pathParam.flush_per_filament.push_back(std::pair(i, weight));
						pathParam.weight += weight;
                    }
                    
                }
            }
		}

        std::vector<std::pair<int, float>> roles_times_pair;
        std::vector<std::pair<int, float>> moves_times_pair;

        std::vector<std::string> _moves_times;
        Stringsplit(moves_times,';', _moves_times);
        for (auto move_time : _moves_times)
        {
            if (!move_time.empty())
            {
                std::vector<std::string> _moves_time;
                Stringsplit(move_time, ',', _moves_time);
                if (_moves_time.size() > 1)
                {
                    std::pair<int, float> _pair;
                    moves_times_pair.push_back(std::pair(std::atoi(_moves_time[0].c_str()), std::atof(_moves_time[1].c_str())));
                }
            }
        }
        if (!moves_times_pair.empty())
        {
            pathParam.have_roles_time = true;
            deal_moves_times(moves_times_pair, pathParam.roles_time);
        }

        std::vector<std::string> _roles_times;
        Stringsplit(roles_times, ';', _roles_times);
        for (auto roles_time : _roles_times)
        {
            if (!roles_time.empty())
            {
                std::vector<std::string> _roles_time;
                Stringsplit(roles_time, ',', _roles_time);
                if (_roles_time.size() > 1)
                {
                    std::pair<int, float> _pair;
                    roles_times_pair.push_back(std::pair(std::atoi(_roles_time[0].c_str()), std::atof(_roles_time[1].c_str())));
                }
            }
        }
        if (!roles_times_pair.empty())
        {
            pathParam.have_roles_time = true;
            deal_roles_times(roles_times_pair, pathParam.roles_time);
        }

        if (!model_time.empty())
        {
            pathParam.have_roles_time = true;
            pathParam.printTime = printTime;
        }

    }

    void paraseGcode(const std::string& gCodeFile, std::vector<std::vector<SliceLine3D>>& m_sliceLayers, trimesh::box3& box, std::unordered_map<std::string, std::string>& kvs, ccglobal::Tracer* tracer)
    {
        SliceCompany sliceCompany = SliceCompany::none;

        //获取原始数据
        _paraseGcode(sliceCompany,gCodeFile,box,m_sliceLayers,kvs);

        //解析成通用参数
        //_paraseKvs(sliceCompany, box, kvs);

        //过滤其他参数
        _removeOthersKvs(kvs);
    }

    void paraseGcodeAndPreview(const std::string& gCodeFile, GcodeTracer* pathData, ccglobal::Tracer* tracer)
    {
        if (!pathData)
        {
            return;
        }
        GCodeProcessor gcodeProcessor;
        gcodeProcessor.m_used_filaments.reset();

        //double s = PI * sqr(0.5* extruder->filament_diameter());
        //double weight = volume.second * extruder->filament_density() * 0.001;
        //total_used_filament += volume.second / s;

        //获取原始数据
        trimesh::box3 box;
        _paraseGcodeAndPreview(gCodeFile, gcodeProcessor, pathData, tracer);

        //解析成通用参数
        _paraseKvs(gcodeProcessor, box);

        //设置参数
        _setParam(gcodeProcessor, pathData);
        pathData->setParam(gcodeProcessor.gcodeParaseInfo);
    }
}