//copy from bbs_3mf.cpp

#ifndef CONSTANT_3MF_H
#define CONSTANT_3MF_H

#include <string>

const std::string MODEL_FOLDER    = "3D/";
const std::string MODEL_EXTENSION = ".model";
const std::string MODEL_FILE      = "3D/3dmodel.model"; // << this is the only format of the string which works with CURA
const std::string MODEL_RELS_FILE = "3D/_rels/3dmodel.model.rels";
// BBS: add metadata_folder
const std::string METADATA_DIR                  = "Metadata/";
const std::string ACCESOR_DIR                   = "accesories/";
const std::string GCODE_EXTENSION               = ".gcode";
const std::string THUMBNAIL_EXTENSION           = ".png";
const std::string CALIBRATION_INFO_EXTENSION    = ".json";
const std::string CONTENT_TYPES_FILE            = "[Content_Types].xml";
const std::string RELATIONSHIPS_FILE            = "_rels/.rels";
const std::string THUMBNAIL_FILE                = "Metadata/plate_1.png";
const std::string THUMBNAIL_FOR_PRINTER_FILE    = "Metadata/bbl_thumbnail.png";
const std::string PRINTER_THUMBNAIL_SMALL_FILE  = "/Auxiliaries/.thumbnails/thumbnail_small.png";
const std::string PRINTER_THUMBNAIL_MIDDLE_FILE = "/Auxiliaries/.thumbnails/thumbnail_middle.png";
const std::string _3MF_COVER_FILE               = "/Auxiliaries/.thumbnails/thumbnail_3mf.png";
// const std::string PRINT_CONFIG_FILE = "Metadata/Slic3r_PE.config";
// const std::string MODEL_CONFIG_FILE = "Metadata/Slic3r_PE_model.config";
const std::string BBS_PRINT_CONFIG_FILE          = "Metadata/print_profile.config";
const std::string BBS_PROJECT_CONFIG_FILE        = "Metadata/project_settings.config";
const std::string BBS_MODEL_CONFIG_FILE          = "Metadata/model_settings.config";
const std::string BBS_MODEL_CONFIG_RELS_FILE     = "Metadata/_rels/model_settings.config.rels";
const std::string SLICE_INFO_CONFIG_FILE         = "Metadata/slice_info.config";
const std::string BBS_LAYER_HEIGHTS_PROFILE_FILE = "Metadata/layer_heights_profile.txt";
const std::string LAYER_CONFIG_RANGES_FILE       = "Metadata/layer_config_ranges.xml";
/*const std::string SLA_SUPPORT_POINTS_FILE = "Metadata/Slic3r_PE_sla_support_points.txt";
const std::string SLA_DRAIN_HOLES_FILE = "Metadata/Slic3r_PE_sla_drain_holes.txt";*/
const std::string CUSTOM_GCODE_PER_PRINT_Z_FILE          = "Metadata/custom_gcode_per_layer.xml";
const std::string AUXILIARY_DIR                          = "Auxiliaries/";
const std::string PROJECT_EMBEDDED_PRINT_PRESETS_FILE    = "Metadata/print_setting_";
const std::string PROJECT_EMBEDDED_SLICE_PRESETS_FILE    = "Metadata/process_settings_";
const std::string PROJECT_EMBEDDED_FILAMENT_PRESETS_FILE = "Metadata/filament_settings_";
const std::string PROJECT_EMBEDDED_PRINTER_PRESETS_FILE  = "Metadata/machine_settings_";
const std::string CUT_INFORMATION_FILE                   = "Metadata/cut_information.xml";

const unsigned int AUXILIARY_STR_LEN = 12;
const unsigned int METADATA_STR_LEN  = 9;

