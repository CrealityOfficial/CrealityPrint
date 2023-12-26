#include "3mfimproter.h"
#include "expat.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/string_file.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/convert.hpp>

namespace creative_kernel
{

    const std::string RELATIONSHIPS_FILE = "_rels/.rels";
    static constexpr const char* RELATIONSHIP_TAG = "Relationship";
    static constexpr const char* TARGET_ATTR = "Target";
    static constexpr const char* RELS_TYPE_ATTR = "Type";

    static constexpr const char* MODEL_TAG = "model";
    static constexpr const char* RESOURCES_TAG = "resources";
    static constexpr const char* COLOR_GROUP_TAG = "m:colorgroup";
    static constexpr const char* COLOR_TAG = "m:color";
    static constexpr const char* OBJECT_TAG = "object";
    static constexpr const char* MESH_TAG = "mesh";
    static constexpr const char* MESH_STAT_TAG = "mesh_stat";
    static constexpr const char* VERTICES_TAG = "vertices";
    static constexpr const char* VERTEX_TAG = "vertex";
    static constexpr const char* TRIANGLES_TAG = "triangles";
    static constexpr const char* TRIANGLE_TAG = "triangle";
    static constexpr const char* COMPONENTS_TAG = "components";
    static constexpr const char* COMPONENT_TAG = "component";
    static constexpr const char* BUILD_TAG = "build";
    static constexpr const char* ITEM_TAG = "item";
    static constexpr const char* METADATA_TAG = "metadata";
    static constexpr const char* FILAMENT_TAG = "filament";
    static constexpr const char* SLICE_WARNING_TAG = "warning";
    static constexpr const char* WARNING_MSG_TAG = "msg";
    static constexpr const char* FILAMENT_ID_TAG = "id";
    static constexpr const char* FILAMENT_TYPE_TAG = "type";
    static constexpr const char* FILAMENT_COLOR_TAG = "color";
    static constexpr const char* FILAMENT_USED_M_TAG = "used_m";
    static constexpr const char* FILAMENT_USED_G_TAG = "used_g";

    static constexpr const char* UNIT_ATTR = "unit";
    static constexpr const char* NAME_ATTR = "name";
    static constexpr const char* COLOR_ATTR = "color";
    static constexpr const char* TYPE_ATTR = "type";
    static constexpr const char* ID_ATTR = "id";
    static constexpr const char* X_ATTR = "x";
    static constexpr const char* Y_ATTR = "y";
    static constexpr const char* Z_ATTR = "z";
    static constexpr const char* V1_ATTR = "v1";
    static constexpr const char* V2_ATTR = "v2";
    static constexpr const char* V3_ATTR = "v3";
    static constexpr const char* OBJECTID_ATTR = "objectid";
    static constexpr const char* TRANSFORM_ATTR = "transform";
    static constexpr const char* PID_ATTR = "pid";
    static constexpr const char* PUUID_ATTR = "p:uuid";

    static constexpr const char* OFFSET_ATTR = "offset";
    static constexpr const char* PRINTABLE_ATTR = "printable";
    static constexpr const char* INSTANCESCOUNT_ATTR = "instances_count";
    static constexpr const char* CUSTOM_SUPPORTS_ATTR = "paint_supports";
    static constexpr const char* CUSTOM_SEAM_ATTR = "paint_seam";
    static constexpr const char* MMU_SEGMENTATION_ATTR = "paint_color";
    static constexpr const char* FACE_PROPERTY_ATTR = "face_property";

    static constexpr const char* OBJECT_UUID_SUFFIX = "-41cb-4c03-9d28-80fed5dfa1dc";


    const unsigned int BBS_VALID_OBJECT_TYPES_COUNT = 2;
    const char* BBS_VALID_OBJECT_TYPES[] =
    {
        "model",
        "other"
    };

