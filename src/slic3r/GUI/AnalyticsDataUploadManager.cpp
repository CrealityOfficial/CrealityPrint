#include "AnalyticsDataUploadManager.hpp"
#include <future>
#include <thread>
#include <chrono>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>
#include <stdexcept>

// OpenSSL MD5
#include <openssl/md5.h>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "nlohmann/json.hpp"
#include "libslic3r/Time.hpp"
#include "slic3r/GUI/print_manage/data/DataType.hpp"
#include "slic3r/GUI/print_manage/data/DataCenter.hpp"
#include "slic3r/GUI/print_manage/AccountDeviceMgr.hpp"
#include "CrProject.hpp"
#include "libslic3r/Platform.hpp"
#include "slic3r/GUI/SystemId/SystemId.hpp"
#include "slic3r/Utils/Http.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/GUI.hpp"
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <zlib.h>

namespace Slic3r {
namespace GUI {

// 辅助函数：Gzip 压缩
static std::string gzip_compress(const std::string& data)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    
    // 15 + 16 = gzip 格式
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return "";
    }
    
    stream.avail_in = data.size();
    stream.next_in = (Bytef*)data.c_str();
    
    std::string compressed;
    compressed.resize(deflateBound(&stream, data.size()));
    
    stream.avail_out = compressed.size();
    stream.next_out = (Bytef*)compressed.data();
    
    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        return "";
    }
    
    compressed.resize(stream.total_out);
    deflateEnd(&stream);
    
    return compressed;
}

// 辅助函数：Base64 编码
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(const std::string& input)
{
    std::string ret;
    int inlen = input.size();
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.c_str());
    
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    while (inlen--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while (i++ < 3)
            ret += '=';
    }
    
    return ret;
}

// 辅助函数：生成 UUID
static std::string generate_uuid()
{
    static boost::uuids::random_generator generator;
    boost::uuids::uuid uuid = generator();
    return boost::uuids::to_string(uuid);
}

// 获取系统架构信息的辅助函数
std::string get_system_architecture() {
    PlatformFlavor flavor = platform_flavor();
    switch (flavor) {
        case PlatformFlavor::OSXOnX86:
            return "x86_64";
        case PlatformFlavor::OSXOnArm:
            return "arm64";
        default:
            // 对于其他平台，使用编译时检测
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64)
            return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
            return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
            return "x86";
#elif defined(__arm__)
            return "arm";
#else
            return "unknown";
#endif
    }
}

template <typename T>
std::string serialize_with_semicolon(const std::vector<T>& items)
{
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) oss << ";";
        oss << items[i];
    }
    return oss.str();
}


AnalyticsDataUploadManager::AnalyticsDataUploadManager()
{
}

AnalyticsDataUploadManager::~AnalyticsDataUploadManager()
{
}

void AnalyticsDataUploadManager::triggerUploadTasks(AnalyticsUploadTiming triggerTiming, const std::vector<AnalyticsDataEventType>& dataEventTypes, int plate_idx, const std::string& device_mac)
{
    try
    {
        if(wxGetApp().is_privacy_checked()) {
            for (const auto& dataEventType : dataEventTypes) {
                processUploadData(dataEventType, plate_idx, device_mac);
            }
        }
    }
    catch (...)
    {

    }
}