static constexpr const char* MODEL_TAG                 = "model";
static constexpr const char* RESOURCES_TAG             = "resources";
static constexpr const char* COLOR_GROUP_TAG           = "m:colorgroup";
static constexpr const char* COLOR_TAG                 = "m:color";
static constexpr const char* OBJECT_TAG                = "object";
static constexpr const char* MESH_TAG                  = "mesh";
static constexpr const char* MESH_STAT_TAG             = "mesh_stat";
static constexpr const char* VERTICES_TAG              = "vertices";
static constexpr const char* VERTEX_TAG                = "vertex";
static constexpr const char* TRIANGLES_TAG             = "triangles";
static constexpr const char* TRIANGLE_TAG              = "triangle";
static constexpr const char* COMPONENTS_TAG            = "components";
static constexpr const char* COMPONENT_TAG             = "component";
static constexpr const char* BUILD_TAG                 = "build";
static constexpr const char* ITEM_TAG                  = "item";
static constexpr const char* METADATA_TAG              = "metadata";
static constexpr const char* FILAMENT_TAG              = "filament";
static constexpr const char* SLICE_WARNING_TAG         = "warning";
static constexpr const char* WARNING_MSG_TAG           = "msg";
static constexpr const char* FILAMENT_ID_TAG           = "id";
static constexpr const char* FILAMENT_TYPE_TAG         = "type";
static constexpr const char* FILAMENT_COLOR_TAG        = "color";
static constexpr const char* FILAMENT_USED_M_TAG       = "used_m";
static constexpr const char* FILAMENT_USED_G_TAG       = "used_g";
static constexpr const char* FILAMENT_TRAY_INFO_ID_TAG = "tray_info_idx";

static constexpr const char* CONFIG_TAG   = "config";
static constexpr const char* VOLUME_TAG   = "volume";
static constexpr const char* PART_TAG     = "part";
static constexpr const char* PLATE_TAG    = "plate";
static constexpr const char* INSTANCE_TAG = "model_instance";
// BBS
static constexpr const char* ASSEMBLE_TAG          = "assemble";
static constexpr const char* ASSEMBLE_ITEM_TAG     = "assemble_item";
static constexpr const char* SLICE_HEADER_TAG      = "header";
static constexpr const char* SLICE_HEADER_ITEM_TAG = "header_item";

// Deprecated: text_info
static constexpr const char* TEXT_INFO_TAG        = "text_info";
static constexpr const char* TEXT_ATTR            = "text";
static constexpr const char* FONT_NAME_ATTR       = "font_name";
static constexpr const char* FONT_INDEX_ATTR      = "font_index";
static constexpr const char* FONT_SIZE_ATTR       = "font_size";
static constexpr const char* THICKNESS_ATTR       = "thickness";
static constexpr const char* EMBEDED_DEPTH_ATTR   = "embeded_depth";
static constexpr const char* ROTATE_ANGLE_ATTR    = "rotate_angle";
static constexpr const char* TEXT_GAP_ATTR        = "text_gap";
static constexpr const char* BOLD_ATTR            = "bold";
static constexpr const char* ITALIC_ATTR          = "italic";
static constexpr const char* SURFACE_TEXT_ATTR    = "surface_text";
static constexpr const char* KEEP_HORIZONTAL_ATTR = "keep_horizontal";
static constexpr const char* HIT_MESH_ATTR        = "hit_mesh";
static constexpr const char* HIT_POSITION_ATTR    = "hit_position";
static constexpr const char* HIT_NORMAL_ATTR      = "hit_normal";

// BBS: encrypt
static constexpr const char* RELATIONSHIP_TAG       = "Relationship";
static constexpr const char* PID_ATTR               = "pid";
static constexpr const char* PUUID_ATTR             = "p:UUID";
static constexpr const char* PUUID_LOWER_ATTR       = "p:uuid";
static constexpr const char* PPATH_ATTR             = "p:path";
static constexpr const char* OBJECT_UUID_SUFFIX     = "-61cb-4c03-9d28-80fed5dfa1dc";
static constexpr const char* OBJECT_UUID_SUFFIX2    = "-71cb-4c03-9d28-80fed5dfa1dc";
static constexpr const char* SUB_OBJECT_UUID_SUFFIX = "-81cb-4c03-9d28-80fed5dfa1dc";
static constexpr const char* COMPONENT_UUID_SUFFIX  = "-b206-40ff-9872-83e8017abed1";
static constexpr const char* BUILD_UUID             = "2c7c17d8-22b5-4d84-8835-1976022ea369";
static constexpr const char* BUILD_UUID_SUFFIX      = "-b1ec-4553-aec9-835e5b724bb4";
static constexpr const char* TARGET_ATTR            = "Target";
static constexpr const char* RELS_TYPE_ATTR         = "Type";

