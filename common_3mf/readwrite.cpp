#include "readwrite.h"
#include "miniz_extension.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/string_file.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>
#include <unordered_set>

#include <sstream>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include "ccglobal/serial.h"
const std::string MODEL_FOLDER = "3D/";
const std::string MODEL_EXTENSION = ".model";
const std::string MODEL_FILE = "3D/3dmodel.model"; // << this is the only format of the string which works with CURA
const std::string MODEL_RELS_FILE = "3D/_rels/3dmodel.model.rels";

const std::string METADATA_DIR = "Metadata/";
const std::string ACCESOR_DIR = "accesories/";
const std::string GCODE_EXTENSION = ".gcode";
const std::string THUMBNAIL_EXTENSION = ".png";
const std::string CALIBRATION_INFO_EXTENSION = ".json";
const std::string CONTENT_TYPES_FILE = "[Content_Types].xml";
const std::string RELATIONSHIPS_FILE = "_rels/.rels";
const std::string THUMBNAIL_FILE = "Metadata/plate_1.png";
const std::string THUMBNAIL_FOR_PRINTER_FILE = "Metadata/bbl_thumbnail.png";
const std::string PRINTER_THUMBNAIL_SMALL_FILE = "/Auxiliaries/.thumbnails/thumbnail_small.png";
const std::string PRINTER_THUMBNAIL_MIDDLE_FILE = "/Auxiliaries/.thumbnails/thumbnail_middle.png";
const std::string _3MF_COVER_FILE = "/Auxiliaries/.thumbnails/thumbnail_3mf.png";

static constexpr const char* RELATIONSHIP_TAG = "Relationship";
static constexpr const char* TARGET_ATTR = "Target";
static constexpr const char* RELS_TYPE_ATTR = "Type";

const std::string PROJECT_CONFIG_FILE = "Metadata/project_settings.config";
const std::string MODEL_SETTINGS_FILE = "Metadata/model_settings.config";
const std::string CUSTOM_GCODE_PER_PRINT_Z_FILE = "Metadata/custom_gcode_per_layer.xml";
const std::string SLICE_INFO_CONFIG_FILE = "Metadata/slice_info.config";
const std::string LAYER_HEIGHTS_FILE = "Metadata/layer_heights_profile.txt";

static constexpr const char* SUBTYPE_ATTR = "subtype";
static constexpr const char* EXTRUDER_ATTR = "extruder";
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
static constexpr const char* PPATH_ATTR = "p:path";
static constexpr const char* OBJECTID_ATTR = "objectid";
static constexpr const char* TRANSFORM_ATTR = "transform";
static constexpr const char* PID_ATTR = "pid";
static constexpr const char* PUUID_ATTR = "p:uuid";
static constexpr const char* KEY_ATTR = "key";
static constexpr const char* VALUE_ATTR = "value";

static constexpr const char* OFFSET_ATTR = "offset";
static constexpr const char* PRINTABLE_ATTR = "printable";
static constexpr const char* INSTANCESCOUNT_ATTR = "instances_count";
static constexpr const char* CUSTOM_SUPPORTS_ATTR = "paint_supports";
static constexpr const char* CUSTOM_SEAM_ATTR = "paint_seam";
static constexpr const char* MMU_SEGMENTATION_ATTR = "paint_color";
static constexpr const char* FACE_PROPERTY_ATTR = "face_property";

static constexpr const char* OBJECT_UUID_SUFFIX = "-41cb-4c03-9d28-80fed5dfa1dc";

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
static constexpr const char* RELIEF_TEXT_TAG = "relief";
static constexpr const char* FILAMENT_TAG = "filament";
static constexpr const char* SLICE_WARNING_TAG = "warning";
static constexpr const char* WARNING_MSG_TAG = "msg";
static constexpr const char* FILAMENT_ID_TAG = "id";
static constexpr const char* FILAMENT_TYPE_TAG = "type";
static constexpr const char* FILAMENT_COLOR_TAG = "color";
static constexpr const char* FILAMENT_USED_M_TAG = "used_m";
static constexpr const char* FILAMENT_USED_G_TAG = "used_g";
static constexpr const char* PLATE_TAG = "plate";
static constexpr const char* MODEL_INSTANCE_TAG = "model_instance";
static constexpr const char* PLATERID_ATTR = "plater_id";
static constexpr const char* PLATER_NAME_ATTR = "plater_name";
static constexpr const char* LOCK_ATTR = "locked";
static constexpr const char* THUMBNAIL_FILE_ATTR = "thumbnail_file";
static constexpr const char* TOP_FILE_ATTR = "top_file";
static constexpr const char* PICK_FILE_ATTR = "pick_file";
static constexpr const char* PLATE_INFO_TAG = "plate_info";
static constexpr const char* ASSEMBLE_INFO_TAG = "assemble";
static constexpr const char* ASSEMBLE_ITEM_TAG = "assemble_item";
static constexpr const char* LAYER_TAG = "layer";
static constexpr const char* TOP_Z_TAG = "top_z";
static constexpr const char* MODE_TAG = "mode";
static constexpr const char* EXTRA_TAG = "extra";
static constexpr const char* CUSTOM_GCODE_TAG = "gcode";
static constexpr const char* EXTRUDER_TAG = "extruder";
static constexpr const char* PART_TAG = "part";

// Allow loading version 2 file as well.
const unsigned int VERSION_BBS_3MF_COMPATIBLE = 2;
const char* BBS_3MF_VERSION1 = "bamboo_slicer:Version3mf"; // definition of the metadata name saved into .model file
const char* BBS_3MF_VERSION = "BambuStudio:3mfVersion"; //compatible with prusa currently

const std::string BBS_FDM_SUPPORTS_PAINTING_VERSION = "BambuStudio:FdmSupportsPaintingVersion";
const std::string BBS_SEAM_PAINTING_VERSION = "BambuStudio:SeamPaintingVersion";
const std::string BBS_MM_PAINTING_VERSION = "BambuStudio:MmPaintingVersion";
const std::string BBL_MODEL_ID_TAG = "model_id";
const std::string BBL_MODEL_NAME_TAG = "Title";
const std::string BBL_ORIGIN_TAG = "Origin";
const std::string BBL_DESIGNER_TAG = "Designer";
const std::string BBL_DESIGNER_USER_ID_TAG = "DesignerUserId";
const std::string BBL_DESIGNER_COVER_FILE_TAG = "DesignerCover";
const std::string BBL_DESCRIPTION_TAG = "Description";
const std::string BBL_COPYRIGHT_TAG = "CopyRight";
const std::string BBL_COPYRIGHT_NORMATIVE_TAG = "Copyright";
const std::string BBL_LICENSE_TAG = "License";
const std::string BBL_REGION_TAG = "Region";
const std::string BBL_MODIFICATION_TAG = "ModificationDate";
const std::string BBL_CREATION_DATE_TAG = "CreationDate";
const std::string BBL_APPLICATION_TAG = "Application";
const std::string BBL_MAKERLAB_TAG = "MakerLab";
const std::string BBL_MAKERLAB_VERSION_TAG = "MakerLabVersion";

const std::string BBL_PROFILE_TITLE_TAG = "ProfileTitle";
const std::string BBL_PROFILE_COVER_TAG = "ProfileCover";
const std::string BBL_PROFILE_DESCRIPTION_TAG = "ProfileDescription";
const std::string BBL_PROFILE_USER_ID_TAG = "ProfileUserId";
const std::string BBL_PROFILE_USER_NAME_TAG = "ProfileUserName";

const unsigned int BBS_VALID_OBJECT_TYPES_COUNT = 2;
const char* BBS_VALID_OBJECT_TYPES[] =
{
    "model",
    "other"
};

const char* BBS_3MF_VERSION3 = "bamboo_slicer:Version3mf"; // definition of the metadata name saved into .model file
const char* BBS_3MF_VERSION2 = "BambuStudio:3mfVersion"; //compatible with prusa currently

namespace common_3mf
{
    std::string decode_path(const char* src)
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

    std::string encode_path(const char* src)
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

    static void reset_stream(std::stringstream& stream)
    {
        stream.str("");
        stream.clear();
        // https://en.cppreference.com/w/cpp/types/numeric_limits/max_digits10
        // Conversion of a floating-point value to text and back is exact as long as at least max_digits10 were used (9 for float, 17 for double).
        // It is guaranteed to produce the same floating-point value, even though the intermediate text representation is not exact.
        // The default value of std::stream precision is 6 digits only!
        stream << std::setprecision(std::numeric_limits<float>::max_digits10);
    }

    //FIXME this has potentially O(n^2) time complexity!
    std::string xml_escape(std::string text, bool is_marked = false)
    {
        std::string::size_type pos = 0;
        for (;;)
        {
            pos = text.find_first_of("\"\'&<>", pos);
            if (pos == std::string::npos)
                break;

            std::string replacement;
            switch (text[pos])
            {
            case '\"': replacement = "&quot;"; break;
            case '\'': replacement = "&apos;"; break;
            case '&':  replacement = "&amp;";  break;
            case '<':  replacement = is_marked ? "<" : "&lt;"; break;
            case '>':  replacement = is_marked ? ">" : "&gt;"; break;
            default: break;
            }

            text.replace(pos, 1, replacement);
            pos += replacement.size();
        }

        return text;
    }

    const char* bbs_get_attribute_value_charptr(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        if ((attributes == nullptr) || (attributes_size == 0) || (attributes_size % 2 != 0) || (attribute_key == nullptr))
            return nullptr;

        for (unsigned int a = 0; a < attributes_size; a += 2) {
            if (::strcmp(attributes[a], attribute_key) == 0)
                return attributes[a + 1];
        }

        return nullptr;
    }

    bool is_decimal_separator_point()
    {
        char str[5] = "";
        sprintf(str, "%.1f", 0.5f);
        return str[1] == '.';
    }

