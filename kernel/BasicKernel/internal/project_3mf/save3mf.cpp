#include "save3mf.h"

#include "data/modeln.h"
#include <QList>
#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cxkernel/data/trimeshutils.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/merge.h"

#include "interface/spaceinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qtusercore/module/progressortracer.h"

#include "miniz_extension.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/string_file.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
//#include <stdexcept>
//#include <limits>
#include  <sstream>

#define EXPORT_3MF_USE_SPIRIT_KARMA_FP 0

namespace creative_kernel
{
    const std::string CONTENT_TYPES_FILE = "[Content_Types].xml";
    static constexpr const char* MODEL_TAG = "model";
    static constexpr const char* RESOURCES_TAG = "resources";
    const std::string BBL_ORIGIN_TAG = "Origin";
    const std::string BBL_DESIGNER_TAG = "Designer";
    const std::string BBL_DESIGNER_USER_ID_TAG = "DesignerUserId";
    const std::string BBL_DESIGNER_COVER_FILE_TAG = "DesignerCover";
    const std::string BBL_DESCRIPTION_TAG = "Description";
    const std::string BBL_COPYRIGHT_TAG = "CopyRight";
    const std::string BBL_LICENSE_TAG = "License";
    const std::string BBL_REGION_TAG = "Region";
    const std::string BBL_MODIFICATION_TAG = "ModificationDate";
    const std::string BBL_CREATION_DATE_TAG = "CreationDate";
    const std::string BBL_APPLICATION_TAG = "Application";
    const std::string BBL_MODEL_ID_TAG = "model_id";
    const std::string BBL_MODEL_NAME_TAG = "Title";
    const char* BBS_3MF_VERSION1 = "bamboo_slicer:Version3mf"; // definition of the metadata name saved into .model file
    const char* BBS_3MF_VERSION = "BambuStudio:3mfVersion"; //compatible with prusa currently
    const unsigned int VERSION_BBS_3MF = 1;
    static constexpr const char* METADATA_TAG = "metadata";
    static constexpr const char* OBJECT_TAG = "object";
    static constexpr const char* MESH_TAG = "mesh";
    static constexpr const char* VERTEX_TAG = "vertex";
    static constexpr const char* VERTICES_TAG = "vertices";
    static constexpr const char* TRIANGLES_TAG = "triangles";
    static constexpr const char* TRIANGLE_TAG = "triangle";
    static constexpr const char* FACE_PROPERTY_ATTR = "face_property";
    static constexpr const char* MMU_SEGMENTATION_ATTR = "paint_color";
    static constexpr const char* CUSTOM_SEAM_ATTR = "paint_seam";
    static constexpr const char* CUSTOM_SUPPORTS_ATTR = "paint_supports";
    static constexpr const char* PUUID_ATTR = "p:uuid";
    static constexpr const char* OBJECT_UUID_SUFFIX = "-41cb-4c03-9d28-80fed5dfa1dc";
    static constexpr const char* COMPONENTS_TAG = "components";
    static constexpr const char* COMPONENT_TAG = "component";
    static constexpr const char* TRANSFORM_ATTR = "transform";
    static constexpr const char* BUILD_TAG = "build";
    static constexpr const char* ITEM_TAG = "item";
    static constexpr const char* OBJECTID_ATTR = "objectid";
    static constexpr const char* PPATH_ATTR = "p:path";
    static constexpr const char* PRINTABLE_ATTR = "printable";
    const std::string MODEL_FILE = "3D/3dmodel.model"; // << this is the only format of the string which works with CURA
    const std::string THUMBNAIL_FILE = "Metadata/plate_1.png";
    const std::string RELATIONSHIPS_FILE = "_rels/.rels";
    const std::string MODEL_RELS_FILE = "3D/_rels/3dmodel.model.rels";

    class PackingTemporaryData
    {
    public:
        std::string _3mf_thumbnail;
        std::string _3mf_printer_thumbnail_middle;
        std::string _3mf_printer_thumbnail_small;

        PackingTemporaryData() {}
    };