static constexpr const char* UNIT_ATTR      = "unit";
static constexpr const char* NAME_ATTR      = "name";
static constexpr const char* COLOR_ATTR     = "color";
static constexpr const char* TYPE_ATTR      = "type";
static constexpr const char* ID_ATTR        = "id";
static constexpr const char* X_ATTR         = "x";
static constexpr const char* Y_ATTR         = "y";
static constexpr const char* Z_ATTR         = "z";
static constexpr const char* V1_ATTR        = "v1";
static constexpr const char* V2_ATTR        = "v2";
static constexpr const char* V3_ATTR        = "v3";
static constexpr const char* OBJECTID_ATTR  = "objectid";
static constexpr const char* TRANSFORM_ATTR = "transform";
// BBS
static constexpr const char* OFFSET_ATTR           = "offset";
static constexpr const char* PRINTABLE_ATTR        = "printable";
static constexpr const char* INSTANCESCOUNT_ATTR   = "instances_count";
static constexpr const char* CUSTOM_SUPPORTS_ATTR  = "paint_supports";
static constexpr const char* CUSTOM_SEAM_ATTR      = "paint_seam";
static constexpr const char* MMU_SEGMENTATION_ATTR = "paint_color";
// BBS
static constexpr const char* FACE_PROPERTY_ATTR = "face_property";

static constexpr const char* KEY_ATTR                              = "key";
static constexpr const char* VALUE_ATTR                            = "value";
static constexpr const char* FIRST_TRIANGLE_ID_ATTR                = "firstid";
static constexpr const char* LAST_TRIANGLE_ID_ATTR                 = "lastid";
static constexpr const char* SUBTYPE_ATTR                          = "subtype";
static constexpr const char* LOCK_ATTR                             = "locked";
static constexpr const char* BED_TYPE_ATTR                         = "bed_type";
static constexpr const char* PRINT_SEQUENCE_ATTR                   = "print_sequence";
static constexpr const char* FIRST_LAYER_PRINT_SEQUENCE_ATTR       = "first_layer_print_sequence";
static constexpr const char* OTHER_LAYERS_PRINT_SEQUENCE_ATTR      = "other_layers_print_sequence";
static constexpr const char* OTHER_LAYERS_PRINT_SEQUENCE_NUMS_ATTR = "other_layers_print_sequence_nums";
static constexpr const char* SPIRAL_VASE_MODE                      = "spiral_mode";
static constexpr const char* GCODE_FILE_ATTR                       = "gcode_file";
static constexpr const char* THUMBNAIL_FILE_ATTR                   = "thumbnail_file";
static constexpr const char* NO_LIGHT_THUMBNAIL_FILE_ATTR          = "thumbnail_no_light_file";
static constexpr const char* TOP_FILE_ATTR                         = "top_file";
static constexpr const char* PICK_FILE_ATTR                        = "pick_file";
static constexpr const char* PATTERN_FILE_ATTR                     = "pattern_file";
static constexpr const char* PATTERN_BBOX_FILE_ATTR                = "pattern_bbox_file";
static constexpr const char* OBJECT_ID_ATTR                        = "object_id";
static constexpr const char* INSTANCEID_ATTR                       = "instance_id";
static constexpr const char* IDENTIFYID_ATTR                       = "identify_id";
static constexpr const char* PLATERID_ATTR                         = "plater_id";
static constexpr const char* PLATER_NAME_ATTR                      = "plater_name";
static constexpr const char* PLATE_IDX_ATTR                        = "index";
static constexpr const char* PRINTER_MODEL_ID_ATTR                 = "printer_model_id";
static constexpr const char* NOZZLE_DIAMETERS_ATTR                 = "nozzle_diameters";
static constexpr const char* SLICE_PREDICTION_ATTR                 = "prediction";
static constexpr const char* SLICE_WEIGHT_ATTR                     = "weight";
static constexpr const char* TIMELAPSE_TYPE_ATTR                   = "timelapse_type";
static constexpr const char* TIMELAPSE_ERROR_CODE_ATTR             = "timelapse_error_code";
static constexpr const char* OUTSIDE_ATTR                          = "outside";
static constexpr const char* SUPPORT_USED_ATTR                     = "support_used";
static constexpr const char* LABEL_OBJECT_ENABLED_ATTR             = "label_object_enabled";
static constexpr const char* SKIPPED_ATTR                          = "skipped";