    std::string bbs_get_attribute_value_string(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);
        return (text != nullptr) ? text : "";
    }

    float bbs_get_unit_factor(const std::string& unit)
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

    float bbs_get_attribute_value_float(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        float value = 0.0f;
        const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);

        //todo
        if (text != nullptr)
            value = std::atof(text);
        //fast_float::from_chars(text, text + strlen(text), value);
        return value;
    }


    int bbs_get_attribute_value_int(const char** attributes, unsigned int attributes_size, const char* attribute_key)
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

    bool bbs_is_valid_object_type(const std::string& type)
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

    std::string xform_to_string(const trimesh::xform& xf)
    {
        std::stringstream stream;
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 3; ++r) {
                stream << xf(r, c);
                if (r != 2 || c != 3)
                    stream << " ";
            }
        }
        return stream.str();
    }

    trimesh::xform bbs_get_transform_from_3mf_specs_string(const std::string& mat_str)
    {
        // check: https://3mf.io/3d-manufacturing-format/ or https://github.com/3MFConsortium/spec_core/blob/master/3MF%20Core%20Specification.md
        // to see how matrices are stored inside 3mf according to specifications
        trimesh::xform ret = trimesh::xform::identity();

        if (mat_str.empty())
            // empty string means default identity matrix
            return ret;

        std::vector<std::string> mat_elements_str;
        boost::split(mat_elements_str, mat_str, boost::is_any_of(" "), boost::token_compress_on);

        unsigned int size = (unsigned int)mat_elements_str.size();
        if (size != 12)
            // invalid data, return identity matrix
            return ret;

        unsigned int i = 0;
        // matrices are stored into 3mf files as 4x3
        // we need to transpose them
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 3; ++r) {
                ret(r, c) = std::atof(mat_elements_str[i++].c_str());
            }
        }
        return ret;
    }

    static std::string xml_unescape(std::string s)
    {
        std::string ret;
        std::string::size_type i = 0;
        std::string::size_type pos = 0;
        while (i < s.size()) {
            std::string rep;
            if (s[i] == '&') {
                if (s.substr(i, 4) == "&lt;") {
                    ret += s.substr(pos, i - pos) + "<";
                    i += 4;
                    pos = i;
                }
                else if (s.substr(i, 4) == "&gt;") {
                    ret += s.substr(pos, i - pos) + ">";
                    i += 4;
                    pos = i;
                }
                else if (s.substr(i, 5) == "&amp;") {
                    ret += s.substr(pos, i - pos) + "&";
                    i += 5;
                    pos = i;
                }
                else {
                    ++i;
                }
            }
            else {
                ++i;
            }
        }

        ret += s.substr(pos);
        return ret;
    }

    ObjectImporter::ObjectImporter(Read3MFImpl* importer, std::string file_path, std::string obj_path)
    {
        top_importer = importer;
        object_path = obj_path;
        zip_path = file_path;
        m_archive = top_importer->m_archive;
    }

    bool ObjectImporter::_extract_object_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            top_importer->add_error("Found invalid size for " + object_path);
            return false;
        }

        object_xml_parser = XML_ParserCreate(nullptr);
        if (object_xml_parser == nullptr) {
            top_importer->add_error("Unable to create parser for " + object_path);
            return false;
        }

        XML_SetUserData(object_xml_parser, (void*)this);
        XML_SetElementHandler(object_xml_parser, ObjectImporter::_handle_object_start_model_xml_element, ObjectImporter::_handle_object_end_model_xml_element);
        XML_SetCharacterDataHandler(object_xml_parser, ObjectImporter::_handle_object_xml_characters);

        struct CallbackData
        {
            XML_Parser& parser;
            ObjectImporter& importer;
            const mz_zip_archive_file_stat& stat;

            CallbackData(XML_Parser& parser, ObjectImporter& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };

        CallbackData data(object_xml_parser, *this, stat);

        mz_bool res = 0;

        mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
            CallbackData* data = (CallbackData*)pOpaque;
            if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0) || data->importer.object_parse_error()) {
                char error_buf[1024];
                ::sprintf(error_buf, "Error (%s) while parsing '%s' at line %d", data->importer.object_parse_error_message(), data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                return false;
            }
            return n;
        };
        void* opaque = &data;
        res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);

        if (res == 0) {
            top_importer->add_error("Error while extracting model data from zip archive for " + object_path);
            return false;
        }

        return true;
    }

    bool ObjectImporter::extract_object_model()
    {
        mz_zip_archive archive;
                mz_zip_archive_file_stat stat;
                mz_zip_zero_struct(&archive);

                if (!open_zip_reader(&archive, zip_path)) {
                    top_importer->add_error("Unable to open the zipfile "+ zip_path);
                    return false;
                }

                if (!top_importer->_extract_from_archive(archive, object_path, [this] (mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
                    return _extract_object_from_archive(archive, stat);
                }, false)) {
                    std::string error_msg = std::string("Archive does not contain a valid model for ") + object_path;
                    top_importer->add_error(error_msg);

                    close_zip_reader(&archive);
                    return false;
                }

                close_zip_reader(&archive);

                if (obj_parse_error) {
                    //already add_error inside
                    //top_importer->add_error(object_parse_error_message());
                    //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Found error while extrace object %1%\n")%object_path;
                    return false;
                }
                return true;
    }

    bool ObjectImporter::_handle_object_start_model(const char** attributes, unsigned int num_attributes)
    {
        object_unit_factor = bbs_get_unit_factor(bbs_get_attribute_value_string(attributes, num_attributes, UNIT_ATTR));
        return true;
    }

    bool ObjectImporter::_handle_object_end_model()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_resources(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_end_resources()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_object(const char** attributes, unsigned int num_attributes)
    {
        std::string object_type = bbs_get_attribute_value_string(attributes, num_attributes, TYPE_ATTR);
        if (bbs_is_valid_object_type(object_type)) {
            std::shared_ptr<Object> object(new Object());
            current_object = object.get();

            current_object->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
            current_object->name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);

            current_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            current_object->type = object_type;
            //current_object->pid = bbs_get_attribute_value_int(attributes, num_attributes, PID_ATTR);

            object_list.insert(std::pair<int, std::shared_ptr<Object>>(object->id, object));
        }

        return true;
    }

    bool ObjectImporter::_handle_object_end_object()
    {
        current_object = nullptr;
        return true;
    }

    bool ObjectImporter::_handle_object_start_color_group(const char** attributes, unsigned int num_attributes)
    {
        object_current_color_group = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        return true;
    }

    bool ObjectImporter::_handle_object_end_color_group()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_color(const char** attributes, unsigned int num_attributes)
    {
        std::string color = bbs_get_attribute_value_string(attributes, num_attributes, COLOR_ATTR);
        object_group_id_to_color[object_current_color_group] = color;
        return true;
    }

    bool ObjectImporter::_handle_object_end_color()
    {
        // do nothing
        return true;
    }
    bool ObjectImporter::restore_backup_mesh(const char** attributes, unsigned int num_attributes)
    {
        
        std::string ppath = bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR);
        std::string cfileName = top_importer->m_backup_path +"/" + std::string(ppath);
		std::fstream in(cfileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			return false;
		}
       int have = 0;
	   ccglobal::cxndLoadT<int>(in, have);
       if (have == 1)
		{
			ccglobal::cxndLoadVectorT(in, current_object->geometry.vertices);
			ccglobal::cxndLoadVectorT(in, current_object->geometry.triangles);
		}
        in.close();
        return true;
    }
    bool ObjectImporter::restore_backup_color(const char** attributes, unsigned int num_attributes)
    {
        std::string ppath = bbs_get_attribute_value_string(attributes, num_attributes, "c:path");
        std::string cfileName = top_importer->m_backup_path +"/" + std::string(ppath);
		std::fstream in(cfileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			return false;
		}
        std::string line;
        int index = 0;
        while (std::getline(in, line)) { 
            if (!line.empty())
            {
                if(line[0] == ':')
                {
                    std::string numberStr = line.substr(1);
                    int num = std::stoi(numberStr);
                    index += num;
                }
                else
                {
                    current_object->geometry.mmu_segmentation.emplace_back(index, line);
                    index++;
                }
            }
            
        }
        in.close();
        return true;
    }
    bool ObjectImporter::restore_backup_seam(const char** attributes, unsigned int num_attributes)
    {
        std::string ppath = bbs_get_attribute_value_string(attributes, num_attributes, "seam:path");
        std::string cfileName = top_importer->m_backup_path +"/" + std::string(ppath);
		std::fstream in(cfileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			return false;
		}
        std::string line;
        int index = 0;
        while (std::getline(in, line)) { 
            if (!line.empty())
            {
                if (line[0] == ':')
                {
                    std::string numberStr = line.substr(1);
                    int num = std::stoi(numberStr);
                    index += num;
                }
                else
                {
                    current_object->geometry.custom_seam.emplace_back(index, line);
                    index++;
                }
            }
            
        }
        in.close();
        return true;
    }
    bool ObjectImporter::restore_backup_support(const char** attributes, unsigned int num_attributes)
    {
        std::string ppath = bbs_get_attribute_value_string(attributes, num_attributes, "support:path");
        std::string cfileName = top_importer->m_backup_path +"/" + std::string(ppath);
		std::fstream in(cfileName, std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			return false;
		}
        std::string line;
        int index = 0;
        while (std::getline(in, line)) { 
            if (!line.empty())
            {
                if (line[0] == ':')
                {
                    std::string numberStr = line.substr(1);
                    int num = std::stoi(numberStr);
                    index += num;
                }
                else
                {
                    current_object->geometry.custom_supports.emplace_back(index, line);
                    index++;
                }
            }
        }
        in.close();
        return true;
    }
    bool ObjectImporter::_handle_object_start_mesh(const char** attributes, unsigned int num_attributes)
    {
        // reset current geometry
        if (current_object)
            current_object->geometry.reset();
        //restore mesh
        restore_backup_mesh(attributes,num_attributes);
        //restore color
        restore_backup_color(attributes,num_attributes);
        //restore seam
        restore_backup_seam(attributes,num_attributes);
        //restore support
        restore_backup_support(attributes,num_attributes);
        return true;
    }

    bool ObjectImporter::_handle_object_end_mesh()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_vertices(const char** attributes, unsigned int num_attributes)
    {
        // reset current vertices
        if (current_object)
            current_object->geometry.vertices.clear();
        return true;
    }

    bool ObjectImporter::_handle_object_end_vertices()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_vertex(const char** attributes, unsigned int num_attributes)
    {
        // appends the vertex coordinates
        // missing values are set equal to ZERO
        if (current_object)
            current_object->geometry.vertices.emplace_back(
                object_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, X_ATTR),
                object_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Y_ATTR),
                object_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Z_ATTR));
        return true;
    }

    bool ObjectImporter::_handle_object_end_vertex()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_triangles(const char** attributes, unsigned int num_attributes)
    {
        // reset current triangles
        if (current_object)
            current_object->geometry.triangles.clear();
        return true;
    }

    bool ObjectImporter::_handle_object_end_triangles()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_triangle(const char** attributes, unsigned int num_attributes)
    {
        // we are ignoring the following attributes:
        // p1
        // p2
        // p3
        // pid
        // see specifications

        // appends the triangle's vertices indices
        // missing values are set equal to ZERO
        if (current_object) {
            current_object->geometry.triangles.emplace_back(
                bbs_get_attribute_value_int(attributes, num_attributes, V1_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V2_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V3_ATTR));

            int64_t currentIndex = current_object->geometry.triangles.size() - 1;

            std::string customInfo = bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SUPPORTS_ATTR);
            if (!customInfo.empty())
            {
                current_object->geometry.custom_supports.emplace_back(currentIndex, customInfo);
            }    

            customInfo = bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SEAM_ATTR);
            if (!customInfo.empty())
            {
                current_object->geometry.custom_seam.emplace_back(currentIndex, customInfo);
            }
                
            customInfo = bbs_get_attribute_value_string(attributes, num_attributes, MMU_SEGMENTATION_ATTR);
            if (!customInfo.empty())
            {
                current_object->geometry.mmu_segmentation.emplace_back(currentIndex, customInfo);
            }
                
            // BBS
            current_object->geometry.face_properties.push_back(bbs_get_attribute_value_string(attributes, num_attributes, FACE_PROPERTY_ATTR));
        }
        return true;
    }

    bool ObjectImporter::_handle_object_end_triangle()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_components(const char** attributes, unsigned int num_attributes)
    {
        // reset current components
        if (current_object)
            current_object->components.clear();
        return true;
    }

    bool ObjectImporter::_handle_object_end_components()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_component(const char** attributes, unsigned int num_attributes)
    {
        if (current_object)
        {
            Component component;
            component.id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
            component.transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));
            component.ppath = bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR);
            component.uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            current_object->components.push_back(component);
        }

        return true;
    }

    bool ObjectImporter::_handle_object_end_component()
    {
        // do nothing
        return true;
    }

    bool ObjectImporter::_handle_object_start_metadata(const char** attributes, unsigned int num_attributes)
    {
        obj_curr_metadata_name.clear();

        std::string name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);
        if (!name.empty()) {
            obj_curr_metadata_name = name;
        }

        return true;
    }

    bool ObjectImporter::_handle_object_end_metadata()
    {
        if ((obj_curr_metadata_name == BBS_3MF_VERSION2) || (obj_curr_metadata_name == BBS_3MF_VERSION3)) {
            is_bbl_3mf = true;
        }
        return true;
    }

    void ObjectImporter::_handle_object_start_model_xml_element(const char* name, const char** attributes)
    {
        if (object_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(object_xml_parser);

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_object_start_model(attributes, num_attributes);
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_object_start_resources(attributes, num_attributes);
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_object_start_object(attributes, num_attributes);
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_object_start_color_group(attributes, num_attributes);
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_object_start_color(attributes, num_attributes);
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_object_start_mesh(attributes, num_attributes);
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_object_start_vertices(attributes, num_attributes);
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_object_start_vertex(attributes, num_attributes);
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_object_start_triangles(attributes, num_attributes);
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_object_start_triangle(attributes, num_attributes);
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_object_start_components(attributes, num_attributes);
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_object_start_component(attributes, num_attributes);
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_object_start_metadata(attributes, num_attributes);
        else if (::strcmp(RELIEF_TEXT_TAG, name) == 0)
            res = _handle_object_start_metadata(attributes, num_attributes);

        if (!res)
            _stop_object_xml_parser();
    }

    void ObjectImporter::_handle_object_end_model_xml_element(const char* name)
    {
        if (object_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(MODEL_TAG, name) == 0)
            res = _handle_object_end_model();
        else if (::strcmp(RESOURCES_TAG, name) == 0)
            res = _handle_object_end_resources();
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_object_end_object();
        else if (::strcmp(COLOR_GROUP_TAG, name) == 0)
            res = _handle_object_end_color_group();
        else if (::strcmp(COLOR_TAG, name) == 0)
            res = _handle_object_end_color();
        else if (::strcmp(MESH_TAG, name) == 0)
            res = _handle_object_end_mesh();
        else if (::strcmp(VERTICES_TAG, name) == 0)
            res = _handle_object_end_vertices();
        else if (::strcmp(VERTEX_TAG, name) == 0)
            res = _handle_object_end_vertex();
        else if (::strcmp(TRIANGLES_TAG, name) == 0)
            res = _handle_object_end_triangles();
        else if (::strcmp(TRIANGLE_TAG, name) == 0)
            res = _handle_object_end_triangle();
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_object_end_components();
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_object_end_component();
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_object_end_metadata();
        else if (::strcmp(RELIEF_TEXT_TAG, name) == 0)
            res = _handle_object_end_metadata();

        if (!res)
            _stop_object_xml_parser();
    }

    void ObjectImporter::_handle_object_xml_characters(const XML_Char* s, int len)
    {
        obj_curr_characters.append(s, len);
    }

    void XMLCALL ObjectImporter::_handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        ObjectImporter* importer = (ObjectImporter*)userData;
        if (importer != nullptr)
            importer->_handle_object_start_model_xml_element(name, attributes);
    }

    void XMLCALL ObjectImporter::_handle_object_end_model_xml_element(void* userData, const char* name)
    {
        ObjectImporter* importer = (ObjectImporter*)userData;
        if (importer != nullptr)
            importer->_handle_object_end_model_xml_element(name);
    }

    void XMLCALL ObjectImporter::_handle_object_xml_characters(void* userData, const XML_Char* s, int len)
    {
        ObjectImporter* importer = (ObjectImporter*)userData;
        if (importer != nullptr)
            importer->_handle_object_xml_characters(s, len);
    }

	Read3MFImpl::Read3MFImpl(const std::string& file_name)
		: m_file_name(file_name)
		, m_open_success(false)
        , m_xml_parser(nullptr)
        , m_unit_factor(1.0f)
	{
        mz_zip_zero_struct(&m_archive);
		if (open_zip_reader(&m_archive, m_file_name))
			m_open_success = true;
	}

	Read3MFImpl::~Read3MFImpl()
	{
		if(m_open_success)
			close_zip_reader(&m_archive);
	}

	bool Read3MFImpl::read_all_3mf(Scene3MF& scene, ccglobal::Tracer* tracer)
	{
        if (!m_open_success)
            return false;

		mz_uint num_entries = mz_zip_reader_get_num_files(&m_archive);
        (void)num_entries;
		mz_zip_archive_file_stat stat;

		if (!_extract_xml_from_archive(m_archive, RELATIONSHIPS_FILE, _handle_start_relationships_element, _handle_end_relationships_element))
			return false;

        if (m_3d_model_path.empty())
            return false;

        std::string sub_rels = m_3d_model_path;
        sub_rels.insert(boost::find_last(sub_rels, "/").end() - sub_rels.begin(), "_rels/");
        sub_rels.append(".rels");
        if (sub_rels.front() == '/') sub_rels = sub_rels.substr(1);

        //check whether sub relation file is exist or not
        int sub_index = mz_zip_reader_locate_file(&m_archive, sub_rels.c_str(), nullptr, 0);

        if (sub_index != -1)
        {
            _extract_xml_from_archive(m_archive, sub_rels, _handle_start_relationships_element, _handle_end_relationships_element);
            for (auto path : m_sub_model_paths) {
                ObjectImporter *object_importer = new ObjectImporter(this, m_file_name, path);
                m_object_importers.push_back(object_importer);
            }
            bool object_load_result = true;
            boost::mutex _mutex;
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0, m_object_importers.size()),
                [this, &_mutex, &object_load_result](const tbb::blocked_range<size_t>& importer_range) {
                    
                    for (size_t object_index = importer_range.begin(); object_index < importer_range.end(); ++ object_index) {
                        bool result = m_object_importers[object_index]->extract_object_model();
                        {
                            boost::unique_lock<boost::mutex> l(_mutex);
                            object_load_result &= result;
                        }
                    }
                }
            );
         
            
            if (!object_load_result) {
                add_error("loading sub-objects error\n");
                return false;
            }
                //merge these objects into one
            for (auto obj_importer : m_object_importers) {
                for (const IdToObjectMap::value_type&  obj : obj_importer->object_list)
                    m_objects_list.insert({ std::move(obj.first), std::move(obj.second) });
                for (auto group_color : obj_importer->object_group_id_to_color)
                    m_group_id_to_color.insert(std::move(group_color));

                delete obj_importer;
            }
            m_object_importers.clear();
        }

        //extract model files
        if (!_extract_from_archive(m_archive, m_3d_model_path, [this](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
            return _extract_model_from_archive(archive, stat);
            })) {
            add_error("Archive does not contain a valid model");
            return false;
        }
        
        for (mz_uint i = 0; i < num_entries; ++i) {
            if (mz_zip_reader_file_stat(&m_archive, i, &stat)) {
                std::string name(stat.m_filename);
                std::replace(name.begin(), name.end(), '\\', '/');
        
                if (name == PROJECT_CONFIG_FILE) {
                    _extract_project_config_from_archive(m_archive, stat);
                    scene.project_settings = PROJECT_CONFIG_FILE;
                }
                else if (name == MODEL_SETTINGS_FILE)
                {
                    _extract_model_settings_from_archive(m_archive, stat);
                }
                else if (name == CUSTOM_GCODE_PER_PRINT_Z_FILE)
                {
                    _extract_custom_gcode_per_print_z_from_archive(m_archive, stat);
                }
                else if (name == LAYER_HEIGHTS_FILE) 
                {
                    _extract_layer_heights_config_from_archive(m_archive, stat);
                }
            }
        }

        build_scene(scene);
		return true;
	}

    bool Read3MFImpl::extract_file(const std::string& zip_name, const std::string& dest_file_name)
    {
        //extract file
        if (!_extract_from_archive(m_archive, zip_name, [this, dest_file_name](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
            return _extract_copy_from_archive(archive, stat, dest_file_name);
            })) {
            add_error("Archive does not contain a valid model");
            return false;
        }
        return true;
    }

    bool Read3MFImpl::_extract_from_archive(mz_zip_archive& archive, std::string const& path, std::function<bool(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)> extract, bool restore)
    {
        if (path.empty())
            return false;

        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
        (void)num_entries;
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
            //restore
        }
        if (!extract(archive, stat)) {
            return false;
        }
        return true;
    }

    bool Read3MFImpl::_extract_xml_from_archive(mz_zip_archive& archive, const std::string& path, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler)
    {
        return _extract_from_archive(archive, path, [this, start_handler, end_handler](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
            return _extract_xml_from_archive(archive, stat, start_handler, end_handler);
            });
    }

    bool Read3MFImpl::build_scene(Scene3MF& scene)
    {
        if (m_objects_list.empty())
        {
            return false;
        }

        int group_count = 0;
        for (std::shared_ptr<BuildItem> build_item : m_build_items)
        {
            int id = build_item->id;
            std::map<int, std::shared_ptr<Object>>::iterator it = m_objects_list.find(id);
            if (it != m_objects_list.end() && it->second->id >= 1)
                ++group_count;
        }

        if (group_count > 0)
        {
            scene.group_counts = group_count;
            scene.groups.resize(group_count);

            int index = 0;
            int model_count = 0;
            for (std::shared_ptr<BuildItem> build_item : m_build_items)
            {
                int id = build_item->id;
                std::map<int, std::shared_ptr<Object>>::iterator it = m_objects_list.find(id);
                if (it != m_objects_list.end() && it->second->id >= 1)
                {
                    std::shared_ptr<Object> object = it->second;
                    Group3MF& group_3mf = scene.groups.at(index);
                    group_3mf.id = object->id;
                    group_3mf.name = object->name;
                    group_3mf.xf = build_item->xf;
                    if(m_group_id_to_layer_heights.find(object->id) != m_group_id_to_layer_heights.end())
                    {
                        group_3mf.layerHeightProfile = m_group_id_to_layer_heights[object->id];
                    }

                    int component_num = (int)object->components.size();
                    if (component_num > 0)
                    {
                        group_3mf.models.resize(component_num);
                        for (int j = 0; j < component_num; ++j)
                        {
                            const Component& comp = object->components.at(j);
                            Model3MF& model_3mf = group_3mf.models.at(j);
                            model_3mf.xf = comp.transform;

                            std::map<int, std::shared_ptr<Object>>::iterator iit = m_objects_list.find(comp.id);
                            if(iit != m_objects_list.end() && comp.id >= 0)
                            {
                                if ((iit->second->geometry.triangles.size() == 0) && (iit->second->components.size() == 1))
                                {
                                    std::map<int, std::shared_ptr<Object>>::iterator other = m_objects_list.find(iit->second->components.at(0).id);
                                    if (other != m_objects_list.end() && other->second->geometry.triangles.size() > 0)
                                    {
                                        const Component& other_comp = iit->second->components.at(0);
                                        model_3mf.xf = model_3mf.xf * other_comp.transform;
                                        model_3mf.id = other_comp.id;
                                        
                                        size_t triangleSize = other->second->geometry.triangles.size();
                                        if (!other->second->geometry.mmu_segmentation.empty())
                                        {
                                            model_3mf.colors.resize(triangleSize);
                                            for (int i = 0; i < other->second->geometry.mmu_segmentation.size(); i++)
                                            {
                                                int curIdx = other->second->geometry.mmu_segmentation[i].first;
                                                if (curIdx >= 0 && curIdx < model_3mf.colors.size())
                                                {
                                                    model_3mf.colors[curIdx] = other->second->geometry.mmu_segmentation[i].second;
                                                }
                                                
                                            }
                                        }
                                        
                                        if (!other->second->geometry.custom_seam.empty())
                                        {
                                            model_3mf.seams.resize(triangleSize);
                                            for (int i = 0; i < other->second->geometry.custom_seam.size(); i++)
                                            {
                                                int curIdx = other->second->geometry.custom_seam[i].first;
                                                if (curIdx >= 0 && curIdx < model_3mf.seams.size())
                                                {
                                                    model_3mf.seams[curIdx] = other->second->geometry.custom_seam[i].second;
                                                }
                                            }
                                        }

                                        if (!other->second->geometry.custom_supports.empty())
                                        {
                                            model_3mf.supports.resize(triangleSize);
                                            for (int i = 0; i < other->second->geometry.custom_supports.size(); i++)
                                            {
                                                int curIdx = other->second->geometry.custom_supports[i].first;
                                                if (curIdx >= 0 && curIdx < model_3mf.supports.size())
                                                {
                                                    model_3mf.supports[curIdx] = other->second->geometry.custom_supports[i].second;
                                                }
                                            }
                                        }

                                        TriMeshPtr mesh(new trimesh::TriMesh());
                                        mesh->vertices = other->second->geometry.vertices;
                                        mesh->faces = other->second->geometry.triangles;
                                        model_3mf.mesh = mesh;
                                    }
                                }
                                else {
                                    model_3mf.id = comp.id;

                                    size_t triangleSize = iit->second->geometry.triangles.size();
                                    if (!iit->second->geometry.mmu_segmentation.empty())
                                    {
                                        model_3mf.colors.resize(triangleSize);
                                        for (int i = 0; i < iit->second->geometry.mmu_segmentation.size(); i++)
                                        {
                                            int curIdx = iit->second->geometry.mmu_segmentation[i].first;
                                            if (curIdx >= 0 && curIdx < model_3mf.colors.size())
                                            {
                                                model_3mf.colors[curIdx] = iit->second->geometry.mmu_segmentation[i].second;
                                            }

                                        }
                                    }

                                    if (!iit->second->geometry.custom_seam.empty())
                                    {
                                        model_3mf.seams.resize(triangleSize);
                                        for (int i = 0; i < iit->second->geometry.custom_seam.size(); i++)
                                        {
                                            int curIdx = iit->second->geometry.custom_seam[i].first;
                                            if (curIdx >= 0 && curIdx < model_3mf.seams.size())
                                            {
                                                model_3mf.seams[curIdx] = iit->second->geometry.custom_seam[i].second;
                                            }
                                        }
                                    }

                                    if (!iit->second->geometry.custom_supports.empty())
                                    {
                                        model_3mf.supports.resize(triangleSize);
                                        for (int i = 0; i < iit->second->geometry.custom_supports.size(); i++)
                                        {
                                            int curIdx = iit->second->geometry.custom_supports[i].first;
                                            if (curIdx >= 0 && curIdx < model_3mf.supports.size())
                                            {
                                                model_3mf.supports[curIdx] = iit->second->geometry.custom_supports[i].second;
                                            }
                                        }
                                    }

                                    TriMeshPtr mesh(new trimesh::TriMesh());
                                    mesh->vertices = iit->second->geometry.vertices;
                                    mesh->faces = iit->second->geometry.triangles;
                                    model_3mf.mesh = mesh;
                                }
                            }
                        }
                    }

                    if (object->geometry.vertices.size() > 0 && object->geometry.triangles.size() > 0)
                    {
                        group_3mf.models.emplace_back(Model3MF());
                        Model3MF& model_3mf = group_3mf.models.back();
                        model_3mf.xf = trimesh::xform::identity();
                        model_3mf.id = object->id;

                        size_t triangleSize = object->geometry.triangles.size();
                        if (!object->geometry.mmu_segmentation.empty())
                        {
                            model_3mf.colors.resize(triangleSize);
                            for (int i = 0; i < object->geometry.mmu_segmentation.size(); i++)
                            {
                                int curIdx = object->geometry.mmu_segmentation[i].first;
                                if (curIdx >= 0 && curIdx < model_3mf.colors.size())
                                {
                                    model_3mf.colors[curIdx] = object->geometry.mmu_segmentation[i].second;
                                }

                            }
                        }

                        if (!object->geometry.custom_seam.empty())
                        {
                            model_3mf.seams.resize(triangleSize);
                            for (int i = 0; i < object->geometry.custom_seam.size(); i++)
                            {
                                int curIdx = object->geometry.custom_seam[i].first;
                                if (curIdx >= 0 && curIdx < model_3mf.seams.size())
                                {
                                    model_3mf.seams[curIdx] = object->geometry.custom_seam[i].second;
                                }
                            }
                        }

                        if (!object->geometry.custom_supports.empty())
                        {
                            model_3mf.supports.resize(triangleSize);
                            for (int i = 0; i < object->geometry.custom_supports.size(); i++)
                            {
                                int curIdx = object->geometry.custom_supports[i].first;
                                if (curIdx >= 0 && curIdx < model_3mf.supports.size())
                                {
                                    model_3mf.supports[curIdx] = object->geometry.custom_supports[i].second;
                                }
                            }
                        }

                        TriMeshPtr mesh(new trimesh::TriMesh());
                        mesh->vertices = object->geometry.vertices;
                        mesh->faces = object->geometry.triangles;
                        model_3mf.mesh = mesh;
                    }

                    model_count += (int)group_3mf.models.size();
                    index++;
                }
            }

            scene.model_counts = model_count;
        }
        
        for(std::shared_ptr<Plate3MF> plate : m_plates_infos)
            scene.plates.push_back(*plate);
        for (std::shared_ptr<ObjectSetting> object : m_object_settings)
        {
            for (Group3MF& group : scene.groups)
            {
                if (group.id == object->id)
                {
                    group.extruder = object->extruder;
                    group.name = object->name;
                    group.settings = object->settings;
                    int part_count = (int)object->parts.size();
                    int comp_count = (int)group.models.size();
                    if (part_count == comp_count)
                    {
                        for (int i = 0; i < part_count; ++i)
                        {
                            Model3MF& model = group.models.at(i);
                            std::shared_ptr<PartSetting> part = object->parts.at(i);
                            if (part->id == model.id)
                            {
                                model.name = part->name;
                                model.extruder = part->extruder;
                                model.subtype = part->subtype;
                                if (model.extruder <= 0)
                                    model.extruder = group.extruder;
                                model.settings = part->settings;
                                model.relief_description = part->relief_description;
                            }
                        }
                    }
                    break;
                }
            }
        }

        for (Plate3MF& plate : scene.plates)
        {
            std::map<int, std::vector<PlateCustomGCode>>::iterator it = m_plate_id_2_gcode_infos.find(plate.plateId);
            if (it != m_plate_id_2_gcode_infos.end())
                plate.gcodes = it->second;
        }

        // If instances contain a single volume, the volume offset should be 0,0,0
        // This equals to say that instance world position and volume world position should match
        // Correct all instances/volumes for which this does not hold
        // fork from orcaslicer
        for (int group_id = 0; group_id < int(scene.groups.size()); ++group_id) {
            Group3MF& group = scene.groups[group_id];
            if (group.models.size() == 1) {
                Model3MF& model = group.models.front();
                const trimesh::xform& group_trafo = group.xf;
                const trimesh::xform& model_trafo = model.xf;
                trimesh::xform xf = group_trafo * model_trafo;

                group.xf[12] = xf[12];
                group.xf[13] = xf[13];
                group.xf[14] = xf[14];
                model.xf[12] = 0.0;
                model.xf[13] = 0.0;
                model.xf[14] = 0.0;
            }
        }

        scene.model_metadata = m_model_metadata;

        return true;
    }

    bool Read3MFImpl::_extract_xml_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler)
    {
        if (stat.m_uncomp_size == 0) {
            add_error("Found invalid size");
            return false;
        }
        
        _destroy_xml_parser();
        
        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            add_error("Unable to create parser");
            return false;
        }
        
        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, start_handler, end_handler);
        XML_SetCharacterDataHandler(m_xml_parser, _handle_xml_characters);
        
        void* parser_buffer = XML_GetBuffer(m_xml_parser, (int)stat.m_uncomp_size);
        if (parser_buffer == nullptr) {
            add_error("Unable to create buffer");
            return false;
        }
        
        mz_bool res = mz_zip_reader_extract_file_to_mem(&archive, stat.m_filename, parser_buffer, (size_t)stat.m_uncomp_size, 0);
        if (res == 0) {
            add_error("Error while reading config data to buffer");
            return false;
        }
        
        if (!XML_ParseBuffer(m_xml_parser, (int)stat.m_uncomp_size, 1)) {
            char error_buf[1024];
            ::snprintf(error_buf, 1024, "Error (%s) while parsing xml file at line %d", XML_ErrorString(XML_GetErrorCode(m_xml_parser)), (int)XML_GetCurrentLineNumber(m_xml_parser));
            add_error(error_buf);
            return false;
        }

        return true;
    }

    void Read3MFImpl::_stop_xml_parser(const std::string& msg)
    {
        //assert(!m_parse_error);
        //assert(m_parse_error_message.empty());
        //assert(m_xml_parser != nullptr);
        //m_parse_error = true;
        //m_parse_error_message = msg;
        XML_StopParser(m_xml_parser, false);
    }

    void Read3MFImpl::_destroy_xml_parser()
    {
        if (m_xml_parser != nullptr) {
            XML_ParserFree(m_xml_parser);
            m_xml_parser = nullptr;
        }
    }

    bool Read3MFImpl::_extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            add_error("Found invalid size");
            return false;
        }
        
        _destroy_xml_parser();
        
        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            add_error("Unable to create parser");
            return false;
        }
        
        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, _handle_start_model_xml_element, _handle_end_model_xml_element);
        XML_SetCharacterDataHandler(m_xml_parser, _handle_xml_characters);
        
        struct CallbackData
        {
            XML_Parser& parser;
            Read3MFImpl& importer;
            const mz_zip_archive_file_stat& stat;
        
            CallbackData(XML_Parser& parser, Read3MFImpl& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };
        
        CallbackData data(m_xml_parser, *this, stat);
        
        mz_bool res = 0;
        
        mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
            CallbackData* data = (CallbackData*)pOpaque;
            if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0)) {
                char error_buf[1024];
                ::snprintf(error_buf, 1024, "Error while parsing '%s' at line %d",  data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                return false;
            }
            return n;
        };
        void* opaque = &data;
        res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);
        
        if (res == 0) {
            add_error("Error while extracting model data from zip archive");
            return false;
        }

        return true;
    }

    bool Read3MFImpl::_handle_end_plate_gcode()
    {
        m_current_customs = nullptr;
        return true;
    }

    bool Read3MFImpl::_extract_copy_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, const std::string& dest_file_name)
    {
        if (stat.m_uncomp_size > 0) {
            std::string dest_file = dest_file_name;
            std::string dest_zip_file = encode_path(dest_file.c_str());
            mz_bool res = mz_zip_reader_extract_to_file(&archive, stat.m_file_index, dest_zip_file.c_str(), 0);
            if (res == 0) {
                return true;
            }
			
            return false;
        }

        return false;
    }

    bool Read3MFImpl::_handle_start_plateinfo_gcode(const char** attributes, unsigned int num_attributes)
    {
        if (!m_start_new_parse_plate_gcode)
            return false;

        int id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        std::pair<std::map<int, std::vector<PlateCustomGCode>>::iterator, bool> result 
            = m_plate_id_2_gcode_infos.insert(std::pair<int, std::vector<PlateCustomGCode>>(id, std::vector<PlateCustomGCode>()));
        if (result.second)
        {
            m_current_customs = &(result.first->second);
        }

        return true;
    }

    bool Read3MFImpl::_handle_end_plateinfo_gcode()
    {
        return true;
    }

    bool Read3MFImpl::_handle_start_mode_gcode(const char** attributes, unsigned int num_attributes)
    {
        return true;
    }

    bool Read3MFImpl::_handle_end_mode_gcode()
    {
        return true;
    }

    bool Read3MFImpl::_handle_start_layer_gcode(const char** attributes, unsigned int num_attributes)
    {
        if (m_current_customs)
        {
            common_3mf::PlateCustomGCode custom;
            custom.print_z = bbs_get_attribute_value_float(attributes, num_attributes, TOP_Z_TAG);
            custom.type = (common_3mf::PlateType)bbs_get_attribute_value_int(attributes, num_attributes, FILAMENT_TYPE_TAG);
            custom.extra = bbs_get_attribute_value_string(attributes, num_attributes, EXTRA_TAG);
            custom.color = bbs_get_attribute_value_string(attributes, num_attributes, FILAMENT_COLOR_TAG);
            custom.extruder = bbs_get_attribute_value_int(attributes, num_attributes, EXTRUDER_TAG);
            m_current_customs->push_back(custom);
        }
        return true;
    }

    bool Read3MFImpl::_handle_end_layer_gcode()
    {
        return true;
    }

    void Read3MFImpl::_extract_project_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size > 0) {
            
        }
    }
    void Read3MFImpl::_extract_model_settings_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            add_error("Found invalid size");
            return;
        }

        _destroy_xml_parser();

        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            add_error("Unable to create parser");
            return;
        }

        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, _handle_start_config_xml_element, _handle_end_config_xml_element);
        XML_SetCharacterDataHandler(m_xml_parser, _handle_xml_characters);

        struct CallbackData
        {
            XML_Parser& parser;
            Read3MFImpl& importer;
            const mz_zip_archive_file_stat& stat;

            CallbackData(XML_Parser& parser, Read3MFImpl& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };

        CallbackData data(m_xml_parser, *this, stat);

        mz_bool res = 0;

        mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
            CallbackData* data = (CallbackData*)pOpaque;
            if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0)) {
                char error_buf[1024];
                ::snprintf(error_buf, 1024, "Error while parsing '%s' at line %d", data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                return false;
            }
            return n;
        };
        void* opaque = &data;
        res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);

        if (res == 0) {
            add_error("Error while extracting model data from zip archive");
            return;
        }

    }

    void Read3MFImpl::_extract_custom_gcode_per_print_z_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
        if (stat.m_uncomp_size == 0) {
            add_error("Found invalid size");
            return;
        }

        _destroy_xml_parser();

        m_xml_parser = XML_ParserCreate(nullptr);
        if (m_xml_parser == nullptr) {
            add_error("Unable to create parser");
            return;
        }

        XML_SetUserData(m_xml_parser, (void*)this);
        XML_SetElementHandler(m_xml_parser, _handle_start_custom_gcode_xml_element, _handle_end_custom_gcode_xml_element);
        XML_SetCharacterDataHandler(m_xml_parser, _handle_xml_characters);

        struct CallbackData
        {
            XML_Parser& parser;
            Read3MFImpl& importer;
            const mz_zip_archive_file_stat& stat;

            CallbackData(XML_Parser& parser, Read3MFImpl& importer, const mz_zip_archive_file_stat& stat) : parser(parser), importer(importer), stat(stat) {}
        };

        CallbackData data(m_xml_parser, *this, stat);

        mz_bool res = 0;

        mz_file_write_func callback = [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
            CallbackData* data = (CallbackData*)pOpaque;
            if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0)) {
                char error_buf[1024];
                ::snprintf(error_buf, 1024, "Error while parsing '%s' at line %d", data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                return false;
            }
            return n;
        };
        void* opaque = &data;
        res = mz_zip_reader_extract_to_callback(&archive, stat.m_file_index, callback, opaque, 0);

        if (res == 0) {
            add_error("Error while extracting model data from zip archive");
            return;
        }
    }

    bool Read3MFImpl::_handle_start_model(const char** attributes, unsigned int num_attributes)
    {
        m_unit_factor = bbs_get_unit_factor(bbs_get_attribute_value_string(attributes, num_attributes, UNIT_ATTR));
        return true;
    }

    bool Read3MFImpl::_handle_start_resources(const char** attributes, unsigned int num_attributes)
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_start_object(const char** attributes, unsigned int num_attributes)
    {
        std::string object_type = bbs_get_attribute_value_string(attributes, num_attributes, TYPE_ATTR);
        if (bbs_is_valid_object_type(object_type)) {
            std::shared_ptr<Object> object(new Object());
            m_current_object = object.get();

            m_current_object->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
            m_current_object->name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);

            m_current_object->uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            m_current_object->type = object_type;
            //current_object->pid = bbs_get_attribute_value_int(attributes, num_attributes, PID_ATTR);

            m_objects_list.insert(std::pair<int, std::shared_ptr<Object>>(object->id, object));
        }

        return true;
    }

    bool Read3MFImpl::_handle_start_color_group(const char** attributes, unsigned int num_attributes)
    {
        m_current_color_group = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        return true;
    }

    bool Read3MFImpl::_handle_start_color(const char** attributes, unsigned int num_attributes)
    {
        std::string color = bbs_get_attribute_value_string(attributes, num_attributes, COLOR_ATTR);
        m_group_id_to_color[m_current_color_group] = color;
        return true;
    }

    bool Read3MFImpl::_handle_start_mesh(const char** attributes, unsigned int num_attributes)
    {
        // reset current geometry
        if (m_current_object)
            m_current_object->geometry.reset();
        return true;
    }

    bool Read3MFImpl::_handle_start_vertices(const char** attributes, unsigned int num_attributes)
    {
        // reset current vertices
        if (m_current_object)
            m_current_object->geometry.vertices.clear();
        return true;
    }

    bool Read3MFImpl::_handle_start_vertex(const char** attributes, unsigned int num_attributes)
    {
        // appends the vertex coordinates
        // missing values are set equal to ZERO
        if (m_current_object)
            m_current_object->geometry.vertices.emplace_back(
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, X_ATTR),
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Y_ATTR),
                m_unit_factor * bbs_get_attribute_value_float(attributes, num_attributes, Z_ATTR));
        return true;
    }

    bool Read3MFImpl::_handle_start_triangles(const char** attributes, unsigned int num_attributes)
    {
        // reset current triangles
        if (m_current_object)
            m_current_object->geometry.triangles.clear();
        return true;
    }

    bool Read3MFImpl::_handle_start_triangle(const char** attributes, unsigned int num_attributes)
    {
        // we are ignoring the following attributes:
        // p1
        // p2
        // p3
        // pid
        // see specifications

        // appends the triangle's vertices indices
        // missing values are set equal to ZERO
        if (m_current_object) {
            m_current_object->geometry.triangles.emplace_back(
                bbs_get_attribute_value_int(attributes, num_attributes, V1_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V2_ATTR),
                bbs_get_attribute_value_int(attributes, num_attributes, V3_ATTR));

            int64_t curIndex = m_current_object->geometry.triangles.size() - 1;
            std::string customInfo = bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SUPPORTS_ATTR);
            if(!customInfo.empty())
                m_current_object->geometry.custom_supports.emplace_back(curIndex,customInfo);

            customInfo = bbs_get_attribute_value_string(attributes, num_attributes, CUSTOM_SEAM_ATTR);
            if(!customInfo.empty())
                m_current_object->geometry.custom_seam.emplace_back(curIndex, customInfo);

            customInfo = bbs_get_attribute_value_string(attributes, num_attributes, MMU_SEGMENTATION_ATTR);
            if(!customInfo.empty())
                m_current_object->geometry.mmu_segmentation.emplace_back(curIndex, customInfo);
            // BBS
            m_current_object->geometry.face_properties.push_back(bbs_get_attribute_value_string(attributes, num_attributes, FACE_PROPERTY_ATTR));
        }
        return true;
    }

    bool Read3MFImpl::_handle_start_metadata(const char** attributes, unsigned int num_attributes)
    {
        m_curr_characters.clear();

        std::string name = bbs_get_attribute_value_string(attributes, num_attributes, NAME_ATTR);
        if (!name.empty()) {
            m_curr_metadata_name = name;
        }

        return true;
    }

    bool Read3MFImpl::_handle_end_metadata()
    {
        if (!m_curr_metadata_name.empty()) {
            m_model_metadata.emplace_back(std::make_pair(m_curr_metadata_name, xml_unescape(m_curr_characters)));
        }

        return true;
    }

    bool Read3MFImpl::_handle_start_components(const char** attributes, unsigned int num_attributes)
    {
        // reset current components
        if (m_current_object)
            m_current_object->components.clear();
        return true;
    }

    void Read3MFImpl::_handle_start_relationships_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;
        
        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);
        
        if (::strcmp(RELATIONSHIP_TAG, name) == 0)
            res = _handle_start_relationship(attributes, num_attributes);
        
        //m_curr_characters.clear();
        if (!res)
            _stop_xml_parser();
    }

    void Read3MFImpl::_handle_end_relationships_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;
        
        bool res = true;
        
        if (!res)
            _stop_xml_parser();
    }

    bool Read3MFImpl::_handle_start_relationship(const char** attributes, unsigned int num_attributes)
    {
        std::string path = bbs_get_attribute_value_string(attributes, num_attributes, TARGET_ATTR);
        std::string type = bbs_get_attribute_value_string(attributes, num_attributes, RELS_TYPE_ATTR);
        if (boost::starts_with(type, "http://schemas.microsoft.com/3dmanufacturing/") && boost::ends_with(type, "3dmodel")) {
            if (m_3d_model_path.empty())
                m_3d_model_path = path;
            else 
                m_sub_model_paths.push_back(path);
        }
        else if (boost::starts_with(type, "http://schemas.openxmlformats.org/") && boost::ends_with(type, "thumbnail")) {
            m_thumbnail_path = path;
        }
        return true;
    }

    bool Read3MFImpl::_handle_start_component(const char** attributes, unsigned int num_attributes)
    {
        if (m_current_object)
        {
            Component component;
            component.id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
            component.transform = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));
            component.ppath = bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR);
            component.uuid = bbs_get_attribute_value_string(attributes, num_attributes, PUUID_ATTR);
            m_current_object->components.push_back(component);
        }
        return true;
    }

    bool bbs_get_attribute_value_bool(const char** attributes, unsigned int attributes_size, const char* attribute_key)
    {
        const char* text = bbs_get_attribute_value_charptr(attributes, attributes_size, attribute_key);
        return (text != nullptr) ? (bool)::atoi(text) : true;
    }

    bool Read3MFImpl::_handle_start_item(const char** attributes, unsigned int num_attributes)
    {
        std::shared_ptr<BuildItem> build_item(new BuildItem());
        m_current_build_item = build_item.get();
        m_build_items.push_back(build_item);

        // we are ignoring the following attributes
        // thumbnail
        // partnumber
        // pid
        // pindex
        // see specifications

        m_current_build_item->id = bbs_get_attribute_value_int(attributes, num_attributes, OBJECTID_ATTR);
        //std::string path = bbs_get_attribute_value_string(attributes, num_attributes, PPATH_ATTR);
        m_current_build_item->xf = bbs_get_transform_from_3mf_specs_string(bbs_get_attribute_value_string(attributes, num_attributes, TRANSFORM_ATTR));
        int printable = bbs_get_attribute_value_bool(attributes, num_attributes, PRINTABLE_ATTR);
        (void)printable;

        return true;
    }

    bool Read3MFImpl::_handle_end_object()
    {
        m_current_object = nullptr;
        return true;
    }

    bool Read3MFImpl::_handle_end_resources()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_color_group()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_color()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_mesh()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_vertices()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_vertex()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_triangles()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_triangle()
    {
        // do nothing
        return true;
    }

    bool Read3MFImpl::_handle_end_item()
    {
        m_current_build_item = nullptr;
        return true;
    }

    bool Read3MFImpl::_handle_start_plate(const char** attributes, unsigned int num_attributes)
    {
        std::shared_ptr<Plate3MF> plate(new Plate3MF());
        m_current_plate_info = plate.get();
        m_plates_infos.push_back(plate);
        return true;
    }

    bool Read3MFImpl::_handle_start_model_instance(const char** attributes, unsigned int num_attributes)
    {
        //do nothing
        return true;
    }

    bool Read3MFImpl::_handle_start_plate_metadata(const char** attributes, unsigned int num_attributes)
    {
        std::string key = bbs_get_attribute_value_string(attributes, num_attributes, KEY_ATTR);
        if (m_current_plate_info)
        {
            assert(!m_current_object_settings);
            assert(!m_current_part_settings);

            if (key == PLATERID_ATTR) {
                m_current_plate_info->plateId = bbs_get_attribute_value_int(attributes, num_attributes, VALUE_ATTR);
            }
            else if (key == PLATER_NAME_ATTR) {
                m_current_plate_info->name = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
            }
            else if (key == LOCK_ATTR){
                m_current_plate_info->locked = bbs_get_attribute_value_bool(attributes, num_attributes, VALUE_ATTR);;
            }

            m_current_plate_info->settings[key] = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
        }
        else {
            if (m_current_part_settings)
            {
                assert(m_current_object_settings);
                if (key == NAME_ATTR) {
                    m_current_part_settings->name = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
                }
                else if (key == EXTRUDER_ATTR) {
                    m_current_part_settings->extruder = bbs_get_attribute_value_int(attributes, num_attributes, VALUE_ATTR);;
                }

                m_current_part_settings->settings[key] = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
            }
            else if (m_current_object_settings) {
                if (key == NAME_ATTR) {
                    m_current_object_settings->name = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
                }
                else if (key == EXTRUDER_ATTR) {
                    m_current_object_settings->extruder = bbs_get_attribute_value_int(attributes, num_attributes, VALUE_ATTR);
                }

                m_current_object_settings->settings[key] = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
            }
        }

        return true;
    }

    bool Read3MFImpl::_handle_start_relief(const char** attributes, unsigned int num_attributes)
    {
        std::string key = bbs_get_attribute_value_string(attributes, num_attributes, KEY_ATTR);
        if (key == "description")
        {
            m_current_part_settings->relief_description = bbs_get_attribute_value_string(attributes, num_attributes, VALUE_ATTR);
        }

        return true;
    }

    bool Read3MFImpl::_handle_start_object_setting(const char** attributes, unsigned int num_attributes)
    {
        std::shared_ptr<ObjectSetting> os(new ObjectSetting());
        m_current_object_settings = os.get();
        m_object_settings.push_back(os);

        m_current_object_settings->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
        return true;
    }

    bool Read3MFImpl::_handle_end_object_setting()
    {
        m_current_object_settings = nullptr;
        return true;
    }

    bool Read3MFImpl::_handle_start_part_setting(const char** attributes, unsigned int num_attributes)
    {
        if (m_current_object_settings)
        {
            std::shared_ptr<PartSetting> ps(new PartSetting());
            m_current_part_settings = ps.get();
            m_current_object_settings->parts.push_back(ps);

            m_current_part_settings->id = bbs_get_attribute_value_int(attributes, num_attributes, ID_ATTR);
            m_current_part_settings->subtype = bbs_get_attribute_value_string(attributes, num_attributes, SUBTYPE_ATTR);
            return true;
        }
        return false;
    }

    bool Read3MFImpl::_handle_end_part_setting()
    {
        m_current_part_settings = nullptr;
        return true;
    }

    bool Read3MFImpl::_handle_end_plate()
    {
        m_current_plate_info = nullptr;
        return true;
    }

    bool Read3MFImpl::_handle_end_model_instance()
    {
        return true;
    }

    // parse the model_settings.config
    void Read3MFImpl::_handle_start_config_xml_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(PLATE_TAG, name) == 0)
            res = _handle_start_plate(attributes, num_attributes);
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_start_object_setting(attributes, num_attributes);
        else if (::strcmp(MODEL_INSTANCE_TAG, name) == 0)
            res = _handle_start_model_instance(attributes, num_attributes);
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_start_plate_metadata(attributes, num_attributes);
        else if (::strcmp(RELIEF_TEXT_TAG, name) == 0)
            res = _handle_start_relief(attributes, num_attributes);
        else if (::strcmp(PART_TAG, name) == 0)
            res = _handle_start_part_setting(attributes, num_attributes);
        if (!res)
            _stop_xml_parser();
    }

    void Read3MFImpl::_handle_end_config_xml_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(PLATE_TAG, name) == 0)
            res = _handle_end_plate();
        else if (::strcmp(MODEL_INSTANCE_TAG, name) == 0)
            res = _handle_end_model_instance();
        else if (::strcmp(OBJECT_TAG, name) == 0)
            res = _handle_end_object_setting();
        else if (::strcmp(PART_TAG, name) == 0)
            res = _handle_end_part_setting();

        if (!res)
            _stop_xml_parser();
    }

    void Read3MFImpl::_handle_start_custom_gcode_xml_element(const char* name, const char** attributes)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;
        unsigned int num_attributes = (unsigned int)XML_GetSpecifiedAttributeCount(m_xml_parser);

        if (::strcmp(PLATE_TAG, name) == 0)
            res = _handle_start_plate_gcode(attributes, num_attributes);
        else if (::strcmp(PLATE_INFO_TAG, name) == 0)
            res = _handle_start_plateinfo_gcode(attributes, num_attributes);
        else if (::strcmp(LAYER_TAG, name) == 0)
            res = _handle_start_layer_gcode(attributes, num_attributes);
        else if (::strcmp(MODE_TAG, name) == 0)
            res = _handle_start_mode_gcode(attributes, num_attributes);

        if (!res)
            _stop_xml_parser();
    }

    void Read3MFImpl::_handle_end_custom_gcode_xml_element(const char* name)
    {
        if (m_xml_parser == nullptr)
            return;

        bool res = true;

        if (::strcmp(PLATE_TAG, name) == 0)
            res = _handle_end_plate_gcode();
        else if (::strcmp(PLATE_INFO_TAG, name) == 0)
            res = _handle_end_plateinfo_gcode();
        else if (::strcmp(LAYER_TAG, name) == 0)
            res = _handle_end_layer_gcode();
        else if (::strcmp(MODE_TAG, name) == 0)
            res = _handle_end_mode_gcode();

        if (!res)
            _stop_xml_parser();
    }

    bool Read3MFImpl::_handle_start_plate_gcode(const char** attributes, unsigned int num_attributes)
    {
        m_start_new_parse_plate_gcode = true;
        return true;
    }

    void Read3MFImpl::_handle_start_model_xml_element(const char* name, const char** attributes)
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
        else if (::strcmp(COMPONENTS_TAG, name) == 0)
            res = _handle_start_components(attributes, num_attributes);
        else if (::strcmp(COMPONENT_TAG, name) == 0)
            res = _handle_start_component(attributes, num_attributes);
        //else if (::strcmp(BUILD_TAG, name) == 0)
        //    res = _handle_start_build(attributes, num_attributes);
        else if (::strcmp(ITEM_TAG, name) == 0)
            res = _handle_start_item(attributes, num_attributes);
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_start_metadata(attributes, num_attributes);
        else if (::strcmp(RELIEF_TEXT_TAG, name) == 0)
            res = _handle_start_metadata(attributes, num_attributes);
        
        if (!res)
            _stop_xml_parser();
    }

    void Read3MFImpl::_handle_end_model_xml_element(const char* name)
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
        else if (::strcmp(ITEM_TAG, name) == 0)
            res = _handle_end_item();
        else if (::strcmp(METADATA_TAG, name) == 0)
            res = _handle_end_metadata();
        
        if (!res)
            _stop_xml_parser();
    }

    void Read3MFImpl::_handle_xml_characters(const XML_Char* s, int len)
    {
        m_curr_characters.append(s, len);
    }

    void Read3MFImpl::_handle_start_relationships_element(void* userData, const char* name, const char** attributes)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_start_relationships_element(name, attributes);
    }
    void Read3MFImpl::_handle_end_relationships_element(void* userData, const char* name)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_end_relationships_element(name);
    }

    void Read3MFImpl::_handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_object_start_model_xml_element(name, attributes);
    }

    void Read3MFImpl::_handle_object_end_model_xml_element(void* userData, const char* name)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_object_end_model_xml_element(name);
    }

    void Read3MFImpl::_handle_object_xml_characters(void* userData, const XML_Char* s, int len)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_object_xml_characters(s, len);
    }

    void Read3MFImpl::_handle_start_model_xml_element(void* userData, const char* name, const char** attributes)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_start_model_xml_element(name, attributes);
    }

    void Read3MFImpl::_handle_end_model_xml_element(void* userData, const char* name)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_end_model_xml_element(name);
    }

    bool Read3MFImpl::_handle_end_model()
    {
        // BBS: Production Extension
        if (!m_sub_model_path.empty())
            return true;

        return true;
    }

    void Read3MFImpl::_handle_xml_characters(void* userData, const XML_Char* s, int len)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_xml_characters(s, len);
    }

    void Read3MFImpl::_handle_start_config_xml_element(void* userData, const char* name, const char** attributes)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_start_config_xml_element(name, attributes);
    }

    void Read3MFImpl::_handle_end_config_xml_element(void* userData, const char* name)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_end_config_xml_element(name);
    }

    void Read3MFImpl::_handle_start_custom_gcode_xml_element(void* userData, const char* name, const char** attributes)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_start_custom_gcode_xml_element(name, attributes);
    }

    void Read3MFImpl::_handle_end_custom_gcode_xml_element(void* userData, const char* name)
    {
        Read3MFImpl* impl = (Read3MFImpl*)userData;
        if (impl != nullptr)
            impl->_handle_end_custom_gcode_xml_element(name);
    }


    Write3MFImpl::Write3MFImpl(const std::string& file_name)
        : m_file_name(file_name)
        , m_open_success(false)
    {
        mz_zip_zero_struct(&m_archive);

        if (open_zip_writer(&m_archive, m_file_name))
            m_open_success = true;
    }

    Write3MFImpl::~Write3MFImpl()
    {
        if (m_open_success)
        {
            mz_zip_writer_finalize_archive(&m_archive);
            close_zip_reader(&m_archive);
        }
    }

    bool Write3MFImpl::write_scene_2_3mf(const Scene3MF& scene, ccglobal::Tracer* tracer,bool is_tmp_project)
    {
        m_is_tmp_project = is_tmp_project;
        if (!m_open_success)
            return false;

        if (!_add_content_file())
            return false;
        if(tracer)
            tracer->progress(0.1f);

        if (!_add_relations_file())
            return false;

        if (tracer)
            tracer->progress(0.4f);

        if (!_add_scene_content(scene))
            return false;

        if (tracer)
            tracer->progress(0.6f);

        if (!_add_model_setting_file(scene))
            return false;

        if (!_add_custom_gcode_file(scene))
            return false;

        if (tracer)
            tracer->progress(0.8f);

        if (!_add_slice_info_config_file(scene))
            return false;

        if (!mz_zip_writer_add_file(&m_archive, PROJECT_CONFIG_FILE.c_str(), 
            scene.project_settings.c_str(), NULL, 0, MZ_DEFAULT_LEVEL))
        {
            return false;
        }

        // write group adaptive layer height profile to archive
        if(!_add_layer_height_profile_to_archive(m_archive, scene))
            return false;

        if (tracer)
            tracer->progress(0.9f);

        if(is_tmp_project)
        {
            return true;
        }
        auto add_file_func = [this](std::vector<std::string> files) {
            for (auto const& file : files)
            {
                int pos = file.find_last_of("/");
                if (pos == -1)
                {
                    continue;
                }
                std::string config_file = "Metadata/" + file.substr(pos+1);
                mz_zip_writer_add_file(&m_archive, config_file.c_str(),
                    file.c_str(), NULL, 0, MZ_DEFAULT_LEVEL);
            }
        };
        add_file_func(scene.plate_thumbnails);
        add_file_func(scene.plate_gcodes);

        return true;
    }

    bool Write3MFImpl::_add_content_file()
    {
        std::stringstream stream;
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n";
        stream << " <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n";
        stream << " <Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n";
        stream << " <Default Extension=\"png\" ContentType=\"image/png\"/>\n";
        stream << " <Default Extension=\"gcode\" ContentType=\"text/x.gcode\"/>\n";
        stream << "</Types>";

        std::string out = stream.str();

        if (!mz_zip_writer_add_mem(&m_archive, CONTENT_TYPES_FILE.c_str(),
            (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            add_error("Unable to add content types file to archive");
            return false;
        }

        return true;
    }
    bool Write3MFImpl::_add_model_relations_file(const std::vector<Group3MF>& groups)
    {
        std::stringstream stream;
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n";
        int index =1;
        for (const Group3MF& group : groups)
        {
             auto  ppath =  "3D/Objects/" + group.name + "_" + std::to_string(group.id) + ".model";
            stream << " <Relationship Target=\"/" << ppath << "\" Id=\"rel-"<<index<<"\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n";
            index++;
        }
        stream << "</Relationships>";

        std::string out = stream.str();
        if (!mz_zip_writer_add_mem(&m_archive, MODEL_RELS_FILE.c_str(),
            (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            add_error("Unable to add relations file to archive");
            return false;
        }

        return true;
    }
    bool Write3MFImpl::_add_relations_file()
    {
        std::stringstream stream;
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n";
        stream << " <Relationship Target=\"/" << MODEL_FILE << "\" Id=\"rel-1\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n";
        stream << "</Relationships>";

        std::string out = stream.str();
        if (!mz_zip_writer_add_mem(&m_archive, RELATIONSHIPS_FILE.c_str(),
            (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            add_error("Unable to add relations file to archive");
            return false;
        }

        return true;
    }
    void Write3MFImpl::set_backup_path(std::string const& path)
    {
        m_backup_path = path;
    }
    bool Write3MFImpl::_add_model_to_file(mz_zip_archive &archive,const Group3MF& group)
    {
        auto  ppath =  "3D/Objects/" + group.name + "_" + std::to_string(group.id) + ".model";
        bool m_zip64 = true;
        mz_zip_writer_staged_context context;
        if (!mz_zip_writer_add_staged_open(&archive, &context, ppath.c_str(),
            m_zip64 ?
            // Maximum expected and allowed 3MF file size is 16GiB.
            // This switches the ZIP file to a 64bit mode, which adds a tiny bit of overhead to file records.
            (uint64_t(1) << 30) * 16 :
            // Maximum expected 3MF file size is 4GB-1. This is a workaround for interoperability with Windows 10 3D model fixing API, see
            // GH issue #6193.
            (uint64_t(1) << 32) - 1,
            //#if WRITE_ZIP_LANGUAGE_ENCODING
            nullptr, nullptr, 0, MZ_DEFAULT_LEVEL, nullptr, 0, nullptr, 0)) 
        {
            add_error("Unable to add model file to archive");
            return false;
        }

        {
            std::stringstream stream;
            //reset_stream(stream);
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            stream << "<" << MODEL_TAG << " unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" xmlns:slic3rpe=\"http://schemas.slic3r.org/3mf/2017/06\"";
            stream << ">\n";
            stream << " <" << RESOURCES_TAG << ">\n";
            std::string buf = stream.str();
            if (!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) {
                add_error("Unable to add model file to archive");
                return false;
            }
            reset_stream(stream);   
            for (const Model3MF& model : group.models)
            {
                if(m_is_tmp_project)
                {
                    if (!_add_model_to_stream2(context, model))
                    {
                        mz_zip_writer_add_staged_finish(&context);
                        return false;
                    }
                }else{
                     if (!_add_model_to_stream(context, model))
                     {
                        mz_zip_writer_add_staged_finish(&context);
                        return false;
                     }
                }
                
                
            }
             stream << "</" << RESOURCES_TAG << ">\n";
            stream << "</" << MODEL_TAG << ">\n";
            buf = stream.str();

            if ((!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) ){
                    //add_error("Unable to add model file to archive");
                    //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add model file to archive\n");
                    return false;
            }
            if(!mz_zip_writer_add_staged_finish(&context)) 
            {
                return false;
            }
            return true;
        }
    }
    bool Write3MFImpl::_add_scene_content(const Scene3MF& scene)
    {
        bool m_zip64 = true;
        mz_zip_writer_staged_context context;
        
        for (const Group3MF& group : scene.groups)
        {
            if(m_is_tmp_project && !group.skip)
            {
                //mz_zip_archive archive;
                //mz_zip_zero_struct(&archive);

                //if (open_zip_writer(&archive, m_backup_path+"/3D/Objects/"+group.name+"_"+std::to_string(group.id)+".model"))
                //{
                    _add_model_to_file(m_archive,group);
                
                //}
                //mz_zip_writer_finalize_archive(&archive);
                //close_zip_writer(&archive);
    
               
            }else{
                _add_model_to_file(m_archive,group);
            }
              
        }
        _add_model_relations_file(scene.groups);
        
        if (!mz_zip_writer_add_staged_open(&m_archive, &context, MODEL_FILE.c_str(),
            m_zip64 ?
            // Maximum expected and allowed 3MF file size is 16GiB.
            // This switches the ZIP file to a 64bit mode, which adds a tiny bit of overhead to file records.
            (uint64_t(1) << 30) * 16 :
            // Maximum expected 3MF file size is 4GB-1. This is a workaround for interoperability with Windows 10 3D model fixing API, see
            // GH issue #6193.
            (uint64_t(1) << 32) - 1,
            //#if WRITE_ZIP_LANGUAGE_ENCODING
            nullptr, nullptr, 0, MZ_DEFAULT_LEVEL, nullptr, 0, nullptr, 0)) 
        {
            add_error("Unable to add model file to archive");
            return false;
        }

        {
            std::stringstream stream;
            reset_stream(stream);
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            stream << "<" << MODEL_TAG << " unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" xmlns:slic3rpe=\"http://schemas.slic3r.org/3mf/2017/06\"";
            stream << ">\n";

            // remember to use metadata_item_map to store metadata info
            bool version_recorded = false;
            std::vector<std::pair<std::string, std::string>> metadata_item_map;
            metadata_item_map.emplace_back(std::make_pair(BBL_APPLICATION_TAG, scene.application));
            for (auto& metadata : scene.model_metadata)
            {
                if (metadata.first == BBS_3MF_VERSION2 || metadata.first == BBS_3MF_VERSION3)
                {
                    metadata_item_map.emplace_back(std::make_pair(metadata.first, metadata.second));
                    version_recorded = true;
                    break;
                }
            }
            if (!version_recorded)
            {
                metadata_item_map.emplace_back(std::make_pair(BBS_3MF_VERSION3, std::to_string(1)));
            }
            const static std::unordered_set<std::string> s_metadata_key =
            {
                boost::algorithm::to_lower_copy(BBL_COPYRIGHT_TAG), boost::algorithm::to_lower_copy(BBL_COPYRIGHT_NORMATIVE_TAG),
                boost::algorithm::to_lower_copy(BBL_CREATION_DATE_TAG),
                boost::algorithm::to_lower_copy(BBL_DESCRIPTION_TAG),
                boost::algorithm::to_lower_copy(BBL_DESIGNER_TAG),
                boost::algorithm::to_lower_copy(BBL_LICENSE_TAG),
                boost::algorithm::to_lower_copy(BBL_ORIGIN_TAG),
                boost::algorithm::to_lower_copy(BBL_MODEL_NAME_TAG)
            };
            for (auto & metadata : scene.model_metadata)
            {
                if(s_metadata_key.count(boost::algorithm::to_lower_copy(metadata.first)) <= 0)
                    continue;
                metadata_item_map.emplace_back(std::make_pair(metadata.first, metadata.second));
            }

            // store metadata info
            for (auto item : metadata_item_map) {
                stream << " <" << METADATA_TAG << " name=\"" << item.first << "\">"
                    << xml_escape(item.second) << "</" << METADATA_TAG << ">\n";
            }

            stream << " <" << RESOURCES_TAG << ">\n";
            std::string buf = stream.str();
            if (!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) {
                add_error("Unable to add model file to archive");
                return false;
            }

            reset_stream(stream);
            for (const Group3MF& group : scene.groups)
            {
                if (!_add_group_to_stream(context, group))
                {
                    mz_zip_writer_add_staged_finish(&context);
                    return false;
                }

                //for (const Model3MF& model : group.models)
                //{
                //    if (!_add_model_to_stream(context, model))
                //    {
                //        mz_zip_writer_add_staged_finish(&context);
                //        return false;
                //    }
                //}
            }
            stream << " </" << RESOURCES_TAG << ">\n";

            stream << " <" << BUILD_TAG << ">\n";
            for (const Group3MF& group : scene.groups)
            {
                stream << "  <" << ITEM_TAG << " objectid=\"" << group.id;
                stream << "\" " << TRANSFORM_ATTR << "=\"" << xform_to_string(group.xf);
                stream << "\"/>\n";
            }
            stream << " </" << BUILD_TAG << ">\n";

            stream << "</" << MODEL_TAG << ">\n";

            buf = stream.str();

            if ((!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) ||
                !mz_zip_writer_add_staged_finish(&context)) {
                //add_error("Unable to add model file to archive");
                //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add model file to archive\n");
                return false;
            }
        }
        return true;
    }
    bool Write3MFImpl::_add_group_to_stream(mz_zip_writer_staged_context & context, const Group3MF & group)
    {
        std::stringstream stream;
        reset_stream(stream);

        stream << "  <" << OBJECT_TAG << " id=\"" << group.id << "\" type=\"model\">\n";
        stream << "   <" << COMPONENTS_TAG << ">\n";
        for (const Model3MF& model : group.models)
        {
            auto  ppath =  "/3D/Objects/" + group.name + "_" + std::to_string(group.id) + ".model";
            stream << "    <" << COMPONENT_TAG <<  " p:path=\"" << xml_escape(ppath) << "\" objectid=\"" << model.id;
            stream << "\" " << TRANSFORM_ATTR << "=\"" << xform_to_string(model.xf);
            stream << "\"/>\n";
        }
        stream << "   </" << COMPONENTS_TAG << ">\n";
        stream << "  </" << OBJECT_TAG << ">\n";

        if (!mz_zip_writer_add_staged_data(&context, stream.str().data(), stream.str().size())) {
            add_error("Error during writing or compression");
            return false;
        }
        return true;
    }
    bool Write3MFImpl::_add_model_to_stream2(mz_zip_writer_staged_context& context, const Model3MF& model)
    {
        std::stringstream stream;
        std::string buf = stream.str();
        reset_stream(stream);
        auto flush = [&context, this](std::string& buf, bool force = false) {
            if ((force && !buf.empty()) || buf.size() >= 65536 * 16) {
                if (!mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) {
                    add_error("Error during writing or compression");
                    return false;
                }
                buf.clear();
            }
            return true;
        };
        auto add_mesh = [&context, this](std::function<bool(std::string&, bool)> const& flush, const Model3MF& model)->bool {
            std::string output_buffer;
            auto format_coordinate = [](float f, char* buf) -> char* {
                assert(is_decimal_separator_point());
                // Round-trippable float, shortest possible.
                return buf + sprintf(buf, "%.9g", f);
            };

            char buf[256];
            //const indexed_triangle_set& its = volume->mesh().its;
          

            std::string type = "model";
            if(model.subtype!="normal_part")
            {
                type = "other";
            }
            output_buffer += "  <";
            output_buffer += OBJECT_TAG;
            output_buffer += " id=\"";
            output_buffer += std::to_string(model.id);
            output_buffer += "\" type=\"";
            output_buffer += type;   
            output_buffer += "\">\n";
            output_buffer += "   <";
            output_buffer += MESH_TAG;
            output_buffer += " ";
            output_buffer += PPATH_ATTR;
            output_buffer +="=\"";
            output_buffer += "3D/Objects/RecentSerializeModel/mesh_data"+std::to_string(model.backup_meshid)+".serial\"";
            output_buffer += " ";
            output_buffer += "c:path";
            output_buffer +="=\"";
            output_buffer += "3D/Objects/RecentSerializeModel/color_spread_data"+std::to_string(model.backup_colorid)+".serial\"";
            output_buffer += " ";
            output_buffer += "seam:path";
            output_buffer +="=\"";
            output_buffer += "3D/Objects/RecentSerializeModel/seam_spread_data"+std::to_string(model.backup_seamid)+".serial\"";
            output_buffer += " ";
            output_buffer += "support:path";
            output_buffer +="=\"";
            output_buffer += "3D/Objects/RecentSerializeModel/support_spread_data"+std::to_string(model.backup_supportid)+".serial";
            output_buffer += "\">\n";
            //bool has_color = model.mesh->faces.size() == model.colors.size();
            //bool has_seam = model.mesh->faces.size() == model.seams.size();
            //bool has_support = model.mesh->faces.size() == model.supports.size();
            output_buffer += "  </";
            output_buffer += MESH_TAG;
            output_buffer += ">\n";
             output_buffer += "  </";
            output_buffer += OBJECT_TAG;
            output_buffer += ">\n";

            // Force flush.
            return flush(output_buffer, true);
        };
        if ((!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) ||
            !add_mesh(flush, model)) {
            add_error("Unable to add mesh to archive");
            return false;
        }
        return true;
    }
    bool Write3MFImpl::_add_model_to_stream(mz_zip_writer_staged_context& context, const Model3MF& model)
    {
        std::stringstream stream;
        std::string buf = stream.str();
        reset_stream(stream);
        auto flush = [&context, this](std::string& buf, bool force = false) {
            if ((force && !buf.empty()) || buf.size() >= 65536 * 16) {
                if (!mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) {
                    add_error("Error during writing or compression");
                    return false;
                }
                buf.clear();
            }
            return true;
        };
        auto add_mesh = [&context, this](std::function<bool(std::string&, bool)> const& flush, const Model3MF& model)->bool {
            std::string output_buffer;
            auto format_coordinate = [](float f, char* buf) -> char* {
                assert(is_decimal_separator_point());
                // Round-trippable float, shortest possible.
                return buf + sprintf(buf, "%.9g", f);
            };

            char buf[256];
            //const indexed_triangle_set& its = volume->mesh().its;
            if (model.mesh->vertices.empty()) {
                add_error("Found invalid mesh");
                return false;
            }

            std::string type = "model";
            if(model.subtype!="normal_part")
            {
                type = "other";
            }
            output_buffer += "  <";
            output_buffer += OBJECT_TAG;
            output_buffer += " id=\"";
            output_buffer += std::to_string(model.id);
            output_buffer += "\" type=\"";
            output_buffer += type;
            output_buffer += "\">\n";
            output_buffer += "   <";
            output_buffer += MESH_TAG;
            output_buffer += ">\n    <";
            output_buffer += VERTICES_TAG;
            output_buffer += ">\n";
            for (size_t i = 0; i < model.mesh->vertices.size(); ++i) {
                trimesh::vec v = model.mesh->vertices[i];
                char* ptr = buf;
                boost::spirit::karma::generate(ptr, boost::spirit::lit("     <") << VERTEX_TAG << " x=\"");
                ptr = format_coordinate(v.x, ptr);
                boost::spirit::karma::generate(ptr, "\" y=\"");
                ptr = format_coordinate(v.y, ptr);
                boost::spirit::karma::generate(ptr, "\" z=\"");
                ptr = format_coordinate(v.z, ptr);
                boost::spirit::karma::generate(ptr, "\"/>\n");
                *ptr = '\0';
                output_buffer += buf;
                if (!flush(output_buffer, false))
                    return false;
            }
            //}

            output_buffer += "    </";
            output_buffer += VERTICES_TAG;
            output_buffer += ">\n    <";
            output_buffer += TRIANGLES_TAG;
            output_buffer += ">\n";

            bool has_color = model.mesh->faces.size() == model.colors.size();
            bool has_seam = model.mesh->faces.size() == model.seams.size();
            bool has_support = model.mesh->faces.size() == model.supports.size();
            for (int i = 0; i < int(model.mesh->faces.size()); ++i) {
                {
                    const trimesh::TriMesh::Face& idx = model.mesh->faces[i];
                    char* ptr = buf;
                    boost::spirit::karma::generate(ptr, boost::spirit::lit("     <") << TRIANGLE_TAG <<
                        " v1=\"" << boost::spirit::int_ <<
                        "\" v2=\"" << boost::spirit::int_ <<
                        "\" v3=\"" << boost::spirit::int_ << "\"",
                        idx[0],
                        idx[1],
                        idx[2]);
                    *ptr = '\0';
                    output_buffer += buf;
                }

                if (has_support)
                {
                    std::string custom_supports_data_string = model.supports.at(i);
                    if (!custom_supports_data_string.empty()) {
                        output_buffer += " ";
                        output_buffer += CUSTOM_SUPPORTS_ATTR;
                        output_buffer += "=\"";
                        output_buffer += custom_supports_data_string;
                        output_buffer += "\"";
                    }
                }

                if (has_seam)
                {
                    std::string custom_seam_data_string = model.seams.at(i);
                    if (!custom_seam_data_string.empty()) {
                        output_buffer += " ";
                        output_buffer += CUSTOM_SEAM_ATTR;
                        output_buffer += "=\"";
                        output_buffer += custom_seam_data_string;
                        output_buffer += "\"";
                    }
                }

                if (has_color)
                {
                    std::string mmu_painting_data_string = model.colors.at(i);
                    if (!mmu_painting_data_string.empty()) {
                        output_buffer += " ";
                        output_buffer += MMU_SEGMENTATION_ATTR;
                        output_buffer += "=\"";
                        output_buffer += mmu_painting_data_string;
                        output_buffer += "\"";
                    }
                }

                output_buffer += "/>\n";

                if (!flush(output_buffer, false))
                    return false;
            }
            output_buffer += "    </";
            output_buffer += TRIANGLES_TAG;
            output_buffer += ">\n   </";
            output_buffer += MESH_TAG;
            output_buffer += ">\n";
            output_buffer += "  </";
            output_buffer += OBJECT_TAG;
            output_buffer += ">\n";

            // Force flush.
            return flush(output_buffer, true);
        };
        if ((!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) ||
            !add_mesh(flush, model)) {
            add_error("Unable to add mesh to archive");
            return false;
        }
        return true;
    }

    bool Write3MFImpl::_add_model_setting_file(const Scene3MF& scene)
    {
        std::stringstream stream;

        auto write_meta = [&stream](const std::string& k, const std::string& v) {
            stream << "    <" << METADATA_TAG << " key=\"" << k << "\" value=\"" << v << "\"/>\n";
        };

        // need more space before
        auto write_meta_sub = [&stream](const std::string& k, const std::string& v) {
            stream << "      <" << METADATA_TAG << " key=\"" << k << "\" value=\"" << v << "\"/>\n";
        };

        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<config>\n";

        // write group info
        for (const Group3MF& group : scene.groups)
        {
            stream << "  <" << OBJECT_TAG << " id=\"" << group.id << "\">\n";
            write_meta("name", group.name);
            write_meta("extruder", std::to_string(group.extruder));
            for (std::map<std::string, std::string>::const_iterator it = group.settings.begin();
                it != group.settings.end(); ++it)
            {
                if (it->first == "name"
                    || it->first == "extruder")
                    continue;

                write_meta(it->first, it->second);
            }

            for (const Model3MF& model : group.models)
            {
                stream << "    <" << PART_TAG << " id=\"" << model.id << "\" subtype=\"" << model.subtype << "\">\n";
                write_meta_sub("name", model.name);
                if (model.extruder != group.extruder)
                {
                    write_meta_sub("extruder", std::to_string(model.extruder));
                }
                write_meta_sub("matrix", xform_to_string(model.xf));
                if (!model.relief_description.empty())
                    stream << model.relief_description;
                    
                for (std::map<std::string, std::string>::const_iterator it = model.settings.begin();
                    it != model.settings.end(); ++it)
                {
                    if (it->first == "name"
                        || it->first == "extruder"
                        || it->first == "matrix")
                        continue;

                    write_meta(it->first, it->second);
                }
                stream << "    </" << PART_TAG << ">\n";
            }
            stream << "  </" << OBJECT_TAG << ">\n";
        }

        // write plate info
        for (const Plate3MF& plate : scene.plates)
        {
            stream << "  <" << PLATE_TAG << ">\n";
            write_meta("plater_id", std::to_string(plate.plateId));
            if (plate.name.empty())
            {
                write_meta("plater_name", "");
            }
            else
            {
                write_meta("plater_name", plate.name);
            }

            std::string tmpVal = "false";
            if (plate.locked)
                tmpVal = "true";

            write_meta("locked", tmpVal);
            for (std::map<std::string, std::string>::const_iterator it = plate.settings.begin();
                it != plate.settings.end(); ++it)
            {
                if (it->first == "plater_name"
                    || it->first == "plater_id"
                    || it->first == "locked")
                    continue;
                write_meta(it->first, it->second);
            }
            for (const int& gid : plate.groupIds)
            {
                stream << "    <" << MODEL_INSTANCE_TAG << ">\n";
                write_meta_sub("object_id", std::to_string(gid));
                write_meta_sub("instance_id", std::to_string(0));
                write_meta_sub("identify_id", std::to_string(gid + 500)); // fix me
                stream << "    </" << MODEL_INSTANCE_TAG << ">\n";
            }
            stream << "  </" << PLATE_TAG << ">\n";
        }

         //write assemble info
        auto write_assemble = [&stream](const std::string& groupId, const std::string& groupXfStr) {
            stream << "    <" << ASSEMBLE_ITEM_TAG << " object_id=\"" << groupId << "\" instance_id=\"0\"" << " transform=\"" << groupXfStr << "\" offset=\"0 0 0\"" << "/>\n";
        };

        stream << "  <" << ASSEMBLE_INFO_TAG << ">\n";
        for (const Group3MF& group : scene.groups)
        {
            write_assemble(std::to_string(group.id), xform_to_string(group.xf));
        }
        stream << "  </" << ASSEMBLE_INFO_TAG << ">\n";

        stream << "</config>\n";

        std::string out = stream.str();
        if (!mz_zip_writer_add_mem(&m_archive, MODEL_SETTINGS_FILE.c_str(),
            (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            add_error("Unable to add relations file to archive");
            return false;
        }

        return true;
    }

    bool Write3MFImpl::_add_custom_gcode_file(const Scene3MF& scene)
    {
        std::stringstream stream;

        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<custom_gcodes_per_layer>\n";
        for (const Plate3MF& plate : scene.plates)
        {
            stream << "<" << PLATE_TAG << ">\n";
            stream << "<plate_info id=\"" << plate.plateId << "\"/>\n";
            for (const PlateCustomGCode& custom : plate.gcodes)
            {
                stream << "<layer top_z=\"" << std::to_string(custom.print_z) << "\"";
                stream << " type=\"" << std::to_string(custom.type) << "\"";
                stream << " extruder=\"" << std::to_string(custom.extruder) << "\"";
                stream << " color=\"" << custom.color << "\"";
                stream << " extra=\"" << custom.extra << "\"";
                stream << " gcode=\"" << custom.extra << "\"/>\n";
            }
            stream << "<mode value=\"" << std::to_string((int)plate.mode) << "\"/>\n";
            stream << "</" << PLATE_TAG << ">\n";
        }
        stream << "</custom_gcodes_per_layer>\n";

        std::string out = stream.str();
        if (!mz_zip_writer_add_mem(&m_archive, CUSTOM_GCODE_PER_PRINT_Z_FILE.c_str(),
            (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            add_error("Unable to add relations file to archive");
            return false;
        }

        return true;
    }

    bool Write3MFImpl::_add_slice_info_config_file(const Scene3MF& scene)
    {
        std::stringstream stream;

        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<config>\n";
        stream << "  <header>\n";
        stream << "    <header_item key = \"X-BBL-Client-Type\" value = \"slicer\"/ >\n";
        stream << "    <header_item key = \"X-BBL-Client-Version\" value = \"01.09.01.66\" / >\n";
        stream << "    <header_item key = \"X-CX-Client-Type\" value = \"creality_print\"/ >\n";
        stream << "    <header_item key = \"X-CX-Client-Version\" value = \"" << scene.create_print_version << "\"/ >\n";
        stream << "  </header>\n";
        stream << "</config>\n";

        std::string out = stream.str();
        if (!mz_zip_writer_add_mem(&m_archive, SLICE_INFO_CONFIG_FILE.c_str(),
            (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            add_error("Unable to add slice info config to archive");
            return false;
        }

        return true;
    }

    bool Write3MFImpl::_add_layer_height_profile_to_archive(mz_zip_archive &archive,const Scene3MF& scene)
    {
        std::string out = "";
        char buffer[1024];

        for (const Group3MF &group : scene.groups) 
        {
          const std::vector<double> &layer_height_profile = group.layerHeightProfile;
          if (layer_height_profile.size() >= 4 && layer_height_profile.size() % 2 == 0) 
          {
            snprintf(buffer, 1024, "object_id=%d|", group.id);
            out += buffer;

            // Store the layer height profile as a single semicolon separated
            // list.
            for (size_t i = 0; i < layer_height_profile.size(); ++i) {
              snprintf(buffer, 1024, (i == 0) ? "%f" : ";%f",
                       layer_height_profile[i]);
              out += buffer;
            }

            out += "\n";
          }
        }

        if (!out.empty()) {
            if (!mz_zip_writer_add_mem(&archive, LAYER_HEIGHTS_FILE.c_str(), (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
                add_error("Unable to add layer heights profile file to archive");
                return false;
            }
        }

        return true;
    }

    void Read3MFImpl::_extract_layer_heights_config_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)
    {
      if (stat.m_uncomp_size > 0) 
      {
        std::string buffer((size_t)stat.m_uncomp_size, 0);
        mz_bool res = mz_zip_reader_extract_to_mem(
            &archive, stat.m_file_index, (void *)buffer.data(),
            (size_t)stat.m_uncomp_size, 0);
        if (res == 0) 
        {
          add_error("Error while reading layer heights profile data to buffer");
          return;
        }

        if (buffer.back() == '\n')
          buffer.pop_back();

        std::vector<std::string> objects;
        boost::split(objects, buffer, boost::is_any_of("\n"),
                     boost::token_compress_off);

        for (const std::string &object : objects) 
        {
          std::vector<std::string> object_data;
          boost::split(object_data, object, boost::is_any_of("|"),
                       boost::token_compress_off);
          if (object_data.size() != 2) 
          {
            add_error("Error while reading object data");
            continue;
          }

          std::vector<std::string> object_data_id;
          boost::split(object_data_id, object_data[0], boost::is_any_of("="),
                       boost::token_compress_off);
          if (object_data_id.size() != 2) {
            add_error("Error while reading object id");
            continue;
          }

          int group_object_id = std::atoi(object_data_id[1].c_str());
          if (group_object_id == 0) {
            add_error("Found invalid object id");
            continue;
          }

          std::map<int, std::vector<double>>::iterator object_item = m_group_id_to_layer_heights.find(group_object_id);
          if (object_item != m_group_id_to_layer_heights.end()) {
            add_error("Found duplicated layer heights profile");
            continue;
          }

          std::vector<std::string> object_data_profile;
          boost::split(object_data_profile, object_data[1],
                       boost::is_any_of(";"), boost::token_compress_off);
          if (object_data_profile.size() <= 4 ||
              object_data_profile.size() % 2 != 0) {
            add_error("Found invalid layer heights profile");
            continue;
          }

          std::vector<double> profile;
          profile.reserve(object_data_profile.size());

          for (const std::string &value : object_data_profile) {
            profile.push_back((double)std::atof(value.c_str()));
          }

          m_group_id_to_layer_heights.insert({group_object_id, profile});
        }
      }
    }

}