void AnalyticsDataUploadManager::triggerUploadTasksWithPayload(const AnalyticsEventPayload& payload, int plate_idx, const std::string& device_mac)
{
    try
    {
        if (wxGetApp().is_privacy_checked()) {
            nlohmann::json js = payload.data;
            switch (payload.type) {
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ADD:
                uploadModelActionAddEvent();
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ADD_PLATE:
                uploadModelActionAddPlateEvent();
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_MOVE:
                uploadModelActionMoveEvent();
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ROTATE:
                uploadModelActionRotateEvent();
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_AUTO_ORIENT:
                track_model_action("model_action_auto_orient", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ARRANGE_ALL:
                track_model_action("model_action_arrange_all", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_LAY_ON_FACE:
                track_model_action("model_action_lay_on_face", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SPLIT_TO_OBJECTS:
                track_model_action("model_action_split_to_objects", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SPLIT_TO_PARTS:
                track_model_action("model_action_split_to_parts", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SCALE:
                track_model_action("model_action_scale", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_HOLLOW:
                track_model_action("model_action_hollow", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ADD_HOLE:
                track_model_action("model_action_add_hole", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_CUT:
                track_model_action("model_action_cut", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_BOOLEAN:
                track_model_action("model_action_boolean", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_MEASURE:
                track_model_action("model_action_measure", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SUPPORT_PAINT:
                track_model_action("model_action_support_paint", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ZSEAM_PAINT:
                track_model_action("model_action_zseam_paint", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_VARIABLE_LAYER:
                track_model_action("model_action_variable_layer", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_PAINT:
                track_model_action("model_action_paint", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_EMBOSS:
                track_model_action("model_action_emboss", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ASSEMBLY_VIEW:
                track_model_action("model_action_assembly_view", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_GOTO_WIKI:
                track_model_action("goto_wiki", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_AI_SERVICE_CALL:
                track_model_action("ai_service_call", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_GOTO_SUPPORT:
                track_model_action("goto_support", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_TAB_HOME:
                track_model_action("tab_home", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_ONLINE_MODELS:
                track_model_action("tab_online_models", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_PREPARE:
                track_model_action("tab_prepare", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_PREVIEW:
                track_model_action("tab_preview", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_DEVICE:
                track_model_action("tab_device", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_SOFTWARE_CLOSE:
                uploadSoftwareCloseData();
                break;
            case AnalyticsDataEventType::ANALYTICS_SLICE_PLATE:
                uploadSlicePlateEventData();
                break;
            case AnalyticsDataEventType::ANALYTICS_DEVICE_INFO:
                uploadDeviceInfoData();
                break;
            case AnalyticsDataEventType::ANALYTICS_GLOBAL_PRINT_PARAMS:
                uploadGlobalPrintParams(plate_idx, device_mac);
                break;
            case AnalyticsDataEventType::ANALYTICS_OBJECT_PRINT_PARAMS:
                uploadObjectPrintParams(plate_idx, device_mac);
                break;
            case AnalyticsDataEventType::ANALYTICS_BAD_ALLOC:
                uploadSoftwareBadAlloc();
                break;
            case AnalyticsDataEventType::ANALYTICS_SOFTWARE_LAUNCH:
                uploadSoftwareLaunchData();
                break;
            case AnalyticsDataEventType::ANALYTICS_ACCOUNT_DEVICE_INFO:
                uploadAccountDeviceInfoData();
                break;
            case AnalyticsDataEventType::ANALYTICS_SOFTWARE_CRASH:
                uploadSoftwareCrashData();
                break;
            case AnalyticsDataEventType::ANALYTICS_FIRST_LAUNCH:
                uploadFirstLaunchEventData();
                break;
            case AnalyticsDataEventType::ANALYTICS_PREFERENCES_CHANGED:
                uploadPreferencesChangedData();
                break;
            case AnalyticsDataEventType::ANALYTICS_SLICE_SINGLE_COMPLETE:
                track_model_action("slice_single", js);   
                break;
            case AnalyticsDataEventType::ANALYTICS_SLICE_ALL_COMPLETE:
                track_model_action("slice_all", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_PRINT_SEND:
                // Use delayed sending to ensure frontend page is ready
                // Pass by value to ensure data persistence in timer callback
                js.clear();
                track_model_action_delayed_print_send(js);
                break;
            case AnalyticsDataEventType::ANALYTICS_PRINT_BEGIN:
                js.clear();
                track_model_action("print_begin", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_PRINT_ERROR:
                //track_model_action("print_error", js);
                break;
            // Click events
            case AnalyticsDataEventType::ANALYTICS_CLICK_SEND_SINGLE:
                track_model_action("click_send_single", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_CLICK_SEND_MULTI:
                track_model_action("click_send_multi", js);
                break;
            // File project events
            case AnalyticsDataEventType::ANALYTICS_FILE_PROJECT_NEW:
                track_model_action("file_project_new", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_FILE_PROJECT_OPEN:
                track_model_action("file_project_open", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_FILE_PROJECT_SAVE:
                track_model_action("file_project_save", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_FILE_PROJECT_SAVE_AS:
                track_model_action("file_project_save_as", js);
                break;
            // File model events
            case AnalyticsDataEventType::ANALYTICS_FILE_IMPORT_MODEL:
                track_model_action("file_import_model", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_FILE_EXPORT_MODEL:
                track_model_action("file_export_model", js);
                break;
            // File preset events
            case AnalyticsDataEventType::ANALYTICS_FILE_IMPORT_PRESET:
                track_model_action("file_import_preset", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_FILE_EXPORT_PRESET:
                track_model_action("file_export_preset", js);
                break;
            // File GCode events
            case AnalyticsDataEventType::ANALYTICS_FILE_EXPORT_GCODE_SINGLE:
                track_model_action("file_export_gcode_single", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_FILE_EXPORT_GCODE_ALL:
                track_model_action("file_export_gcode_all", js);
                break;
            // Model action events
            case AnalyticsDataEventType::ANALYTICS_MODEL_BOOLEAN:
                track_model_action("model_boolean", js);
                break;
            // OTA update events
            case AnalyticsDataEventType::ANALYTICS_OTA_DOWNLOAD_START:
                BOOST_LOG_TRIVIAL(warning) << "[OTA_ANALYTICS] ota_download_start triggered, data=" << js.dump();
                track_model_action("ota_download_start", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_OTA_DOWNLOAD_CANCEL:
                BOOST_LOG_TRIVIAL(warning) << "[OTA_ANALYTICS] ota_download_cancel triggered, data=" << js.dump();
                track_model_action("ota_download_cancel", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_OTA_UPDATE_START:
                BOOST_LOG_TRIVIAL(warning) << "[OTA_ANALYTICS] ota_update_start triggered, data=" << js.dump();
                track_model_action("ota_update_start", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_OTA_UPDATE_CANCEL:
                BOOST_LOG_TRIVIAL(warning) << "[OTA_ANALYTICS] ota_update_cancel triggered, data=" << js.dump();
                track_model_action("ota_update_cancel", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_OTA_UPDATE_SUCCESS:
                BOOST_LOG_TRIVIAL(warning) << "[OTA_ANALYTICS] ota_update_success triggered, data=" << js.dump();
                track_model_action("ota_update_success", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_OTA_UPDATE_FAIL:
                BOOST_LOG_TRIVIAL(warning) << "[OTA_ANALYTICS] ota_update_fail triggered, data=" << js.dump();
                track_model_action("ota_update_fail", js);
                break;
            case AnalyticsDataEventType::ANALYTICS_PRINT_VIDEO_CONNECT:
                track_model_action("print_video_connect", js);
                break;
            default:
                break;
            }
            boost::log::core::get()->flush();
        }
    }
    catch (...)
    {

    }
}

void AnalyticsDataUploadManager::mark_analytics_project_info(const std::string& full_url,
                                            const std::string& model_id,
                                            const std::string& file_id,
                                            const std::string& file_format,
                                            const std::string& name)
{
    m_analytics_project_info.url = full_url;
    m_analytics_project_info.model_id = model_id;
    m_analytics_project_info.file_id = file_id;
    // file_format 仅非空时覆盖（主线程同步设定的值不被异步回调的空串覆盖）
    if (!file_format.empty()) {
        m_analytics_project_info.file_format = file_format;
    }
    m_analytics_project_info.name = name;
}

void AnalyticsDataUploadManager::set_analytics_project_info_valid(bool valid)
{
    m_analytics_project_info.is_valid = valid;
}

void AnalyticsDataUploadManager::clear_analytics_project_info()
{
    m_analytics_project_info.url = "";
    m_analytics_project_info.model_id = "";
    m_analytics_project_info.file_id = "";
    m_analytics_project_info.file_format = "";
    m_analytics_project_info.name = "";

    m_analytics_project_info.is_valid = false;
}

void AnalyticsDataUploadManager::processUploadData(AnalyticsDataEventType dataEventType, int plate_idx, const std::string& device_mac)
{
#if AUTO_CONVERT_3MF
    return;
#endif
    switch (dataEventType)
    {
    case AnalyticsDataEventType::ANALYTICS_GLOBAL_PRINT_PARAMS:
        uploadGlobalPrintParams(plate_idx, device_mac);
        break;
    case AnalyticsDataEventType::ANALYTICS_OBJECT_PRINT_PARAMS:
        uploadObjectPrintParams(plate_idx, device_mac);
        break;
    case AnalyticsDataEventType::ANALYTICS_SLICE_PLATE:
        uploadSlicePlateEventData();
        break;
    case AnalyticsDataEventType::ANALYTICS_FIRST_LAUNCH:
        uploadFirstLaunchEventData();
        break;
    case AnalyticsDataEventType::ANALYTICS_PREFERENCES_CHANGED:
        uploadPreferencesChangedData();
        break;
    case AnalyticsDataEventType::ANALYTICS_SOFTWARE_LAUNCH:
        uploadSoftwareLaunchData();
        break;
    case AnalyticsDataEventType::ANALYTICS_SOFTWARE_CRASH:
        uploadSoftwareCrashData();
        break;
    case AnalyticsDataEventType::ANALYTICS_BAD_ALLOC:
        uploadSoftwareBadAlloc();
        break;
    case AnalyticsDataEventType::ANALYTICS_SOFTWARE_CLOSE:
        uploadSoftwareCloseData();
        break;
    case AnalyticsDataEventType::ANALYTICS_DEVICE_INFO:
        uploadDeviceInfoData();
        break;
    case AnalyticsDataEventType::ANALYTICS_ACCOUNT_DEVICE_INFO:
        uploadAccountDeviceInfoData();
        break;
    case AnalyticsDataEventType::ANALYTICS_ONLINE_MODELS:
        uploadOnlineModelsEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_PREPARE:
        uploadPrepareEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_PREVIEW:
        uploadPreviewEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_DEVICE:
        uploadDeviceEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_PROJECTS:
        uploadClickHomePageProjectsEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_ONLINE_PARAMS:
        uploadClickHomePageOnlineParamsEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_TUTORIALS:
        uploadClickHomePageTutorialsEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_PERSON_CENTER:
        uploadClickHomePagePersonCenterEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_FEEDBACK:
        uploadClickHomePageFeedbackEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_MAKENOW:
        uploadClickHomePageMakenowEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_CLICK_HOME_PAGE_CREALITYMALL:
        uploadClickHomePageCrealitymallEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ADD:
        uploadModelActionAddEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ADD_PLATE:
        uploadModelActionAddPlateEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_MOVE:
        uploadModelActionMoveEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ROTATE:
        uploadModelActionRotateEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_AUTO_ORIENT:
        uploadModelActionAutoOrientEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ARRANGE_ALL:
        uploadModelActionArrangeAllEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_LAY_ON_FACE:
        uploadModelActionLayOnFaceEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SPLIT_TO_OBJECTS:
        uploadModelActionSplitToObjectsEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SPLIT_TO_PARTS:
        uploadModelActionSplitToPartsEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SCALE:
        uploadModelActionScaleEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_HOLLOW:
        uploadModelActionHollowEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ADD_HOLE:
        uploadModelActionAddHoleEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_CUT:
        uploadModelActionCutEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_BOOLEAN:
        uploadModelActionBooleanEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_MEASURE:
        uploadModelActionMeasureEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_SUPPORT_PAINT:
        uploadModelActionSupportPaintEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ZSEAM_PAINT:
        uploadModelActionZseamPaintEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_VARIABLE_LAYER:
        uploadModelActionVariableLayerEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_PAINT:
        uploadModelActionPaintEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_EMBOSS:
        uploadModelActionEmbossEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_MODEL_ACTION_ASSEMBLY_VIEW:
        uploadModelActionAssemblyViewEvent();
        break;
    case AnalyticsDataEventType::ANALYTICS_AI_SERVICE_CALL:
        uploadAiServiceCallEvent();
        break;
    default:
        break;
    }
}

void AnalyticsDataUploadManager::uploadGlobalPrintParams(int plate_idx, const std::string& device_mac)
{
    Plater* plater = wxGetApp().plater();
    PartPlateList& plate_list = plater->get_partplate_list();
    PartPlate* plate = plate_list.get_plate(plate_idx);
    if(!plate)
        return;

    ModelObjectPtrs objs_on_plate = plate->get_objects_on_this_plate();
    if(objs_on_plate.size() <= 0)
        return;

    const Print& plate_print = plate->get_print();

    const DynamicPrintConfig& print_full_config = plate_print.full_print_config();

    Slic3r::AuxiliariesInfo auxiliaries_info = plater->get_auxiliaries_info();

    try
    {
        json js;

        js["printer_mac"]   = device_mac;
        //js["printer_mac"] = "FCEE2806B0DB"; // 测试时用的数据

        // according to document description
        js["type_code"] = "slice801";

        js["printer_model"] = print_full_config.opt_serialize("printer_model");
        js["print_uuid"] = plate_print.get_print_uuid();

        js["nozzle_type"] = print_full_config.opt_serialize("nozzle_type");

        js["curr_bed_type"] = print_full_config.opt_serialize("curr_bed_type");
        js["filament_colour"] = print_full_config.opt_serialize("filament_colour");

        float koef = wxGetApp().app_config->get("use_inches") == "1" ? GizmoObjectManipulation::in_to_mm : 1000.0;
        std::ostringstream ss1;
        ss1 << std::fixed << std::setprecision(2) << plate_print.print_statistics().total_used_filament / koef;
        js["total_material_length"] = ss1.str();
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << plate_print.print_statistics().total_weight;
        js["total_filament_cost"] = ss.str();

        wxString print_time = wxString::Format("%s", short_time(get_time_dhms(plate->get_slice_result()->print_statistics.modes[0].model_time_s())));
        js["print_estimated_duration"] = print_time.ToStdString();

        //js["slice_preview_duration"] = "";

        js["slice_layer_count"] = wxString::Format("%d",plate_print.print_statistics().total_layer_count).ToStdString();

        std::vector<int> obj_loaded_ids;

        for (ModelObject* mo : objs_on_plate) {
            obj_loaded_ids.push_back(mo->from_loaded_id);
        }

        js["object_ids"] = serialize_with_semicolon(obj_loaded_ids);

        js["layer_height"] = print_full_config.opt_serialize("layer_height");
        js["initial_layer_print_height"]   = print_full_config.opt_serialize("initial_layer_print_height");
        js["curr_bed_type"] = print_full_config.opt_serialize("curr_bed_type");
        js["wall_loops"] = print_full_config.opt_serialize("wall_loops");

        js["sparse_infill_density"] = print_full_config.opt_serialize("sparse_infill_density");
        js["sparse_infill_pattern"] = print_full_config.opt_serialize("sparse_infill_pattern");
        js["enable_support"] = print_full_config.opt_serialize("enable_support");
        js["support_type"]          = print_full_config.opt_serialize("support_type");
        js["support_style"] = print_full_config.opt_serialize("support_style");
        js["support_threshold_angle"] = print_full_config.opt_serialize("support_threshold_angle");
        js["fan_min_speed"] = print_full_config.opt_serialize("fan_min_speed");
        js["fan_cooling_layer_time"]  = print_full_config.opt_serialize("fan_cooling_layer_time");
        js["fan_max_speed"]           = print_full_config.opt_serialize("fan_max_speed");
        js["slow_down_layer_time"]    = print_full_config.opt_serialize("slow_down_layer_time");
        js["close_fan_the_first_x_layers"] = print_full_config.opt_serialize("close_fan_the_first_x_layers");
        js["full_fan_speed_layer"]         = print_full_config.opt_serialize("full_fan_speed_layer");
        js["filament_type"] = print_full_config.opt_serialize("filament_type");
        js["filament_diameter"]            = print_full_config.opt_serialize("filament_diameter");
        js["default_filament_colour"]            = print_full_config.opt_serialize("default_filament_colour");
        js["default_filament_profile"]            = print_full_config.opt_serialize("default_filament_profile");
        js["default_filament_type"]            = print_full_config.opt_serialize("default_filament_type");

        //js["file_format"] = "";
        //js["source_path"] = "";
        //js["user_id"] = "";
        //js["collect_id"] = "";

        if(m_analytics_project_info.is_valid) {
            js["cloud_url"] = m_analytics_project_info.url;
            js["cloud_file_id"] = m_analytics_project_info.file_id;
            js["cloud_file_format"] = m_analytics_project_info.file_format;
            js["cloud_model_id"] = m_analytics_project_info.model_id;
            js["cloud_name"] = m_analytics_project_info.name;
        }

        js["application"] = auxiliaries_info.get_metadata_application();
        js["platform"] = auxiliaries_info.get_metadata_platform();
        js["projectInfoId"] = auxiliaries_info.get_metadata_project_id();

        js["operation_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
        js["app_version"] = GUI_App::format_display_version().c_str();
        js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();

        track_model_action("print_global_parameters", js);
    }
    catch (const std::exception& err)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": json create " << " got a generic exception, reason = " << err.what();
    } 
    catch (...) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": json create " << " got an unknown exception";
    }

}

void AnalyticsDataUploadManager::uploadObjectPrintParams(int plate_idx,const std::string& device_mac)
{
    Plater* plater = wxGetApp().plater();
    PartPlateList& plate_list = plater->get_partplate_list();
    PartPlate* plate = plate_list.get_plate(plate_idx);
    if(!plate)
        return;

    ModelObjectPtrs objs_on_plate = plate->get_objects_on_this_plate();
    if(objs_on_plate.size() <= 0)
        return;

    const Print& plate_print = plate->get_print();

    const DynamicPrintConfig& print_full_config = plate_print.full_print_config();

    try
    {
        json js;

        js["printer_mac"]   = device_mac;
        //js["printer_mac"] = "FCEE2806B0DB";  // 测试时用的数据

        // according to document description
        js["type_code"] = "slice802";

        js["printer_model"] = print_full_config.opt_serialize("printer_model");
        js["print_uuid"] = plate_print.get_print_uuid();

        std::vector<int> obj_loaded_ids;

        std::vector<std::map<std::string, std::string>> obj_options;
        std::set<std::string> all_keys;

        for (ModelObject* mo : objs_on_plate) {
            obj_loaded_ids.push_back(mo->from_loaded_id);
            ModelConfigObject& obj_config = mo->config;

            for (const std::string& opt_key : obj_config.keys()) {
                all_keys.insert(opt_key); // std::set 自动去重
            }

        }

        js["object_ids"] = serialize_with_semicolon(obj_loaded_ids);

        //js["object_enable_support"] = serialize_with_comma(obj_enable_supports);
        // 2. 对每个 key，收集所有对象的值，拼接成 "v1;v2;v3;"
        for (const std::string& key : all_keys) {
            std::string joined;
            for (size_t i = 0; i < objs_on_plate.size(); ++i) {
                ModelConfigObject& obj_config = objs_on_plate[i]->config;
                if (obj_config.has(key))
                    joined += obj_config.opt_serialize(key);
                // 即使没有也要占位
                joined += ";";
            }
            js[key] = joined;
        }

        if(m_analytics_project_info.is_valid) {
            js["cloud_url"] = m_analytics_project_info.url;
            js["cloud_file_id"] = m_analytics_project_info.file_id;
            js["cloud_file_format"] = m_analytics_project_info.file_format;
            js["cloud_model_id"] = m_analytics_project_info.model_id;
            js["cloud_name"] = m_analytics_project_info.name;
        }

        js["operation_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
        js["app_version"] = GUI_App::format_display_version().c_str();
        js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();

        track_model_action("object_print_parameters", js);
    }
    catch (const std::exception& err)
    {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__<< ": json create " << " got a generic exception, reason = " << err.what();
    } 
    catch (...) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": json create " << " got an unknown exception";
    }
}

void AnalyticsDataUploadManager::uploadSlicePlateEventData()
{
     Plater* plater = wxGetApp().plater();
    Slic3r::AuxiliariesInfo auxiliaries_info = plater->get_auxiliaries_info();

    // only report cubeme project 3mf slice event
    if(auxiliaries_info.get_metadata_application().empty() || auxiliaries_info.get_metadata_project_id().empty())
        return;

    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " cubeme_slice_event";

    json js;
    js["type_code"] = "slice821";
    js["client_id"] = wxGetApp().get_client_id();

    js["application"] = auxiliaries_info.get_metadata_application();
    js["platform"] = auxiliaries_info.get_metadata_platform();
    js["projectInfoId"] = auxiliaries_info.get_metadata_project_id();

    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();
    track_model_action("cubeme_slice_event", js);
}

void AnalyticsDataUploadManager::uploadSoftwareLaunchData()
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";

    json js;
    js["type_code"] = "slice806";

    js["client_id"] = wxGetApp().get_client_id();
    
    js["startup_duration"] = wxGetApp().get_app_startup_duration();
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();
    js["launch_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());

    track_model_action("software_launch", js);
}

void AnalyticsDataUploadManager::uploadSoftwareCrashData()
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";
    json js;
    js["type_code"] = "slice807";
    js["client_id"] = wxGetApp().get_client_id();
    
    js["send_crash_report"] = wxGetApp().get_send_crash_report();
    js["category"] = "client_crash";
    js["action"]   = "show_error_report";
    js["label"]    = "crash_report_dialog";
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();
    js["system_architecture"] = get_system_architecture();
    js["crash_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());

    track_model_action("software_crash", js);
}

void AnalyticsDataUploadManager::uploadSoftwareBadAlloc()
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";
    json js;
    js["type_code"] = "slice820";
    js["client_id"] = wxGetApp().get_client_id();

    js["send_crash_report"] = wxGetApp().get_send_crash_report();
    js["category"]          = "client_crash";
    js["action"]            = "show_error_report";
    js["label"]             = "crash_report_dialog";
    js["app_version"]       = GUI_App::format_display_version().c_str();
    js["operating_system"]  = wxGetOsDescription().ToStdString().c_str();
    js["crash_date"]        = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());

    track_model_action("software_bad_alloc", js);
}


void AnalyticsDataUploadManager::uploadSoftwareCloseData()
{
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " start";
    json js;
    js["type_code"] = "slice808";
    js["client_id"] = wxGetApp().get_client_id();

    js["usage_duration"] = wxGetApp().get_app_running_duration(); // in minutes
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();
    js["close_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());

    track_model_action("software_close", js);
    BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << " end";
    boost::log::core::get()->flush();
}

void AnalyticsDataUploadManager::uploadDeviceInfoData()
{
    std::vector<std::string> all_macs = wxGetApp().mainframe->get_printer_mgr_view()->get_all_device_macs();
    if (all_macs.empty())
        return;

    // 统计每种modelName的数量
    std::map<std::string, int> model_count;
    for(const auto& mac : all_macs) {
        nlohmann::json printer_json = DM::DataCenter::Ins().find_printer_by_mac(mac);
        if (!printer_json.is_null()) {
            DM::Device device = DM::Device::deserialize(printer_json);
            if (!device.modelName.empty())
                model_count[device.modelName]++;
        }
    }

    if (model_count.empty())
        return;

    // only upload for one time
    wxGetApp().mainframe->get_printer_mgr_view()->set_finish_upload_device_state(true);

    std::string user_login_id = "";
    if (Slic3r::GUI::wxGetApp().is_login()) {
        Slic3r::GUI::UserInfo user = Slic3r::GUI::wxGetApp().get_user();
        user_login_id = user.userId;
    }

    json js;
    js["type_code"] = "slice812";

    js["client_id"] = wxGetApp().get_client_id();

    // 拼接成 [modelName,count];[modelName,count]; 格式
    std::string device_infos;
    for (const auto& kv : model_count) {
        device_infos += "[" + kv.first + "," + std::to_string(kv.second) + "];";
    }

    js["printer_infos"] = device_infos;

    js["app_version"] = GUI_App::format_display_version().c_str();
    //js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();

    track_model_action("device_info", js);
}

void AnalyticsDataUploadManager::uploadAccountDeviceInfoData()
{
    std::string user_account_id = "";
    if (!Slic3r::GUI::wxGetApp().is_login()) {
        return;
    }
    else {
        Slic3r::GUI::UserInfo user = Slic3r::GUI::wxGetApp().get_user();
        user_account_id = user.userId;
    }

    // 获取当前账号下的设备信息
    const AccountDeviceMgr::AccountDeviceInfo& account_device_info = AccountDeviceMgr::getInstance().get_account_device_info();
    auto account_it = account_device_info.account_infos.find(user_account_id);
    if (account_it == account_device_info.account_infos.end()) {
        return ;
    }

    std::map<std::string, int> model_count;

    auto it = account_device_info.account_infos.find(user_account_id);
    if (it != account_device_info.account_infos.end()) {
        const auto& devices = it->second.my_devices;
        for (const auto& device : devices) {
            if (!device.model.empty())
                model_count[device.model]++;
        }
    }

    if(model_count.empty())
        return;
        
    json js;
    js["type_code"] = "slice812";

    js["client_id"] = wxGetApp().get_client_id();

    js["user_login_id"] = user_account_id;

    // 拼接成 [model,count],[model,count] 格式
    std::string device_infos;
    for (auto iter = model_count.begin(); iter != model_count.end(); ++iter) {
        if (iter != model_count.begin()) device_infos += ",";
        device_infos += "[" + iter->first + "," + std::to_string(iter->second) + "]";
    }
    js["printer_infos"] = device_infos;

    js["app_version"] = GUI_App::format_display_version().c_str();
    //js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();

    track_model_action("device_info", js);
}

// software first launch (when "AppData\Roaming\Creality" directory first created)
void AnalyticsDataUploadManager::uploadFirstLaunchEventData()
{
    json js;
    js["type_code"] = "slice804";

    js["client_id"] = wxGetApp().get_client_id();

    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"]      = wxGetOsDescription().ToStdString().c_str();
    js["launch_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());

    track_model_action("software_first_launch", js);
}

void AnalyticsDataUploadManager::uploadPreferencesChangedData()
{
    json js;
    js["type_code"] = "slice805";

    js["dark_color_mode"] = wxGetApp().app_config->get("dark_color_mode");
    js["language"] = wxGetApp().app_config->get("language");
    js["region"] = wxGetApp().app_config->get("region");
    js["use_inches"] = wxGetApp().app_config->get("use_inches");
    js["download_path"] = wxGetApp().app_config->get("download_path");
    js["zoom_to_mouse"] = wxGetApp().app_config->get("zoom_to_mouse");
    js["is_arrange"] = wxGetApp().app_config->get("is_arrange");
    js["user_mode"] = wxGetApp().app_config->get("user_mode");
    js["default_page"] = wxGetApp().app_config->get("default_page");
    js["sync_user_preset"] = wxGetApp().app_config->get("sync_user_preset");
    js["user_exp"] = wxGetApp().app_config->get("user_exp");
    js["save_preset_choise"] = wxGetApp().app_config->get("save_preset_choise");
    js["save_project_choise"] = wxGetApp().app_config->get("save_project_choise");
    js["operation_date"] = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());
    js["lod_preparation"] = wxGetApp().app_config->get("enable_lod");
    js["lod_preview"] = wxGetApp().app_config->get("enable_preview_lod");

    track_model_action("preferences_changed", js);
}

void AnalyticsDataUploadManager::uploadOnlineModelsEvent()
{
    json js;
    track_model_action("tab_online_models", js);
}

void AnalyticsDataUploadManager::uploadPrepareEvent()
{
    json js;
    track_model_action("tab_prepare", js);
}

void AnalyticsDataUploadManager::uploadPreviewEvent()
{
    json js;
    track_model_action("tab_preview", js);
}

void AnalyticsDataUploadManager::uploadDeviceEvent()
{
    json js;
    track_model_action("tab_device", js);
}

void AnalyticsDataUploadManager::uploadClickHomePageProjectsEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_projects", js);
}

void AnalyticsDataUploadManager::uploadClickHomePageOnlineParamsEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_online_params", js);
}

void AnalyticsDataUploadManager::uploadClickHomePageTutorialsEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_tutorials", js);
}

void AnalyticsDataUploadManager::uploadClickHomePagePersonCenterEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_person_center", js);
}

void AnalyticsDataUploadManager::uploadClickHomePageFeedbackEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_feedback", js);
}

void AnalyticsDataUploadManager::uploadClickHomePageMakenowEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_makenow", js);
}

void AnalyticsDataUploadManager::uploadClickHomePageCrealitymallEvent()
{
    json js;
    js["app_version"] = GUI_App::format_display_version().c_str();
    js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    track_model_action("click_home_page_crealitymall", js);
}

void AnalyticsDataUploadManager::track_model_action(const std::string& event_name, nlohmann::json& js)
{
    if (js.find("app_version") == js.end())
        js["app_version"] = GUI_App::format_display_version().c_str();
    if (js.find("operating_system") == js.end())
        js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    if (js.find("device_id") == js.end() || js.find("client_id") == js.end()) {
        const std::string system_id = SystemId::get_system_id();
        if (js.find("device_id") == js.end())
            js["device_id"] = system_id;
        if (js.find("client_id") == js.end())
            js["client_id"] = system_id;
    }
    if (js.find("user_id") == js.end())
        js["user_id"] = wxGetApp().get_user().userId;
    wxGetApp().track_event(event_name, js.dump());
}

void AnalyticsDataUploadManager::track_model_action_delayed_print_send(const nlohmann::json& js)
{
    // Serialize JSON to string - this is safe and persists across timer callbacks
    const std::string json_str = js.dump();

    // Use wxTimer to delay execution by 2 seconds without blocking
    wxTimer* timer = new wxTimer();

    // IMPORTANT: Only capture the serialized string (JSON object may become invalid in lambda)
    timer->Bind(wxEVT_TIMER, [this, timer, json_str](wxTimerEvent&) {
        try {
            // Re-parse JSON from string - this creates a fresh, valid JSON object
            nlohmann::json js_parsed = nlohmann::json::parse(json_str);

            // Call the actual handler with freshly parsed data
            on_delayed_print_send_timer(js_parsed);
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " Failed to parse JSON: " << e.what();
        } catch (...) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " Unknown exception while parsing JSON";
        }

                // Delete timer after use to prevent memory leak
        timer->DeletePendingEvents();
        delete timer;
    });

    // Start one-shot timer for 2 seconds
    timer->Start(2000, wxTIMER_ONE_SHOT);
}

void AnalyticsDataUploadManager::on_delayed_print_send_timer(nlohmann::json js)
{
    // Add metadata and send
    if (js.find("app_version") == js.end())
        js["app_version"] = GUI_App::format_display_version().c_str();
    if (js.find("operating_system") == js.end())
        js["operating_system"] = wxGetOsDescription().ToStdString().c_str();
    if (js.find("device_id") == js.end() || js.find("client_id") == js.end()) {
        const std::string system_id = SystemId::get_system_id();
        if (js.find("device_id") == js.end())
            js["device_id"] = system_id;
        if (js.find("client_id") == js.end())
            js["client_id"] = system_id;
    }
    if (js.find("user_id") == js.end())
        js["user_id"] = wxGetApp().get_user().userId;

    wxGetApp().track_event("print_send", js.dump());
}

void AnalyticsDataUploadManager::uploadModelActionAddEvent()
{
    json js;
    //track_model_action("model_action_add", js);
}

void AnalyticsDataUploadManager::uploadModelActionAddPlateEvent()
{
    json js;
    //track_model_action("model_action_add_plate", js);
}

void AnalyticsDataUploadManager::uploadModelActionMoveEvent()
{
    json js;
    //track_model_action("model_action_move", js);
}

void AnalyticsDataUploadManager::uploadModelActionRotateEvent()
{
    json js;
    //track_model_action("model_action_rotate", js);
}

void AnalyticsDataUploadManager::uploadModelActionAutoOrientEvent()
{
    json js;
    track_model_action("model_action_auto_orient", js);
}

void AnalyticsDataUploadManager::uploadModelActionArrangeAllEvent()
{
    json js;
    track_model_action("model_action_arrange_all", js);
}

void AnalyticsDataUploadManager::uploadModelActionLayOnFaceEvent()
{
    json js;
    track_model_action("model_action_lay_on_face", js);
}

void AnalyticsDataUploadManager::uploadModelActionSplitToObjectsEvent()
{
    json js;
    track_model_action("model_action_split_to_objects", js);
}

void AnalyticsDataUploadManager::uploadModelActionSplitToPartsEvent()
{
    json js;
    track_model_action("model_action_split_to_parts", js);
}

void AnalyticsDataUploadManager::uploadModelActionScaleEvent()
{
    json js;
    track_model_action("model_action_scale", js);
}

void AnalyticsDataUploadManager::uploadModelActionHollowEvent()
{
    json js;
    track_model_action("model_action_hollow", js);
}

void AnalyticsDataUploadManager::uploadModelActionAddHoleEvent()
{
    json js;
    track_model_action("model_action_add_hole", js);
}

void AnalyticsDataUploadManager::uploadModelActionCutEvent()
{
    json js;
    track_model_action("model_action_cut", js);
}

void AnalyticsDataUploadManager::uploadModelActionBooleanEvent()
{
    json js;
    track_model_action("model_action_boolean", js);
}

void AnalyticsDataUploadManager::uploadModelActionMeasureEvent()
{
    json js;
    track_model_action("model_action_measure", js);
}

void AnalyticsDataUploadManager::uploadModelActionSupportPaintEvent()
{
    json js;
    track_model_action("model_action_support_paint", js);
}

void AnalyticsDataUploadManager::uploadModelActionZseamPaintEvent()
{
    json js;
    track_model_action("model_action_zseam_paint", js);
}

void AnalyticsDataUploadManager::uploadModelActionVariableLayerEvent()
{
    json js;
    track_model_action("model_action_variable_layer", js);
}

void AnalyticsDataUploadManager::uploadModelActionPaintEvent()
{
    json js;
    track_model_action("model_action_paint", js);
}

void AnalyticsDataUploadManager::uploadModelActionEmbossEvent()
{
    json js;
    track_model_action("model_action_emboss", js);
}

void AnalyticsDataUploadManager::uploadModelActionAssemblyViewEvent()
{
    json js;
    track_model_action("model_action_assembly_view", js);
}

void AnalyticsDataUploadManager::uploadAiServiceCallEvent()
{
    json js;
    track_model_action("ai_service_call", js);
}

void AnalyticsDataUploadManager::uploadSlice822ClickEvent(const std::string& module, int id)
{
    try {
        nlohmann::json payload;
        payload["type_code"] = "slice822";
        payload["event_type"]      = "click_event";
        payload["function_module"] = module;
        payload["module_id"]       = id;
        payload["app_version"]     = GUI_App::format_display_version().c_str();
        payload["operating_system"] = wxGetOsDescription().ToStdString().c_str();
        payload["timestamp"]       = Slic3r::Utils::utc_timestamp(Slic3r::Utils::get_current_time_utc());

        //nlohmann::json root;
        wxGetApp().track_event("click_event", payload.dump());
    } catch (const std::exception& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": json create got a generic exception, reason = " << err.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": json create got an unknown exception";
    }
}

// ============================================================
// 创想云神策埋点上报接口实现（新系统 - 独立区域）
// ============================================================

bool AnalyticsDataUploadManager::should_send_print_event() const
{
    // 统一入口：封装所有不上报条件，后续新增条件在此追加。
    
    // 模型被本质修改后不上报。
    auto& tracker = ProjectModificationTracker::getInstance();
    if (tracker.is_essentially_modified()) {
        return false;
    }

    return true;
}

void AnalyticsDataUploadManager::send_print_send_event(int plate_idx)
{
    // 统一的发送条件判断
    if (!should_send_print_event()) {
        return;
    }
    
    // 先快照主线程数据（避免后台线程读取时主线程修改）
    std::string snapshot_model_id = m_analytics_project_info.model_id;
    std::string snapshot_file_format = m_analytics_project_info.file_format;

    // 全部放入后台线程执行，不阻塞 UI
    std::thread([this, snapshot_model_id, plate_idx]() {
        try {
            
            // 1. 内部可直接获取的参数
            auto now = std::chrono::system_clock::now();
            long long timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            
            std::string collect_id = generate_uuid();
            std::string device_id = SystemId::get_system_id();
            std::string app_version = GUI_App::format_display_version().c_str();
            int app_type = 6;  // Creality Print
            
            // 2. 根据传入的 plate_idx 从缓存读取所有参数
            auto& tracker = ProjectModificationTracker::getInstance();  // 子线程内重新获取单例

            std::string printer_info_str = tracker.get_printer_info(0);   // 取第0盘的 printer_info
            std::string filament_info_str = tracker.get_filament_info(0); // 取第0盘的 filament_info
            std::string slice_param_str;
            if (plate_idx == PLATE_ALL_IDX) {
                // 导出全部GCode场景：合并所有盘的 objects + plates
                // 盘数通过 m_slice_param 的缓存项数量推断（每盘切完后都会 cache）
                int plate_count = static_cast<int>(tracker.get_cached_plate_count());
                slice_param_str = tracker.get_merged_slice_param(plate_count);
            } else {
                printer_info_str = tracker.get_printer_info(plate_idx);
                filament_info_str = tracker.get_filament_info(plate_idx);
                slice_param_str = tracker.get_slice_param(plate_idx);
            }

            // 使用切片时预生成的 task_id（导出全部时取盘0的，与 gcode 中写入的一致）
            // 需求格式：{device_id}_{timestamp}，不含盘索引
            int task_plate_idx = (plate_idx == PLATE_ALL_IDX) ? 0 : plate_idx;
            std::string task_id = tracker.get_plate_task_id(task_plate_idx);
            if (task_id.empty()) {
                // 兼容：如果没有预生成，则降级为动态生成
                task_id = device_id + "_" + std::to_string(timestamp_ms);
            }
            // 掐掉末尾的盘索引后缀 _XX，对齐需求 {device_id}_{timestamp}
            {
                auto pos = task_id.rfind('_');
                if (pos != std::string::npos && pos + 1 < task_id.size()) {
                    std::string suffix = task_id.substr(pos + 1);
                    if (suffix.size() == 2 && std::isdigit((unsigned char)suffix[0]) && std::isdigit((unsigned char)suffix[1])) {
                        task_id = task_id.substr(0, pos);
                    }
                }
            }
            
            // 3. 生成 file_md5（使用已计算的 model_id 作为文件指纹）
            std::string model_id = snapshot_model_id;
            std::string file_md5 = model_id;
            
            // 4. 构建 properties（核心业务数据）
            nlohmann::json properties;
            properties["collect_id"] = collect_id;
            // event_key 按文件格式映射：3mf→print_002，其他→print_001（后续扩展改此表）
            {
                static const std::map<std::string, std::string> EVENT_KEY_MAP = {
                    {"3mf", "print_002"},
                };
                auto it = EVENT_KEY_MAP.find(m_analytics_project_info.file_format);
                properties["event_key"] = (it != EVENT_KEY_MAP.end()) ? it->second : "print_001";
            }
            properties["task_id"] = task_id;
            properties["event_time"] = timestamp_ms;
            properties["app_type"] = app_type;
            properties["app_version"] = app_version;
            properties["device_id"] = device_id;
            properties["file_md5"] = file_md5;
            
            // 4.1 自动获取 uid
            std::string user_id = "";
            try {
                user_id = wxGetApp().get_user().userId;
            } catch (...) {
                // 未登录或获取失败，保持空字符串
            }
            properties["uid"] = user_id;
            
            // 【删除】以下字段是谷歌分析上报使用的，不是创想云一级字段
            // properties["printer"] = "";       // ❌ 谷歌字段
            // properties["format"] = "GCode";   // ❌ 谷歌字段
            // properties["network"] = "Local";   // ❌ 谷歌字段
            // properties["entry"] = "ExportGCode"; // ❌ 谷歌字段
            // properties["error_code"] = "OK";   // ❌ 谷歌字段
            
            // 5. Add printer_info, filament_info, slice_param from cache (all parsed as native JSON objects)
            if (!printer_info_str.empty()) {
                try {
                    properties["printer_info"] = nlohmann::json::parse(printer_info_str);
                } catch (...) {
                    properties["printer_info"] = printer_info_str;
                }
            }
            if (!filament_info_str.empty()) {
                try {
                    properties["filament_info"] = nlohmann::json::parse(filament_info_str);
                } catch (...) {
                    properties["filament_info"] = filament_info_str;
                }
            }
            if (!slice_param_str.empty()) {
                // Parse as native JSON object (not string), avoid \" double-escaping during outer dump()
                try {
                    properties["slice_param"] = nlohmann::json::parse(slice_param_str);
                } catch (...) {
                    // Fallback to string on parse failure, ensure data is not lost
                    properties["slice_param"] = slice_param_str;
                }
            }
            
            // 6. 构建完整的 payload
            nlohmann::json payload;
            payload["event"] = "data_center_event";
            payload["time"] = timestamp_ms;
            payload["distinct_id"] = device_id;
            payload["properties"] = properties;

            // 8. 发送数据到神策
            send_sensors_payload_to_creality(payload);
            
        } catch (const std::exception& err) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Exception: " << err.what();
        } catch (...) {
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Unknown exception";
        }
    }).detach();  // 后台线程执行，立即返回
}

void AnalyticsDataUploadManager::init_sensors_config_if_needed()
{
    if (m_sensors_config_initialized) {
        return;
    }
    
    // 1. 获取版本类型和地区
    std::string version_type = get_vertion_type();
    std::string country_code = wxGetApp().app_config->get_country_code();
    
    // 2. 选择对应的上报 URL
    if (version_type == "Alpha" || version_type == "Beta") {
        m_sensors_upload_url = "https://api-dev.crealitycloud.cn/api/rest/bicollector/front/sa/data";
    } else {
        if (country_code == "CN") {
            m_sensors_upload_url = "https://www.crealitycloud.cn/api/rest/bicollector/front/sa/data";
        } else {
            m_sensors_upload_url = "https://www.crealitycloud.com/api/rest/bicollector/front/sa/data";
        }
    }
    
    // 3. 记录配置信息（WARNING 级别）
    BOOST_LOG_TRIVIAL(warning) << "[CrealitySensors] Config initialized: "
                               << "Version=" << version_type 
                               << ", Country=" << country_code
                               << ", URL=" << m_sensors_upload_url;
    
    m_sensors_config_initialized = true;
}

bool AnalyticsDataUploadManager::test_sensors_connection()
{
    try {
        // 先初始化配置获取 URL
        getInstance().init_sensors_config_if_needed();
        std::string url = getInstance().m_sensors_upload_url;

        bool connected = false;

        Http http = Http::post(url);
        http.header("Content-Type", "application/x-www-form-urlencoded")
            .set_post_body(std::string("{}"))  // 发送空 JSON，显式类型避免歧义
            .timeout_connect(10)
            .timeout_max(30)
            .on_complete([&connected](std::string body, unsigned http_status) {
                // 即使不是 200，只要能收到响应也算网络连通
                connected = true;
            })
            .on_error([](std::string body, std::string error, unsigned http_status) {
                BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] Connection test FAILED: status=" << http_status
                                         << ", error=" << error;
            });

        http.perform_sync();

        return connected;

    } catch (const std::exception& err) {
        BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] Connection test exception: " << err.what();
        return false;
    }
}

void AnalyticsDataUploadManager::send_sensors_payload_to_creality(const nlohmann::json& payload)
{
    try {
        // 初始化配置（仅首次调用时执行）
        init_sensors_config_if_needed();
        
        // 1. 将 JSON 转为字符串
        std::string json_str = payload.dump();
        
        // 2. Gzip 压缩
        std::string compressed = gzip_compress(json_str);
        if (compressed.empty()) {
            BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] Gzip compression failed";
            return;
        }
        
        // 3. Base64 编码
        std::string encoded = base64_encode(compressed);
        if (encoded.empty()) {
            BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] Base64 encoding failed";
            return;
        }
        
        // 4. 构造 form-urlencoded 格式的请求体
        std::string post_body = "data=" + encoded + "&gzip=1";

        // 5. 发送 HTTP POST 请求
        Http http = Http::post(m_sensors_upload_url);
        http.header("Content-Type", "application/x-www-form-urlencoded")
            .set_post_body(std::string(post_body))  // 显式转换为 std::string 避免歧义
            .timeout_connect(10)      // 连接超时 10 秒
            .timeout_max(30)          // 总超时 30 秒
            .on_complete([this](std::string body, unsigned http_status) {
                if (http_status != 200) {
                    BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] Upload failed, HTTP " << http_status
                                             << ", body=" << (body.empty() ? "(empty)" : body);
                }
            })
            .on_error([this](std::string body, std::string error, unsigned http_status) {
                BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] Upload error, HTTP " << http_status
                                         << ", error=" << error
                                         << ", body=" << (body.empty() ? "(empty)" : body);
            });

        http.perform();  // 异步发送，不阻塞 UI 线程

    } catch (const std::exception& err) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Exception: " << err.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": Unknown exception";
    }
}

// ============================================================
// 3MF文件指纹管理实现
// 【修改】去掉缓存，每次都重新计算，保证文件修改后指纹更新
// ============================================================

std::string AnalyticsDataUploadManager::computeModelFingerprint(const std::string& file_path)
{
    // 不再使用缓存，每次都重新计算 MD5
    // 原因：同一路径的文件内容可能被修改，缓存会返回错误的指纹
    std::string fingerprint = computeMD5(file_path);
    return fingerprint;
}

// ============================================================
// 异步计算并设置 model_id（不阻塞调用线程）
// 计算完成后自动调用 mark_analytics_project_info
// ============================================================
void AnalyticsDataUploadManager::computeAndSetModelIdAsync(
    const std::string& file_path,
    const std::string& full_url,
    const std::string& file_format,
    const std::string& name)
{
    // 启动异步计算，不阻塞调用线程
    std::thread([this, file_path, full_url, file_format, name]() {
        try {
            // 后台线程计算指纹
            std::string model_id = computeModelFingerprint(file_path);

            // 计算完成后，在主线程调用赋值（通过 wxWidgets 的主线程调用机制）
            // 注意：file_format 已在 load_files 中同步设置，此处不重复赋值（防竞态）
            wxGetApp().CallAfter([this, full_url, model_id, name]() {
                // 仅设置 url/model_id/name，不覆盖 file_format（已在主线程同步设定）
                mark_analytics_project_info(full_url, model_id, "", "", name);
            });
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "computeAndSetModelIdAsync FAILED: file=" << file_path << " error=" << e.what();
        }
    }).detach();
}

std::string AnalyticsDataUploadManager::getCachedFingerprint(const std::string& file_path)
{
    std::lock_guard<std::mutex> lock(m_fingerprint_mutex);
    auto it = m_fingerprint_cache.find(file_path);
    if (it != m_fingerprint_cache.end()) {
        return it->second;
    }
    return "";
}

void AnalyticsDataUploadManager::clearFingerprintCache()
{
    std::lock_guard<std::mutex> lock(m_fingerprint_mutex);
    m_fingerprint_cache.clear();
}

std::string AnalyticsDataUploadManager::computeMD5(const std::string& file_path, size_t chunk_size)
{
    boost::nowide::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        // 用 boost::filesystem 交叉验证文件是否真实存在
        bool fs_exists = boost::filesystem::exists(file_path);
        std::string err = "Cannot open file: " + file_path;
        BOOST_LOG_TRIVIAL(error) << "computeMD5 FAILED: " << err
                                 << " fs_exists=" << (fs_exists ? "true" : "false");
        throw std::runtime_error(err);
    }

    MD5_CTX ctx;
    MD5_Init(&ctx);

    std::vector<char> buffer(chunk_size);

    while (file.good()) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            MD5_Update(&ctx, buffer.data(), bytes_read);
        }
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &ctx);

    // 转换为十六进制字符串
    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    return oss.str();
}

// ============================================================
// 项目几何体修改追踪实现
// ============================================================

AnalyticsDataUploadManager::ProjectModificationTracker& 
AnalyticsDataUploadManager::ProjectModificationTracker::getInstance() 
{
    static ProjectModificationTracker instance;
    return instance;
}

void AnalyticsDataUploadManager::ProjectModificationTracker::mark_modified(ModelModifyType type) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_is_modified = true;
    m_modify_history.push_back(type);
}

bool AnalyticsDataUploadManager::ProjectModificationTracker::is_essentially_modified() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_is_modified;
}

void AnalyticsDataUploadManager::ProjectModificationTracker::reset() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_is_modified = false;
    m_modify_history.clear();
    // 阶段3：同时清除切片信息缓存
    m_printer_info.clear();
    m_slice_param.clear();
    m_filament_info.clear();
    m_current_task_id.clear();  // 【新增】清除当前 task_id
    m_plate_task_ids.clear();   // 【新增】清除按盘存储的 task_id
}

const std::vector<AnalyticsDataUploadManager::ModelModifyType>& 
AnalyticsDataUploadManager::ProjectModificationTracker::get_history() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_modify_history;
}

// ============================================================
// 阶段3：切片信息缓存实现
// ============================================================

void AnalyticsDataUploadManager::ProjectModificationTracker::cache_slice_info(
    int plate_idx, 
    const std::string& printer_info,
    const std::string& slice_param,
    const std::string& filament_info)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_printer_info[plate_idx] = printer_info;
    m_slice_param[plate_idx] = slice_param;
    m_filament_info[plate_idx] = filament_info;
}

std::string AnalyticsDataUploadManager::ProjectModificationTracker::get_printer_info(int plate_idx) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_printer_info.find(plate_idx);
    if (it != m_printer_info.end()) {
        return it->second;
    }
    return {};
}

std::string AnalyticsDataUploadManager::ProjectModificationTracker::get_slice_param(int plate_idx) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_slice_param.find(plate_idx);
    if (it != m_slice_param.end()) {
        return it->second;
    }
    return {};
}

std::string AnalyticsDataUploadManager::ProjectModificationTracker::get_filament_info(int plate_idx) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_filament_info.find(plate_idx);
    if (it != m_filament_info.end()) {
        return it->second;
    }
    return {};
}

std::string AnalyticsDataUploadManager::ProjectModificationTracker::get_merged_slice_param(int plate_count) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 以第0盘的 slice_param 为基础（global_param/objects/plates 都从这里取起点）
    auto it0 = m_slice_param.find(0);
    if (it0 == m_slice_param.end() || it0->second.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "[SensorsAnalytics] get_merged_slice_param: plate 0 has no cached slice_param";
        return {};
    }
    
    nlohmann::json merged;
    try {
        merged = nlohmann::json::parse(it0->second);
    } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] get_merged_slice_param: failed to parse plate 0 slice_param";
        return it0->second;
    }
    
    // 确保 objects / plates 字段存在
    if (!merged.contains("objects") || !merged["objects"].is_object()) {
        merged["objects"] = nlohmann::json::object();
    }
    if (!merged.contains("plates") || !merged["plates"].is_array()) {
        merged["plates"] = nlohmann::json::array();
    }
    
    // 合并其余各盘的 objects 和 plates
    for (int i = 1; i < plate_count; ++i) {
        auto it = m_slice_param.find(i);
        if (it == m_slice_param.end() || it->second.empty()) {
            BOOST_LOG_TRIVIAL(warning) << "[SensorsAnalytics] get_merged_slice_param: plate " << i << " has no cached slice_param, skipping";
            continue;
        }
        try {
            nlohmann::json plate_j = nlohmann::json::parse(it->second);
            // 合并 objects（key=object_id，跨盘不重复）
            if (plate_j.contains("objects") && plate_j["objects"].is_object()) {
                for (auto& [key, val] : plate_j["objects"].items()) {
                    merged["objects"][key] = val;
                }
            }
            // 追加 plates 数组项
            if (plate_j.contains("plates") && plate_j["plates"].is_array()) {
                for (auto& plate_entry : plate_j["plates"]) {
                    merged["plates"].push_back(plate_entry);
                }
            }
        } catch (...) {
            BOOST_LOG_TRIVIAL(error) << "[SensorsAnalytics] get_merged_slice_param: failed to parse plate " << i << " slice_param, skipping";
        }
    }
    
    BOOST_LOG_TRIVIAL(warning) << "[SensorsAnalytics] get_merged_slice_param: merged " << plate_count
                               << " plates, total objects=" << merged["objects"].size()
                               << ", plates=" << merged["plates"].size();
    return merged.dump();
}

size_t AnalyticsDataUploadManager::ProjectModificationTracker::get_cached_plate_count() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_slice_param.size();
}

// 【新增】设置当前 task_id（供 GCode.cpp 写入 G-code header）
void AnalyticsDataUploadManager::ProjectModificationTracker::set_current_task_id(const std::string& task_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_current_task_id = task_id;
}

// 【新增】获取当前 task_id
std::string AnalyticsDataUploadManager::ProjectModificationTracker::get_current_task_id() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_task_id;
}

// 【新增】设置当前盘索引
void AnalyticsDataUploadManager::ProjectModificationTracker::set_current_plate_id(int plate_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_current_plate_id = plate_id;
}

// 【新增】获取当前盘索引
int AnalyticsDataUploadManager::ProjectModificationTracker::get_current_plate_id() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_plate_id;
}

// 【新增】生成 task_id（格式：{device_id}_{timestamp_ms}_{plate_id:02d}）
std::string AnalyticsDataUploadManager::ProjectModificationTracker::generate_task_id(int plate_id)
{
    auto now = std::chrono::system_clock::now();
    long long timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return generate_task_id(plate_id, timestamp_ms);
}

// 【新增】重载：使用外部传入的 timestamp 生成 task_id（用于多盘共享同一时间戳）
std::string AnalyticsDataUploadManager::ProjectModificationTracker::generate_task_id(int plate_id, long long timestamp_ms)
{
    std::string device_id = SystemId::get_system_id();
    char buf[128];
    snprintf(buf, sizeof(buf), "%s_%lld_%02d", device_id.c_str(), timestamp_ms, plate_id);
    return std::string(buf);
}

// Replace task_id_pending placeholder in gcode file, renaming field to creality_task_id (in-place overwrite, no file content shift)
// Layout: "; creality_task_id_pending:73B_placeholder\n" (101B) → "; creality_task_id:81B_real_task_id\n" (101B)
bool AnalyticsDataUploadManager::ProjectModificationTracker::replace_task_id_placeholder_in_gcode(
    const std::string& gcode_path, const std::string& task_id)
{
    if (gcode_path.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "[TaskIdReplace] gcode_path is empty";
        return false;
    }
    if (!boost::filesystem::exists(gcode_path)) {
        BOOST_LOG_TRIVIAL(warning) << "[TaskIdReplace] gcode file not found: " << gcode_path;
        return false;
    }
    
    boost::nowide::fstream file(gcode_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "[TaskIdReplace] Failed to open gcode file (exists but likely locked by another process): " << gcode_path;
        return false;
    }
    
    // Read first 4KB (header block should be within this range)
    const size_t buffer_size = 4096;
    std::string buffer(buffer_size, '\0');
    file.read(&buffer[0], buffer_size);
    size_t bytes_read = file.gcount();
    buffer.resize(bytes_read);
    
    const std::string marker = "; creality_task_id_pending: ";
    size_t marker_pos = buffer.find(marker);
    if (marker_pos == std::string::npos) {
        BOOST_LOG_TRIVIAL(warning) << "[TaskIdReplace] Placeholder marker not found in: " << gcode_path;
        file.close();
        return false;
    }
    
    const size_t value_len = 81;
    
    // Clear eofbit to ensure seekp works even when file is smaller than buffer_size
    file.clear();
    
    // Seek to start of marker (replace entire line, not just value)
    file.seekp(static_cast<std::streamoff>(marker_pos));
    if (!file) {
        BOOST_LOG_TRIVIAL(warning) << "[TaskIdReplace] seekp failed in: " << gcode_path;
        file.close();
        return false;
    }
    
    std::string task_id_padded = task_id;
    if (task_id_padded.length() < value_len) {
        task_id_padded.append(value_len - task_id_padded.length(), ' ');
    } else if (task_id_padded.length() > value_len) {
        task_id_padded = task_id_padded.substr(0, value_len);
    }
    std::string new_line = "; creality_task_id: ";
    new_line += task_id_padded;
    new_line += "\n";
    // Use str.length() instead of hardcoded constant to avoid missed updates when field name or value changes
    file.write(new_line.c_str(), new_line.length());
    if (!file) {
        BOOST_LOG_TRIVIAL(warning) << "[TaskIdReplace] write failed in: " << gcode_path;
        file.close();
        return false;
    }
    file.close();
    return true;
}

// 【新增】按盘索引存储 task_id（导出时写入，发送时复用）
void AnalyticsDataUploadManager::ProjectModificationTracker::set_plate_task_id(int plate_index, const std::string& task_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_plate_task_ids[plate_index] = task_id;
}

// 【新增】清除指定盘的 task_id 缓存（用于强制重新生成）
void AnalyticsDataUploadManager::ProjectModificationTracker::clear_plate_task_id(int plate_index)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_plate_task_ids.erase(plate_index);
}

// 【新增】按盘索引获取 task_id（优先已存储的，无则生成新值并存储）
std::string AnalyticsDataUploadManager::ProjectModificationTracker::get_plate_task_id(int plate_index)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plate_task_ids.find(plate_index);
    if (it != m_plate_task_ids.end()) {
        return it->second;
    }
    // 未存储则生成新的（兼容导出前未调用 export_3mf 的场景）
    std::string task_id = generate_task_id(plate_index + 1);
    m_plate_task_ids[plate_index] = task_id;
    return task_id;
}

// ============================================================
// ProjectModificationTracker::collect_params 实现
// ============================================================

using namespace Slic3r;

// 辅助函数：添加参数值（添加异常保护，防止配置项异常导致崩溃）
void AnalyticsDataUploadManager::ProjectModificationTracker::add_param(const DynamicPrintConfig& config,
                                    nlohmann::json& output,
                                    const char* config_key,
                                    const char* output_key,
                                    ParamType type)
{
    // 防御性检查 - 在函数入口就记录日志
    if (!config_key || !output_key) {
        BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] add_param called with null key: config_key=" << (void*)config_key << ", output_key=" << (void*)output_key;
        boost::log::core::get()->flush();
        return;
    }
    
    try {
        switch (type) {
        case ParamType::Float: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing Float: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionFloat>(config_key);
            if (opt) {
                output[output_key] = std::to_string(opt->value);
            }
            break;
        }
        case ParamType::FloatFirst: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing FloatFirst: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionFloats>(config_key);
            if (opt && !opt->values.empty()) {
                output[output_key] = std::to_string(opt->values[0]);
            }
            break;
        }
        case ParamType::Int: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing Int: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionInt>(config_key);
            if (opt) {
                output[output_key] = std::to_string(opt->value);
            }
            break;
        }
        case ParamType::IntFirst: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing IntFirst: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionInts>(config_key);
            if (opt && !opt->values.empty()) {
                output[output_key] = std::to_string(opt->values[0]);
            } else if (config.has(config_key)) {
                // ConfigOptionEnumsGeneric 继承自 ConfigOptionInts，但 option<ConfigOptionInts> 无法转换
                // 使用 opt_enum(key, 0) 读取第一个枚举值
                try {
                    int value = config.opt_enum(config_key, 0);
                    output[output_key] = std::to_string(value);
                } catch (...) {}
            }
            break;
        }
        case ParamType::Bool: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing Bool: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionBool>(config_key);
            if (opt) {
                output[output_key] = opt->value ? "true" : "false";
            } else if (config.has(config_key)) {
                // 尝试使用其他方式获取
                int value = config.opt_int(config_key);
                output[output_key] = value ? "true" : "false";
            } else {
                // 字段不存在（如默认值被3MF跳过），输出默认值 false
                output[output_key] = "false";
            }
            break;
        }
        case ParamType::BoolFirst: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing BoolFirst: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionBools>(config_key);
            if (opt && !opt->values.empty()) {
                output[output_key] = opt->values[0] ? "true" : "false";
            }
            break;
        }
        case ParamType::String: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing String: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionString>(config_key);
            if (opt) {
                output[output_key] = opt->value;
            }
            break;
        }
        case ParamType::StringFirst: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing StringFirst: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionStrings>(config_key);
            if (opt && !opt->values.empty()) {
                output[output_key] = opt->values[0];
            }
            break;
        }
        case ParamType::StringMulti: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing StringMulti: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionStrings>(config_key);
            if (opt && !opt->values.empty()) {
                std::string value;
                for (size_t i = 0; i < opt->values.size(); i++) {
                    if (i > 0) value += ";";
                    value += opt->values[i];
                }
                output[output_key] = value;
            }
            break;
        }
        case ParamType::Percent: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing Percent: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionPercent>(config_key);
            if (opt) {
                output[output_key] = std::to_string(opt->value);
            }
            break;
        }
        case ParamType::PercentFirst: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing PercentFirst: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionPercents>(config_key);
            if (opt && !opt->values.empty()) {
                output[output_key] = std::to_string(opt->values[0]);
            }
            break;
        }
        case ParamType::FloatOrPercent: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing FloatOrPercent: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionFloatOrPercent>(config_key);
            if (opt) {
                output[output_key] = std::to_string(opt->get_abs_value(0));
            }
            break;
        }
        case ParamType::FloatOrPercentFirst: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing FloatOrPercentFirst: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            auto* opt = config.option<ConfigOptionFloatsOrPercents>(config_key);
            if (opt && !opt->values.empty()) {
                // 直接取 value（percent=true 时 value 本身也应该是可用的绝对值）
                output[output_key] = std::to_string(opt->values[0].value);
            }
            break;
        }
        case ParamType::Enum: {
            BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Processing Enum: " << config_key << " -> " << output_key;
            boost::log::core::get()->flush();
            // 尝试多种方式获取枚举值
            auto* opt = config.option<ConfigOptionInt>(config_key);
            if (opt) {
                output[output_key] = std::to_string(opt->value);
            } else if (config.has(config_key)) {
                // 如果字段存在但类型不匹配，尝试使用 opt_int
                try {
                    int value = config.opt_int(config_key);
                    output[output_key] = std::to_string(value);
                } catch (...) {
                    BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Enum field failed to read: " << config_key << " -> " << output_key;
                }
            } else {
                // DEBUG: 记录获取失败的枚举字段
                BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Enum field missing: " << config_key << " -> " << output_key;
            }
            break;
        }
        }  // end switch
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Exception in add_param: " << config_key << " -> " << output_key << ": " << e.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << "[ParamDebug] Unknown exception in add_param: " << config_key << " -> " << output_key;
    }
}