    // Encode an 8-bit string from a local code page to UTF-8.
// Multibyte to utf8
    std::string _BBS_3MF_Importer::decode_path(const char* src)
    {
#ifdef WIN32
        int len = int(strlen(src));
        if (len == 0)
            return std::string();
        // Convert the string encoded using the local code page to a wide string.
        int size_needed = ::MultiByteToWideChar(0, 0, src, len, nullptr, 0);
        std::wstring wstr_dst(size_needed, 0);

        wchar_t* _data = const_cast<wchar_t*>(wstr_dst.data());
        ::MultiByteToWideChar(0, 0, src, len, _data, size_needed);
        // Convert a wide string to utf8.
        return boost::nowide::narrow(wstr_dst.c_str());
#else /* WIN32 */
        return src;
#endif /* WIN32 */
    }

    std::string _BBS_3MF_Importer::encode_path(const char* src)
    {
#ifdef WIN32
        // Convert the source utf8 encoded string to a wide string.
        std::wstring wstr_src = boost::nowide::widen(src);
        if (wstr_src.length() == 0)
            return std::string();
        // Convert a wide string to a local code page.
        int size_needed = ::WideCharToMultiByte(0, 0, wstr_src.data(), (int)wstr_src.size(), nullptr, 0, nullptr, nullptr);
        std::string str_dst(size_needed, 0);
        char* _data = const_cast<char*>(str_dst.data());
        ::WideCharToMultiByte(0, 0, wstr_src.data(), (int)wstr_src.size(), _data, size_needed, nullptr, nullptr);
        return str_dst;
#else /* WIN32 */
        return src;
#endif /* WIN32 */
    }


    struct ZipUnicodePathExtraField
    {
        // Multibyte to utf8
        static std::string decode_path(const char* src)
        {
#ifdef WIN32
            int len = int(strlen(src));
            if (len == 0)
                return std::string();
            // Convert the string encoded using the local code page to a wide string.
            int size_needed = ::MultiByteToWideChar(0, 0, src, len, nullptr, 0);
            std::wstring wstr_dst(size_needed, 0);

            wchar_t* _data = const_cast<wchar_t*>(wstr_dst.data());
            ::MultiByteToWideChar(0, 0, src, len, _data, size_needed);
            // Convert a wide string to utf8.
            return boost::nowide::narrow(wstr_dst.c_str());
#else /* WIN32 */
            return src;
#endif /* WIN32 */
        }

        static std::string encode(std::string const& u8path, std::string const& path) {
            std::string extra;
            if (u8path != path) {
                // 0x7075 - for Unicode filenames
                extra.push_back('\x75');
                extra.push_back('\x70');
                boost::uint16_t len = 5 + u8path.length();
                extra.push_back((char)(len & 0xff));
                extra.push_back((char)(len >> 8));
                auto crc = mz_crc32(0, (unsigned char*)path.c_str(), path.length());
                extra.push_back('\x01'); // version 1
                extra.append((char*)&crc, (char*)&crc + 4); // Little Endian
                extra.append(u8path);
            }
            return extra;
        }
        static std::string decode(std::string const& extra, std::string const& path = {}) {
            char const* p = extra.data();
            char const* e = p + extra.length();
            while (p + 4 < e) {
                boost::uint16_t len = ((boost::uint16_t)p[2]) | ((boost::uint16_t)p[3] << 8);
                if (p[0] == '\x75' && p[1] == '\x70' && len >= 5 && p + 4 + len < e && p[4] == '\x01') {
                    return std::string(p + 9, p + 4 + len);
                }
                else {
                    p += 4 + len;
                }
            }
            return decode_path(path.c_str());
        }
    };


    const char* _BBS_3MF_Importer::bbs_get_attribute_value_charptr(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        if ((attributes == nullptr) || (attributes_size == 0) || (attributes_size % 2 != 0) || (attribute_key == nullptr))
            return nullptr;

        for (unsigned int a = 0; a < attributes_size; a += 2) {
            if (::strcmp(attributes[a], attribute_key) == 0)
                return attributes[a + 1];
        }

        return nullptr;
    }