    Save3MFCommand::Save3MFCommand(QObject* parent)
        :ActionCommand(parent)
    {
        //m_shortcut = "Ctrl+S";
        m_actionname = tr("Save 3MF");
        m_actionNameWithout = "Save 3MF";
        m_insertKey = "save3MF";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this);
    }

    Save3MFCommand::~Save3MFCommand()
    {
    }


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

    bool _add_content_types_file_to_archive(mz_zip_archive& archive)
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

        if (!mz_zip_writer_add_mem(&archive, CONTENT_TYPES_FILE.c_str(), (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            //add_error("Unable to add content types file to archive");
            //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add content types file to archive\n");
            return false;
        }

        return true;
    }

    bool is_decimal_separator_point()
    {
        char str[5] = "";
        sprintf(str, "%.1f", 0.5f);
        return str[1] == '.';
    }

    //BBS: change volume to seperate objects
    bool _add_mesh_to_object_stream(std::function<bool(std::string&, bool)> const& flush, creative_kernel::ModelN* model, unsigned int backup_id, unsigned int& obj_idx)
    {
        bool m_from_backup_save = false;

        trimesh::TriMesh* mesh = model->mesh();

        std::string output_buffer;

        /*output_buffer += "   <";
        output_buffer += MESH_TAG;
        output_buffer += ">\n    <";
        output_buffer += VERTICES_TAG;
        output_buffer += ">\n";*/

        auto format_coordinate = [](float f, char* buf) -> char* {
            assert(is_decimal_separator_point());
#if EXPORT_3MF_USE_SPIRIT_KARMA_FP
            // Slightly faster than sprintf("%.9g"), but there is an issue with the karma floating point formatter,
            // https://github.com/boostorg/spirit/pull/586
            // where the exported string is one digit shorter than it should be to guarantee lossless round trip.
            // The code is left here for the ocasion boost guys improve.
            coordinate_type_fixed      const coordinate_fixed = coordinate_type_fixed();
            coordinate_type_scientific const coordinate_scientific = coordinate_type_scientific();
            // Format "f" in a fixed format.
            char* ptr = buf;
            boost::spirit::karma::generate(ptr, coordinate_fixed, f);
            // Format "f" in a scientific format.
            char* ptr2 = ptr;
            boost::spirit::karma::generate(ptr2, coordinate_scientific, f);
            // Return end of the shorter string.
            auto len2 = ptr2 - ptr;
            if (ptr - buf > len2) {
                // Move the shorter scientific form to the front.
                memcpy(buf, ptr, len2);
                ptr = buf + len2;
            }
            // Return pointer to the end.
            return ptr;
#else
            // Round-trippable float, shortest possible.
            return buf + sprintf(buf, "%.9g", f);
#endif
        };

        char buf[256];
        unsigned int vertices_count = 0;
        //unsigned int triangles_count = 0;
        unsigned int volume_idx, volume_count = 0;
        //for (ModelVolume* volume : object.volumes) {
            //if (volume == nullptr)
            //    continue;

            //if (!volume->mesh().stats().repaired())
            //	throw Slic3r::FileIOError("store_3mf() requires repair()");
        unsigned int first_vertex_id = 0;
        volume_count++;
        if (m_from_backup_save)
            volume_idx = (volume_count << 16 | obj_idx);
        else {
            volume_idx = obj_idx;
            obj_idx++;
        }
        //volumes_objectID.insert({ volume, volume_idx });

        //const indexed_triangle_set& its = volume->mesh().its;
        if (mesh->vertices.empty()) {
            //add_error("Found invalid mesh");
            //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Found invalid mesh\n");
            return false;
        }

        std::string type = "model";

        output_buffer += "  <";
        output_buffer += OBJECT_TAG;
        output_buffer += " id=\"";
        output_buffer += std::to_string(volume_idx);
        /*if (m_production_ext) {
            std::stringstream stream;
            reset_stream(stream);
            stream << "\" " << PUUID_ATTR << "=\"" << hex_wrap<boost::uint32_t>{(boost::uint32_t)backup_id} << OBJECT_UUID_SUFFIX;
            //output_buffer += "\" ";
            //output_buffer += PUUID_ATTR;
            //output_buffer += "=\"";
            //output_buffer += std::to_string(hex_wrap<boost::uint32_t>{(boost::uint32_t)backup_id});
            //output_buffer += OBJECT_UUID_SUFFIX;
            output_buffer += stream.str();
        }*/
        output_buffer += "\" type=\"";
        output_buffer += type;
        output_buffer += "\">\n";
        output_buffer += "   <";
        output_buffer += MESH_TAG;
        output_buffer += ">\n    <";
        output_buffer += VERTICES_TAG;
        output_buffer += ">\n";

        vertices_count += (int)mesh->vertices.size();

        QMatrix4x4 matrix = model->globalMatrix();
        trimesh::fxform _matrix(static_cast<trimesh::fxform>(matrix.data()));

        for (size_t i = 0; i < mesh->vertices.size(); ++i) {
            //don't save the volume's matrix into vertex data
            //add the shared mesh logic
            //Vec3f v = (matrix * its.vertices[i].cast<double>()).cast<float>();
            trimesh::vec v = mesh->vertices[i];
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

        //for (ModelVolume* volume : object.volumes) {
        //    if (volume == nullptr)
        //        continue;

            //BBS: as we stored matrix seperately, not multiplied into vertex
            //we don't need to consider this left hand case specially
            //bool is_left_handed = volume->is_left_handed();
        bool is_left_handed = false;
        //VolumeToOffsetsMap::iterator volume_it = volumes_objectID.find(volume);
        //assert(volume_it != volumes_objectID.end());

        //const indexed_triangle_set &its = volume->mesh().its;

        // updates triangle offsets
        //unsigned int first_triangle_id = triangles_count;
        //triangles_count += (int)its.indices.size();
        //unsigned int last_triangle_id = triangles_count - 1;

        for (int i = 0; i < int(mesh->faces.size()); ++i) {
            {
                const trimesh::TriMesh::Face& idx = mesh->faces[i];
                char* ptr = buf;
                boost::spirit::karma::generate(ptr, boost::spirit::lit("     <") << TRIANGLE_TAG <<
                    " v1=\"" << boost::spirit::int_ <<
                    "\" v2=\"" << boost::spirit::int_ <<
                    "\" v3=\"" << boost::spirit::int_ << "\"",
                    idx[is_left_handed ? 2 : 0] + first_vertex_id,
                    idx[1] + first_vertex_id,
                    idx[is_left_handed ? 0 : 2] + first_vertex_id);
                *ptr = '\0';
                output_buffer += buf;
            }

            std::string custom_supports_data_string = model->getSupport2Facets(i);
            if (!custom_supports_data_string.empty()) {
                output_buffer += " ";
                output_buffer += CUSTOM_SUPPORTS_ATTR;
                output_buffer += "=\"";
                output_buffer += custom_supports_data_string;
                output_buffer += "\"";
            }

            std::string custom_seam_data_string = model->getSeam2Facets(i);
            if (!custom_seam_data_string.empty()) {
                output_buffer += " ";
                output_buffer += CUSTOM_SEAM_ATTR;
                output_buffer += "=\"";
                output_buffer += custom_seam_data_string;
                output_buffer += "\"";
            }

            std::string mmu_painting_data_string = model->getColors2Facets(i);
            if (!mmu_painting_data_string.empty()) {
                output_buffer += " ";
                output_buffer += MMU_SEGMENTATION_ATTR;
                output_buffer += "=\"";
                output_buffer += mmu_painting_data_string;
                output_buffer += "\"";
            }

            // BBS
            //if (i < its.properties.size()) {
            //    std::string prop_str = its.properties[i].to_string();
            //    if (!prop_str.empty()) {
            //        output_buffer += " ";
            //        output_buffer += FACE_PROPERTY_ATTR;
            //        output_buffer += "=\"";
            //        output_buffer += prop_str;
            //        output_buffer += "\"";
            //    }
            //}

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
    }


    bool _add_object_to_model_stream(mz_zip_writer_staged_context& context, creative_kernel::ModelN* model, unsigned int object_id, unsigned int backup_id, ccglobal::Tracer* tracer)
    {
        bool m_production_ext = false;
        bool m_from_backup_save = false;
        //int backup_id = 2;

        std::stringstream stream;
        reset_stream(stream);
        unsigned int id = 0;
        unsigned int volume_start_id = object_id;

        {

            if (id == 0) {
                std::string buf = stream.str();
                reset_stream(stream);
                // backup: make _add_mesh_to_object_stream() reusable
                auto flush = [&context](std::string& buf, bool force = false) {
                    if ((force && !buf.empty()) || buf.size() >= 65536 * 16) {
                        if (!mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) {
                            //add_error("Error during writing or compression");
                            //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Error during writing or compression\n");
                            return false;
                        }
                        buf.clear();
                    }
                    return true;
                };
                if ((!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) ||
                    !_add_mesh_to_object_stream(flush, model, backup_id, volume_start_id)) {
                    //add_error("Unable to add mesh to archive");
                    //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add mesh to archive\n");
                    return false;
                }
            }

            if (tracer)
                tracer->progress(0.6);

            stream << "  <" << OBJECT_TAG << " id=\"" << volume_start_id + id;
            if ((id == 0) && m_production_ext)
                stream << "\" " << PUUID_ATTR << "=\"" << (boost::uint32_t)backup_id << OBJECT_UUID_SUFFIX;
            stream << "\" type=\"model\">\n";
            stream << "   <" << COMPONENTS_TAG << ">\n";

            if (id == 0) {
                if (m_from_backup_save) {
                    for (unsigned int index = 1; index <= 1; index++) {
                        unsigned int ref_id = object_id | (index << 16);
                        stream << "    <" << COMPONENT_TAG << " objectid=\"" << ref_id; // << "\"/>\n";
                        //add the transform of the volume
                        //ModelVolume* volume = object.volumes[index - 1];
                        //const Transform3d& transf = volume->get_matrix();

                        QMatrix4x4 matrix = model->globalMatrix();
                        //trimesh::fxform transf(static_cast<trimesh::fxform>(matrix.data()));
                        stream << "\" " << TRANSFORM_ATTR << "=\"";
                        for (unsigned c = 0; c < 4; ++c) {
                            for (unsigned r = 0; r < 3; ++r) {
                                stream << matrix(r, c);
                                if (r != 2 || c != 3)
                                    stream << " ";
                            }
                        }
                        stream << "\"/>\n";
                    }
                }
                else {
                    for (unsigned int index = object_id; index < volume_start_id; index++) {
                        stream << "    <" << COMPONENT_TAG << " objectid=\"" << index; // << "\"/>\n";
                        //add the transform of the volume
                        QMatrix4x4 matrix = model->globalMatrix();
                        stream << "\" " << TRANSFORM_ATTR << "=\"";
                        for (unsigned c = 0; c < 4; ++c) {
                            for (unsigned r = 0; r < 3; ++r) {
                                stream << matrix(r, c);
                                if (r != 2 || c != 3)
                                    stream << " ";
                            }
                        }
                        stream << "\"/>\n";
                    }
                }
            }
            else {
                stream << "    <" << COMPONENT_TAG << " objectid=\"" << volume_start_id << "\"/>\n";
            }
            stream << "   </" << COMPONENTS_TAG << ">\n";

            stream << "  </" << OBJECT_TAG << ">\n";

            ++id;

        }

        std::string buf = stream.str();
        return buf.empty() || mz_zip_writer_add_staged_data(&context, buf.data(), buf.size());
    }

    bool _add_build_to_model_stream(std::stringstream& stream, std::vector<std::pair<int, QMatrix4x4>>& build_items)
    {
        // This happens for empty projects

        stream << " <" << BUILD_TAG;
        //if (m_production_ext)
        //    stream << " " << PUUID_ATTR << "=\"" << BUILD_UUID << "\"";
        stream << ">\n";

        for (auto& item : build_items) {
            stream << "  <" << ITEM_TAG << " " << OBJECTID_ATTR << "=\"" << item.first;
            //if (m_production_ext)
            //    stream << "\" " << PUUID_ATTR << "=\"" << hex_wrap<boost::uint32_t>{item.id} << BUILD_UUID_SUFFIX;
            //if (!item.path.empty())
            //    stream << "\" " << PPATH_ATTR << "=\"" << xml_escape(item.path);

            QMatrix4x4& matrix = item.second;
            stream << "\" " << TRANSFORM_ATTR << "=\"";
            for (unsigned c = 0; c < 4; ++c) {
                for (unsigned r = 0; r < 3; ++r) {
                    stream << matrix(r, c);
                    if (r != 2 || c != 3)
                        stream << " ";
                }
            }
            stream << "\" " << PRINTABLE_ATTR << "=\"" << 1 << "\"/>\n";
        }

        stream << " </" << BUILD_TAG << ">\n";

        return true;
    }

    bool _add_relationships_file_to_archive(
        mz_zip_archive& archive, std::string const& from, std::vector<std::string> const& targets, std::vector<std::string> const& types, PackingTemporaryData data = PackingTemporaryData(), int export_plate_idx = -1)
    {
        std::stringstream stream;
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n";
        if (from.empty()) {
            stream << " <Relationship Target=\"/" << MODEL_FILE << "\" Id=\"rel-1\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n";

            if (data._3mf_thumbnail.empty()) {
                if (export_plate_idx < 0) {
                    stream << " <Relationship Target=\"/" << THUMBNAIL_FILE
                        << "\" Id=\"rel-2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail\"/>\n";
                }
                else {
                    //std::string thumbnail_file_str = (boost::format("Metadata/plate_%1%.png") % (export_plate_idx + 1)).str();
                    std::string thumbnail_file_str = "Metadata/plate_.png";
                    stream << " <Relationship Target=\"/"<< thumbnail_file_str
                        << "\" Id=\"rel-2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail\"/>\n";
                }
            }
            else {
                stream << " <Relationship Target=\"/" << data._3mf_thumbnail
                    << "\" Id=\"rel-2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail\"/>\n";
            }

            if (!data._3mf_printer_thumbnail_middle.empty()) {
                stream << " <Relationship Target=\"/" << data._3mf_printer_thumbnail_middle
                    << "\" Id=\"rel-4\" Type=\"http://schemas.bambulab.com/package/2021/cover-thumbnail-middle\"/>\n";
            }
            if (!data._3mf_printer_thumbnail_small.empty())
                stream << " <Relationship Target=\"/" << data._3mf_printer_thumbnail_small
                << "\" Id=\"rel-5\" Type=\"http://schemas.bambulab.com/package/2021/cover-thumbnail-small\"/>\n";
        }
        else if (targets.empty()) {
            return false;
        }
        else {
            int i = 0;
            for (auto& path : targets) {
                for (auto& type : types)
                    stream << " <Relationship Target=\"/" << xml_escape(path) << "\" Id=\"rel-" << std::to_string(++i) << "\" Type=\"" << type << "\"/>\n";
            }
        }
        stream << "</Relationships>";

        std::string out = stream.str();

        if (!mz_zip_writer_add_mem(&archive, from.empty() ? RELATIONSHIPS_FILE.c_str() : from.c_str(), (const void*)out.data(), out.length(), MZ_DEFAULT_COMPRESSION)) {
            //add_error("Unable to add relationships file to archive");
            //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add relationships file to archive\n");
            return false;
        }

        return true;
    }


    bool _add_model_file_to_archive(const std::string& filename, mz_zip_archive& archive, QList<creative_kernel::ModelN*>& selections,ccglobal::Tracer* tracer)
    {
        bool sub_model = false;
        bool m_split_model = false;
        bool m_zip64 = true;
        bool m_production_ext = false;
        bool write_object = sub_model || !m_split_model;

        auto& zip_filename = filename;

        mz_zip_writer_staged_context context;
        if (!mz_zip_writer_add_staged_open(&archive, &context, sub_model ? zip_filename.c_str() : MODEL_FILE.c_str(),
            m_zip64 ?
            // Maximum expected and allowed 3MF file size is 16GiB.
            // This switches the ZIP file to a 64bit mode, which adds a tiny bit of overhead to file records.
            (uint64_t(1) << 30) * 16 :
            // Maximum expected 3MF file size is 4GB-1. This is a workaround for interoperability with Windows 10 3D model fixing API, see
            // GH issue #6193.
            (uint64_t(1) << 32) - 1,
            //#if WRITE_ZIP_LANGUAGE_ENCODING
            nullptr, nullptr, 0, MZ_DEFAULT_LEVEL, nullptr, 0, nullptr, 0)) {
            //#else
            //           nullptr, nullptr, 0, MZ_DEFAULT_COMPRESSION, extra.c_str(), extra.length(), extra.c_str(), extra.length())) {
            //#endif
                        //add_error("Unable to add model file to archive");
                        //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add model file to archive\n");
            return false;
        }

        if (tracer)
            tracer->progress(0.3);

        {
            std::stringstream stream;
            reset_stream(stream);
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            stream << "<" << MODEL_TAG << " unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" xmlns:slic3rpe=\"http://schemas.slic3r.org/3mf/2017/06\"";
            if (m_production_ext)
                stream << " xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\" requiredextensions=\"p\"";
            stream << ">\n";

            std::string origin;
            std::string name;
            std::string user_name;
            std::string user_id;
            std::string design_cover;
            std::string license;
            std::string description;
            std::string copyright;
            std::string rating;
            std::string model_id;
            std::string region_code;

            // remember to use metadata_item_map to store metadata info
            std::map<std::string, std::string> metadata_item_map;

            if (!sub_model) {
                // update metadat_items
                //if (model.model_info && model.model_info.get()) {
                //    metadata_item_map = model.model_info.get()->metadata_items;
                //}

                metadata_item_map[BBL_MODEL_NAME_TAG] = xml_escape(name);
                metadata_item_map[BBL_ORIGIN_TAG] = xml_escape(origin);
                metadata_item_map[BBL_DESIGNER_TAG] = xml_escape(user_name);
                metadata_item_map[BBL_DESIGNER_USER_ID_TAG] = user_id;
                metadata_item_map[BBL_DESIGNER_COVER_FILE_TAG] = xml_escape(design_cover);
                metadata_item_map[BBL_DESCRIPTION_TAG] = xml_escape(description);
                metadata_item_map[BBL_COPYRIGHT_TAG] = xml_escape(copyright);
                metadata_item_map[BBL_LICENSE_TAG] = xml_escape(license);

                /* save model info */
                if (!model_id.empty()) {
                    metadata_item_map[BBL_MODEL_ID_TAG] = model_id;
                    metadata_item_map[BBL_REGION_TAG] = region_code;
                }

                //TODO
                //std::string date = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
                // keep only the date part of the string
                //date = date.substr(0, 10);
                metadata_item_map[BBL_CREATION_DATE_TAG] = "";
                metadata_item_map[BBL_MODIFICATION_TAG] = "";
                //SoftFever: write BambuStudio tag to keep it compatible 
                metadata_item_map[BBL_APPLICATION_TAG] = "";
            }
            metadata_item_map[BBS_3MF_VERSION] = std::to_string(VERSION_BBS_3MF);

            // store metadata info
            //for (auto item : metadata_item_map) {
            //    BOOST_LOG_TRIVIAL(info) << "bbs_3mf: save key= " << item.first << ", value = " << item.second;
            //    stream << " <" << METADATA_TAG << " name=\"" << item.first << "\">"
            //        << xml_escape(item.second) << "</" << METADATA_TAG << ">\n";
            //}

            stream << " <" << RESOURCES_TAG << ">\n";
            std::string buf = stream.str();
            if (!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) {
                //add_error("Unable to add model file to archive");
                //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add model file to archive\n");
                return false;
            }

            int backup_id = selections.size();
            std::vector<std::pair<int, QMatrix4x4>>build_items;
            unsigned int curr_id;

            std::vector<std::string> object_paths;

            int object_id = 1;
            for (size_t i = 0; i < selections.size(); i++)
            {
                creative_kernel::ModelN* model = selections[i];
                //int object_id = i + 1;
                if (!_add_object_to_model_stream(context, model, object_id, backup_id,tracer))
                {
                    mz_zip_writer_add_staged_finish(&context);
                    return false;
                }
                
                curr_id = object_id + 1;
                build_items.push_back(std::pair<int, QMatrix4x4>(curr_id, model->globalMatrix()));

                std::string filename = MODEL_FILE;
                //auto filename = boost::format("3D/Objects/%s_%d.model") % name % object_id;
                if (i == 0)
                    object_paths.push_back(filename);

                object_id += 2;
            }

            if (tracer)
                tracer->progress(0.8);

            reset_stream(stream);
            stream << " </" << RESOURCES_TAG << ">\n";


            // Store the transformations of all the ModelInstances of all ModelObjects, indexed in a linear fashion.
            if (!_add_build_to_model_stream(stream, build_items)) {
                //add_error("Unable to add build to archive");
                //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add build to archive\n");
                return false;
            }

            stream << "</" << MODEL_TAG << ">\n";

            buf = stream.str();

            if ((!buf.empty() && !mz_zip_writer_add_staged_data(&context, buf.data(), buf.size())) ||
                !mz_zip_writer_add_staged_finish(&context)) {
                //add_error("Unable to add model file to archive");
                //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Unable to add model file to archive\n");
                return false;
            }

            // write model rels
            _add_relationships_file_to_archive(archive, RELATIONSHIPS_FILE, object_paths, { "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel" });

            if (tracer)
                tracer->progress(0.9);

        }
    }

    bool save3MF(const std::string& filename, ccglobal::Tracer* tracer)
    {
        mz_zip_archive archive;
        mz_zip_zero_struct(&archive);

        if (!open_zip_writer(&archive, filename)) {
            printf("Unable to open the 3mf file\n");
            return false;
        }

        if (tracer)
            tracer->progress(0.2);
        if (!_add_content_types_file_to_archive(archive)) {
            return false;
        }
        QList<creative_kernel::ModelN*> selections = creative_kernel::selectionms();
        if (selections.isEmpty())
            return false;
        if (!_add_model_file_to_archive(filename, archive, selections, tracer)) { return false; }
       
        mz_zip_writer_finalize_archive(&archive);
        close_zip_writer(&archive);
        if (tracer)
            tracer->progress(1.0);
        return true;
    }

    void Save3MFCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Save as 3MF");
        QList<ModelN*> selections = selectionms();
        if (selections.size() == 0)
        {
            requestQmlDialog(this, "messageNoModelDlg");
            return;
        }

        cxkernel::saveFile(this, QString("%1.3mf").arg(mainModelName()));
    }

    void Save3MFCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void Save3MFCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Save 3MF") + "    " + m_shortcut;
    }

    QString Save3MFCommand::filter()
    {
        QString _filter = QString("Mesh File (*.3mf)");
        return _filter;
    }

    void Save3MFCommand::handle(const QString& fileName)
    {
        //save3MF(fileName.toStdString());
        //m_strMessageText = tr("Save 3mf file Finished");
        ////requestQmlDialog(this, "messageDlg");
        //return;

        //std::vector<trimesh::TriMesh*> meshs;
        //QList<ModelN*> models = selectionms();
        //size_t size = models.size();


        QList<qtuser_core::JobPtr> jobs;
        Save3MFJob* loadJob = new Save3MFJob(this);
        loadJob->setFileName(fileName);
        jobs.push_back(qtuser_core::JobPtr(loadJob));
        cxkernel::executeJobs(jobs);
    }

    void Save3MFCommand::accept()
    {
        openFileLocation(m_fileName);
    }

    Save3MFJob::Save3MFJob(QObject* parent)
        : Job(parent)
    {
    }

    Save3MFJob::~Save3MFJob()
    {
    }

    void Save3MFJob::setFileName(const QString& fileName)
    {
        m_fileName = fileName;
    }

    QString Save3MFJob::name()
    {
        return "";
    }

    QString Save3MFJob::description()
    {
        return "";
    }

    QString Save3MFJob::getMessageText()
    {
        return m_strMessageText;
    }

    void Save3MFJob::failed()
    {
        qDebug() << "";
    }

    void Save3MFJob::successed(qtuser_core::Progressor* progressor)
    {
        m_strMessageText = tr("Save 3mf file Finished");
        requestQmlDialog(this, "messageDlg");
    }

    void Save3MFJob::work(qtuser_core::Progressor* progressor)
    {
        qtuser_core::ProgressorTracer tracer(progressor);

        save3MF(m_fileName.toStdString(), &tracer);
    }
}