// 参数定义表（打印机参数）
const AnalyticsDataUploadManager::ProjectModificationTracker::ParamDef 
    AnalyticsDataUploadManager::ProjectModificationTracker::s_printer_params[] = {
    {"nozzle_diameter", "nozzle_diameter", ParamType::FloatFirst},
    {"min_layer_height", "min_layer_height", ParamType::FloatFirst},
    {"max_layer_height", "max_layer_height", ParamType::FloatFirst},
    {"printer_model", "printer_model", ParamType::String},
    {"curr_bed_type", "material_bed_type", ParamType::Enum},
};

// 参数定义表（材料参数）
const AnalyticsDataUploadManager::ProjectModificationTracker::ParamDef 
    AnalyticsDataUploadManager::ProjectModificationTracker::s_filament_params[] = {
    // 基础信息
    {"filament_type", "filament_type", ParamType::StringMulti},
    {"filament_vendor", "filament_vendor", ParamType::StringMulti},
    {"filament_soluble", "filament_soluble", ParamType::BoolFirst},
    {"filament_is_support", "filament_is_support", ParamType::BoolFirst},
    {"required_nozzle_HRC", "required_nozzle_HRC", ParamType::IntFirst},
    {"default_filament_colour", "default_filament_colour", ParamType::StringFirst},
    {"filament_diameter", "filament_diameter", ParamType::FloatFirst},
    {"filament_flow_ratio", "filament_flow_ratio", ParamType::FloatFirst},
    {"enable_pressure_advance", "enable_pressure_advance", ParamType::BoolFirst},
    {"pressure_advance", "pressure_advance", ParamType::FloatFirst},
    {"filament_density", "filament_density", ParamType::FloatFirst},
    {"filament_shrink", "filament_shrink", ParamType::PercentFirst},
    {"filament_shrinkage_compensation_z", "filament_shrinkage_compensation_z", ParamType::PercentFirst},
    {"filament_cost", "filament_cost", ParamType::FloatFirst},
    {"temperature_vitrification", "temperature_vitrification", ParamType::IntFirst},
    {"idle_temperature", "idle_temperature", ParamType::IntFirst},
    // 打印仓温度
    {"chamber_temperature", "chamber_temperature", ParamType::IntFirst},
    {"activate_chamber_temp_control", "activate_chamber_temp_control", ParamType::BoolFirst},
    {"activate_chamber_layer", "activate_chamber_layer", ParamType::IntFirst},
    // 打印温度
    {"nozzle_temperature_initial_layer", "nozzle_temperature_initial_layer", ParamType::IntFirst},
    {"nozzle_temperature", "nozzle_temperature", ParamType::IntFirst},
    {"material_flow_dependent_temperature", "material_flow_dependent_temperature", ParamType::BoolFirst},
    {"material_flow_temp_graph", "material_flow_temp_graph", ParamType::String},
    // 床温
    {"hot_plate_temp_initial_layer", "hot_plate_temp_initial_layer", ParamType::IntFirst},
    {"hot_plate_temp", "hot_plate_temp", ParamType::IntFirst},
    {"textured_plate_temp_initial_layer", "textured_plate_temp_initial_layer", ParamType::IntFirst},
    {"textured_plate_temp", "textured_plate_temp", ParamType::IntFirst},
    {"customized_plate_temp_initial_layer", "customized_plate_temp_initial_layer", ParamType::IntFirst},
    {"customized_plate_temp", "customized_plate_temp", ParamType::IntFirst},
    {"epoxy_resin_plate_temp_initial_layer", "epoxy_resin_plate_temp_initial_layer", ParamType::IntFirst},
    {"epoxy_resin_plate_temp", "epoxy_resin_plate_temp", ParamType::IntFirst},
    // 体积速度限制
    {"filament_max_volumetric_speed", "filament_max_volumetric_speed", ParamType::FloatFirst},
    // 特定层冷却
    {"close_fan_the_first_x_layers", "close_fan_the_first_x_layers", ParamType::IntFirst},
    {"full_fan_speed_layer", "full_fan_speed_layer", ParamType::IntFirst},
    // 模型风扇
    {"fan_min_speed", "fan_min_speed", ParamType::FloatFirst},
    {"fan_cooling_layer_time", "fan_cooling_layer_time", ParamType::FloatFirst},
    {"fan_max_speed", "fan_max_speed", ParamType::FloatFirst},
    {"slow_down_layer_time", "slow_down_layer_time", ParamType::FloatFirst},
    {"reduce_fan_stop_start_freq", "reduce_fan_stop_start_freq", ParamType::BoolFirst},
    {"slow_down_for_layer_cooling", "slow_down_for_layer_cooling", ParamType::BoolFirst},
    {"slow_down_min_speed", "slow_down_min_speed", ParamType::FloatFirst},
    {"enable_overhang_bridge_fan", "enable_overhang_bridge_fan", ParamType::BoolFirst},
    {"overhang_fan_threshold", "overhang_fan_threshold", ParamType::IntFirst},
    {"overhang_fan_speed", "overhang_fan_speed", ParamType::IntFirst},
    {"support_material_interface_fan_speed", "support_material_interface_fan_speed", ParamType::IntFirst},
    // 辅助风扇
    {"additional_cooling_fan_speed", "additional_cooling_fan_speed", ParamType::IntFirst},
    {"enable_special_area_additional_cooling_fan", "enable_special_area_additional_cooling_fan", ParamType::BoolFirst},
    {"cool_special_cds_fan_speed", "cool_special_cds_fan_speed", ParamType::IntFirst},
    {"cool_cds_fan_start_at_height", "cool_cds_fan_start_at_height", ParamType::FloatFirst},
    // 机箱风扇
    {"activate_air_filtration", "activate_air_filtration", ParamType::BoolFirst},
    {"during_print_exhaust_fan_speed", "during_print_exhaust_fan_speed", ParamType::IntFirst},
    {"complete_print_exhaust_fan_speed", "complete_print_exhaust_fan_speed", ParamType::IntFirst},
};

