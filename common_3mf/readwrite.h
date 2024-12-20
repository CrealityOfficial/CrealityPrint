#ifndef READ_WRITE_H
#define READ_WRITE_H
#include "common_3mf.h"
#include "miniz.h"
#include "expat.h"
#include <map>
#include <mutex>
#include <memory>

namespace common_3mf
{
    class Read3MFImpl;

    struct Component
    {
        int id;
        trimesh::xform transform;
        std::string ppath;
        std::string uuid;
    };

    typedef std::vector<Component> ComponentsList;

    struct Geometry
    {
        std::vector<trimesh::vec> vertices;
        std::vector<trimesh::TriMesh::Face> triangles;
        std::vector< std::pair<int64_t,std::string> > custom_supports;
        std::vector< std::pair<int64_t,std::string> > custom_seam;
        std::vector< std::pair<int64_t,std::string> > mmu_segmentation;
        // BBS
        std::vector<std::string> face_properties;

        bool empty() { return vertices.empty() || triangles.empty(); }

        // backup & restore
        void swap(Geometry& o) {
            std::swap(vertices, o.vertices);
            std::swap(triangles, o.triangles);
            std::swap(custom_supports, o.custom_supports);
            std::swap(custom_seam, o.custom_seam);
        }

        void reset() {
            vertices.clear();
            triangles.clear();
            custom_supports.clear();
            custom_seam.clear();
            mmu_segmentation.clear();
        }
    };

    struct Object
    {
        // ID of the object inside the 3MF file, 1 based.
        int id;
        std::string type;
        std::string name;
        std::string ppath;
        std::string uuid;
        
        Geometry geometry;
        ComponentsList components;
    };

    struct BuildItem
    {
        int id;
        trimesh::xform xf;
    };

    typedef std::map<int, std::shared_ptr<Object>> IdToObjectMap;
    struct ObjectImporter
    {
        Read3MFImpl* top_importer = nullptr;
        std::string object_path;
        std::string zip_path;
        mz_zip_archive m_archive;

        IdToObjectMap object_list;
        Object* current_object = nullptr;
        XML_Parser object_xml_parser;
        bool obj_parse_error{ false };
        std::string obj_parse_error_message;

        //local parsed datas
        std::string obj_curr_metadata_name;
        std::string obj_curr_characters;
        float object_unit_factor;
        int object_current_color_group{ -1 };
        std::map<int, std::string> object_group_id_to_color;
        bool is_bbl_3mf{ false };

        ObjectImporter(Read3MFImpl* importer, std::string file_path, std::string obj_path);
        ~ObjectImporter()
        {
            _destroy_object_xml_parser();
        }

        void _destroy_object_xml_parser()
        {
            if (object_xml_parser != nullptr) {
                XML_ParserFree(object_xml_parser);
                object_xml_parser = nullptr;
            }
        }

        void _stop_object_xml_parser(const std::string& msg = std::string())
        {
            assert(!obj_parse_error);
            assert(obj_parse_error_message.empty());
            assert(object_xml_parser != nullptr);
            obj_parse_error = true;
            obj_parse_error_message = msg;
            XML_StopParser(object_xml_parser, false);
        }

        bool        object_parse_error()         const { return obj_parse_error; }
        const char* object_parse_error_message() const {
            return obj_parse_error ?
                // The error was signalled by the user code, not the expat parser.
                (obj_parse_error_message.empty() ? "Invalid 3MF format" : obj_parse_error_message.c_str()) :
                // The error was signalled by the expat parser.
                XML_ErrorString(XML_GetErrorCode(object_xml_parser));
        }
        bool restore_backup_mesh(const char** attributes, unsigned int num_attributes);
        bool restore_backup_color(const char** attributes, unsigned int num_attributes);
        bool restore_backup_seam(const char** attributes, unsigned int num_attributes);
        bool restore_backup_support(const char** attributes, unsigned int num_attributes);
        bool _extract_object_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

        bool extract_object_model();