    std::string _BBS_3MF_Importer::bbs_get_attribute_value_string(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);
        return (text != nullptr) ? text : "";
    }

    bool _BBS_3MF_Importer::_handle_start_relationship(const char** attributes, unsigned int num_attributes)
    {
        std::string path = bbs_get_attribute_value_string(attributes, num_attributes, TARGET_ATTR);
        std::string type = bbs_get_attribute_value_string(attributes, num_attributes, RELS_TYPE_ATTR);
        if (boost::starts_with(type, "http://schemas.microsoft.com/3dmanufacturing/") && boost::ends_with(type, "3dmodel")) {
            if (m_start_part_path.empty()) m_start_part_path = path;
            else m_sub_model_paths.push_back(path);
        }
        else if (boost::starts_with(type, "http://schemas.openxmlformats.org/") && boost::ends_with(type, "thumbnail")) {
            m_thumbnail_path = path;
        }
        return true;
    }

    void _BBS_3MF_Importer::_stop_xml_parser(const std::string& msg)
    {
        //assert(!m_parse_error);
        //assert(m_parse_error_message.empty());
        //assert(m_xml_parser != nullptr);
        //m_parse_error = true;
        //m_parse_error_message = msg;
        XML_StopParser(m_xml_parser, false);
    }

    void _BBS_3MF_Importer::_handle_start_relationships_element(void* userData,const char* name, const char** attributes)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_start_relationships_element(name, attributes);
    }
    void _BBS_3MF_Importer::_handle_end_relationships_element(void* userData, const char* name)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_end_relationships_element(name);
    }

    void _BBS_3MF_Importer::_handle_start_relationships_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(RELATIONSHIP_TAG, name) == 0)
            res = _handle_start_relationship(attributes, num_attributes);

        m_curr_characters.clear();
        if (!res)
            _stop_xml_parser();
    }

    void _BBS_3MF_Importer::_handle_end_relationships_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (!res)
            _stop_xml_parser();
    }

    bool _BBS_3MF_Importer::_extract_from_archive(mz_zip_archive& archive, std::string const& path, std::function<bool(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)> extract, bool restore)
    {
        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
        mz_zip_archive_file_stat stat;
        std::string path2 = path;
        if (path2.front() == '/') path2 = path2.substr(1);
        // try utf8 encoding
        int index = mz_zip_reader_locate_file(&archive, path2.c_str(), nullptr, 0);
        if (index < 0) {
            // try native encoding
            std::string native_path = encode_path(path2.c_str());
            index = mz_zip_reader_locate_file(&archive, native_path.c_str(), nullptr, 0);
        }
        if (index < 0) {
            // try unicode path extra
            std::string extra(1024, 0);
            for (mz_uint i = 0; i < archive.m_total_files; ++i) {
                char* _data = const_cast<char*>(extra.data());
                size_t n = mz_zip_reader_get_extra(&archive, i, _data, extra.size());
                if (n > 0 && path2 == ZipUnicodePathExtraField::decode(extra.substr(0, n))) {
                    index = i;
                    break;
                }
            }
        }
        if (index < 0 || !mz_zip_reader_file_stat(&archive, index, &stat)) {
            if (restore) {
                std::vector<std::string> paths = { m_backup_path + path };
                if (!m_origin_file.empty()) paths.push_back(m_origin_file);
                for (auto& path2 : paths) {
                    bool result = false;
                    if (boost::filesystem::exists(path2)) {
                        mz_zip_archive archive;
                        mz_zip_zero_struct(&archive);
                        if (open_zip_reader(&archive, path2)) {
                            result = _extract_from_archive(archive, path, extract);
                            close_zip_reader(&archive);
                        }
                    }
                    if (result) return result;
                }
            }
            char error_buf[1024];
            ::sprintf(error_buf, "File %s not found from archive", path.c_str());
            //add_error(error_buf);
            return false;
        }
        try
        {
            if (!extract(archive, stat)) {
                return false;
            }
        }
        catch (const std::exception& e)
        {
            // ensure the zip archive is closed and rethrow the exception
            //add_error(e.what());
            return false;
        }
        return true;
    }

    void _BBS_3MF_Importer::_destroy_xml_parser()
    {
        if (m_xml_parser != nullptr) {
            XML_ParserFree(m_xml_parser);
            m_xml_parser = nullptr;
        }
    }

    bool _BBS_3MF_Importer::_extract_xml_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler)
    {
        if (stat.m_uncomp_size == 0) {
            //add_error("Found invalid size");
            return false;
        }

        _destroy_xml_parser();

        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            //add_error("Unable to create parser");
            return false;
        }

        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, start_handler, end_handler);
        XML_CharacterDataHandler handler;
        XML_SetCharacterDataHandler(m_xml_parser, _BBS_3MF_Importer::_handle_xml_characters);///

        void* parser_buffer = XML_GetBuffer(m_xml_parser, (int)stat.m_uncomp_size);
        if (parser_buffer == nullptr) {
            //add_error("Unable to create buffer");
            return false;
        }

        mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, parser_buffer, (size_t)stat.m_uncomp_size, 0);
        if (res == 0) {
            //add_error("Error while reading config data to buffer");
            return false;
        }

        if (!XML_ParseBuffer(m_xml_parser, (int)stat.m_uncomp_size, 1)) {
            char error_buf[1024];
            ::sprintf(error_buf, "Error (%s) while parsing xml file at line %d", XML_ErrorString(XML_GetErrorCode(m_xml_parser)), (int)XML_GetCurrentLineNumber(m_xml_parser));
            //add_error(error_buf);
            return false;
        }

        return true;
    }


    bool _BBS_3MF_Importer::_extract_xml_from_archive(mz_zip_archive& archive, const std::string& path, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler)
    {
        return _extract_from_archive(archive, path, [this, start_handler, end_handler](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
            return _extract_xml_from_archive(archive, stat, start_handler, end_handler);
            });
    }

    float _BBS_3MF_Importer::bbs_get_unit_factor(const std::string& unit)
    {
        const char* text = unit.c_str();

        if (::strcmp(text, "micron") == 0)
            return 0.001f;
        else if (::strcmp(text, "centimeter") == 0)
            return 10.0f;
        else if (::strcmp(text, "inch") == 0)
            return 25.4f;
        else if (::strcmp(text, "foot") == 0)
            return 304.8f;
        else if (::strcmp(text, "meter") == 0)
            return 1000.0f;
        else
            // default "millimeters" (see specification)
            return 1.0f;
    }

    float _BBS_3MF_Importer::bbs_get_attribute_value_float(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        float value = 0.0f;
        const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);

        //todo
        if (text != nullptr)
            value = std::atof(text);
            //fast_float::from_chars(text, text + strlen(text), value);
            return value;
    }


    int _BBS_3MF_Importer::bbs_get_attribute_value_int(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        int value = 0;
        const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);
        if (text != nullptr)
        {
            //boost::spirit::qi::parse(text, text + strlen(text), boost::spirit::qi::int_, value);
            //todo
            value = std::atoi(text);
        }
        //if (const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key); text != nullptr)

        return value;
    }

    bool _BBS_3MF_Importer::_handle_start_model(const char** attributes, unsigned int num_attributes)
    {
        m_unit_factor = bbs_get_unit_factor(bbs_get_attribute_value_string(attributes, num_attributes, UNIT_ATTR));

        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_resources(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::bbs_is_valid_object_type(const std::string& type)
    {
        // if the type is empty defaults to "model" (see specification)
        if (type.empty())
            return true;

        for (unsigned int i = 0; i < BBS_VALID_OBJECT_TYPES_COUNT; ++i) {
            if (::strcmp(type.c_str(), BBS_VALID_OBJECT_TYPES[i]) == 0)
                return true;
        }

        return false;
    }


    bool _BBS_3MF_Importer::_handle_start_object(const char** attributes, unsigned int num_attributes)
    {
        // reset current object data
        if (m_curr_object) {
            delete m_curr_object;
            m_curr_object = nullptr;
        }

        std::string object_type = bbs_get_attribute_value_string(attributes, num_attributes, TYPE_ATTR);

        if (bbs_is_valid_object_type(object_type)) {
            if (!m_curr_object) {
                m_curr_object = new CurrentObject();
                // create new object (it may be removed later if no instances are generated from it)
                /*m_curr_object->model_object_idx = (int)m_model->objects.size();
                m_curr_object.object = m_model->add_object();
                if (m_curr_object.object == nullptr) {
                    add_error("Unable to create object");
                    return false;
                }*/
            }

            m_curr_object->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
            m_curr_object->name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);

            m_curr_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            m_curr_object->pid = bbs_get_attribute_value_int(attributes, num_attributes, PID_ATTR);
        }

        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_color_group(const char** attributes, unsigned int num_attributes)
    {
        m_current_color_group = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_color(const char** attributes, unsigned int num_attributes)
    {
        std::string color = bbs_get_attribute_value_string(attributes, num_attributes, COLOR_ATTR);
        m_group_id_to_color[m_current_color_group] = color;
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_mesh(const char** attributes, unsigned int num_attributes)
    {
        // reset current geometry
        if (m_curr_object)
            m_curr_object->geometry.reset();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_vertices(const char** attributes, unsigned int num_attributes)
    {
        // reset current vertices
        if (m_curr_object)
            m_curr_object->geometry.vertices.clear();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_vertex(const char** attributes, unsigned int num_attributes)
    {
        // appends the vertex coordinates
        // missing values are set equal to ZERO
        if (m_curr_object)
            m_curr_object->geometry.vertices.emplace_back(
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, X_ATTR),
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Y_ATTR),
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Z_ATTR));
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_triangles(const char** attributes, unsigned int num_attributes)
    {
        // reset current triangles
        if (m_curr_object)
            m_curr_object->geometry.triangles.clear();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_triangle(const char** attributes, unsigned int num_attributes)
    {
        // we are ignoring the following attributes:
        // p1
        // p2
        // p3
        // pid
        // see specifications

        // appends the triangle's vertices indices
        // missing values are set equal to ZERO
        if (m_curr_object) {
            m_curr_object->geometry.triangles.emplace_back(
                bbs_get_attribute_value_int(attributes, num_attributes, V1_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V2_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V3_ATTR));

            m_curr_object->geometry.custom_supports.push_back(bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SUPPORTS_ATTR));
            m_curr_object->geometry.custom_seam.push_back(bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SEAM_ATTR));
            m_curr_object->geometry.mmu_segmentation.push_back(bbs_get_attribute_value_string(attributes, num_attributes, MMU_SEGMENTATION_ATTR));
            // BBS
            m_curr_object->geometry.face_properties.push_back(bbs_get_attribute_value_string(attributes, num_attributes, FACE_PROPERTY_ATTR));
        }
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_components(const char** attributes, unsigned int num_attributes)
    {
        // reset current components
        if (m_curr_object)
            m_curr_object->components.clear();
        return true;
    }

    bool _BBS_3MF_Importer::_handle_start_component(const char** attributes, unsigned int num_attributes)
    {
        //std::string path = bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR);
        //int         object_id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
        //Transform3d transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));

        ///*Id id = std::make_pair(m_sub_model_path, object_id);
        //IdToModelObjectMap::iterator object_item = m_objects.find(id);
        //if (object_item == m_objects.end()) {
        //    IdToAliasesMap::iterator alias_item = m_objects_aliases.find(id);
        //    if (alias_item == m_objects_aliases.end()) {
        //        add_error("Found component with invalid object id");
        //        return false;
        //    }
        //}*/

        //if (m_curr_object) {
        //    Id id = std::make_pair(m_sub_model_path.empty() ? path : m_sub_model_path, object_id);
        //    m_curr_object->components.emplace_back(id, transform);
        //}

        return true;
    }

    void _BBS_3MF_Importer::_handle_start_model_xml_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_start_model(attributes, num_attributes);
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_start_resources(attributes, num_attributes);
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_start_object(attributes, num_attributes);
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_start_color_group(attributes, num_attributes);
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_start_color(attributes, num_attributes);
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_start_mesh(attributes, num_attributes);
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_start_vertices(attributes, num_attributes);
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_start_vertex(attributes, num_attributes);
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_start_triangles(attributes, num_attributes);
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_start_triangle(attributes, num_attributes);
        //else if (::strcmp(COMPONENTS_TAG, name) == 0)
        //    res = _handle_start_components(attributes, num_attributes);
        //else if (::strcmp(COMPONENT_TAG, name) == 0)
        //    res = _handle_start_component(attributes, num_attributes);
        //else if (::strcmp(BUILD_TAG, name) == 0)
        //    res = _handle_start_build(attributes, num_attributes);
        //else if (::strcmp(ITEM_TAG, name) == 0)
        //    res = _handle_start_item(attributes, num_attributes);
        //else if (::strcmp(METADATA_TAG, name) == 0)
        //    res = _handle_start_metadata(attributes, num_attributes);

        if (!res)
            _stop_xml_parser();
    }

    bool _BBS_3MF_Importer::_handle_end_object()
    {
        if (!m_curr_object || (m_curr_object->id == -1)) {
            //add_error("Found invalid object");
            return false;
        }
        else {
            //if (m_is_bbl_3mf && boost::ends_with(m_curr_object->uuid, OBJECT_UUID_SUFFIX) && m_load_restore) {
            //    std::istringstream iss(m_curr_object->uuid);
            //    int backup_id;
            //    bool need_replace = false;
            //    if (iss >> std::hex >> backup_id) {
            //        need_replace = (m_curr_object->id != backup_id);
            //        m_curr_object->id = backup_id;
            //    }
            //    //if (need_replace)
            //    {
            //        for (int index = 0; index < m_curr_object->components.size(); index++)
            //        {
            //            int temp_id = (index + 1) << 16 | backup_id;
            //            Component& component = m_curr_object->components[index];
            //            std::string new_path = component.object_id.first;
            //            Id new_id = std::make_pair(new_path, temp_id);
            //            IdToCurrentObjectMap::iterator current_object = m_current_objects.find(component.object_id);
            //            if (current_object != m_current_objects.end()) {
            //                CurrentObject new_object;
            //                new_object.geometry = std::move(current_object->second.geometry);
            //                new_object.id = temp_id;
            //                new_object.model_object_idx = current_object->second.model_object_idx;
            //                new_object.name = current_object->second.name;
            //                new_object.uuid = current_object->second.uuid;

            //                m_current_objects.erase(current_object);
            //                m_current_objects.insert({ new_id, std::move(new_object) });
            //            }
            //            else {
            //                add_error("can not find object for component, id=" + std::to_string(component.object_id.second));
            //                delete m_curr_object;
            //                m_curr_object = nullptr;
            //                return false;
            //            }

            //            component.object_id.second = temp_id;
            //        }
            //    }
            //}
            Id id = std::make_pair(m_sub_model_path, m_curr_object->id);
            if (m_current_objects.find(id) == m_current_objects.end()) {
                m_current_objects.insert({ id, std::move(*m_curr_object) });
                delete m_curr_object;
                m_curr_object = nullptr;
            }
            else {
                //add_error("Found object with duplicate id");
                delete m_curr_object;
                m_curr_object = nullptr;
                return false;
            }
        }

        /*if (m_curr_object.object != nullptr) {
            if (m_curr_object.id != -1) {
                if (m_curr_object.geometry.empty()) {
                    // no geometry defined
                    // remove the object from the model
                    m_model->delete_object(m_curr_object.object);

                    if (m_curr_object.components.empty()) {
                        // no components defined -> invalid object, delete it
                        IdToModelObjectMap::iterator object_item = m_objects.find(id);
                        if (object_item != m_objects.end())
                            m_objects.erase(object_item);

                        IdToAliasesMap::iterator alias_item = m_objects_aliases.find(id);
                        if (alias_item != m_objects_aliases.end())
                            m_objects_aliases.erase(alias_item);
                    }
                    else
                        // adds components to aliases
                        m_objects_aliases.insert({ id, m_curr_object.components });
                }
                else {
                    // geometry defined, store it for later use
                    m_geometries.insert({ id, std::move(m_curr_object.geometry) });

                    // stores the object for later use
                    if (m_objects.find(id) == m_objects.end()) {
                        m_objects.insert({ id, m_curr_object.model_object_idx });
                        m_objects_aliases.insert({ id, { 1, Component(m_curr_object.id) } }); // aliases itself
                    }
                    else {
                        add_error("Found object with duplicate id");
                        return false;
                    }
                }
            }
            else {
                //sub objects
            }
        }*/

        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_resources()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_color_group()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_color()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_mesh()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_vertices()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_vertex()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_triangles()
    {
        // do nothing
        return true;
    }

    bool _BBS_3MF_Importer::_handle_end_triangle()
    {
        // do nothing
        return true;
    }

    void _BBS_3MF_Importer::_handle_end_model_xml_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_end_model();
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_end_resources();
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_end_object();
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_end_color_group();
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_end_color();
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_end_mesh();
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_end_vertices();
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_end_vertex();
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_end_triangles();
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_end_triangle();
        //else if (::strcmp(COMPONENTS_TAG, name) == 0)
        //    res = _handle_end_components();
        //else if (::strcmp(COMPONENT_TAG, name) == 0)
        //    res = _handle_end_component();
        //else if (::strcmp(BUILD_TAG, name) == 0)
        //    res = _handle_end_build();
        //else if (::strcmp(ITEM_TAG, name) == 0)
        //    res = _handle_end_item();
        //else if (::strcmp(METADATA_TAG, name) == 0)
        //    res = _handle_end_metadata();

        if (!res)
            _stop_xml_parser();
    }

    void _BBS_3MF_Importer::_handle_xml_characters(const XML_Char* s, int len)
    {
        m_curr_characters.append(s, len);
    }

    bool _BBS_3MF_Importer::_extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            //add_error("Found invalid size");
            return false;
        }

        _destroy_xml_parser();

        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            //add_error("Unable to create parser");
            return false;
        }

        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, _handle_start_model_xml_element, _handle_end_model_xml_element);
        XML_SetCharacterDataHandler(m_xml_parser, _handle_xml_characters);

        struct CallbackData
        {
            XML_Parser& parser;
            _BBS_3MF_Importer& importer;
            const mz_zip_archive_file_stat& stat;

            CallbackData(XML_Parser& parser, _BBS_3MF_Importer& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };

        CallbackData data(m_xml_parser,*this,stat);
        mz_bool res = 0;

        try
        {
            mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
                CallbackData* data = (CallbackData*)pOpaque;
                if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0) /*|| data->importer.parse_error()*/) {
                    //char error_buf[1024];
                    //::sprintf(error_buf, "Error (%s) while parsing '%s' at line %d", data->importer.parse_error_message(), data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                    throw;
                }
                return n;
            };
            void* opaque = &data;
            res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);
        }
        catch (std::exception& e)
        {
            // rethrow the exception
            //throw Slic3r::FileIOError(e.what());
        }

        if (res == 0) {
            //add_error("Error while extracting model data from zip archive");
            return false;
        }

        return true;
    }

    void _BBS_3MF_Importer::_handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_object_start_model_xml_element(name, attributes);
    }

    void _BBS_3MF_Importer::_handle_object_end_model_xml_element(void* userData, const char* name)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_object_end_model_xml_element(name);
    }

    void _BBS_3MF_Importer::_handle_object_xml_characters(void* userData, const XML_Char* s, int len)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_object_xml_characters(s, len);
    }

    void _BBS_3MF_Importer::_handle_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_start_model_xml_element(name, attributes);
    }

    void _BBS_3MF_Importer::_handle_end_model_xml_element(void* userData, const char* name)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_end_model_xml_element(name);
    }

    void _BBS_3MF_Importer::_handle_xml_characters(void* userData, const XML_Char* s, int len)
    {
        _BBS_3MF_Importer* importer = (_BBS_3MF_Importer*)userData;
        if (importer != nullptr)
            importer->_handle_xml_characters(s, len);
    }

    bool _BBS_3MF_Importer::_handle_end_model()
    {
        // BBS: Production Extension
        if (!m_sub_model_path.empty())
            return true;

        return true;
    }

    bool _BBS_3MF_Importer::load3MF(const std::string& filename)
    {
        m_xml_parser = nullptr;
        mz_zip_archive archive;
        mz_zip_zero_struct(&archive);

        m_current_objects.clear();

        if (!open_zip_reader(&archive, filename)) {
            //add_error("Unable to open the file" + filename);
            return false;
        }

        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);

        mz_zip_archive_file_stat stat;

        std::string m_name = boost::filesystem::path(filename).stem().string();

         // BBS: load relationships
        if (!_extract_xml_from_archive(archive, RELATIONSHIPS_FILE, _handle_start_relationships_element, _handle_end_relationships_element))
            return false;

        if (m_start_part_path.empty())
            return false;

        std::string sub_rels = m_start_part_path;

        sub_rels.insert(boost::find_last(sub_rels, "/").end() - sub_rels.begin(), "_rels/");
        sub_rels.append(".rels");
        if (sub_rels.front() == '/') sub_rels = sub_rels.substr(1);

        //check whether sub relation file is exist or not
        int sub_index = mz_zip_reader_locate_file(&archive, sub_rels.c_str(), nullptr, 0);


        //extract model files
        if (!_extract_from_archive(archive, m_start_part_path, [this](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
            return _extract_model_from_archive(archive, stat);
            })) {
            //add_error("Archive does not contain a valid model");
            //return false;
        }
        close_zip_writer(&archive);

        return true;
    }

    bool _BBS_3MF_Importer::load3MF(const std::string& filename
        , std::vector<trimesh::TriMesh*>& meshs
        , std::vector<std::vector<std::string>>& colors
        , std::vector<std::vector<std::string>>& seams
        , std::vector<std::vector<std::string>>& supports)
    {
        bool result = false;
        result = load3MF(filename);
        
        if (m_current_objects.empty())
        {
            return false;
        }

        for (auto iter = m_current_objects.begin() ;iter != m_current_objects.end();)
        {
            if (iter->second.geometry.vertices.empty() || iter->second.geometry.triangles.empty())
            {
                iter = m_current_objects.erase(iter);
            }
            else
                iter++;
        }

        int objectsNum = m_current_objects.size();
        meshs.reserve(objectsNum);
        colors.reserve(objectsNum);
        seams.reserve(objectsNum);
        supports.reserve(objectsNum);

        for (auto iter = m_current_objects.begin(); iter != m_current_objects.end();)
        {
            trimesh::TriMesh* mesh = new trimesh::TriMesh();
            mesh->vertices = iter->second.geometry.vertices;
            mesh->faces = iter->second.geometry.triangles;
            meshs.push_back(mesh);

            colors.push_back(iter->second.geometry.mmu_segmentation);
            seams.push_back(iter->second.geometry.custom_seam);
            supports.push_back(iter->second.geometry.custom_supports);
            iter++;
        }

        m_current_objects;
        return result;
    }

};