// 参数定义表（工艺参数）
const AnalyticsDataUploadManager::ProjectModificationTracker::ParamDef 
    AnalyticsDataUploadManager::ProjectModificationTracker::s_process_params[] = {
    // 层高和线宽
    {"layer_height", "layer_height", ParamType::Float},
    {"initial_layer_print_height", "initial_layer_print_height", ParamType::Float},
    {"line_width", "line_width", ParamType::FloatOrPercent},
    {"initial_layer_line_width", "initial_layer_line_width", ParamType::FloatOrPercent},
    {"outer_wall_line_width", "outer_wall_line_width", ParamType::FloatOrPercent},
    {"inner_wall_line_width", "inner_wall_line_width", ParamType::FloatOrPercent},
    {"top_surface_line_width", "top_surface_line_width", ParamType::FloatOrPercent},
    {"sparse_infill_line_width", "sparse_infill_line_width", ParamType::FloatOrPercent},
    {"internal_solid_infill_line_width", "internal_solid_infill_line_width", ParamType::FloatOrPercent},
    {"support_line_width", "support_line_width", ParamType::FloatOrPercent},
    // 壁厚和填充
    {"wall_loops", "wall_loops", ParamType::Int},
    {"sparse_infill_density", "sparse_infill_density", ParamType::Percent},
    {"top_shell_layers", "top_shell_layers", ParamType::Int},
    {"top_shell_thickness", "top_shell_thickness", ParamType::Float},
    {"bottom_shell_layers", "bottom_shell_layers", ParamType::Int},
    {"bottom_shell_thickness", "bottom_shell_thickness", ParamType::Float},
    // 支撑和底座
    {"enable_support", "enable_support", ParamType::Bool},
    {"support_type", "support_type", ParamType::Enum},
    {"support_threshold_angle", "support_threshold_angle", ParamType::Int},
    {"raft_layers", "raft_layers", ParamType::Int},
    {"skirt_loops", "skirt_loops", ParamType::Int},
    {"brim_type", "brim_type", ParamType::Enum},
    {"brim_width", "brim_width", ParamType::Float},
    {"brim_object_gap", "brim_object_gap", ParamType::Float},
    {"brim_ears", "brim_ears", ParamType::Bool},
    {"brim_ears_max_angle", "brim_ears_max_angle", ParamType::Float},
    {"brim_ears_detection_length", "brim_ears_detection_length", ParamType::Float},
    // 质量和图案
    {"seam_position", "seam_position", ParamType::Enum},
    {"staggered_inner_seams", "staggered_inner_seams", ParamType::Bool},
    {"seam_gap", "seam_gap", ParamType::FloatOrPercent},
    // 接缝和擦拭
    {"seam_slope_type", "seam_slope_type", ParamType::Enum},
    {"role_based_wipe_speed", "role_based_wipe_speed", ParamType::Bool},
    {"wipe_on_loops", "wipe_on_loops", ParamType::Bool},
    {"wipe_before_external_loop", "wipe_before_external_loop", ParamType::Bool},
    {"wipe_speed", "wipe_speed", ParamType::FloatOrPercent},
    {"wall_generator", "wall_generator", ParamType::Enum},
    {"wall_transition_angle", "wall_transition_angle", ParamType::Float},
    {"wall_transition_filter_deviation", "wall_transition_filter_deviation", ParamType::Percent},
    {"wall_transition_length", "wall_transition_length", ParamType::Percent},
    {"wall_distribution_count", "wall_distribution_count", ParamType::Int},
    {"initial_layer_min_bead_width", "initial_layer_min_bead_width", ParamType::Percent},
    {"min_bead_width", "min_bead_width", ParamType::Percent},
    {"min_feature_size", "min_feature_size", ParamType::Percent},
    {"min_length_factor", "min_length_factor", ParamType::Float},
    {"alternate_extra_wall", "alternate_extra_wall", ParamType::Bool},
    {"top_surface_pattern", "top_surface_pattern", ParamType::Enum},
    {"bottom_surface_pattern", "bottom_surface_pattern", ParamType::Enum},
    {"sparse_infill_pattern", "sparse_infill_pattern", ParamType::Enum},
    {"top_bottom_infill_wall_overlap", "top_bottom_infill_wall_overlap", ParamType::Percent},
    {"ai_infill", "ai_infill", ParamType::Bool},
    // 速度参数
    {"initial_layer_speed", "initial_layer_speed", ParamType::Float},
    {"outer_wall_speed", "outer_wall_speed", ParamType::Float},
    {"inner_wall_speed", "inner_wall_speed", ParamType::Float},
    {"sparse_infill_speed", "sparse_infill_speed", ParamType::Float},
    {"top_surface_speed", "top_surface_speed", ParamType::Float},
    {"support_speed", "support_speed", ParamType::Float},
    {"travel_speed", "travel_speed", ParamType::Float},
    {"bridge_speed", "bridge_speed", ParamType::Float},
    {"initial_layer_infill_speed", "initial_layer_infill_speed", ParamType::Float},
    {"initial_layer_travel_speed", "initial_layer_travel_speed", ParamType::FloatOrPercent},
    {"slow_down_layers", "slow_down_layers", ParamType::Int},
    {"small_perimeter_speed", "small_perimeter_speed", ParamType::FloatOrPercent},
    {"small_perimeter_threshold", "small_perimeter_threshold", ParamType::Float},
    {"internal_solid_infill_speed", "internal_solid_infill_speed", ParamType::Float},
    {"gap_infill_speed", "gap_infill_speed", ParamType::Float},
    {"support_interface_speed", "support_interface_speed", ParamType::Float},
    {"enable_overhang_speed", "enable_overhang_speed", ParamType::Bool},
    {"overhang_speed_classic", "overhang_speed_classic", ParamType::Bool},
    {"overhang_1_4_speed", "overhang_1_4_speed", ParamType::FloatOrPercent},
    {"overhang_2_4_speed", "overhang_2_4_speed", ParamType::FloatOrPercent},
    {"overhang_3_4_speed", "overhang_3_4_speed", ParamType::FloatOrPercent},
    {"overhang_4_4_speed", "overhang_4_4_speed", ParamType::FloatOrPercent},
    {"overhang_totally_speed", "overhang_totally_speed", ParamType::FloatOrPercent},
    {"internal_bridge_speed", "internal_bridge_speed", ParamType::FloatOrPercent},
    {"smooth_speed_discontinuity_area", "smooth_speed_discontinuity_area", ParamType::Bool},
    {"smooth_coefficient", "smooth_coefficient", ParamType::Float},
    // 加速度和抖动
    {"default_acceleration", "default_acceleration", ParamType::Float},
    {"outer_wall_acceleration", "outer_wall_acceleration", ParamType::Float},
    {"inner_wall_acceleration", "inner_wall_acceleration", ParamType::Float},
    {"bridge_acceleration", "bridge_acceleration", ParamType::FloatOrPercent},
    {"sparse_infill_acceleration", "sparse_infill_acceleration", ParamType::FloatOrPercent},
    {"internal_solid_infill_acceleration", "internal_solid_infill_acceleration", ParamType::FloatOrPercent},
    {"initial_layer_acceleration", "initial_layer_acceleration", ParamType::Float},
    {"top_surface_acceleration", "top_surface_acceleration", ParamType::Float},
    {"travel_acceleration", "travel_acceleration", ParamType::Float},
    {"default_jerk", "default_jerk", ParamType::Float},
    {"outer_wall_jerk", "outer_wall_jerk", ParamType::Float},
    {"inner_wall_jerk", "inner_wall_jerk", ParamType::Float},
    {"infill_jerk", "infill_jerk", ParamType::Float},
    {"top_surface_jerk", "top_surface_jerk", ParamType::Float},
    {"initial_layer_jerk", "initial_layer_jerk", ParamType::Float},
    {"travel_jerk", "travel_jerk", ParamType::Float},
    {"max_volumetric_extrusion_rate_slope", "max_volumetric_extrusion_rate_slope", ParamType::Float},
    {"acceleration_limit_mess_enable", "acceleration_limit_mess_enable", ParamType::Bool},
    {"speed_limit_to_height_enable", "speed_limit_to_height_enable", ParamType::Bool},
    // 支撑样式
    {"support_style", "support_style", ParamType::Enum},
    {"support_on_build_plate_only", "support_on_build_plate_only", ParamType::Bool},
    {"support_remove_small_overhang", "support_remove_small_overhang", ParamType::Bool},
    {"minimum_support_area", "minimum_support_area", ParamType::Float},
    // 擦拭塔
    {"enable_prime_tower", "enable_prime_tower", ParamType::Bool},
    {"prime_volume", "prime_volume", ParamType::Float},
    {"prime_tower_width", "prime_tower_width", ParamType::Float},
    {"prime_tower_brim_width", "prime_tower_brim_width", ParamType::Float},
    {"wipe_tower_rotation_angle", "wipe_tower_rotation_angle", ParamType::Float},
    {"prime_tower_enhance_type", "prime_tower_enhance_type", ParamType::Enum},
    {"wipe_tower_extra_flow", "wipe_tower_extra_flow", ParamType::Percent},
    {"wipe_tower_no_sparse_layers", "wipe_tower_no_sparse_layers", ParamType::Bool},
    // 裙边/Brim
    {"skirt_type", "skirt_type", ParamType::Enum},
    {"min_skirt_length", "min_skirt_length", ParamType::Float},
    {"skirt_distance", "skirt_distance", ParamType::Float},
    {"skirt_height", "skirt_height", ParamType::Int},
    {"skirt_speed", "skirt_speed", ParamType::Float},
    {"draft_shield", "draft_shield", ParamType::Enum},
    // 2026.5.15 added - Quality: Ironing
    {"ironing_type", "ironing_type", ParamType::Enum},
    // 2026.5.15 added - Quality: Wall and Surface
    {"wall_sequence", "wall_sequence", ParamType::Enum},
    {"is_infill_first", "is_infill_first", ParamType::Bool},
    {"wall_direction", "wall_direction", ParamType::Enum},
    {"top_solid_infill_flow_ratio", "top_solid_infill_flow_ratio", ParamType::Float},
    {"bottom_solid_infill_flow_ratio", "bottom_solid_infill_flow_ratio", ParamType::Float},
    {"only_one_wall_top", "only_one_wall_top", ParamType::Bool},
    {"min_width_top_surface", "min_width_top_surface", ParamType::FloatOrPercent},
    {"only_one_wall_first_layer", "only_one_wall_first_layer", ParamType::Bool},
    {"reduce_crossing_wall", "reduce_crossing_wall", ParamType::Bool},
    {"small_area_infill_flow_compensation", "small_area_infill_flow_compensation", ParamType::Bool},
    {"z_direction_outwall_speed_continuous", "z_direction_outwall_speed_continuous", ParamType::Bool},
    // 2026.5.15 added - Quality: Bridging
    {"bridge_flow", "bridge_flow", ParamType::Float},
    {"internal_bridge_flow", "internal_bridge_flow", ParamType::Float},
    {"bridge_density", "bridge_density", ParamType::Percent},
    {"thick_bridges", "thick_bridges", ParamType::Bool},
    {"thick_internal_bridges", "thick_internal_bridges", ParamType::Bool},
    {"dont_filter_internal_bridges", "dont_filter_internal_bridges", ParamType::Enum},
    {"counterbore_hole_bridging", "counterbore_hole_bridging", ParamType::Enum},
    // 2026.5.15 added - Quality: Overhang
    {"detect_overhang_wall", "detect_overhang_wall", ParamType::Bool},
    {"make_overhang_printable", "make_overhang_printable", ParamType::Bool},
    {"extra_perimeters_on_overhangs", "extra_perimeters_on_overhangs", ParamType::Bool},
    {"overhang_reverse", "overhang_reverse", ParamType::Bool},
    {"overhang_optimization", "overhang_optimization", ParamType::Bool},
    // 2026.5.15 added - Strength: Wall
    {"embedding_wall_into_infill", "embedding_wall_into_infill", ParamType::Bool},
    // 2026.5.15 added - Strength: Infill
    {"internal_solid_infill_pattern", "internal_solid_infill_pattern", ParamType::Enum},
    {"infill_direction", "infill_direction", ParamType::Float},
    {"solid_infill_direction", "solid_infill_direction", ParamType::Float},
    {"rotate_solid_infill_direction", "rotate_solid_infill_direction", ParamType::Bool},
    {"bridge_angle", "bridge_angle", ParamType::Float},
    {"minimum_sparse_infill_area", "minimum_sparse_infill_area", ParamType::Float},
    {"infill_combination", "infill_combination", ParamType::Bool},
    {"detect_narrow_internal_solid_infill", "detect_narrow_internal_solid_infill", ParamType::Bool},
    {"ensure_vertical_shell_thickness", "ensure_vertical_shell_thickness", ParamType::Enum},
    // 2026.5.15 added - Speed: Acceleration
    {"initial_layer_travel_acceleration", "initial_layer_travel_acceleration", ParamType::FloatFirst},
    {"accel_to_decel_factor", "accel_to_decel_factor", ParamType::Percent},
    {"travel_short_distance_acceleration", "travel_short_distance_acceleration", ParamType::Float},
    {"travel_short_distance_threshold", "travel_short_distance_threshold", ParamType::Float},
    // 2026.5.15 added - Support: Raft
    {"raft_contact_distance", "raft_contact_distance", ParamType::Float},
    {"raft_first_layer_density", "raft_first_layer_density", ParamType::Percent},
    {"raft_first_layer_expansion", "raft_first_layer_expansion", ParamType::Float},
    // 2026.5.15 added - Support: Filament
    {"support_filament", "support_filament", ParamType::Int},
    {"support_interface_filament", "support_interface_filament", ParamType::Int},
    // 2026.5.15 added - Support: Advanced
    {"support_top_z_distance", "support_top_z_distance", ParamType::Float},
    {"support_base_pattern", "support_base_pattern", ParamType::Enum},
    {"tree_support_wall_count", "tree_support_wall_count", ParamType::Int},
    {"support_base_pattern_spacing", "support_base_pattern_spacing", ParamType::Float},
    {"support_angle", "support_angle", ParamType::Float},
    {"support_interface_top_layers", "support_interface_top_layers", ParamType::Int},
    {"support_interface_bottom_layers", "support_interface_bottom_layers", ParamType::Int},
    {"support_interface_min_area", "support_interface_min_area", ParamType::Float},
    {"support_interface_pattern", "support_interface_pattern", ParamType::Enum},
    {"support_interface_spacing", "support_interface_spacing", ParamType::Float},
    {"support_expansion", "support_expansion", ParamType::Float},
    {"support_object_xy_distance", "support_object_xy_distance", ParamType::Float},
    {"support_object_first_layer_gap", "support_object_first_layer_gap", ParamType::Float},
    {"support_xy_overrides_z", "support_xy_overrides_z", ParamType::Enum},
    {"independent_support_layer_height", "independent_support_layer_height", ParamType::Bool},
    {"bridge_no_support", "bridge_no_support", ParamType::Bool},
    {"max_bridge_length", "max_bridge_length", ParamType::Float},
    // 2026.5.15 added - Support: Tree Support
    {"tree_support_branch_distance", "tree_support_branch_distance", ParamType::Float},
    {"tree_support_branch_diameter", "tree_support_branch_diameter", ParamType::Float},
    {"tree_support_branch_angle", "tree_support_branch_angle", ParamType::Float},
    {"tree_support_branch_diameter_angle", "tree_support_branch_diameter_angle", ParamType::Float},
    {"support_base_pattern_tree", "support_base_pattern_tree", ParamType::Enum},
    {"tree_support_wall_count_tree", "tree_support_wall_count_tree", ParamType::Int},
    // 2026.5.15 added - Material: Prime Tower
    {"prime_tower_rib_wall", "prime_tower_rib_wall", ParamType::Bool},
    {"prime_tower_skip_points", "prime_tower_skip_points", ParamType::Bool},
    {"prime_tower_enable_framework", "prime_tower_enable_framework", ParamType::Bool},
    // 2026.5.15 added - Material: Flush Options
    {"flush_into_infill", "flush_into_infill", ParamType::Bool},
    {"flush_into_support", "flush_into_support", ParamType::Bool},
    // 2026.5.15 added - Material: Advanced
    {"interlocking_beam", "interlocking_beam", ParamType::Bool},
    {"mmu_segmented_region_max_width", "mmu_segmented_region_max_width", ParamType::Float},
    {"mmu_segmented_region_interlocking_depth", "mmu_segmented_region_interlocking_depth", ParamType::Float},
};