        bool _handle_object_start_model(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_model();

        bool _handle_object_start_resources(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_resources();

        bool _handle_object_start_object(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_object();

        bool _handle_object_start_color_group(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_color_group();

        bool _handle_object_start_color(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_color();

        bool _handle_object_start_mesh(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_mesh();

        bool _handle_object_start_vertices(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_vertices();

        bool _handle_object_start_vertex(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_vertex();

        bool _handle_object_start_triangles(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_triangles();

        bool _handle_object_start_triangle(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_triangle();

        bool _handle_object_start_components(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_components();

        bool _handle_object_start_component(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_component();

        bool _handle_object_start_metadata(const char** attributes, unsigned int num_attributes);
        bool _handle_object_end_metadata();

        void _handle_object_start_model_xml_element(const char* name, const char** attributes);
        void _handle_object_end_model_xml_element(const char* name);
        void _handle_object_xml_characters(const XML_Char* s, int len);
        
        // callbacks to parse the .model file of an object
        static void XMLCALL _handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes);
        static void XMLCALL _handle_object_end_model_xml_element(void* userData, const char* name);
        static void XMLCALL _handle_object_xml_characters(void* userData, const XML_Char* s, int len);
    };

    struct PartSetting
    {
        int id;
        std::string name;
        std::string subtype;
        int extruder;
        std::map<std::string, std::string> settings;
        std::string relief_description;
    };

    struct ObjectSetting
    {
        int id;
        std::string name;
        int extruder;
        std::map<std::string, std::string> settings;
        std::vector<std::shared_ptr<PartSetting>> parts;
    };
    
    class _3MF_Base
    {
        mutable std::mutex mutex;
        mutable std::vector<std::string> m_errors;

    public:
        void add_error(const std::string& error) const { std::lock_guard<std::mutex> l(mutex); m_errors.push_back(error); }
        void clear_errors() { m_errors.clear(); }

        void log_errors(ccglobal::Tracer* tracer)
        {
            if (tracer)
            {
                for (const std::string& error : m_errors)
                    tracer->message(error.c_str());
            }
        }
    };

	class Read3MFImpl : public _3MF_Base
	{
        friend struct ObjectImporter;
	public:
		Read3MFImpl(const std::string& file_name);
		~Read3MFImpl();
        void set_backup_path(const std::string &path) {m_backup_path=path;}
		bool read_all_3mf(Scene3MF& scene, ccglobal::Tracer* tracer = nullptr);
        bool extract_file(const std::string& zip_name, const std::string& dest_file_name);
        bool _extract_from_archive(mz_zip_archive& archive, std::string const& path, std::function<bool(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)> extract, bool restore = false);
    protected:
        XML_Parser m_xml_parser;

        std::string m_3d_model_path;
        std::vector<std::string> m_sub_model_paths;

        IdToObjectMap m_objects_list;
        Object* m_current_object = nullptr;

        std::map<int, std::string> m_group_id_to_color;
        std::map<int, std::vector<double>> m_group_id_to_layer_heights;

        std::string m_thumbnail_path;

        std::string m_curr_characters;
        std::string m_curr_metadata_name;
        std::vector<std::pair<std::string, std::string>> m_model_metadata;
        std::string m_backup_path;//
        std::string m_origin_file;
        std::string m_name;
        float m_unit_factor;
        int m_current_color_group{ -1 };

        std::string m_sub_model_path;
        BuildItem* m_current_build_item = nullptr;
        std::vector<std::shared_ptr<BuildItem>> m_build_items; // the group transform

        bool m_start_new_parse_plate_gcode { false };
        std::vector<PlateCustomGCode>* m_current_customs = nullptr;
        std::map<int, std::vector<PlateCustomGCode>> m_plate_id_2_gcode_infos;

        Plate3MF* m_current_plate_info = nullptr;
        std::vector<std::shared_ptr<Plate3MF>> m_plates_infos;

        std::vector<std::shared_ptr<ObjectSetting>> m_object_settings;
        ObjectSetting* m_current_object_settings = nullptr;
        PartSetting* m_current_part_settings = nullptr;
        std::vector<ObjectImporter*> m_object_importers;
    protected:
        bool build_scene(Scene3MF& scene);

        bool _extract_xml_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler);
        bool _extract_xml_from_archive(mz_zip_archive& archive, const std::string& path, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler);
        void _destroy_xml_parser();
        void _stop_xml_parser(const std::string& msg = std::string());

        void _handle_start_relationships_element(const char* name, const char** attributes);
        void _handle_end_relationships_element(const char* name);
        bool _handle_start_relationship(const char** attributes, unsigned int num_attributes);
        bool _handle_start_model(const char** attributes, unsigned int num_attributes);
        bool _handle_start_resources(const char** attributes, unsigned int num_attributes);
        bool _handle_start_object(const char** attributes, unsigned int num_attributes);
        bool _handle_start_color_group(const char** attributes, unsigned int num_attributes);
        bool _handle_start_color(const char** attributes, unsigned int num_attributes);
        bool _handle_start_mesh(const char** attributes, unsigned int num_attributes);
        bool _handle_start_vertices(const char** attributes, unsigned int num_attributes);
        bool _handle_start_vertex(const char** attributes, unsigned int num_attributes);
        bool _handle_start_triangles(const char** attributes, unsigned int num_attributes);
        bool _handle_start_triangle(const char** attributes, unsigned int num_attributes);
        bool _handle_start_metadata(const char** attributes, unsigned int num_attributes);
        bool _handle_start_components(const char** attributes, unsigned int num_attributes);
        bool _handle_start_component(const char** attributes, unsigned int num_attributes);
        bool _handle_start_item(const char** attributes, unsigned int num_attributes);
        void _handle_start_model_xml_element(const char* name, const char** attributes);
        void _handle_end_model_xml_element(const char* name);
        void _handle_xml_characters(const XML_Char* s, int len);
        bool _handle_end_model();
        bool _handle_end_resources();
        bool _handle_end_object();
        bool _handle_end_color_group();
        bool _handle_end_color();
        bool _handle_end_mesh();
        bool _handle_end_vertices();
        bool _handle_end_vertex();
        bool _handle_end_triangles();
        bool _handle_end_triangle();
        bool _handle_end_metadata();
        bool _handle_end_item();
        //void _handle_end_model_xml_element(void* userData, const char* name);
        //void _handle_xml_characters(void* userData, const XML_Char* s, int len);

        bool _handle_start_plate(const char** attributes, unsigned int num_attributes);
        bool _handle_start_model_instance(const char** attributes, unsigned int num_attributes);
        bool _handle_start_plate_metadata(const char** attributes, unsigned int num_attributes);
        bool _handle_start_relief(const char** attributes, unsigned int num_attributes);
        bool _handle_start_object_setting(const char** attributes, unsigned int num_attributes);
        bool _handle_start_part_setting(const char** attributes, unsigned int num_attributes);
        bool _handle_end_plate();
        bool _handle_end_model_instance();
        bool _handle_end_object_setting();
        bool _handle_end_part_setting();
        void _handle_start_config_xml_element(const char* name, const char** attributes);
        void _handle_end_config_xml_element(const char* name);


        void _handle_start_custom_gcode_xml_element(const char* name, const char** attributes);
        void _handle_end_custom_gcode_xml_element(const char* name);
        bool _handle_start_plate_gcode(const char** attributes, unsigned int num_attributes);
        bool _handle_end_plate_gcode();
        bool _handle_start_plateinfo_gcode(const char** attributes, unsigned int num_attributes);
        bool _handle_end_plateinfo_gcode();
        bool _handle_start_layer_gcode(const char** attributes, unsigned int num_attributes);
        bool _handle_end_layer_gcode();
        bool _handle_start_mode_gcode(const char** attributes, unsigned int num_attributes);
        bool _handle_end_mode_gcode();

        bool _extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        bool _extract_copy_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, const std::string& dest_file_name);
        void _extract_project_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        void _extract_model_settings_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        void _extract_custom_gcode_per_print_z_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);
        void _extract_layer_heights_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

        void _handle_object_start_model_xml_element(const char* name, const char** attributes) {};
        void _handle_object_end_model_xml_element(const char* name) {};
        void _handle_object_xml_characters(const XML_Char* s, int len) {};

        static void _handle_start_relationships_element(void* userData, const char* name, const char** attributes);
        static void _handle_end_relationships_element(void* userData, const char* name);

        // callbacks to parse the .model file of an object
        static void  _handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes);
        static void  _handle_object_end_model_xml_element(void* userData, const char* name);
        static void  _handle_object_xml_characters(void* userData, const XML_Char* s, int len);

        // callbacks to parse the .model file
        static void _handle_start_model_xml_element(void* userData, const char* name, const char** attributes);
        static void _handle_end_model_xml_element(void* userData, const char* name);
        static void _handle_xml_characters(void* userData, const XML_Char* s, int len);

        // callback to parse the model_settings.config file
        static void _handle_start_config_xml_element(void* userData, const char* name, const char** attributes);
        static void _handle_end_config_xml_element(void* userData, const char* name);

        // callback to parse the custom_gcode_per_layer.xml file
        static void _handle_start_custom_gcode_xml_element(void* userData, const char* name, const char** attributes);
        static void _handle_end_custom_gcode_xml_element(void* userData, const char* name);


	protected:
		bool m_open_success;
		std::string m_file_name;

		mz_zip_archive m_archive;
	};

    class Write3MFImpl : public _3MF_Base
    {
    public:
        Write3MFImpl(const std::string& file_name);
        ~Write3MFImpl();

        bool write_scene_2_3mf(const Scene3MF& scene, ccglobal::Tracer* tracer, bool is_tmp_project=false);
        void set_backup_path(const std::string &path);
    
    protected:
        bool _add_content_file();
        bool _add_relations_file();
        bool _add_model_relations_file(const std::vector<Group3MF>& groups);
        bool _add_scene_content(const Scene3MF& scene);
        bool _add_model_to_file(mz_zip_archive &archive,const Group3MF& group);
        bool _add_group_to_stream(mz_zip_writer_staged_context& context, const Group3MF& group);
        bool _add_model_to_stream2(mz_zip_writer_staged_context& context, const Model3MF& model);
        bool _add_model_to_stream(mz_zip_writer_staged_context& context, const Model3MF& model);
        bool _add_model_setting_file(const Scene3MF& scene);
        bool _add_custom_gcode_file(const Scene3MF& scene);
        bool _add_slice_info_config_file(const Scene3MF& scene);
        bool _add_layer_height_profile_to_archive(mz_zip_archive &archive,const Scene3MF& scene);
    protected:
        bool m_open_success;
        bool m_is_tmp_project = false;
        std::string m_backup_path;
        std::string m_file_name;

        mz_zip_archive m_archive;
    };
}

#endif // READ_WRITE_H
