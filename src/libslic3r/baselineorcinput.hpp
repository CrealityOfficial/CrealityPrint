#ifndef BASELINEINPUTORCA_H
#define BASELINEINPUTORCA_H
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/adapters.hpp>

#include "libslic3r/Model.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Slicing.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PrintBase.hpp"
#include "nlohmann/json.hpp"
#include <boost/nowide/fstream.hpp>

#include "GCode/ThumbnailData.hpp"
#include "libslic3r/Semver.hpp"

#include <sstream>

#include "baseline.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

struct TempParamater
{
    bool is_bbl_printer = false;
    int plate_index = 0;
    Slic3r::Vec3d plate_origin = Slic3r::Vec3d(0.0, 0.0, 0.0);
    std::string outFile;
    std::string temp_directory;
    int extruderCount = 1;
};

class ScopeGuard
{
public:
    template<class Callable>
    ScopeGuard(Callable&& func) try
        : m_handler(std::forward<Callable>(func))
    {}
    catch (...)
    {
        func();
        throw;
    }

    ScopeGuard(ScopeGuard&& other) : m_handler(std::move(other.m_handler))
    {
        other.m_handler = nullptr;
    }

    ~ScopeGuard()
    {
        if (!m_handler)
            return;

        try
        {
            m_handler(); // must not throw
        }
        catch (...)
        {
            /* std::terminate(); ? */
        }
    }

    void Dismiss() noexcept
    {
        m_handler = nullptr;
    }

private:
    ScopeGuard(const ScopeGuard&) = delete;
    void operator = (const ScopeGuard&) = delete;

    std::function<void()> m_handler;
};

constexpr char* BaseLineOrcaInputName = "orca_input";
constexpr char* BaseLineOrcaOutputName = "orca_output";

#define BLName_Def(X) constexpr char* N_##X = #X
#define BLName_Val(X) N_##X

#define BLReturnBoolen(expr) {bool __err__ = expr; /*assert(__err__);*/ return __err__;}

BLName_Def(slicer);
BLName_Def(model);
BLName_Def(dynamic_print_config);
BLName_Def(temp_param);
BLName_Def(calib_param);
BLName_Def(thumbnail);
BLName_Def(config);
BLName_Def(object);
BLName_Def(wipe_tower);
BLName_Def(positions);
BLName_Def(rotation);
BLName_Def(extruderParamsMap);
BLName_Def(materialName);
BLName_Def(bedTemp);
BLName_Def(heatEndTemp);
BLName_Def(print_speed);
BLName_Def(printSpeedMap);
BLName_Def(perimeterSpeed);
BLName_Def(externalPerimeterSpeed);
BLName_Def(infillSpeed);
BLName_Def(solidInfillSpeed);
BLName_Def(topSolidInfillSpeed);
BLName_Def(supportSpeed);
BLName_Def(smallPerimeterSpeed);
BLName_Def(maxSpeed);
BLName_Def(bed_poly);
BLName_Def(stl_design_id);
BLName_Def(design_info);
BLName_Def(DesignId);
BLName_Def(Designer);
BLName_Def(DesignerUserId);
BLName_Def(model_info);
BLName_Def(cover_file);
BLName_Def(license);
BLName_Def(description);
BLName_Def(copyright);
BLName_Def(model_name);
BLName_Def(origin);
BLName_Def(metadata_items);
BLName_Def(profile_info);
BLName_Def(ProfileTile);
BLName_Def(ProfileCover);
BLName_Def(ProfileDescription);
BLName_Def(ProfileUserId);
BLName_Def(ProfileUserName);
BLName_Def(curr_plate_index);
BLName_Def(plates_custom_gcodes);
BLName_Def(gcodes);
BLName_Def(mode);
BLName_Def(print_z);
BLName_Def(type);
BLName_Def(extruder);
BLName_Def(color);
BLName_Def(extra);
BLName_Def(calib_pa_pattern);
BLName_Def(handle_xy_size);
BLName_Def(handle_spacing);
BLName_Def(print_size_x);
BLName_Def(print_size_y);
BLName_Def(max_layer_z);
BLName_Def(m_starting_point);
BLName_Def(start_offset);
BLName_Def(material);
BLName_Def(bbl_priner);
BLName_Def(plate_index);
BLName_Def(plate_origin);
BLName_Def(extruder_count);
BLName_Def(start);
BLName_Def(end);
BLName_Def(step);
BLName_Def(print_numbers);
BLName_Def(CalibMode);
BLName_Def(width);
BLName_Def(height);
BLName_Def(pixels);
BLName_Def(attribute);
BLName_Def(name);
BLName_Def(module_name);
BLName_Def(input_file);
BLName_Def(instance);
BLName_Def(volume);
BLName_Def(layer_config_ranges);
BLName_Def(layer_height_profile);
BLName_Def(printable);
BLName_Def(sla_support_points);
BLName_Def(pos);
BLName_Def(head_front_radius);
BLName_Def(is_new_island);
BLName_Def(sla_points_status);
BLName_Def(normal);
BLName_Def(radius);
BLName_Def(failed);
BLName_Def(steps);
BLName_Def(position);
BLName_Def(sla_drain_holes);
BLName_Def(cut_connectors);
BLName_Def(check_sum);
BLName_Def(connectors_cnt);
BLName_Def(origin_translation);
BLName_Def(volume_ids);
BLName_Def(transformation);
BLName_Def(assemble_transform);
BLName_Def(offset_to_assembly);
BLName_Def(assemble_initialized);
BLName_Def(print_volume_state);
BLName_Def(use_loaded_id_for_label);
BLName_Def(arrange_order);
BLName_Def(loaded_id);
BLName_Def(points);
BLName_Def(contour);
BLName_Def(holes);
BLName_Def(style);
BLName_Def(path);
BLName_Def(prop);
BLName_Def(char_gap);
BLName_Def(line_gap);
BLName_Def(boldness);
BLName_Def(skew);
BLName_Def(collection_number);
BLName_Def(per_glyph);
BLName_Def(align);
BLName_Def(size_in_mm);
BLName_Def(family);
BLName_Def(face_name);
BLName_Def(weight);
BLName_Def(text);
BLName_Def(source);
BLName_Def(object_idx);
BLName_Def(volume_idx);
BLName_Def(mesh_offset);
BLName_Def(transform);
BLName_Def(is_converted_from_inches);
BLName_Def(is_converted_from_meters);
BLName_Def(is_from_builtin_objects);
BLName_Def(cut_info);
BLName_Def(from_upper);
BLName_Def(is_connector);
BLName_Def(is_processed);
BLName_Def(connector_type);
BLName_Def(radius_tolerance);
BLName_Def(height_tolerance);
BLName_Def(z_angle);
BLName_Def(cut_id);
BLName_Def(attribs);
BLName_Def(supported_facets);
BLName_Def(seam_facets);
BLName_Def(mmu_segmentation_facets);
BLName_Def(mmuseg_extruders);
BLName_Def(exterior_facets);
BLName_Def(text_configuration);
BLName_Def(emboss_shape);
BLName_Def(mesh);
BLName_Def(material_id);
BLName_Def(convex_hull);
BLName_Def(m_convex_hull_2d);
BLName_Def(cached_trans_matrix);
BLName_Def(cached_2d_polygon);
BLName_Def(shapes_with_ids);
BLName_Def(id);
BLName_Def(expolygon);
BLName_Def(is_healed);
BLName_Def(final_shape);
BLName_Def(expolygons);
BLName_Def(scale);
BLName_Def(projection);
BLName_Def(depth);
BLName_Def(use_surface);
BLName_Def(fix_3mf_tr);
BLName_Def(svg_file);
BLName_Def(path_in_3mf);
BLName_Def(image);
BLName_Def(nsvgimage);
BLName_Def(file_data);
BLName_Def(shape);
BLName_Def(fill);
BLName_Def(stroke);
BLName_Def(opacity);
BLName_Def(strokeWidth);
BLName_Def(strokeDashOffset);
BLName_Def(strokeDashArray);
BLName_Def(strokeDashCount);
BLName_Def(strokeLineCap);
BLName_Def(miterLimit);
BLName_Def(fillRule);
BLName_Def(flags);
BLName_Def(bounds);
BLName_Def(paths);
BLName_Def(pts);
BLName_Def(npts);
BLName_Def(closed);
BLName_Def(gradient);
BLName_Def(xform);
BLName_Def(spread);
BLName_Def(fx);
BLName_Def(fy);
BLName_Def(nstops);
BLName_Def(stops);
BLName_Def(offset);
BLName_Def(stats);
BLName_Def(number_of_facets);
BLName_Def(max);
BLName_Def(min);
BLName_Def(size);
BLName_Def(number_of_parts);
BLName_Def(open_edges);
BLName_Def(repaired_errors);
BLName_Def(edges_fixed);
BLName_Def(degenerate_facets);
BLName_Def(facets_removed);
BLName_Def(facets_reversed);
BLName_Def(backwards_edges);
BLName_Def(init_shift);
BLName_Def(indexed_triangle_set);
BLName_Def(its);
BLName_Def(indices);
BLName_Def(vertices);
BLName_Def(properties);
BLName_Def(area);