static constexpr const char* OBJECT_TYPE = "object";
static constexpr const char* VOLUME_TYPE = "volume";
static constexpr const char* PART_TYPE   = "part";

static constexpr const char* NAME_KEY             = "name";
static constexpr const char* VOLUME_TYPE_KEY      = "volume_type";
static constexpr const char* PART_TYPE_KEY        = "part_type";
static constexpr const char* MATRIX_KEY           = "matrix";
static constexpr const char* SOURCE_FILE_KEY      = "source_file";
static constexpr const char* SOURCE_OBJECT_ID_KEY = "source_object_id";
static constexpr const char* SOURCE_VOLUME_ID_KEY = "source_volume_id";
static constexpr const char* SOURCE_OFFSET_X_KEY  = "source_offset_x";
static constexpr const char* SOURCE_OFFSET_Y_KEY  = "source_offset_y";
static constexpr const char* SOURCE_OFFSET_Z_KEY  = "source_offset_z";
static constexpr const char* SOURCE_IN_INCHES     = "source_in_inches";
static constexpr const char* SOURCE_IN_METERS     = "source_in_meters";

static constexpr const char* MESH_SHARED_KEY = "mesh_shared";

static constexpr const char* MESH_STAT_EDGES_FIXED        = "edges_fixed";
static constexpr const char* MESH_STAT_DEGENERATED_FACETS = "degenerate_facets";
static constexpr const char* MESH_STAT_FACETS_REMOVED     = "facets_removed";
static constexpr const char* MESH_STAT_FACETS_RESERVED    = "facets_reversed";
static constexpr const char* MESH_STAT_BACKWARDS_EDGES    = "backwards_edges";

// Store / load of TextConfiguration
static constexpr const char* TEXT_TAG       = "slic3rpe:text";
static constexpr const char* TEXT_DATA_ATTR = "text";
// TextConfiguration::EmbossStyle
static constexpr const char* STYLE_NAME_ATTR           = "style_name";
static constexpr const char* FONT_DESCRIPTOR_ATTR      = "font_descriptor";
static constexpr const char* FONT_DESCRIPTOR_TYPE_ATTR = "font_descriptor_type";

// TextConfiguration::FontProperty
static constexpr const char* CHAR_GAP_ATTR          = "char_gap";
static constexpr const char* LINE_GAP_ATTR          = "line_gap";
static constexpr const char* LINE_HEIGHT_ATTR       = "line_height";
static constexpr const char* BOLDNESS_ATTR          = "boldness";
static constexpr const char* SKEW_ATTR              = "skew";
static constexpr const char* PER_GLYPH_ATTR         = "per_glyph";
static constexpr const char* HORIZONTAL_ALIGN_ATTR  = "horizontal";
static constexpr const char* VERTICAL_ALIGN_ATTR    = "vertical";
static constexpr const char* COLLECTION_NUMBER_ATTR = "collection";

static constexpr const char* FONT_FAMILY_ATTR    = "family";
static constexpr const char* FONT_FACE_NAME_ATTR = "face_name";
static constexpr const char* FONT_STYLE_ATTR     = "style";
static constexpr const char* FONT_WEIGHT_ATTR    = "weight";

// Store / load of EmbossShape
static constexpr const char* SHAPE_TAG                 = "slic3rpe:shape";
static constexpr const char* SHAPE_SCALE_ATTR          = "scale";
static constexpr const char* UNHEALED_ATTR             = "unhealed";
static constexpr const char* SVG_FILE_PATH_ATTR        = "filepath";
static constexpr const char* SVG_FILE_PATH_IN_3MF_ATTR = "filepath3mf";

// EmbossProjection
static constexpr const char* DEPTH_ATTR       = "depth";
static constexpr const char* USE_SURFACE_ATTR = "use_surface";
// static constexpr const char *FIX_TRANSFORMATION_ATTR = "transform";

const unsigned int BBS_VALID_OBJECT_TYPES_COUNT = 2;
#endif // !CONSTANT_3MF_H