// 采集参数入口函数
nlohmann::json AnalyticsDataUploadManager::ProjectModificationTracker::collect_params(const DynamicPrintConfig& config)
{
    nlohmann::json output = nlohmann::json::object();
    
    // 新格式：global_param 分为三个子对象
    nlohmann::json machine_params = nlohmann::json::object();
    nlohmann::json filament_params = nlohmann::json::object();
    nlohmann::json process_params = nlohmann::json::object();
    
    // 采集打印机参数 → machine_params
    collect_printer_params(config, machine_params);
    
    // 采集材料参数 → filament_params
    collect_filament_params(config, filament_params);
    
    // 采集工艺参数 → process_params
    collect_process_params(config, process_params);
    
    output["machine_params"] = machine_params;
    output["filament_params"] = filament_params;
    output["process_params"] = process_params;
    
    return output;
}

// ============================================================
// 阶段4：采集对象/部件修改参数
// ============================================================

// 序列化单个配置选项为字符串
static std::string serialize_config_value(const ConfigOption* opt)
{
    if (!opt) return "";
    return opt->serialize();
}

// 将 Transform3d (4x4矩阵) 序列化为3MF格式字符串
// 3MF格式：m00 m01 m02 m10 m11 m12 m20 m21 m22 tx ty tz
// 注意：Eigen内部以列优先存储，非对称旋转矩阵的行列访问会得到转置结果。
// 因此按列遍历3x3子矩阵（等价于输出原始矩阵的行优先顺序），最后输出平移向量。
static std::string transform3d_to_3mf_string(const Transform3d& matrix)
{
    std::string result;
    auto append = [&result](double val) {
        if (!result.empty()) result += " ";
        result += std::to_string(val);
    };
    // 第0列（对应3MF的第0行）：m00 m01 m02
    append(matrix(0, 0)); append(matrix(1, 0)); append(matrix(2, 0));
    // 第1列（对应3MF的第1行）：m10 m11 m12
    append(matrix(0, 1)); append(matrix(1, 1)); append(matrix(2, 1));
    // 第2列（对应3MF的第2行）：m20 m21 m22
    append(matrix(0, 2)); append(matrix(1, 2)); append(matrix(2, 2));
    // 平移（第4列）：tx ty tz
    append(matrix(0, 3)); append(matrix(1, 3)); append(matrix(2, 3));
    return result;
}