BLName_Def(key);
BLName_Def(value);

namespace cxbaseline
{
    class BaseLineLogger
    {
    public:
        void PushKey(const std::string& key);
        void PopKey();

        void LogError(const std::string& msg = std::string());
        std::string ErrorMsg() const { return m_error_msg; }
    public :
        std::string m_error_msg = "";
    private:
        std::vector<std::string> m_key_group;
        
    };

    class BaselineOrcaFileUtils
    {
    public:
        static std::string CreateCompareEorrorFile(const std::string& root_dir, const std::string& name,const bool error = true);
        static std::string CreateBaselineFile(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group = {});
        static std::string UpdateBaselineFile(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group = {});

        static std::string GetBaselineFileLatest(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group = {});

        static bool RemoveBaselineFileLatest(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group = {});
        static bool RemoveBaselineFileAll(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group = {});

    private:
        static constexpr char* EXT = ".cxbl";
        static constexpr int VER_SIZE = 3;
        static constexpr int VER_ERR = 0;
        static constexpr int VER_INIT = 1;

        static bool _IsDirExist(const std::string& path);
        static bool _IsFileExist(const std::string& path);
        static bool _CreateDir(const std::string& path);
        static bool _CreateFile(const std::string& path);
        static bool _RemoveDir(const std::string& path);
        static bool _RemoveFile(const std::string& path);

        static std::string _ComposeDir(const std::string& root_dir, const std::string& module);
        static std::string _ComposeDir(const std::string& root_dir, const std::vector<std::string>& module);

        static int _GetBaselineInitVersion();
        static int _GetBaselineNextVersion(int current_version);
        static int _GetBaselineCurrentVersion(const std::string& dir, const std::string name);
        static std::string _ComposeBaselinePath(const std::string& dir, const std::string name, int version);
    };

    class BaselineOrcaFileHelper
    {
    public:
        static bool CreateBaselineFile(nlohmann::json& json, const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group);
        static bool UpdateBaselineFile(nlohmann::json& json, const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group);
        static bool ReadBaselineFileLatest(nlohmann::json& json, const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group);
    };

    class BaselineOrcaInput final : public Baseline
    {
    public:
        BaselineOrcaInput(const std::string& name, const std::vector<std::string>& modules);
        virtual ~BaselineOrcaInput() = default;

        bool Generate() override;
        bool Compare() override;
        bool Update() override;

        void Add(const Slic3r::Model* model);
        void Add(const Slic3r::DynamicPrintConfig* config);
        void Add(const TempParamater* param);
        void Add(const Slic3r::Calib_Params* param);
        void Add(const Slic3r::ThumbnailsList* thumbnailData);

    private:
        void _GenerateBaseline(nlohmann::json& json_root);
        bool _CompareBaseline(const nlohmann::json& json_file);
        void writeResultData(const std::string& message,bool cache = false);

    private:
        static void _BuildEntityModel(nlohmann::json& json_object, const Slic3r::Model& model);
        static bool _CompareEntityModel(const nlohmann::json& json_model, const Slic3r::Model& model, BaseLineLogger& log);

        static void _BuildEntityModelConfig(nlohmann::json& json_object, const Slic3r::ModelConfig& config);
        static bool _CompareEntityModelConfig(const nlohmann::json& json_config, const Slic3r::ModelConfig& config, BaseLineLogger& log);

        static void _BuildEntityModelConfigObject(nlohmann::json& json_config, const Slic3r::ModelConfigObject& config);
        static bool _CompareEntityModelConfigObject(const nlohmann::json& json_config, const Slic3r::ModelConfigObject& config, BaseLineLogger& log);

        static void _BuildBlockDynamicPrintConfig(nlohmann::json& json_object, const Slic3r::DynamicPrintConfig& config);
        static bool _CompareBlockDynamicPrintConfig(const nlohmann::json& json_config, const std::string& name, const Slic3r::DynamicPrintConfig& config, BaseLineLogger& log);
        static bool _CompareEntityDynamicPrintConfig(const nlohmann::json& json_config, const Slic3r::DynamicPrintConfig& config, BaseLineLogger& log);

        static void _BuildBlockTempParam(nlohmann::json& json_param, const TempParamater& param);
        static bool _CompareBlockTempParam(const nlohmann::json& json_object, const std::string& name, const TempParamater& param, BaseLineLogger& log);

        static void _BuildBlockCalibParam(nlohmann::json& json_param, const Slic3r::Calib_Params& param);
        static bool _CompareBlockCalibParam(const nlohmann::json& json_param, const std::string& name, const Slic3r::Calib_Params& param, BaseLineLogger& log);

        static void _BuildBlockThumbnail(nlohmann::json& json_thumbnail, const Slic3r::ThumbnailsList& thumbnailList);
        static bool _CompareBlockThumbnail(const nlohmann::json& json_thumbnail, const std::string& name, const Slic3r::ThumbnailsList& thumbnailList, BaseLineLogger& log);

        static void _BuildEntityModelObject(nlohmann::json& json_object, const Slic3r::ModelObject& object);
        static bool _CompareEntityModelObject(const nlohmann::json& json_object, const Slic3r::ModelObject& object, BaseLineLogger& log);

        static void _BuildEntityPath(nlohmann::json& json_object, const std::string& file);
        static bool _CompareEntityPath(const nlohmann::json& json_object, const std::string& file, BaseLineLogger& log);

        static void _BuildEntityModelInstance(nlohmann::json& json_object, const Slic3r::ModelInstance& instance);
        static bool _CompareEntityModelInstance(const nlohmann::json& json_object, const Slic3r::ModelInstance& instance, BaseLineLogger& log);

        static void _BuildEntityTransformation(nlohmann::json& json_object, const Slic3r::Geometry::Transformation& transform);
        static bool _CompareEntityTransformation(const nlohmann::json& json_object, const Slic3r::Geometry::Transformation& transform, BaseLineLogger& log);

        static void _BuildEntityTranform3d(nlohmann::json& json_object, const Slic3r::Transform3d& transform);
        static bool _CompareEntityTranform3d(const nlohmann::json& json_object, const Slic3r::Transform3d& transform, BaseLineLogger& log);

        static void _BuildEntityPolygon(nlohmann::json& json_object, const Slic3r::Polygon& polygon);
        static bool _CompareEntityPolygon(const nlohmann::json& json_object, const Slic3r::Polygon& polygon, BaseLineLogger& log);

        static void _BuildEntityExPolygon(nlohmann::json& json_object, const Slic3r::ExPolygon& polygon);
        static bool _CompareEntityExPolygon(const nlohmann::json& json_object, const Slic3r::ExPolygon& polygon, BaseLineLogger& log);

        static void _BuildEntityVtxIdx(nlohmann::json& json_object, const stl_triangle_vertex_indices& vec);
        static bool _CompareEntityVtxIdx(const nlohmann::json& json_object, const stl_triangle_vertex_indices& vec, BaseLineLogger& log);

        static void _BuildEntityVec2d(nlohmann::json& json_object, const Slic3r::Vec2d& vec);
        static bool _CompareEntityVec2d(const nlohmann::json& json_object, const Slic3r::Vec2d& vec, BaseLineLogger& log);

        static void _BuildEntityVec2crd(nlohmann::json& json_object, const Slic3r::Vec2crd& vec);
        static bool _CompareEntityVec2crd(const nlohmann::json& json_object, const Slic3r::Vec2crd& vec, BaseLineLogger& log);

        static void _BuildEntityVec3d(nlohmann::json& json_object, const Slic3r::Vec3d& vec);
        static bool _CompareEntityVec3d(const nlohmann::json& json_object, const Slic3r::Vec3d& vec, BaseLineLogger& log);

        static void _BuildEntityVec3f(nlohmann::json& json_object, const Slic3r::Vec3f& vec);
        static bool _CompareEntityVec3f(const nlohmann::json& json_object, const Slic3r::Vec3f& vec, BaseLineLogger& log);

        static void _BuildEntityTextConfig(nlohmann::json& json_object, const Slic3r::TextConfiguration& text_config);
        static bool _CompareEntityTextConfig(const nlohmann::json& json_object, const Slic3r::TextConfiguration& text_config, BaseLineLogger& log);

        static void _BuildEntityModelVolume(nlohmann::json& json_object, const Slic3r::ModelVolume& volume);
        static bool _CompareEntityModelVolume(const nlohmann::json& json_object, const Slic3r::ModelVolume& volume, BaseLineLogger& log);

        static void _BuildEntityFacetsAnnotation(nlohmann::json& json_object_group, const Slic3r::FacetsAnnotation& facets);
        static bool _CompareEntityFacetsAnnotation(const nlohmann::json& json_object, const Slic3r::FacetsAnnotation& facets, BaseLineLogger& log);

        static void _BuildEntityEmbossShape(nlohmann::json& json_object, const Slic3r::EmbossShape& emboss_shape);
        static bool _CompareEntityEmbossShape(const nlohmann::json & json_object, const Slic3r::EmbossShape & emboss_shape, BaseLineLogger & log);

        static void _BuildEntitySvgFile(nlohmann::json& json_object, const Slic3r::EmbossShape::SvgFile& svg_file);
        static bool _CompareEntitySvgFile(const nlohmann::json& json_object, const Slic3r::EmbossShape::SvgFile& svg_file, BaseLineLogger& log);

        static void _BuildEntityImage(nlohmann::json& json_object, const NSVGimage& image);
        static bool _CompareEntityImage(const nlohmann::json& json_object, const NSVGimage& image, BaseLineLogger& log);

        static void _BuildEntityPaint(nlohmann::json& json_object, const NSVGpaint& paint);
        static bool _CompareEntityPaint(const nlohmann::json& json_object, const NSVGpaint& paint, BaseLineLogger& log);

        static void _BuildEntityMesh(nlohmann::json& json_object, const Slic3r::TriangleMesh& mesh);
        static bool _CompareEntityMesh(const nlohmann::json& json_object, const Slic3r::TriangleMesh& mesh, BaseLineLogger& log);