// 将 ModelVolumeType 枚举转换为 model_settings.config 中的 subtype 字符串
static std::string volume_type_to_subtype_string(ModelVolumeType type)
{
    switch (type) {
    case ModelVolumeType::MODEL_PART:        return "normal_part";
    case ModelVolumeType::NEGATIVE_VOLUME:   return "negative_part";
    case ModelVolumeType::PARAMETER_MODIFIER: return "modifier";
    case ModelVolumeType::SUPPORT_ENFORCER:  return "support_enforcer";
    case ModelVolumeType::SUPPORT_BLOCKER:   return "support_blocker";
    default:                                 return "unknown";
    }
}

nlohmann::json AnalyticsDataUploadManager::ProjectModificationTracker::collect_obj_params(PartPlate* plate, int plate_idx)
{
    // 新格式：objects 是字典，key = object id（数字字符串），value = 对象详情
    nlohmann::json objects = nlohmann::json::object();
    
    if (!plate) {
        BOOST_LOG_TRIVIAL(warning) << "[ObjParams-Debug] Plate " << plate_idx << " | plate is null";
        return objects;
    }
    
    // 获取当前盘上的所有对象
    ModelObjectPtrs objs_on_plate = plate->get_objects_on_this_plate();
    
    BOOST_LOG_TRIVIAL(warning) << "[ObjParams-Debug] Plate " << plate_idx 
                               << " | total objects on plate: " << objs_on_plate.size();
    
    // 遍历当前盘的对象
    for (const auto& obj : objs_on_plate) {
        if (!obj) continue;
        
        nlohmann::json obj_entry;
        
        // 1. object key: 使用 from_loaded_id（3dmodel.model 里的 object id）
        std::string obj_key;
        if (obj->from_loaded_id >= 0) {
            obj_key = std::to_string(obj->from_loaded_id);
        } else {
            // 备选：使用内部 id（用于调试）
            obj_key = std::to_string(obj->id().id);
            BOOST_LOG_TRIVIAL(warning) << "[ObjParams-Debug] ModelObject.from_loaded_id is -1, using internal id: " << obj_key;
        }
        
        // 2. UUID: 3dmodel.model 里 p:UUID
        obj_entry["UUID"] = obj->uuid;
        
        // 3. extruder: 默认挤出机
        if (obj->config.has("extruder")) {
            const ConfigOption* opt = obj->config.option("extruder");
            if (opt) obj_entry["extruder"] = serialize_config_value(opt);
        }
        
        // 4. transform: 实例变换矩阵（取第一个 instance）
        if (!obj->instances.empty()) {
            obj_entry["transform"] = transform3d_to_3mf_string(obj->instances.front()->get_matrix());
        }
        
        // 5. 对象级修改参数（扁平铺在对象身上，不嵌套在 obj_param 里）
        if (!obj->config.empty()) {
            for (const std::string& key : obj->config.keys()) {
                if (key == "extruder") continue;  // extruder 已单独处理
                const ConfigOption* opt = obj->config.option(key);
                if (opt) {
                    obj_entry[key] = serialize_config_value(opt);
                }
            }
        }
        
        // 6. 采集部件级参数 (ModelVolume)
        nlohmann::json parts_array = nlohmann::json::array();
        for (const auto& vol : obj->volumes) {
            if (!vol) continue;
            
            nlohmann::json part_entry;
            
            // 6.1 part_id: 使用 from_loaded_id（3dmodel.model 里的 component objectid）
            if (vol->from_loaded_id >= 0) {
                part_entry["part_id"] = std::to_string(vol->from_loaded_id);
            } else {
                part_entry["part_id"] = std::to_string(vol->id().id);
                BOOST_LOG_TRIVIAL(warning) << "[ObjParams-Debug] ModelVolume.from_loaded_id is -1, using internal id: " << vol->id().id;
            }
            
            // 6.2 UUID: component 的 p:UUID
            part_entry["UUID"] = vol->uuid;
            
            // 6.3 extruder: 部件挤出机
            int extruder_id = vol->extruder_id();
            if (extruder_id >= 0) {
                part_entry["extruder"] = std::to_string(extruder_id);
            }
            
            // 6.4 transform: 部件变换矩阵
            part_entry["transform"] = transform3d_to_3mf_string(vol->get_matrix());
            
            // 6.5 subtype: 部件类型
            part_entry["subtype"] = volume_type_to_subtype_string(vol->type());
            
            // 6.6 部件级修改参数（扁平铺在 part 身上，不嵌套在 part_param 里）
            if (!vol->config.empty()) {
                for (const std::string& key : vol->config.keys()) {
                    const ConfigOption* opt = vol->config.option(key);
                    if (opt) {
                        part_entry[key] = serialize_config_value(opt);
                    }
                }
            }
            
            parts_array.push_back(part_entry);
        }
        
        if (!parts_array.empty()) {
            obj_entry["parts"] = parts_array;
        }
        
        objects[obj_key] = obj_entry;
    }
    
    // 【调试日志】输出采集结果
    if (!objects.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "[ObjParams-Debug] Plate " << plate_idx 
                                   << " | collected " << objects.size() << " objects"
                                   << " | objects: " << objects.dump(2);
    } else {
        BOOST_LOG_TRIVIAL(warning) << "[ObjParams-Debug] Plate " << plate_idx 
                                   << " | no objects found";
    }
    
    return objects;
}

// 采集打印机参数
void AnalyticsDataUploadManager::ProjectModificationTracker::collect_printer_params(const DynamicPrintConfig& config, nlohmann::json& output)
{
    for (size_t i = 0; i < s_printer_params_count; i++) {
        const auto& def = s_printer_params[i];
        add_param(config, output, def.config_key, def.output_key, def.type);
    }
}

// 采集材料参数
void AnalyticsDataUploadManager::ProjectModificationTracker::collect_filament_params(const DynamicPrintConfig& config, nlohmann::json& output)
{
    for (size_t i = 0; i < s_filament_params_count; i++) {
        const auto& def = s_filament_params[i];
        add_param(config, output, def.config_key, def.output_key, def.type);
    }
}

// 采集工艺参数
void AnalyticsDataUploadManager::ProjectModificationTracker::collect_process_params(const DynamicPrintConfig& config, nlohmann::json& output)
{
    for (size_t i = 0; i < s_process_params_count; i++) {
        const auto& def = s_process_params[i];
        add_param(config, output, def.config_key, def.output_key, def.type);
    }
}

} // namespace GUI
} // namespace Slic3r