    private:
        static std::string _Type2String(const nlohmann::json& json_object)
        {
            switch (json_object.type())
            {
            case nlohmann::json::value_t::null:
            {
                return "'null'";
            }
            case nlohmann::json::value_t::object:
            {
                return "'object'";
            }
            case nlohmann::json::value_t::array:
            {
                return "'array'";
            }
            case nlohmann::json::value_t::string:
            {
                return "'string'";
            }
            case nlohmann::json::value_t::boolean:
            {
                return "'boolean'";
            }
            case nlohmann::json::value_t::number_integer:
            {
                return "'integral'";
            }
            case nlohmann::json::value_t::number_unsigned:
            {
                return "'integral'";
            }
            case nlohmann::json::value_t::number_float:
            {
                return "'float'";
            }
            default:
            {
                return "'UNKNOWN'";
            }
            }
        }

    private:
        static void _BuildObjectInit(nlohmann::json& json_object)
        {
            using namespace nlohmann;

            if (json_object.is_null())
            {
                json_object = json::object();
            }

            assert(json_object.is_object() && json_object.empty());
        }

        static void _BuildObjectGroupInit(nlohmann::json& json_object_group)
        {
            using namespace nlohmann;

            if (json_object_group.is_null())
            {
                json_object_group = json::array();
            }

            assert(json_object_group.is_array() && json_object_group.empty());
        }

        static void _BuildObjectMapInit(nlohmann::json& json_object_group)
        {
            _BuildObjectGroupInit(json_object_group);
        }

        template<typename T, typename F>
        static void _BuildObject(nlohmann::json& json_object, const T& object, F func)
        {
            _BuildObjectInit(json_object);

            func(json_object, object);
        }

        template<typename T, typename F>
        static void _BuildObject(nlohmann::json& json_object, const T* object, F func)
        {
            _BuildObjectInit(json_object);

            if (!object)
                return;

            func(json_object, *object);
        }

        template<typename T, typename F>
        static void _BuildObjectOptional(nlohmann::json& json_object, const std::optional<T>& object, F func)
        {
            _BuildObjectInit(json_object);

            if (object.has_value())
            {
                func(json_object, object.value());
            }
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static void _BuildObjectOptional(nlohmann::json& json_object, const std::optional<T>& object)
        {
            _BuildObjectInit(json_object);

            if (object.has_value())
            {
                json_object = object.value();
            }
        }

        static void _BuildObjectOptional(nlohmann::json& json_object, const std::optional<std::string>& object)
        {
            _BuildObjectInit(json_object);

            if (object.has_value())
            {
                json_object = object.value();
            }
        }

        template<typename T, typename F>
        static void _BuildObjectUniquePtr(nlohmann::json& json_object, const std::unique_ptr<T>& object, F func)
        {
            if (object)
            {
                _BuildObject(json_object, *object, func);
            }
        }

        template<typename T, typename F>
        static void _BuildObjectSharedPtr(nlohmann::json& json_object, const std::shared_ptr<T>& object, F func)
        {
            if (object)
            {
                _BuildObject(json_object, *object, func);
            }
        }
        
        template<typename T, typename F>
        static void _BuildObjectGroup(nlohmann::json& json_object_group, const std::vector<T>* object_group, F func)
        {
            using namespace nlohmann;

            _BuildObjectGroupInit(json_object_group);

            if (!object_group)
                return;

            for (auto& object : *object_group)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                func(json_object, object);
            }
        }

        template<typename T, typename F, typename std::enable_if<!std::is_pointer<T>::value> ::type* = nullptr>
        static void _BuildObjectGroup(nlohmann::json& json_object_group, const std::vector<T>& object_group, F func)
        {
            using namespace nlohmann;

            _BuildObjectGroupInit(json_object_group);

            for (auto& object : object_group)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                func(json_object, object);
            }
        }

        template<typename T, typename F, typename std::enable_if<std::is_pointer<T>::value> ::type* = nullptr>
        static void _BuildObjectGroup(nlohmann::json& json_object_group, const std::vector<T>& object_group, F func)
        {
            using namespace nlohmann;

            _BuildObjectGroupInit(json_object_group);

            for (auto& object : object_group)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                if (object)
                {
                    func(json_object, *object);
                }
            }
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static void _BuildObjectGroup(nlohmann::json& json_object_group, const std::vector<T>& object_group)
        {
            using namespace nlohmann;

            _BuildObjectGroupInit(json_object_group);

            for (auto& object : object_group)
            {
                json_object_group.push_back(object);
            }
        }

        static void _BuildObjectGroup(nlohmann::json& json_object_group, const std::vector<std::string>& object_group)
        {
            _BuildObjectGroupInit(json_object_group);

            for (auto& object : object_group)
            {
                json_object_group.push_back(object);
            }
        }

        template<typename T, typename F>
        static void _BuildObjectGroup(nlohmann::json& json_object_group, const T* object_group, int object_group_count, F func)
        {
            using namespace nlohmann;

            _BuildObjectGroupInit(json_object_group);

            for (int i = 0; i < object_group_count; ++i)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                func(json_object, object_group[i]);
            }
        }

        template<typename T, typename F>
        static void _BuildObjectGroup(nlohmann::json& json_object_group, const T* object_group, F func)
        {
            using namespace nlohmann;

            _BuildObjectGroupInit(json_object_group);

            while (object_group)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                object_group = func(json_object, *object_group);
            }
        }

        template<typename K, typename V, typename KF, typename VF>
        static void _BuildObjectMap(nlohmann::json& json_object_group, const std::map<K, V>& object_map, KF k_func, VF v_func)
        {
            using namespace nlohmann;

            _BuildObjectMapInit(json_object_group);

            for (auto & iter : object_map)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                k_func(json_object[BLName_Val(key)], iter.first);
                v_func(json_object[BLName_Val(value)], iter.second);
            }
        }

        template<typename K, typename V, typename VF, typename std::enable_if<std::is_arithmetic<K>::value> ::type* = nullptr>
        static void _BuildObjectMap(nlohmann::json& json_object_group, const std::map<K, V>& object_map, VF v_func)
        {
            using namespace nlohmann;

            _BuildObjectMapInit(json_object_group);

            for (auto & iter : object_map)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                json_object[BLName_Val(key)] = iter.first;
                v_func(json_object[BLName_Val(value)], iter.second);
            }
        }

        template<typename K, typename V, typename VF, typename std::enable_if<std::is_arithmetic<K>::value> ::type* = nullptr>
        static void _BuildObjectMap(nlohmann::json& json_object_group, const std::map<std::pair<K, K>, V>& object_map, VF v_func)
        {
            using namespace nlohmann;

            _BuildObjectMapInit(json_object_group);

            for (auto& iter : object_map)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                json_object[BLName_Val(key)] = { iter.first.first, iter.first.second };
                v_func(json_object[BLName_Val(value)], iter.second);
            }
        }

        template<typename V, typename VF, typename std::enable_if<!std::is_pointer<V>::value> ::type* = nullptr>
        static void _BuildObjectMap(nlohmann::json& json_object_group, const std::map<std::string, V>& object_map, VF v_func)
        {
            using namespace nlohmann;

            _BuildObjectMapInit(json_object_group);

            for (auto& iter : object_map)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                json_object[BLName_Val(key)] = iter.first;
                v_func(json_object[BLName_Val(value)], iter.second);
            }
        }

        template<typename V, typename VF, typename std::enable_if<std::is_pointer<V>::value> ::type* = nullptr>
        static void _BuildObjectMap(nlohmann::json& json_object_group, const std::map<std::string, V>& object_map, VF v_func)
        {
            using namespace nlohmann;

            _BuildObjectMapInit(json_object_group);

            for (auto& iter : object_map)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                json_object[BLName_Val(key)] = iter.first;

                if (iter.second)
                {
                    v_func(json_object[BLName_Val(value)], *iter.second);
                }
                else
                {
                    json_object[BLName_Val(value)] = json::object();
                }
            }
        }

        static void _BuildObjectMap(nlohmann::json& json_object_group, const std::map<std::string, std::string>& object_map)
        {
            using namespace nlohmann;

            _BuildObjectMapInit(json_object_group);

            for (auto& iter : object_map)
            {
                json json_object;
                ScopeGuard guard([&json_object_group, &json_object]()
                    { json_object_group.push_back(json_object); });

                json_object[BLName_Val(key)] = iter.first;
                json_object[BLName_Val(value)] = iter.second;
            }
        }

    private:
        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static std::string _CompareLogErrorValue(T baseline_value, T entity_value)
        {
            return "The baseline value is " + std::to_string(baseline_value) + " while the entity value is " + std::to_string(entity_value);
        }

        static std::string _CompareLogErrorType(const nlohmann::json& json_entity, const std::string& entity_type)
        {
            return "The baseline type is " + _Type2String(json_entity) + " while the entity type is '" + entity_type + "'";
        }

        static std::string _CompareLogErrorValue(const std::string& baseline_value, const std::string& entity_value)
        {
            return "The baseline value is " + baseline_value + " while the entity value is " + entity_value;
        }

        static std::string _CompareLogErrorGroupSize(int baseline_size, int entity_size)
        {
            return "The baseline group size is" + std::to_string(baseline_size) + "while the entity group size is" + std::to_string(entity_size);
        }

        static std::string _CompareLogErrorGroupSize2(int baseline_size, int entity_size)
        {
            return "The baseline group size is" + std::to_string(baseline_size) + "while the entity group size is more than " + std::to_string(entity_size);
        }

        static std::string _CompareLogErrorMapSize(int baseline_size, int entity_size)
        {
            return "The baseline map size is" + std::to_string(baseline_size) + "while the entity map size is " + std::to_string(entity_size);
        }

        static std::string _CompareLogErrorMapIndex(int baseline_size)
        {
            return "The baseline map has not entity at index" + std::to_string(baseline_size);
        }

        template<typename T>
        static bool _CompareEntityExist(const nlohmann::json& json_entity, const T* entity, BaseLineLogger& log)
        {
            if (json_entity.empty() || !entity)
                BLReturnBoolen(true);

            if (!json_entity.empty() && !entity)
            {
                log.LogError("The baseline is NOT empty while the entity is");
                BLReturnBoolen(false);
            }

            if (json_entity.empty() && entity)
            {
                log.LogError("The baseline is empty while the entity is NOT");
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        template<typename T>
        static bool _CompareEntityExist(const nlohmann::json& json_entity, const std::optional<T>& entity, BaseLineLogger& log)
        {
            if (json_entity.empty() || !entity)
                BLReturnBoolen(true);

            if (!json_entity.empty() && !entity)
            {
                log.LogError("The baseline is NOT empty while the entity is");
                BLReturnBoolen(false);
            }

            if (json_entity.empty() && entity)
            {
                log.LogError("The baseline is empty while the entity is NOT");
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        template<typename T>
        static bool _CompareEntityExist(const nlohmann::json& json_entity, const std::unique_ptr<T>& entity, BaseLineLogger& log)
        {
            if (json_entity.empty() || !entity)
                BLReturnBoolen(true);

            if (!json_entity.empty() && !entity)
            {
                log.LogError("The baseline is NOT empty while the entity is");
                BLReturnBoolen(false);
            }

            if (json_entity.empty() && entity)
            {
                log.LogError("The baseline is empty while the entity is NOT");
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        template<typename T>
        static bool _CompareEntityExist(const nlohmann::json& json_entity, const std::shared_ptr<T>& entity, BaseLineLogger& log)
        {
            if (json_entity.empty() || !entity)
                BLReturnBoolen(true);

            if (!json_entity.empty() && !entity)
            {
                log.LogError("The baseline is NOT empty while the entity is");
                BLReturnBoolen(false);
            }

            if (json_entity.empty() && entity)
            {
                log.LogError("The baseline is empty while the entity is NOT");
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        static bool _CompareObjectExist(const nlohmann::json& json_object, const std::string& name, BaseLineLogger& log)
        {
            auto iter = json_object.find(name);
            if (iter == json_object.end())
            {
                log.LogError("The baseline does NOT have object named " + name + " while the object does");
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        template<typename T>
        static bool _CompareObjectExist(const nlohmann::json& json_object, const std::string& name, const T* entity_group, BaseLineLogger& log)
        {
            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_enity = json_object.find(name).value();

            BLReturnBoolen(_CompareEntityExist(json_enity, entity_group, log));
        }

        static bool _CompareEntityGroupValid(const nlohmann::json& json_entity, BaseLineLogger& log)
        {
            if (!json_entity.is_array())
            {
                log.LogError("The baseline is not an array while the entity group is.");
                BLReturnBoolen(false);
            }
            BLReturnBoolen(true);
        }

        static bool _CompareEntityMapValid(const nlohmann::json& json_entity, BaseLineLogger& log)
        {
            if (!json_entity.is_array() && json_entity.size() != 2 && json_entity.contains(BLName_Val(key)) && json_entity.contains(BLName_Val(value)))
            {
                log.LogError("The baseline is not an map while the entity map is.");
                BLReturnBoolen(false);
            }
            BLReturnBoolen(true);
        }

        template<typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value> ::type* = nullptr>
        static inline bool _CompareValue(T value1, T value2)
        {
            BLReturnBoolen(value1 == value2);
        }

        template<typename T, typename std::enable_if<std::is_floating_point<T>::value> ::type* = nullptr>
        static inline bool _CompareValue(T value1, T value2)
        {
            BLReturnBoolen(std::abs(value1 - value2) < 1e-6);
        }

        static inline bool _CompareValue(const std::string& value1, const std::string& value2)
        {
            BLReturnBoolen(value1 == value2);
        }

        static inline bool _CompareValue(bool value1, bool value2)
        {
            BLReturnBoolen(value1 == value2);
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static inline bool _CompareEntity(const nlohmann::json& json_entity, T entity, BaseLineLogger& log)
        {
            if (json_entity.type() != nlohmann::json::value_t::boolean
                && json_entity.type() != nlohmann::json::value_t::number_integer
                && json_entity.type() != nlohmann::json::value_t::number_unsigned
                && json_entity.type() != nlohmann::json::value_t::number_float)
            {
                log.LogError(_CompareLogErrorType(json_entity, "arithmetic"));
                BLReturnBoolen(false);
            }

            T baseline_value = json_entity.template get<T>();
            if (!_CompareValue(baseline_value, entity))
            {
                log.LogError(_CompareLogErrorValue(baseline_value, entity));
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        static inline bool _CompareEntity(const nlohmann::json& json_entity, const std::string& entity, BaseLineLogger& log)
        {
            if (json_entity.type() != nlohmann::json::value_t::string)
            {
                log.LogError(_CompareLogErrorType(json_entity, "string"));
                BLReturnBoolen(false);
            }

            const std::string& baseline_value = json_entity.template get<std::string>();
            if (!_CompareValue(baseline_value, entity))
            {
                log.LogError(_CompareLogErrorValue(baseline_value, entity));
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        template<typename T, typename F>
        static bool _CompareEntity(const nlohmann::json& json_entity, const T* entity, BaseLineLogger& log, F func)
        {
            if (!_CompareEntityExist(json_entity, entity, log))
                BLReturnBoolen(false);

            if (!entity)
                BLReturnBoolen(true);

            BLReturnBoolen(func(json_entity, *entity, log));
        }

        template<typename T, typename std::enable_if<std::is_integral<T>::value> ::type* = nullptr>
        static bool _CompareObject(const nlohmann::json& json_object, const std::string& name, T entity, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto iter = json_object.find(name);
            auto& json_value = iter.value();
            if (json_value.type() != nlohmann::json::value_t::number_integer 
             && json_value.type() != nlohmann::json::value_t::number_unsigned
             && json_value.type() != nlohmann::json::value_t::boolean)
            {
                log.LogError(_CompareLogErrorType(json_object, "integral"));
                BLReturnBoolen(false);
            }

            BLReturnBoolen(_CompareEntity(json_value, entity, log));
        }

        template<typename T, typename std::enable_if<std::is_floating_point<T>::value> ::type* = nullptr>
        static bool _CompareObject(const nlohmann::json& json_object, const std::string& name, T entity, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto iter = json_object.find(name);
            if (iter.value().type() != nlohmann::json::value_t::number_float)
            {
                log.LogError(_CompareLogErrorType(json_object, "floating_point"));
                BLReturnBoolen(false);
            }

            T baseline_value = iter.value().template get<T>();
            if (!_CompareValue(baseline_value, entity))
            {
                log.LogError(_CompareLogErrorValue(baseline_value, entity));
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        template<typename T, typename std::enable_if<std::is_enum<T>::value> ::type* = nullptr>
        static bool _CompareObject(const nlohmann::json& json_object, const std::string& name, T entity, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto iter = json_object.find(name);
            if (iter.value().type() != nlohmann::json::value_t::number_integer
                && iter.value().type() != nlohmann::json::value_t::number_unsigned)
            {
                log.LogError(_CompareLogErrorType(json_object, "enum"));
                BLReturnBoolen(false);
            }

            std::underlying_type_t<T> baseline_value = iter.value().template get<std::underlying_type_t<T>>();
            if (!_CompareValue(baseline_value, std::underlying_type_t<T>(entity)))
            {
                log.LogError(_CompareLogErrorValue(baseline_value, std::underlying_type_t<T>(entity)));
                BLReturnBoolen(false);
            }

            BLReturnBoolen(true);
        }

        static bool _CompareObject(const nlohmann::json& json_object, const std::string& name, const std::string& entity, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            BLReturnBoolen(_CompareEntity(json_object.find(name).value(), entity, log));
        }

        template<typename T, typename F, typename std::enable_if<!std::is_pointer<T>::value> ::type* = nullptr>
        static bool _CompareObject(const nlohmann::json& json_object, const std::string& name, const T& entity, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            BLReturnBoolen(func(json_object.find(name).value(), entity, log));
        }

        template<typename T, typename F, typename std::enable_if<std::is_pointer<T>::value> ::type* = nullptr>
        static bool _CompareObject(const nlohmann::json& json_object, const std::string& name, const T entity, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_entity = json_object.find(name).value();
            if (!_CompareEntityExist(json_entity, entity, log))
                BLReturnBoolen(false);

            if (!entity)
                BLReturnBoolen(true);

            BLReturnBoolen(func(json_entity, *entity, log));
        }

        template<typename T, typename F>
        static bool _CompareObjectOptional(const nlohmann::json& json_object, const std::string& name, const std::optional<T>& object, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_entity = json_object.find(name).value();
            if (!_CompareEntityExist(json_entity, object, log))
                BLReturnBoolen(false);

            if (!object)
                BLReturnBoolen(true);

            BLReturnBoolen(func(json_entity, object.value(), log));
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static bool _CompareObjectOptional(const nlohmann::json& json_object, const std::string& name, const std::optional<T>& object, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_entity = json_object.find(name).value();
            if (!_CompareEntityExist(json_entity, object, log))
                BLReturnBoolen(false);

            if (!object)
                BLReturnBoolen(true);

            BLReturnBoolen(_CompareEntity(json_object, object.value(), log));
        }

        static bool _CompareObjectOptional(const nlohmann::json& json_object, const std::string& name, const std::optional<std::string>& object, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_entity = json_object.find(name).value();
            if (!_CompareEntityExist(json_entity, object, log))
                BLReturnBoolen(false);

            if (!object)
                BLReturnBoolen(true);

            BLReturnBoolen(_CompareEntity(json_object, object.value(), log));
        }

        template<typename T, typename F>
        static bool _CompareObjectSharedPtr(const nlohmann::json& json_object, const std::string& name, const std::shared_ptr<T>& object, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_entity = json_object.find(name).value();
            if (!_CompareEntityExist(json_entity, object, log))
                BLReturnBoolen(false);

            if (!object)
                BLReturnBoolen(true);

            BLReturnBoolen(func(json_entity, *object, log));
        }

        template<typename T, typename F>
        static bool _CompareObjectUniquePtr(const nlohmann::json& json_object, const std::string& name, const std::unique_ptr<T>& object, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto& json_entity = json_object.find(name).value();
            if (!_CompareEntityExist(json_entity, object, log))
                BLReturnBoolen(false);

            if (!object)
                BLReturnBoolen(true);

            BLReturnBoolen(func(json_entity, *object, log));
        }

        template<typename T, typename F>
        static bool _CompareEntityGroupImpl(const nlohmann::json& json_entity_group, const std::vector<T>& entity_group, BaseLineLogger& log, F func)
        {
            if (!_CompareEntityGroupValid(json_entity_group, log))
                BLReturnBoolen(false);

            bool err = true;
            int json_size = json_entity_group.size();
            int group_size = entity_group.size();
            int i = 0;
            for (; i < (std::min)(json_size, group_size); ++i)
            {
                err &= func(json_entity_group.at(i), entity_group.at(i), log);
            }
            if (i < json_size || i < group_size)
            {
                log.LogError(_CompareLogErrorGroupSize(json_size, group_size));
                err = false;
            }

            BLReturnBoolen(err);
        }

        template<typename T, typename F, typename std::enable_if<!std::is_pointer<T>::value> ::type* = nullptr>
        static bool _CompareEntityGroup(const nlohmann::json& json_entity_group, const std::vector<T>& entity_group, BaseLineLogger& log, F func)
        {
            BLReturnBoolen(_CompareEntityGroupImpl(json_entity_group, entity_group, log, func));
        }

        template<typename T, typename F, typename std::enable_if<std::is_pointer<T>::value> ::type* = nullptr>
        static bool _CompareEntityGroup(const nlohmann::json& json_entity_group, const std::vector<T>& entity_group, BaseLineLogger& log, F func)
        {
            BLReturnBoolen(_CompareEntityGroupImpl(json_entity_group, entity_group, log,
                [&func](const nlohmann::json& json_entity, const T& entity, BaseLineLogger& log)
                {
                    if (!_CompareEntityExist(json_entity, entity, log))
                        BLReturnBoolen(false);

                    if (!entity)
                        BLReturnBoolen(true);

                    BLReturnBoolen(func(json_entity, *entity, log));
                }));
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static bool _CompareEntityGroup(const nlohmann::json& json_entity_group, const std::vector<T>& entity_group, BaseLineLogger& log)
        {
            BLReturnBoolen(_CompareEntityGroupImpl(json_entity_group, entity_group, log,
                [](const nlohmann::json& json_entity, T entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }));
        }

        template<typename T, typename F, typename N>
        static bool _CompareEntityGroupEx(const nlohmann::json& json_entity_group, const T* entity_group, BaseLineLogger& log, F func, N next_func)
        {
            if (!_CompareEntityGroupValid(json_entity_group, log))
                BLReturnBoolen(false);

            bool err = true;
            int json_size = json_entity_group.size();
            const T* entity = entity_group;
            for (int i = 0; i < json_size; ++i)
            {
                if (entity)
                {
                    err &= func(json_entity_group.at(i), *entity, log);
                    entity = next_func(*entity);
                }
                else
                {
                    log.LogError(_CompareLogErrorGroupSize(json_size, i + 1));
                    BLReturnBoolen(false);
                }
            }
            if (entity)
            {
                log.LogError(_CompareLogErrorGroupSize2(json_size, json_size + 1));
                BLReturnBoolen(false);
            }

            BLReturnBoolen(err);
        }

        template<typename T, typename F>
        static bool _CompareEntityGroupEx(const nlohmann::json& json_entity_group, const T* entity_group, int entity_group_size, BaseLineLogger& log, F func)
        {
            if (!_CompareEntityGroupValid(json_entity_group, log))
                BLReturnBoolen(false);

            bool err = true;
            int json_entity_group_size = json_entity_group.size();
            int i = 0;
            for (; i < (std::min)(json_entity_group_size, entity_group_size); ++i)
            {
                err &= func(json_entity_group.at(i), entity_group[i], log);
            }
            if (i < json_entity_group_size || i < entity_group_size)
            {
                log.LogError(_CompareLogErrorGroupSize(json_entity_group_size, entity_group_size));
                err = false;
            }

            BLReturnBoolen(err);
        }

        template<typename T, typename F>
        static bool _CompareObjectGroup(const nlohmann::json& json_object, const std::string& name, const std::vector<T>& entity_group, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroup(json_entity_group, entity_group, log, func));
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static bool _CompareObjectGroup(const nlohmann::json& json_object, const std::string& name, const std::vector<T>& entity_group, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroup(json_entity_group, entity_group, log,
                [](const nlohmann::json& json_entity, const T& entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }));
        }

        template<typename T, typename F>
        static bool _CompareObjectGroup(const nlohmann::json& json_object, const std::string& name, const std::vector<T*>& entity_group, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroup(json_entity_group, entity_group, log, func));
        }

        template<typename T, typename F>
        static bool _CompareObjectGroup(const nlohmann::json& json_object, const std::string& name, const std::vector<T>* entity_group, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, entity_group, log))
                BLReturnBoolen(false);

            if (!entity_group)
                BLReturnBoolen(true);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroup(json_entity_group, *entity_group, log, func));
        }

        template<typename T, typename F, typename N>
        static bool _CompareObjectGroupEx(const nlohmann::json& json_object, const std::string& name, const T* entity_group, BaseLineLogger& log, F func, N next_func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, entity_group, log))
                BLReturnBoolen(false);

            if (!entity_group)
                BLReturnBoolen(true);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroupEx(json_entity_group, entity_group, log, func, next_func));
        }

        template<typename T, typename F>
        static bool _CompareObjectGroupEx(const nlohmann::json& json_object, const std::string& name, const T* entity_group, int entity_group_size, BaseLineLogger& log, F func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, entity_group, log))
                BLReturnBoolen(false);

            if (!entity_group)
                BLReturnBoolen(true);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroupEx(json_entity_group, entity_group, entity_group_size, log, func));
        }

        template<typename T, typename std::enable_if<std::is_arithmetic<T>::value> ::type* = nullptr>
        static bool _CompareObjectGroupEx(const nlohmann::json& json_object, const std::string& name, const T* entity_group, int object_group_size, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, entity_group, log))
                BLReturnBoolen(false);

            if (!entity_group)
                BLReturnBoolen(true);

            const nlohmann::json& json_entity_group = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityGroupEx(json_entity_group, entity_group, object_group_size, log,
                [](const nlohmann::json& json_entity, const T& entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }));
        }

        template<typename K, typename V, typename KF, typename VF>
        static bool _CompareEntityMap(const nlohmann::json& json_entity_map, const std::map<K, V>& entity_map, BaseLineLogger& log, KF k_func, VF v_func)
        {
            if (!_CompareEntityMapValid(json_entity_map, log))
                BLReturnBoolen(false);

            bool err = true;
            int json_map_size = json_entity_map.size();
            int entity_map_size = entity_map.size();
            auto entity_iter = entity_map.begin();
            auto json_index = 0;
            for (; json_index < (std::min)(json_map_size, entity_map_size); ++json_index, entity_iter = std::next(entity_iter))
            {
                auto json_entity_key = json_entity_map.at(json_index)[BLName_Val(key)];
                auto json_entity_value = json_entity_map.at(json_index)[BLName_Val(value)];
                if (json_entity_key.is_null() || json_entity_value.is_null())
                {
                    log.LogError(_CompareLogErrorMapIndex(json_map_size));
                    err = false;
                    continue;
                }

                err &= k_func(json_entity_key, entity_iter->first, log);
                err &= v_func(json_entity_value, entity_iter->second, log);
            }
            if (json_index < json_map_size || entity_iter != entity_map.end())
            {
                log.LogError(_CompareLogErrorMapSize(json_map_size, entity_map_size));
                err = false;
            }

            BLReturnBoolen(err);
        }
        
        template<typename K, typename V, typename KF, typename VF>
        static bool _CompareObjectMap(const nlohmann::json& json_object, const std::string& name, const std::map<K, V>& entity_map, BaseLineLogger& log, KF k_func, VF v_func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_map = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log, k_func, v_func));
        }

        template<typename K, typename V, typename VF, typename std::enable_if<std::is_integral<K>::value> ::type* = nullptr>
        static bool _CompareObjectMap(const nlohmann::json& json_object, const std::string& name, const std::map<K, V>& entity_map, BaseLineLogger& log, VF v_func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_map = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log,
                [](const nlohmann::json& json_entity, K entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }
            , v_func));
        }

        template<typename V, typename VF, typename std::enable_if<!std::is_pointer<V>::value> ::type* = nullptr>
        static bool _CompareEntityMap(const nlohmann::json& json_entity_map, const std::map<std::string, V>& entity_map, BaseLineLogger& log, VF v_func)
        {
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log,
                [](const nlohmann::json& json_entity, const std::string& entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }
            , v_func));
        }

        template<typename V, typename VF, typename std::enable_if<std::is_pointer<V>::value> ::type* = nullptr>
        static bool _CompareEntityMap(const nlohmann::json& json_entity_map, const std::map<std::string, V>& entity_map, BaseLineLogger& log, VF v_func)
        {
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log,
                [](const nlohmann::json& json_entity, const std::string& entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }
                , [&v_func](const nlohmann::json& json_entity, const V entity, BaseLineLogger& log)
                {
                    if (!_CompareEntityExist(json_entity, entity, log))
                        BLReturnBoolen(false);
                    if (!entity)
                        BLReturnBoolen(true);

                    BLReturnBoolen(v_func(json_entity, *entity, log));
                }));
        }

        template<typename V, typename VF>
        static bool _CompareObjectMap(const nlohmann::json& json_object, const std::string& name, const std::map<std::string, V>& entity_map, BaseLineLogger& log, VF v_func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_map = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log, v_func));
        }

        static bool _CompareObjectMap(const nlohmann::json& json_object, const std::string& name, const std::map<std::string, std::string>& entity_map, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_map = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log,
                [](const nlohmann::json& json_entity, const std::string& entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }
                , [](const nlohmann::json& json_entity, const std::string& entity, BaseLineLogger& log)
                {
                    BLReturnBoolen(_CompareEntity(json_entity, entity, log));
                }));
        }

        template<typename K, typename V, typename VF, typename std::enable_if<std::is_floating_point<K>::value> ::type* = nullptr>
        static bool _CompareObjectMap(const nlohmann::json& json_object, const std::string& name, const std::map<std::pair<K, K>, V>& entity_map, BaseLineLogger& log, VF v_func)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            const nlohmann::json& json_entity_map = json_object.find(name).value();
            BLReturnBoolen(_CompareEntityMap(json_entity_map, entity_map, log,
                [](const nlohmann::json& json_entity, const std::pair<K, K>& entity, BaseLineLogger& log)
                {
                    std::vector<K> entity_group = { entity.first, entity.second };
                    BLReturnBoolen(_CompareEntityGroup(json_entity, entity_group, log));
                }
                , v_func));
        }

        static bool _CompareObjectPath(const nlohmann::json& json_object, const std::string& name, const std::string& file, BaseLineLogger& log)
        {
            log.PushKey(name);
            ScopeGuard guard([&log]() { log.PopKey(); });

            if (!_CompareObjectExist(json_object, name, log))
                BLReturnBoolen(false);

            auto iter = json_object.find(name);
            BLReturnBoolen(_CompareEntityPath(iter.value(), file, log));
        }

    private:
        const Slic3r::Model* m_elem_model = nullptr;
        const Slic3r::DynamicPrintConfig* m_elem_dynamic_print_config = nullptr;
        const TempParamater* m_elem_temp_param = nullptr;
        const Slic3r::Calib_Params* m_elem_calib_param = nullptr;
        const Slic3r::ThumbnailsList* m_elem_thumbnail = nullptr;
    };

    class BaselineOrcaOutput final : public Baseline
    {
    public:
        BaselineOrcaOutput(const std::string& name, const std::vector<std::string>& modules);
        virtual ~BaselineOrcaOutput() = default;

        bool Generate() override;
        bool Compare() override;
        bool Update() override;
    };
}

#endif